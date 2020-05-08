#ifndef BBR_STATE_H
#define BBR_STATE_H

#include "ns3/object.h"
#include "ns3/nstime.h"

namespace ns3 {

class MyTcpBbr; 
class BbrStates;

namespace MyBbr {

// Defined  states.
enum BbrStates_t
        {
            STARTUP,        
            DRAIN,          
            PROBE_BW,      
            PROBE_RTT,      
            UNDEF = -100,
        } ;

} 
class BbrSMController : public Object {

 public:
    BbrStates *currentState;           
    MyTcpBbr *flowowner; 
    BbrSMController();
    BbrSMController(MyTcpBbr *_flowowner);
    static TypeId GetTypeId();
    MyBbr::BbrStates_t getStateT() const;
    std::string GetName() const;
    void changeState(BbrStates *p_new_state); 
    void start();

};

class BbrStates: public Object {

 public:
  static TypeId GetTypeId();
  virtual std::string GetName() const;
  BbrStates();
  BbrStates(MyTcpBbr *flowowner);
  virtual ~BbrStates();

    void enterState();
    virtual void handleState()=0;
    void exitState();

    virtual MyBbr:: BbrStates_t getState() const =0;
    MyTcpBbr *flowowner;             //  flow that owns state.
    bool is_pipefull;
};



class BBRStartup : public BbrStates {
public:
        double maxPrevBW;                        
        int countNoBWChange;

 public:
  
  static TypeId GetTypeId();

  
  std::string GetName() const;
  
  
  BBRStartup(MyTcpBbr *flowowner);
  BBRStartup();

 void enterState();
    void handleState();
    void exitState();
    MyBbr:: BbrStates_t getState() const;
};



class BBRDrain : public BbrStates {

 public:
        uint32_t inflightLimit; 
        uint32_t roundC;    
        


public:
static TypeId GetTypeId();
  std::string GetName() const;
  BBRDrain(MyTcpBbr *flowowner);
  BBRDrain();
 void enterState();
    void handleState();
    void exitState();
    MyBbr:: BbrStates_t getState() const;
};


class BBRProbeBW : public BbrStates {
public:
int gainCyclePhase;

 public:
  static TypeId GetTypeId();
  std::string GetName() const;

  BBRProbeBW(MyTcpBbr *flowowner);
  BBRProbeBW();

 void enterState();
    void handleState();
    void exitState();
    MyBbr:: BbrStates_t getState() const;                 
};


class BBRProbeRtt : public BbrStates {
    public:
  Time timetoremain;
public:
 static TypeId GetTypeId();

  
  std::string GetName() const;
  
  
  BBRProbeRtt(MyTcpBbr *flowowner);
  BBRProbeRtt();

 void enterState();
    void handleState();
    void exitState();
    MyBbr:: BbrStates_t getState() const;
};

} // end of namespace ns3
#endif

