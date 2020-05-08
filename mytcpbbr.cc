/*** Implementation of mytcpbbr class ***/

/****************************** BBR State Definition *******************************/
//  State transition diagram:
//          |
//          V
//       STARTUP  
//          |     
//          V     
//        DRAIN   
//          |     
//          V     
// +---> PROBE_BW ----+
// |      ^    |      |
// |      |    |      |
// |      +----+      |
// |                  |
// +---- PROBE_RTT <--+


#include <iostream>
#include <stdio.h>

#include "ns3/simulator.h"
#include "tcp-socket-base.h"         

#include "rtt-estimator.h"
#include "mytcpbbr.h"


using namespace ns3;

NS_OBJECT_ENSURE_REGISTERED(MyTcpBbr);

// Default constructor.
MyTcpBbr::MyTcpBbr(void) :
  TcpCongestionOps(),
  pacing_gain(0.0),
  cwnd_gain(0.0),
  smachine(this),
  state_startup(this),
  state_drain(this),
  state_probebw(this),
  state_probertt(this),
  m_round(0),
  m_delivered(0),
  m_next_round_delivered(0),
  current_bytes_in_flight(0),
  min_rtt_change(Time(0)),
  m_cwnd(0.0),
  m_prior_cwnd(0.0), 
  packet_conserv(Time(0)),
  inRetrans(false),
  retrans_seq(0)
  {

  
  // First state is STARTuP.
  smachine.changeState(&state_startup);
}

// Copy constructor.
MyTcpBbr::MyTcpBbr(const MyTcpBbr &sock) :
  TcpCongestionOps(sock),
  pacing_gain(0.0),
  cwnd_gain(0.0),
  smachine(this),
  state_startup(this),
  state_drain(this),
  state_probebw(this),
  state_probertt(this),
  m_round(0),
  m_delivered(0),
  m_next_round_delivered(0),
  current_bytes_in_flight(0),
  min_rtt_change(Time(0)),
  m_cwnd(0.0),
  m_prior_cwnd(0.0), 
  packet_conserv(Time(0)),
  inRetrans(false),
  retrans_seq(0)
   {  
  
}

MyTcpBbr::~MyTcpBbr(void) {
 
}

TypeId MyTcpBbr::GetTypeId(void) {
  static TypeId tid = TypeId("ns3::MyTcpBbr")
    .SetParent<TcpCongestionOps>()
    .SetGroupName("Internet")
    .AddConstructor<MyTcpBbr>();
  return tid;
}

std::string MyTcpBbr::GetName() const {
  return "MyTcpBbr";
}

Ptr<TcpCongestionOps> MyTcpBbr::Fork() {
  return CopyObject<MyTcpBbr> (this);
}

//Not Used in BBR
void MyTcpBbr::IncreaseWindow(Ptr<TcpSocketState> tcb, uint32_t segs_acked) {

  return;
}
//not used in BBR
uint32_t MyTcpBbr::GetSsThresh(Ptr<const TcpSocketState> tcb,
                             uint32_t b_in_flight) {
  return 65535; // returns highest value
}


// Send() 
  //   Record W_a
  //   Record time W_t
  //   Send W_s
// PktsAcked()
  //   Record time W_t'
  //   Compute BW: bw = (W_s - W_a) / (W_t' - W_t)


