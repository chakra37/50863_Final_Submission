#include "mytcpbbr.h"
#include "mytcpbbrextra.h"

using namespace ns3;
NS_OBJECT_ENSURE_REGISTERED(BbrStates);
NS_OBJECT_ENSURE_REGISTERED(BbrSMController);

/******************************BbrStateMachineController*******************************/
BbrSMController::BbrSMController() {
  
  flowowner = NULL;
  currentState = NULL;
}

// Default constructor.
BbrSMController::BbrSMController(MyTcpBbr *_flowowner) {
  
  flowowner = _flowowner;
  currentState = NULL;
}


TypeId BbrSMController::GetTypeId(void) {
  static TypeId tid = TypeId("ns3::BbrSMController")
    .SetParent<Object>()
    .SetGroupName("Internet")
    .AddConstructor<BbrSMController>();
  return tid;
}


std::string BbrSMController::GetName() const {
  
  return "BbrSMController";
}

// Get type of current state.
MyBbr::BbrStates_t BbrSMController::getStateT() const {
  return currentState -> getState();
}

// Update by executing current state.
void BbrSMController::start() {
  

  if (currentState == NULL) {
    return;
  }

  // Check if should enterState PROBE_RTT.
  if (flowowner -> checkProbeRTT())
    changeState(&flowowner -> state_probertt);

  
  currentState -> handleState();

  
  flowowner -> reduceRTTwindow();


  flowowner -> reduceBWwindow();

  Time rtt = flowowner -> getminRTT();
  if (!rtt.IsNegative()) {
    Simulator::Schedule(rtt, &BbrSMController::start, this);
    
  }  
}

// Change current state to new state.
void BbrSMController::changeState(BbrStates *newState) {
  if (newState == NULL) {
    return;
  }
  currentState = newState;

  // Call enterState on new state.
  currentState -> enterState();
}



BbrStates::BbrStates(MyTcpBbr *_flowowner) {
  flowowner = _flowowner;
  is_pipefull=false;
}

BbrStates::BbrStates() {
  flowowner = NULL;
  is_pipefull=false;
}

BbrStates::~BbrStates() {
}


TypeId BbrStates::GetTypeId(void) {
  static TypeId tid = TypeId("ns3::BbrStates")
    .SetParent<BbrSMController>()
    .SetGroupName("Internet");
  return tid;
}


std::string BbrStates::GetName() const {
  return "BbrStates";
}

void BbrStates::enterState() {
}

/****************************** Each BBR State Definition *******************************/

            /******************************START UP *******************************/

BBRStartup::BBRStartup(MyTcpBbr *flowowner) : BbrStates(flowowner),
  maxPrevBW(0),
  countNoBWChange(0) {
  
}

BBRStartup::BBRStartup() : BbrStates(),
  maxPrevBW(0),
  countNoBWChange(0) {
  
}


TypeId BBRStartup::GetTypeId(void) {
  static TypeId tid = TypeId("ns3::BBRStartup")
    .SetParent<BbrSMController>()
    .SetGroupName("Internet")
    .AddConstructor<BBRStartup>();
  return tid;
}


std::string BBRStartup::GetName() const {
  return "BBRStartup";
}


MyBbr::BbrStates_t BBRStartup::getState(void)const  {
  return MyBbr::STARTUP;
}

void BBRStartup::enterState() {
  
  

  // Set gains to 2/ln(2).
  flowowner -> pacing_gain = MyBbr::STARTUP_GAIN;
  flowowner -> cwnd_gain = MyBbr::STARTUP_GAIN;
}


void BBRStartup::handleState() {
  
  
  double new_bw = flowowner -> getmaxBW();

  if (new_bw < 0) {
    
    return;
  }
  

  if (new_bw > maxPrevBW * MyBbr::STARTUP_THRESHOLD_EXIT) { 
    
    maxPrevBW = new_bw;
    countNoBWChange = 0;
    return;
  }
  countNoBWChange++;
  
  
  
  if (countNoBWChange > 3) {
    
    this->exitState();
  }

  return;
}
void BBRStartup::exitState() {
flowowner -> smachine.changeState(&flowowner -> state_drain);
}

/******************************DRAIN *******************************/

  
BBRDrain::BBRDrain(MyTcpBbr *flowowner) :
  BbrStates(flowowner),
  inflightLimit(0),
  roundC(0) {
  
}

BBRDrain::BBRDrain() :
  BbrStates(),
  inflightLimit(0),
  roundC(0) {
  
}


TypeId BBRDrain::GetTypeId(void) {
  static TypeId tid = TypeId("ns3::BBRDrain")
    .SetParent<BbrSMController>()
    .SetGroupName("Internet")
    .AddConstructor<BBRDrain>();
  return tid;
}


std::string BBRDrain::GetName() const {
  return "BBRDrain";
}


MyBbr::BbrStates_t BBRDrain::getState(void) const {
  return MyBbr::DRAIN;
}

