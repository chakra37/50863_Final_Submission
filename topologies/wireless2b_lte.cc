
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/tcp-header.h"
#include "ns3/traffic-control-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/lte-module.h"
 #include "ns3/lte-rlc-header.h"
 #include "ns3/lte-rlc-um.h"
 #include "ns3/config-store.h"
//#include "ns3/gtk-config-store.h"

using namespace ns3;

// Uncomment one of the below.
//#define TCP_PROTOCOL     "ns3::TcpCubic"
#define TCP_PROTOCOL     "ns3::TcpBbr"
//#define TCP_PROTOCOL     "ns3::TcpNewReno"
#define PACKET_SIZE      1000      // Bytes.

// For logging. 
//void ThroughputMonitor (FlowMonitorHelper *fmhelper, Ptr<FlowMonitor> flowMon,Gnuplot2dDataset DataSet);
bool firstCwnd = true;
bool firstSshThr = true;
bool firstRtt = true;
bool firstRto = true;
Ptr<OutputStreamWrapper> cWndStream;
Ptr<OutputStreamWrapper> ssThreshStream;
Ptr<OutputStreamWrapper> rttStream;
Ptr<OutputStreamWrapper> rtoStream;
Ptr<OutputStreamWrapper> nextTxStream;
Ptr<OutputStreamWrapper> nextRxStream;
Ptr<OutputStreamWrapper> inFlightStream;
uint32_t cWndValue;
uint32_t ssThreshValue;
//NS_LOG_COMPONENT_DEFINE ("main");
/////////////////////////////////////////////////
static void
CwndTracer (uint32_t oldval, uint32_t newval)
{
  if (firstCwnd)
    {
      *cWndStream->GetStream () << "0.0 " << oldval << std::endl;
      firstCwnd = false;
    }
  *cWndStream->GetStream () << Simulator::Now ().GetSeconds () << " " << newval << std::endl;
  cWndValue = newval;

  if (!firstSshThr)
    {
      *ssThreshStream->GetStream () << Simulator::Now ().GetSeconds () << " " << ssThreshValue << std::endl;
    }
}

static void
SsThreshTracer (uint32_t oldval, uint32_t newval)
{
  if (firstSshThr)
    {
      *ssThreshStream->GetStream () << "0.0 " << oldval << std::endl;
      firstSshThr = false;
    }
  *ssThreshStream->GetStream () << Simulator::Now ().GetSeconds () << " " << newval << std::endl;
  ssThreshValue = newval;

  if (!firstCwnd)
    {
      *cWndStream->GetStream () << Simulator::Now ().GetSeconds () << " " << cWndValue << std::endl;
    }
}

static void
RttTracer (Time oldval, Time newval)
{
  if (firstRtt)
    {
      *rttStream->GetStream () << "0.0 " << oldval.GetSeconds () << std::endl;
      firstRtt = false;
    }
  *rttStream->GetStream () << Simulator::Now ().GetSeconds () << " " << newval.GetSeconds () << std::endl;
}

static void
RtoTracer (Time oldval, Time newval)
{
  if (firstRto)
    {
      *rtoStream->GetStream () << "0.0 " << oldval.GetSeconds () << std::endl;
      firstRto = false;
    }
  *rtoStream->GetStream () << Simulator::Now ().GetSeconds () << " " << newval.GetSeconds () << std::endl;
}

static void
NextTxTracer (SequenceNumber32 old, SequenceNumber32 nextTx)
{
  *nextTxStream->GetStream () << Simulator::Now ().GetSeconds () << " " << nextTx << std::endl;
}

static void
InFlightTracer (uint32_t old, uint32_t inFlight)
{
  *inFlightStream->GetStream () << Simulator::Now ().GetSeconds () << " " << inFlight << std::endl;
}

static void
NextRxTracer (SequenceNumber32 old, SequenceNumber32 nextRx)
{
  *nextRxStream->GetStream () << Simulator::Now ().GetSeconds () << " " << nextRx << std::endl;
}

static void
TraceCwnd (std::string cwnd_tr_file_name)
{
  AsciiTraceHelper ascii;
  cWndStream = ascii.CreateFileStream (cwnd_tr_file_name.c_str ());
  Config::ConnectWithoutContext ("/NodeList/0/$ns3::TcpL4Protocol/SocketList/*/CongestionWindow", MakeCallback (&CwndTracer));
}

