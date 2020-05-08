//Dumble Topology Created to Compare TCP Variants

#include <string>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <map>

// NS3 includes.
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/packet-sink.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/gnuplot.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/enum.h"
#include "ns3/event-id.h"
#include "ns3/traffic-control-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"

typedef uint32_t uint;

#define NUM_FLOWS 1   // change this number to create different leaves on each side of dumble
#define NUM_FLOWS_BBR 1   // change this number to create different leaves on each side of dumble
#define NUM_FLOWS_CUBIC 0   // change this number to create different leaves on each side of dumble
#define NUM_FLOWS_RENO 0   // change this number to create different leaves on each side of dumble

// Constants.
#define ENABLE_PCAP      true     // Set to "true" to enable pcap
#define ENABLE_TRACE     true     // Set to "true" to enable trace
#define BIG_QUEUE        2000      // Packets
#define QUEUE_SIZE       100       // Packets
#define START_TIME       0.0       // Seconds
#define DELAYED_START  	 0.0			// Any flow with delayed start
#define STOP_TIME        8.0       // Seconds
#define S_TO_R_BW        "150Mbps" // node to router
#define S_TO_R_DELAY     "1ms"
#define R_TO_R_BW        "10Mbps"  // Router to router (bttlneck)
#define R_TO_R_DELAY     "20ms"
#define PACKET_SIZE      1000      // Bytes.
#define ERROR            0.000001
#define QNR  (150000*10)/PACKET_SIZE //Delay Bandwidth Product / packetsize
#define QR1R2 (10000*1)/PACKET_SIZE //Delay Bandwidth Product / packetsize

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Dumble");
Ptr<PacketSink> sinkp[50];                         /* Pointer to the packet sink application */
 uint64_t lastTotalRx[50];
Ptr<Queue<Packet>> qwe;  
 void
 CalculateThroughput ()
 {
   Time now = Simulator::Now ();
   std::cout << now.GetSeconds () << "s: " ;
   for(int i=0;i<NUM_FLOWS;i++){                                    /* Return the simulator's virtual time. */
   double cur = (sinkp[i]->GetTotalRx () - lastTotalRx[i]) * (double) 8 / 1e5;     /* Convert Application RX Packets to MBits. */
   std::cout << "F: "<<i <<"  "<< cur << " Mbit/s";
   lastTotalRx[i] = sinkp[i]->GetTotalRx ();
   }
	std::cout << " " << std::endl;
   Simulator::Schedule (MilliSeconds (100), &CalculateThroughput);
 }

Ptr<OutputStreamWrapper> rttStream;
bool firstRtt = true;
std::string prefix_file_name = "reverse";
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
TraceRtt (std::string rtt_tr_file_name)
{
  AsciiTraceHelper ascii;
  rttStream = ascii.CreateFileStream (rtt_tr_file_name.c_str ());
  Config::ConnectWithoutContext ("/NodeList/3/$ns3::TcpL4Protocol/SocketList/*/RTT", MakeCallback (&RttTracer));
}