void MyTcpBbr::BwEstimate(Ptr<TcpSocketState> tcb, uint32_t packets_acked) {
  SequenceNumber32 ack = tcb->m_lastAckedSeq;  

  Time now = Simulator::Now();                      
  MyBbr::packetS packet;

  // Update packet-timed RTT.
  m_delivered += tcb->m_segmentSize;
  packet.delivered = -1;
  for (auto it = pkt_window.begin(); it != pkt_window.end(); it++) {
    if (it->sentP == ack)
      packet = *it;
  }
  if (packet.delivered >= m_next_round_delivered) {
    m_next_round_delivered = m_delivered;
    m_round++;
  }

  // check if retransmission sequence should end.
  bool do_est_bw = true;
  if (inRetrans) {
    if (ack != retrans_seq) {
      inRetrans = false;
    } // Don't estimate BW if in retrans sequence 
    do_est_bw = false; 
  }

  if (pkt_window.size() == 0) {

    return; 
  }
  auto first = pkt_window.begin()->sentP;
  if (ack < first) {

    return;  
  }


  packet.sentP = 0;
  packet.time = Time(0);
  for (auto it = pkt_window.begin(); it != pkt_window.end(); it++)
    if (it->sentP <= ack && it->sentP > packet.sentP)
      packet = *it;  // W_a

  
  for (unsigned int i=0; i < pkt_window.size(); )
    if (pkt_window[i].sentP <= packet.sentP) 
      pkt_window.erase(pkt_window.begin() + i); // got lost or retransmission
    else
      i++;

  // Estimate BW.
  double bwEst = 0.0;
  if (do_est_bw) {

    // Estimate BW: bw = (W_s - W_a) / (W_t' - W_t)
    bwEst = (ack - packet.ackedP) /
             (now.GetSeconds() - packet.time.GetSeconds());
    bwEst *= 8;          // Convert to b/s.
    bwEst /= 1000000;    // Convert to Mb/s.

 
    MyBbr::bwS bw;
    bw.bwEst = bwEst;
    bw.time = now;
    bw.roundT = m_round;
    bw_window.push_back(bw);
  }
                         
 }

void MyTcpBbr::PktsAcked(Ptr<TcpSocketState> tcb, uint32_t packets_acked,
                       const Time &rtt) {
  uint32_t bytes_delivered = packets_acked * 1500;
 
  if (tcb->m_congState == TcpSocketState::CA_RECOVERY) {
    
    if (packet_conserv > Simulator::Now()) {
      
      if ((current_bytes_in_flight + bytes_delivered) > m_cwnd)
        m_cwnd = current_bytes_in_flight + bytes_delivered;
    }
  } else {
    // else call update target cwnd.
    updTargetCwnd();
  }
  if (tcb -> m_cWnd < m_cwnd) {
    
    tcb -> m_cWnd = tcb -> m_cWnd + bytes_delivered;
  } else
    
    tcb -> m_cWnd = (uint32_t) m_cwnd;

  if (rtt.IsZero() || rtt.IsNegative()) {
    return;
  }

  // check if  min RTT changed
  Time now = Simulator::Now();
  Time min_rtt = getminRTT();
  if (rtt < getminRTT()) {
    
    min_rtt_change = now;  // update when minimum rtt changed 
  }
  //Record Rtt
  rtt_window[now] = rtt;

  if (rtt_window.size() == 1) {
    
    smachine.start(); // On first ack start the state machine
  }

  BwEstimate(tcb,packets_acked); // estimate Bandwidth

  // Set pacing rate (in Mb/s), adjusted by gain.
  double pacing_rate = getmaxBW() * pacing_gain;

  if (pacing_gain == 1)
    pacing_rate *= MyBbr::PACING_FACTOR;
  
  if (pacing_rate < 0)
    pacing_rate = 0.0;

  if (PACING_CONFIG != NO_PACING) {

    if (smachine.getStateT() == MyBbr::PROBE_RTT) {
      double probe_rtt_pacing_rate = MyBbr::MIN_CWND;   // Bytes (B).
      probe_rtt_pacing_rate /=  min_rtt.GetSeconds(); // B/s.
      probe_rtt_pacing_rate *= 8;                     // Convert to b/s.
      probe_rtt_pacing_rate /= 1000000;               // Convert to Mb/s.
     
      if (probe_rtt_pacing_rate < pacing_rate)
        pacing_rate = probe_rtt_pacing_rate;
    }
    
    // Set Pacing rate.
    tcb -> SetPacingRate(pacing_rate);
  }

  
  
}

void MyTcpBbr::recordSeqInfoBWest(Ptr<TcpSocketState> tcb, SequenceNumber32 seq) {
MyBbr::packetS p;
    p.ackedP = tcb -> m_lastAckedSeq;
    p.sentP = seq;
    p.time = Simulator::Now();
    p.delivered = m_delivered;
    pkt_window.push_back(p);
}


void MyTcpBbr::Send(Ptr<TcpSocketBase> tsb, Ptr<TcpSocketState> tcb,
                  SequenceNumber32 seq, bool isRetrans) {

  current_bytes_in_flight = tsb -> BytesInFlight();

  if (isRetrans) {
    inRetrans = true;
    retrans_seq = seq;
  }
  if (!inRetrans) {
   // record info 
    recordSeqInfoBWest(tcb,seq);  
  } 
}