static void
TraceSsThresh (std::string ssthresh_tr_file_name)
{
  AsciiTraceHelper ascii;
  ssThreshStream = ascii.CreateFileStream (ssthresh_tr_file_name.c_str ());
  Config::ConnectWithoutContext ("/NodeList/0/$ns3::TcpL4Protocol/SocketList/*/SlowStartThreshold", MakeCallback (&SsThreshTracer));
}

static void
TraceRtt (std::string rtt_tr_file_name)
{
  AsciiTraceHelper ascii;
  rttStream = ascii.CreateFileStream (rtt_tr_file_name.c_str ());
  Config::ConnectWithoutContext ("/NodeList/0/$ns3::TcpL4Protocol/SocketList/*/RTT", MakeCallback (&RttTracer));
}

static void
TraceRto (std::string rto_tr_file_name)
{
  AsciiTraceHelper ascii;
  rtoStream = ascii.CreateFileStream (rto_tr_file_name.c_str ());
  Config::ConnectWithoutContext ("/NodeList/0/$ns3::TcpL4Protocol/SocketList/*/RTO", MakeCallback (&RtoTracer));
}

static void
TraceNextTx (std::string &next_tx_seq_file_name)
{
  AsciiTraceHelper ascii;
  nextTxStream = ascii.CreateFileStream (next_tx_seq_file_name.c_str ());
  Config::ConnectWithoutContext ("/NodeList/0/$ns3::TcpL4Protocol/SocketList/*/NextTxSequence", MakeCallback (&NextTxTracer));
}

static void
TraceInFlight (std::string &in_flight_file_name)
{
  AsciiTraceHelper ascii;
  inFlightStream = ascii.CreateFileStream (in_flight_file_name.c_str ());
  Config::ConnectWithoutContext ("/NodeList/0/$ns3::TcpL4Protocol/SocketList/*/BytesInFlight", MakeCallback (&InFlightTracer));
}


static void
TraceNextRx (std::string &next_rx_seq_file_name)
{
  AsciiTraceHelper ascii;
  nextRxStream = ascii.CreateFileStream (next_rx_seq_file_name.c_str ());
  Config::ConnectWithoutContext ("/NodeList/2/$ns3::TcpL4Protocol/SocketList/*/RxBuffer/NextRxSequence", MakeCallback (&NextRxTracer));
}

 Ptr<PacketSink> sinkp;                         /* Pointer to the packet sink application */
 uint64_t lastTotalRx = 0;
 void
 CalculateThroughput ()
 {
   Time now = Simulator::Now ();                                         /* Return the simulator's virtual time. */
   double cur = (sinkp->GetTotalRx () - lastTotalRx) * (double) 8 / 1e5;     /* Convert Application RX Packets to MBits. */
   //std::cout << now.GetSeconds () << "\t" << cur << " " << std::endl;
   std::cout << cur << std::endl;
   lastTotalRx = sinkp->GetTotalRx ();
   Simulator::Schedule (MilliSeconds (100), &CalculateThroughput);
 }

//void
// CheckQueueSize (Ptr<QueueDisc> queue,std::string queue_disc_type)
// {
//   //double qSize = queue->GetCurrentSize ().GetValue ();
//   double qSize = queue->GetNPackets();
//   // check queue size every 1/10 of a second
//   Simulator::Schedule (Seconds (0.1), &CheckQueueSize, queue, queue_disc_type);
// 
//   std::ofstream fPlotQueue (dir + queue_disc_type + "/queueTraces/queue.plotme", std::ios::out | std::ios::app);
//   fPlotQueue << Simulator::Now ().GetSeconds () << " " << qSize << std::endl;
//   fPlotQueue.close ();
// }
//
NS_LOG_COMPONENT_DEFINE ("LenaSimpleEpc");

	int