void BBRDrain::enterState() {
  
 

  // Set pacing gain to 1/[2/ln(2)].
  flowowner -> pacing_gain = 1 / MyBbr::STARTUP_GAIN;

  
  if (PACING_CONFIG == NO_PACING)
    flowowner -> cwnd_gain = 1 / MyBbr::STARTUP_GAIN; 
  else
    flowowner -> cwnd_gain = MyBbr::STARTUP_GAIN; 
  double bdp = flowowner -> getBDP();
  bdp = bdp * 1000000 / 8; // Convert to bytes.
  inflightLimit = (uint32_t) bdp;  
  roundC = 0;
}


void BBRDrain::handleState() {
  
  roundC++;
  if (flowowner -> current_bytes_in_flight < inflightLimit ||
      roundC == 5) {
    
    this->exitState();
  }
}
void BBRDrain::exitState() {
flowowner -> smachine.changeState(&flowowner -> state_probebw);
}

/******************************Probe BW *******************************/

  
BBRProbeBW::BBRProbeBW(MyTcpBbr *flowowner) : BbrStates(flowowner) {
  
}

BBRProbeBW::BBRProbeBW() : BbrStates() {
  
}


TypeId BBRProbeBW::GetTypeId(void) {
  static TypeId tid = TypeId("ns3::BBRProbeBW")
    .SetParent<BbrSMController>()
    .SetGroupName("Internet")
    .AddConstructor<BBRProbeBW>();
  return tid;
}


std::string BBRProbeBW::GetName() const {
  return "BBRProbeBW";
}


MyBbr::BbrStates_t BBRProbeBW::getState(void) const {
  return MyBbr::PROBE_BW;
}

void BBRProbeBW::enterState() {
  
  do {
    gainCyclePhase = rand() % 8;
  } while (gainCyclePhase == 1);  // Phase 1 is "low" cycle.


  // Set gains based on phase.
  flowowner -> pacing_gain = MyBbr::STEADY_FACTOR;
  if (gainCyclePhase == 0) // Phase 0 is "high" cycle.
    flowowner -> pacing_gain += MyBbr::PROBE_FACTOR;
  if (PACING_CONFIG == NO_PACING)
    flowowner -> cwnd_gain = flowowner -> pacing_gain;
  else
    flowowner -> cwnd_gain = MyBbr::STEADY_FACTOR * 2;
}


void BBRProbeBW::handleState() {
  
  

  // Set gain rate: [high, low, stdy, stdy, stdy, stdy, stdy, stdy]
  if (gainCyclePhase == 0)
    flowowner -> pacing_gain = MyBbr::STEADY_FACTOR + MyBbr::PROBE_FACTOR;
  else if (gainCyclePhase == 1)
      flowowner -> pacing_gain = MyBbr::STEADY_FACTOR - MyBbr::DRAIN_FACTOR;
  else
    flowowner -> pacing_gain = MyBbr::STEADY_FACTOR;

  if (PACING_CONFIG == NO_PACING)
   
    flowowner -> cwnd_gain = flowowner -> pacing_gain;
  else
    
    flowowner -> cwnd_gain = 2 * MyBbr::STEADY_FACTOR;
  gainCyclePhase++;
  if (gainCyclePhase > 7)
    gainCyclePhase = 0;
}

/******************************Probe RTT*******************************/

  
BBRProbeRtt::BBRProbeRtt(MyTcpBbr *flowowner) : BbrStates(flowowner) {
  
}

BBRProbeRtt::BBRProbeRtt() : BbrStates() {
  
}


TypeId BBRProbeRtt::GetTypeId(void) {
  static TypeId tid = TypeId("ns3::BBRProbeRtt")
    .SetParent<BbrSMController>()
    .SetGroupName("Internet")
    .AddConstructor<BBRProbeRtt>();
  return tid;
}


std::string BBRProbeRtt::GetName() const {
  return "BBRProbeRtt";
}


MyBbr::BbrStates_t BBRProbeRtt::getState(void) const {
  return MyBbr::PROBE_RTT;
}

void BBRProbeRtt::enterState() {
  
  

  
  flowowner -> pacing_gain = MyBbr::STEADY_FACTOR;
  flowowner -> cwnd_gain = MyBbr::STEADY_FACTOR;

  
  Time rtt = flowowner -> getminRTT();
  if (rtt.GetSeconds() > 0.2)
    timetoremain = rtt;
  else
    timetoremain = Time(0.2 * 1000000000);
  timetoremain = timetoremain + Simulator::Now();
    
  
}


void BBRProbeRtt::handleState() {
  
  flowowner -> m_cwnd = MyBbr::MIN_CWND * 1500; // In bytes.

 
  Time now = Simulator::Now();
  if (now > timetoremain) {
      
      this->exitState();
  }
}
void BBRProbeRtt::exitState() {
    flowowner -> smachine.changeState(&flowowner -> state_probebw);
}