double MyTcpBbr::getmaxBW() {
  double max_bw = 0;
  if (bw_window.size() == 0)

    max_bw = -1.0;

  else
    
    for (auto it = bw_window.begin(); it != bw_window.end(); it++)
      max_bw = std::max(max_bw, it->bwEst);
 
  return max_bw;
}

Time MyTcpBbr::getminRTT(){
  Time min_rtt = Time::Max();


  if (rtt_window.size() == 0)

    min_rtt = Time(-1.0);

  else
    
    for (auto it = rtt_window.begin(); it != rtt_window.end(); it++)
      min_rtt = std::min(min_rtt, it->second);

  return min_rtt;
}

double MyTcpBbr::getBDP()  {
  Time rtt = getminRTT();
  if (rtt.IsNegative())
    rtt = MyBbr::INIT_RTT;
  double bw = getmaxBW();
  if (bw < 0)
    bw = MyBbr::INIT_BW;
  return (double) (rtt.GetSeconds() * bw);
}

// Store only the last 10RTT's
void MyTcpBbr::reduceBWwindow() {

  double bw = getmaxBW();
  if (bw < 0)
    return;

  Time rtt = getminRTT();  //NO BW or RTT
  if (rtt.IsNegative())
    return;

  Time now = Simulator::Now();
  
  int rndDiff = m_round - MyBbr::BW_WINDOW_TIME;
  
  auto it = bw_window.begin();
  while (it != bw_window.end()) {                       
      if (it -> roundT < rndDiff) //Remove old data
        it = bw_window.erase(it);
      else
        it++;  
  }
 
}

// Store only the last 10RTT's
void MyTcpBbr::reduceRTTwindow() {

  Time rtt = getminRTT();
  if (rtt.IsNegative())
    return;

  Time now = Simulator::Now();
  Time deltaT = Time(now - MyBbr::RTT_WINDOW_TIME * 1000000000.0); // Units are nanoseconds.

  auto it = rtt_window.begin();
  while (it != rtt_window.end()) {
    if (it -> first < deltaT)
      it = rtt_window.erase(it);
    else
      it++;
  }
  
}


bool MyTcpBbr::checkProbeRTT() {

  Time now = Simulator::Now();
  if (smachine.getStateT() == MyBbr::PROBE_BW &&
      (now.GetSeconds() - min_rtt_change.GetSeconds()) >
      MyBbr::RTT_NO_CHANGE_LIMIT) {
      min_rtt_change = now;
    return true; // enter PROBE_RTT.
  }
  return false; 
}

void MyTcpBbr::CongestionStateSet(Ptr<TcpSocketState> tcb,
                        const TcpSocketState::TcpCongState_t new_state) {

  auto old_state = tcb->m_congState;
  
    
  // Enter RECOVERY GO BACK TO MINIMAL CWND
  if (new_state == TcpSocketState::CA_LOSS) {
    m_prior_cwnd = m_cwnd;
    m_cwnd = 1000; // bytes
  }

  // Enter Fast Recovery --> save cwnd.
  // Modulate cwnd for 1 RTT.
  if (new_state == TcpSocketState::CA_RECOVERY) {
    m_prior_cwnd = m_cwnd;
    m_cwnd = current_bytes_in_flight + 1;
    packet_conserv = Simulator::Now() + getminRTT(); // Modulate for 1 RTT.
    
  }

  // Exit RTO or Fast Recovery --> restore cwnd.
  if ((old_state == TcpSocketState::CA_RECOVERY ||
       old_state == TcpSocketState::CA_LOSS) &&
      (new_state != TcpSocketState::CA_RECOVERY &&
       new_state != TcpSocketState::CA_LOSS)) {

    packet_conserv = Simulator::Now(); // Stop packet conservation.
    if (m_prior_cwnd > m_cwnd)
      m_cwnd = m_prior_cwnd;
    
  }
}
void MyTcpBbr::updTargetCwnd() {


  double bdp = getBDP();
  if (PACING_CONFIG == NO_PACING)

    m_cwnd = bdp * pacing_gain;
  else
 
    m_cwnd = bdp * cwnd_gain;

  m_cwnd = (m_cwnd * 1000000 / 8); //convert to bytes


  if (m_cwnd < MyBbr::MIN_CWND) {
    
    m_cwnd = MyBbr::MIN_CWND; 
  }
}