main (int argc, char *argv[])
{
	uint16_t numNodePairs = 1;
	Time simTime = Seconds (8);
	double distance = 5000.0;
	std::string sharkName = "shark";

Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue(TCP_PROTOCOL));
Config::SetDefault ("ns3::LteEnbRrc::DefaultTransmissionMode", UintegerValue (1));
Config::SetDefault("ns3::LteEnbRrc::EpsBearerToRlcMapping",EnumValue(3));


	// Command line arguments
	CommandLine cmd;
	cmd.AddValue ("numNodePairs", "Number of eNodeBs + UE pairs", numNodePairs);
	cmd.AddValue ("simTime", "Total duration of the simulation", simTime);
	cmd.AddValue ("distance", "Distance between eNBs [m]", distance);
	cmd.AddValue ("sharkName", "shark file base name", sharkName);
	cmd.Parse (argc, argv);

	ConfigStore inputConfig;
	inputConfig.ConfigureDefaults ();

	std::cout << "distance: " << distance << std::endl;

	cmd.Parse(argc, argv);

	Config::SetDefault ("ns3::LteRlcAm::PollRetransmitTimer", TimeValue (MilliSeconds (20)));
    Config::SetDefault ("ns3::LteRlcAm::ReorderingTimer", TimeValue (MilliSeconds (10)));
    Config::SetDefault ("ns3::LteRlcAm::PollRetransmitTimer", TimeValue (MilliSeconds (20)));
   	Config::SetDefault ("ns3::LteRlcAm::ReorderingTimer", TimeValue (MilliSeconds (10)));
	Config::SetDefault ("ns3::LteRlcUm::MaxTxBufferSize", UintegerValue(512000));
	//Config::SetDefault ("ns3::LteEnbNetDevice:DlBandwidth",  IntegerValue(50));
   	Config::SetDefault ("ns3::LteRlcAm::StatusProhibitTimer", TimeValue (MilliSeconds (40)));Config::SetDefault ("ns3::LteRlcAm::StatusProhibitTimer", TimeValue (MilliSeconds (40)));
	Config::SetDefault ("ns3::RrFfMacScheduler::HarqEnabled", BooleanValue (true));
  	Config::SetDefault ("ns3::PfFfMacScheduler::HarqEnabled", BooleanValue (true));
	Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
	Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper> ();
	lteHelper->SetEpcHelper (epcHelper);
	//lteHelper->SetAttribute("RlcEntity", StringValue ("RlcAm"));
	Ptr<Node> pgw = epcHelper->GetPgwNode ();
	Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (20));
  	lteHelper->SetEnbAntennaModelType ("ns3::IsotropicAntennaModel");
	lteHelper->SetEnbDeviceAttribute ("DlEarfcn", UintegerValue (100));
	lteHelper->SetEnbDeviceAttribute ("UlEarfcn", UintegerValue (100 + 18000));
	lteHelper->SetEnbDeviceAttribute ("DlBandwidth", UintegerValue (50));
	lteHelper->SetEnbDeviceAttribute ("UlBandwidth", UintegerValue (50));
	lteHelper->SetSchedulerType("ns3::PfFfMacScheduler");

	// Create a single RemoteHost
	NodeContainer remoteHostContainer;
	remoteHostContainer.Create (1);
	Ptr<Node> remoteHost = remoteHostContainer.Get (0);
	InternetStackHelper internet;
	internet.Install (remoteHostContainer);

	// Create the Internet
	PointToPointHelper p2ph;
	p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("150Mb/s")));
	p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
	p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.01)));
	NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
	Ipv4AddressHelper ipv4h;
	ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
	Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
	// interface 0 is localhost, 1 is the p2p device
	//Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);

	Ipv4StaticRoutingHelper ipv4RoutingHelper;
	Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
	remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

	NodeContainer ueNodes;
	NodeContainer enbNodes;
	enbNodes.Create (numNodePairs);
	ueNodes.Create (numNodePairs);

	// Install Mobility Model
	Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
	for (uint16_t i = 0; i < numNodePairs; i++)
	{
		//positionAlloc->Add (Vector (distance * i, 0, 0));
		positionAlloc->Add (Vector (distance, 0, 0));
	}
	MobilityHelper mobility;
	mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	mobility.SetPositionAllocator(positionAlloc);
	mobility.Install(ueNodes);

	// Install Mobility Model2
	Ptr<ListPositionAllocator> positionAlloc2 = CreateObject<ListPositionAllocator> ();
	for (uint16_t i = 0; i < numNodePairs; i++)
	{
		positionAlloc2->Add (Vector (0, 0, 0));
	}
	MobilityHelper mobility2;
	mobility2.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	mobility2.SetPositionAllocator(positionAlloc2);
	mobility2.Install(enbNodes);
	

	// Install LTE Devices to the nodes
	NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);
	NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes);

	// Install the IP stack on the UEs
	internet.Install (ueNodes);
	Ipv4InterfaceContainer ueIpIface;
	ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));
	// Assign IP address to UEs, and install applications
	for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
	{
		Ptr<Node> ueNode = ueNodes.Get (u);
		// Set the default gateway for the UE
		Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
		ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
	}

	// Attach one UE per eNodeB
	for (uint16_t i = 0; i < numNodePairs; i++)
	{
		lteHelper->Attach (ueLteDevs.Get(i), enbLteDevs.Get(i));
		// side effect: the default EPS bearer will be activated
	}
 

	ApplicationContainer clientApps;
	ApplicationContainer serverApps;
	Ptr<PacketSink> p_sink;


	//ueIpIface.GetAddress (u)

	//Source 
	BulkSendHelper source("ns3::TcpSocketFactory", InetSocketAddress(ueIpIface.GetAddress (0), 1000));
	// Set the amount of data to send in bytes (0 for unlimited).
	source.SetAttribute("MaxBytes", UintegerValue(0));
	source.SetAttribute("SendSize", UintegerValue(PACKET_SIZE));
	serverApps.Add(source.Install(remoteHost));

	// Sink 
	PacketSinkHelper sink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 1000));
	clientApps.Add(sink.Install(ueNodes.Get(0)));
	p_sink = DynamicCast<PacketSink> (clientApps.Get(0)); // 4 stats
	sinkp=p_sink;

	serverApps.Start (Seconds (3.01));
  	clientApps.Start (Seconds (3.01));













	//NS_LOG_INFO("Total bytes received: " << p_sink->GetTotalRx());
	//double tput = p_sink->GetTotalRx() / (simTime);


	lteHelper->EnableTraces ();
	// Uncomment to enable PCAP tracing
	//p2ph.EnablePcapAll("lena-simple-epc");