Ptr<Socket> prepareTCPSocket(ns3::Ipv4Address sinkAddress, 
					uint sinkPort, 
					std::string tcpVariant, 
					Ptr<Node> hostNode, 
					Ptr<Node> sinkNode, 
					double startTime, 
					double stopTime,
					uint packetSize,
					uint numPackets,
					double appStartTime,
					double appStopTime,int i_val) {

	if(tcpVariant.compare("ns3::TcpCubic") == 0 || tcpVariant.compare("ns3::TcpBbr") == 0
         || tcpVariant.compare("ns3::TcpNewReno") == 0) {
		std::cout<<tcpVariant<<std::endl;
			TypeId tid = TypeId::LookupByName (tcpVariant);
			std::stringstream nodeId;
				nodeId << hostNode->GetId ();
			std::string specificNode = "/NodeList/" + nodeId.str () + "/$ns3::TcpL4Protocol/SocketType";
			Config::Set (specificNode, TypeIdValue (tid));
	} else {
		fprintf(stderr, "Invalid TCP version\n");
		exit(EXIT_FAILURE);
	}
	PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory",
     InetSocketAddress(Ipv4Address::GetAny(), sinkPort));
	ApplicationContainer sinkApps = packetSinkHelper.Install(sinkNode);
	sinkApps.Start(Seconds(startTime));
	sinkApps.Stop(Seconds(stopTime));
	Ptr<PacketSink> p_sink = DynamicCast<PacketSink> (sinkApps.Get(0)); // 4 stats
  	sinkp[i_val]=p_sink;
	lastTotalRx[i_val]=0;
	

	Ptr<Socket> ns3TcpSocket = Socket::CreateSocket(hostNode, TcpSocketFactory::GetTypeId());

    // Source (at nodes: left side).
     // Well-known port for server.
 
  BulkSendHelper source("ns3::TcpSocketFactory",
                        InetSocketAddress(sinkAddress, sinkPort));
  // Set the amount of data to send in bytes (0 for unlimited else use numPackets).
  source.SetAttribute("MaxBytes", UintegerValue(0));
  source.SetAttribute("SendSize", UintegerValue(packetSize));
  ApplicationContainer apps = source.Install(hostNode);
  apps.Start(Seconds(appStartTime));
  apps.Stop(Seconds(appStopTime));
  return ns3TcpSocket;
}
void setupTopology (){


 // Create links.
  NS_LOG_INFO("Creating links.");
  int mtu = 1500;
  
	std::cout << "Creating channel" << std::endl;
	PointToPointHelper p2pNR, p2pR1R2;
	//Node to Routers
	p2pNR.SetDeviceAttribute("DataRate", StringValue(S_TO_R_BW));
	p2pNR.SetChannelAttribute("Delay", StringValue(S_TO_R_DELAY));
	p2pNR.SetDeviceAttribute ("Mtu", UintegerValue(mtu));

	// R1 to R2 (AP)
	p2pR1R2.SetDeviceAttribute("DataRate", StringValue(R_TO_R_BW));
	p2pR1R2.SetChannelAttribute("Delay", StringValue(R_TO_R_DELAY));
	p2pR1R2.SetQueue("ns3::DropTailQueue",
               "Mode", StringValue ("QUEUE_MODE_PACKETS"),
							 "MaxPackets", UintegerValue(QUEUE_SIZE));
	p2pR1R2.SetDeviceAttribute ("Mtu", UintegerValue(mtu));

	//Adding some errorrate
	double errorP = ERROR;
	std::cout << "Adding some errorrate::"<< errorP << std::endl;
	Ptr<RateErrorModel> em = CreateObjectWithAttributes<RateErrorModel> ("ErrorRate", DoubleValue (errorP));

	NodeContainer routers, senders, wifiStaNodes;
	routers.Create(2);  //R1 and R2
	senders.Create(NUM_FLOWS);  // on the left side
	wifiStaNodes.Create(NUM_FLOWS); // wifi nodes
	

	//Adding links between R1 and R2 routers
	NetDeviceContainer routerDevs = p2pR1R2.Install(routers);

	//Adding links to R1 and R2 routers
	std::cout << "Adding links" << std::endl;
	NetDeviceContainer leftRDevs, rightRDevs, senderDevs, recvDevs;
	NodeContainer wifiApNode = routers.Get (1); //thw access point

	YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
	YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
	phy.SetChannel (channel.Create ());

	WifiHelper wifi;
	wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

	WifiMacHelper mac;
	Ssid ssid = Ssid ("ns-3-ssid");
	mac.SetType ("ns3::StaWifiMac",
				"Ssid", SsidValue (ssid),
				"ActiveProbing", BooleanValue (false));

	NetDeviceContainer staDevices;
	staDevices = wifi.Install (phy, mac, wifiStaNodes);

	mac.SetType ("ns3::ApWifiMac",
				"Ssid", SsidValue (ssid));

	NetDeviceContainer apDevices;
	apDevices = wifi.Install (phy, mac, wifiApNode);

	MobilityHelper mobility;

	mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
									"MinX", DoubleValue (0.0),
									"MinY", DoubleValue (0.0),
									"DeltaX", DoubleValue (5.0),
									"DeltaY", DoubleValue (10.0),
									"GridWidth", UintegerValue (3),
									"LayoutType", StringValue ("RowFirst"));

	mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
								"Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));
	mobility.Install (wifiStaNodes);

	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobility.Install (wifiApNode);

	for(uint i = 0; i < NUM_FLOWS; i++) {
		NetDeviceContainer left = p2pNR.Install(senders.Get(i),routers.Get(0));  // Nodes to R1
		leftRDevs.Add(left.Get(1));
		senderDevs.Add(left.Get(0));
		left.Get(0)->SetAttribute("ReceiveErrorModel", PointerValue(em));

		/*NetDeviceContainer right = p2pNR.Install(routers.Get(1), receivers.Get(i));  //R2 to Nodes
		rightRDevs.Add(right.Get(0));
		recvDevs.Add(right.Get(1));
		right.Get(0)->SetAttribute("ReceiveErrorModel", PointerValue(em));*/
	}
	//Install Internet Stack
	std::cout << "Install Internet Stack" << std::endl;
	InternetStackHelper stack;
	stack.Install(routers);
	stack.Install(senders);
	stack.Install(wifiStaNodes);


	//Adding IP addresses
	std::cout << "Adding IP addresses" << std::endl;
	Ipv4AddressHelper routerIP = Ipv4AddressHelper("10.1.0.0", "255.255.255.0");
	Ipv4AddressHelper senderIP = Ipv4AddressHelper("10.2.0.0", "255.255.255.0");
	Ipv4AddressHelper receiverIP = Ipv4AddressHelper("10.3.0.0", "255.255.255.0");

	Ipv4InterfaceContainer routerIC, senderIC, leftRIFC, rightRIFC;

	// Assigning address to the routers
	routerIC = routerIP.Assign(routerDevs);

	for(uint i = 0; i < NUM_FLOWS; i++) {
		NetDeviceContainer senderDev;
		senderDev.Add(senderDevs.Get(i));
		senderDev.Add(leftRDevs.Get(i));
		Ipv4InterfaceContainer senderIFC = senderIP.Assign(senderDev);
		senderIC.Add(senderIFC.Get(0));
		leftRIFC.Add(senderIFC.Get(1));
		senderIP.NewNetwork();
	}
	
	Ipv4InterfaceContainer wifistaIFCs = receiverIP.Assign(staDevices);
	Ipv4InterfaceContainer wifiapIFCs = receiverIP.Assign(apDevices);
	

	//Turning on  Routing

	Ipv4GlobalRoutingHelper::PopulateRoutingTables();

	Ptr<Socket> n3TCPSocket[NUM_FLOWS];

	uint i=0;
	uint port = 9200;
	//For Creating BBR sockets
	for (; i<NUM_FLOWS_BBR; i++){
		n3TCPSocket[i] = prepareTCPSocket(senderIC.GetAddress(i), 
											port, "ns3::TcpBbr",  wifiStaNodes.Get(i), senders.Get(i), 
											START_TIME, STOP_TIME,
	 										PACKET_SIZE, 0, START_TIME+3, STOP_TIME,i); 
											 // START_TIME or DELAYED_START can be used

		
	}
	std::cout << "No. of BBR flows created" << i << std::endl;

	for (; i<NUM_FLOWS_BBR + NUM_FLOWS_CUBIC; i++){
		n3TCPSocket[i] = prepareTCPSocket(senderIC.GetAddress(i), 
											port, "ns3::TcpCubic", wifiStaNodes.Get(i), senders.Get(i), 
											START_TIME, STOP_TIME,
	 										PACKET_SIZE, 0, START_TIME+3, STOP_TIME,i); // START_TIME or DELAYED_START can be used

	}
	std::cout << "No. of BBR + CUBIC flows created" << i << std::endl;

	for (; i<NUM_FLOWS; i++){
		n3TCPSocket[i] = prepareTCPSocket(senderIC.GetAddress(i), 
											port, "ns3::TcpNewReno",  wifiStaNodes.Get(i), senders.Get(i), 
											START_TIME, STOP_TIME,
	 										PACKET_SIZE, 0, START_TIME+3, STOP_TIME,i);

	}
	std::cout << "No. of BBR + CUBIC+RENO flows created" << i << std::endl;
	NS_LOG_INFO("Running simulation.");
	p2pNR.EnablePcapAll("wdrshark",true);
	p2pR1R2.EnablePcapAll("wdr1r2",true);
	phy.EnablePcapAll ("third", true);
	//wifiStaNodes.EnablePcapAll("third", true);
	Simulator::Schedule (Seconds (0.1), &CalculateThroughput);
	Simulator::Schedule (Seconds (0.00001), &TraceRtt, prefix_file_name + "-rtt.data");
  
  Simulator::Stop(Seconds(STOP_TIME));
  NS_LOG_INFO("Simulation time: [" << 
              START_TIME << "," <<
              STOP_TIME << "]");
  NS_LOG_INFO("---------------- Start -----------------------");
  AnimationInterface anim ("test_dumble_wireless.xml");
  Simulator::Run();
  NS_LOG_INFO("---------------- Stop ------------------------");
	// Done.
  Simulator::Destroy();
}

int main(int argc, char **argv) {
	LogComponentEnable("Dumble", LOG_LEVEL_INFO);
	setupTopology();
	return 0;
}  
