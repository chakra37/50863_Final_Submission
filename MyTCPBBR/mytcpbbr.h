#ifndef MYTCPBBR_H
#define MYTCPBBR_H

#include "tcp-congestion-ops.h"         
#include "mytcpbbrextra.h"                  

namespace ns3 {

    namespace MyBbr{
        const float STARTUP_THRESHOLD_EXIT = 1.25; 
        const float STARTUP_GAIN = 2.89;      //  2/ln(2).
        const Time INIT_RTT = Time(1000000);  
        const double INIT_BW = 6.0;           
        const int RTT_WINDOW_TIME = 10;      
        const int BW_WINDOW_TIME = 10;        
        const int MIN_CWND = 4 * 1000;        
        const float PACING_FACTOR = 0.95;   
        // Gain rates per cycle: [1.25, 0.75, 1, 1, 1, 1, 1, 1]  // From Paper
        const float STEADY_FACTOR = 1.0;      
        const float PROBE_FACTOR = 0.25;     
        const float DRAIN_FACTOR = 0.25;     
        const float RTT_NO_CHANGE_LIMIT = 10;  
        const float PROBE_RTT_MIN_TIME = 0.2; 
        
        struct packetS {
        SequenceNumber32 ackedP;  
        SequenceNumber32 sentP;   
        Time time;              
        int delivered;           
        };

       
        struct bwS {
        Time time;               
        int roundT;              
        double bwEst;           
        };
    }
    class MyTcpBbr : public TcpCongestionOps {

    public:

        friend class BbrStates;
        friend class BBRStartup;
        friend class BBRDrain;
        friend class BBRProbeBW;
        friend class BBRProbeRtt;
        friend class BbrSMController;
        double pacing_gain;                    
        double cwnd_gain;  
        BbrSMController smachine;         
        BBRStartup state_startup;          
        BBRDrain state_drain;              
        BBRProbeBW state_probebw;        
        BBRProbeRtt state_probertt;                       
        int m_round;                           
        int m_delivered;                       
        int m_next_round_delivered;            
        uint32_t current_bytes_in_flight;      
        Time min_rtt_change;                   
        double m_cwnd;                         
        double m_prior_cwnd;                   
        Time packet_conserv;               
        bool inRetrans;                   
        SequenceNumber32 retrans_seq; 
        std::map<Time, Time> rtt_window;       
        std::vector<MyBbr::packetS> pkt_window;
        std::vector<MyBbr::bwS> bw_window;    
           

 
  static TypeId GetTypeId(void);

 
  std::string GetName() const;

  
  MyTcpBbr();

  
  MyTcpBbr(const MyTcpBbr &sock);

  // Destructor.
  virtual ~MyTcpBbr();
    //Inherited functions
  virtual uint32_t GetSsThresh(Ptr<const TcpSocketState> tcb, uint32_t b_in_flight);  
  virtual void IncreaseWindow(Ptr<TcpSocketState> tcb, uint32_t segs_acked);
   virtual void PktsAcked(Ptr<TcpSocketState> tcb, uint32_t packets_acked,
                         const Time &rtt);
  
  virtual void Send(Ptr<TcpSocketBase> tsb, Ptr<TcpSocketState> tcb,
                    SequenceNumber32 seq, bool isRetrans);
  virtual void CongestionStateSet(Ptr<TcpSocketState> tcb, const TcpSocketState::TcpCongState_t new_state);                  
  virtual Ptr<TcpCongestionOps> Fork();

      
    
    //functions required for BBR
void recordSeqInfoBWest(Ptr<TcpSocketState> tcb, SequenceNumber32 seq);
void BwEstimate(Ptr<TcpSocketState> tcb, uint32_t packets_acked) ;
  Time getminRTT() ; 
  double getmaxBW() ;

  double getBDP() ;

  void reduceBWwindow();

  
  void reduceRTTwindow();

  
  void updTargetCwnd();

  bool checkProbeRTT();

 
  
};
} 

#endif