/////////////////////////////////////////
  // Setup tracing (as appropriate).

    NS_LOG_INFO("Enabling trace files.");
    AsciiTraceHelper ath;
    p2ph.EnableAsciiAll(ath.CreateFileStream("trace.tr"));

  std::string  prefix_file_name="wireless2b";

      std::ofstream ascii;
      Ptr<OutputStreamWrapper> ascii_wrap;
      ascii.open ((prefix_file_name + "-ascii").c_str ());
      ascii_wrap = new OutputStreamWrapper ((prefix_file_name + "-ascii").c_str (),
                                            std::ios::out);
      internet.EnableAsciiIpv4All (ascii_wrap);

      Simulator::Schedule (Seconds (0.00001), &TraceCwnd, prefix_file_name + "-cwnd.data");
      Simulator::Schedule (Seconds (0.00001), &TraceSsThresh, prefix_file_name + "-ssth.data");
      Simulator::Schedule (Seconds (0.00001), &TraceRtt, prefix_file_name + "-rtt.data");
      Simulator::Schedule (Seconds (0.00001), &TraceRto, prefix_file_name + "-rto.data");
      Simulator::Schedule (Seconds (0.00001), &TraceNextTx, prefix_file_name + "-next-tx.data");
      Simulator::Schedule (Seconds (0.00001), &TraceInFlight, prefix_file_name + "-inflight.data");
      Simulator::Schedule (Seconds (0.1), &TraceNextRx, prefix_file_name + "-next-rx.data");
      Simulator::Schedule (Seconds (0.1), &CalculateThroughput);

//  std::string queue_disc_type = "placeholder";
//   Ptr<QueueDisc> queue = queueDiscs.Get (0);
//   Simulator::ScheduleNow (&CheckQueueSize, queue,queue_disc_type);
// 
//   std::string dirToSave = "mkdir -p " + dir + queue_disc_type;
//   if (system ((dirToSave + "/cwndTraces/").c_str ()) == -1
//       || system ((dirToSave + "/queueTraces/").c_str ()) == -1)
//     {
//       exit (1);
//     }

    NS_LOG_INFO("Enabling pcap files.");
    p2ph.EnablePcapAll(sharkName, true);

  // Flow monitor
  /*Ptr<FlowMonitor> flowMonitor;
  FlowMonitorHelper flowHelper;
  flowMonitor = flowHelper.InstallAll();*/
  

  /////////////////////////////////////////

	Simulator::Stop (simTime);
	Simulator::Run ();

	/*GtkConfigStore config;
	  config.ConfigureAttributes();*/

	Simulator::Destroy ();
	return 0;
}


