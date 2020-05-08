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
#include "ns3/queue.h"
#include "ns3/netanim-module.h"
#include "ns3/drop-tail-queue.h"

typedef uint32_t uint;

#define NUM_FLOWS 2   // change this number to create different leaves on each side of dumble
#define NUM_FLOWS_BBR 1   // change this number to create different leaves on each side of dumble
#define NUM_FLOWS_CUBIC 1   // change this number to create different leaves on each side of dumble
#define NUM_FLOWS_RENO 0   // change this number to create different leaves on each side of dumble
#define TCP_BBR_PROTOCOL     "ns3::TcpBbr"

// Constants.
#define ENABLE_PCAP      true     // Set to "true" to enable pcap
#define ENABLE_TRACE     true     // Set to "true" to enable trace
#define BIG_QUEUE        20000      // Packets
#define QUEUE_SIZE       20       // Packets =BDP
#define QUEUE_SIZE_P     "10p"       // Packets =BDP
#define START_TIME       0.0       // Seconds
#define DELAYED_START  	 0.0			// Any flow with delayed start
#define STOP_TIME        400.0       // Seconds
#define S_TO_R_BW        "1Gbps" // node to router
#define S_TO_R_DELAY     "1ms"
#define R_TO_R_BW        "10Mbps"  // Router to router (bttlneck)
#define R_TO_R_DELAY     "20ms"
#define R_TO_C_BW        "10Mbps"  // Router to router (bttlneck)
#define R_TO_C_DELAY     "1ms"
#define PACKET_SIZE      1000      // Bytes.
#define ERROR            0.000001
//#define QNR  (150000*10)/PACKET_SIZE //Delay Bandwidth Product / packetsize
//#define QR1R2 (10000*1)/PACKET_SIZE //Delay Bandwidth Product / packetsize

using namespace ns3;
#define STRINGIZE_(x) #x
#define STRINGIZE(x) STRINGIZE_(x)

NS_LOG_COMPONENT_DEFINE ("Dumble");
std::string d=  STRINGIZE_(10);
std::string e=  STRINGIZE_(2);
std::string directory=  "dumble_wired/";


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
 void
 CalculateTQ ()
 {
   Time now = Simulator::Now ();                                         /* Return the simulator's virtual time. */
       
   std::cout << now.GetSeconds () << "s: \t" << qwe->GetNPackets() << std::endl;
     
   Simulator::Schedule (MilliSeconds (100), &CalculateTQ);
 }

void prepareTCPSocket(ns3::Ipv4Address sinkAddress, 
					uint sinkPort, 
					std::string tcpVariant, 
					Ptr<Node> hostNode, 
					Ptr<Node> sinkNode, 
					double startTime, 
					double stopTime,
					uint packetSize,
					uint numPackets,
					double appStartTime,
					double appStopTime,
					int i_val
					) {
	
	std::cout<<tcpVariant<<std::endl;
	TypeId tid = TypeId::LookupByName (tcpVariant);
	std::stringstream nodeId;
		nodeId << hostNode->GetId ();
	std::string specificNode = "/NodeList/" + nodeId.str () + "/$ns3::TcpL4Protocol/SocketType";
	Config::Set (specificNode, TypeIdValue (tid));
	//Ptr<Socket> localSocket = Socket::CreateSocket (hostNode, TcpSocketFactory::GetTypeId ());

	PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory",
     InetSocketAddress(Ipv4Address::GetAny(), sinkPort));
	ApplicationContainer sinkApps = packetSinkHelper.Install(sinkNode);
	sinkApps.Start(Seconds(startTime));
	sinkApps.Stop(Seconds(stopTime));
	Ptr<PacketSink> p_sink = DynamicCast<PacketSink> (sinkApps.Get(0)); // 4 stats
  	sinkp[i_val]=p_sink;
	lastTotalRx[i_val]=0;
	//Ptr<Socket> ns3TcpSocket = Socket::CreateSocket(hostNode, TcpSocketFactory::GetTypeId());

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
	Ptr<BulkSendApplication>bapp= hostNode->GetApplication(0)->GetObject<BulkSendApplication>();
	//bapp->SetMtid(tid);
}
void ThroughputMonitor (FlowMonitorHelper *fmhelper, Ptr<FlowMonitor> flowMon,Gnuplot2dDataset DataSet);
int main(int argc, char **argv) {
	
	LogComponentEnable ("Dumble",LOG_LEVEL_ALL);
	//Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpBbr"));
	Config::SetDefault("ns3::TcpSocket::SegmentSize",
                     UintegerValue(PACKET_SIZE)); 
	//Config::SetDefault ("ns3::PfifoFastQueueDisc::Limit", UintegerValue (QUEUE_SIZE));
	NS_LOG_INFO("Creating links.");
  	int mtu = 1500;
  	Config::SetDefault("ns3::TcpSocket::DelAckCount", UintegerValue(0));

	std::cout << "Creating channel" << std::endl;
	PointToPointHelper p2pNR, p2pR1R2,p2pRC;
	//Node to Routers
	p2pNR.SetDeviceAttribute("DataRate", StringValue(S_TO_R_BW));
	p2pNR.SetChannelAttribute("Delay", StringValue(S_TO_R_DELAY));
	p2pNR.SetDeviceAttribute ("Mtu", UintegerValue(mtu));
	
	p2pRC.SetDeviceAttribute("DataRate", StringValue(R_TO_C_BW));
	p2pRC.SetChannelAttribute("Delay", StringValue(R_TO_C_DELAY));
	p2pRC.SetDeviceAttribute ("Mtu", UintegerValue(mtu));
	
	

	// R1 to R2
	p2pR1R2.SetDeviceAttribute("DataRate", StringValue(R_TO_R_BW));
	p2pR1R2.SetChannelAttribute("Delay", StringValue(R_TO_R_DELAY));
	/*p2pR1R2.SetQueue("ns3::DropTailQueue",
               "Mode", StringValue ("QUEUE_MODE_PACKETS"),
							 "MaxPackets", UintegerValue(QUEUE_SIZE));
	p2pR1R2.SetDeviceAttribute ("Mtu", UintegerValue(mtu));*/


	//Adding some errorrate
	double errorP = ERROR;
	std::cout << "Adding some errorrate::"<< errorP << std::endl;
	Ptr<RateErrorModel> em = CreateObjectWithAttributes<RateErrorModel> ("ErrorRate", DoubleValue (errorP));

	NodeContainer routers, senders, receivers;
	routers.Create(2);  //R1 and R2
	senders.Create(NUM_FLOWS);  // on the left side
	receivers.Create(NUM_FLOWS); // on the left side

	NetDeviceContainer routerDevices = p2pR1R2.Install(routers);
	//Empty netdevicecontatiners
	NetDeviceContainer leftRouterDevices, rightRouterDevices, senderDevices, receiverDevices;

	//Adding links
	std::cout << "Adding links" << std::endl;
	for(uint i = 0; i < NUM_FLOWS; ++i) {
		NetDeviceContainer cleft = p2pNR.Install(routers.Get(0), senders.Get(i));
		leftRouterDevices.Add(cleft.Get(0));
		senderDevices.Add(cleft.Get(1));
		cleft.Get(0)->SetAttribute("ReceiveErrorModel", PointerValue(em));

		NetDeviceContainer cright = p2pRC.Install(routers.Get(1), receivers.Get(i));
		rightRouterDevices.Add(cright.Get(0));
		receiverDevices.Add(cright.Get(1));
		cright.Get(0)->SetAttribute("ReceiveErrorModel", PointerValue(em));
	}
	Ptr<PointToPointNetDevice> device = routerDevices.Get(0)->GetObject<PointToPointNetDevice> ();
  	qwe = device->GetQueue(); 
	qwe->SetAttribute("Mode",StringValue ("QUEUE_MODE_PACKETS"));
	qwe->SetAttribute("MaxPackets",UintegerValue (QUEUE_SIZE));

	
	std::cout << "---------------Queueu Size:" << qwe->GetMaxPackets()<< std::endl;


	//Install Internet Stack
	std::cout << "Install Internet Stack" << std::endl;
	InternetStackHelper stack;
	stack.Install(routers);
	stack.Install(senders);
	stack.Install(receivers);


	//Adding IP addresses
	std::cout << "Adding IP addresses" << std::endl;
	Ipv4AddressHelper routerIP = Ipv4AddressHelper("10.3.0.0", "255.255.255.0");	//(network, mask)
	Ipv4AddressHelper senderIP = Ipv4AddressHelper("10.1.0.0", "255.255.255.0");
	Ipv4AddressHelper receiverIP = Ipv4AddressHelper("10.2.0.0", "255.255.255.0");
	

	Ipv4InterfaceContainer routerIFC, senderIFCs, receiverIFCs, leftRouterIFCs, rightRouterIFCs;

	//Assign IP addresses to the net devices specified in the container 
	//based on the current network prefix and address base
	routerIFC = routerIP.Assign(routerDevices);

	for(uint i = 0; i < NUM_FLOWS; ++i) {
		NetDeviceContainer senderDevice;
		senderDevice.Add(senderDevices.Get(i));
		senderDevice.Add(leftRouterDevices.Get(i));
		Ipv4InterfaceContainer senderIFC = senderIP.Assign(senderDevice);
		senderIFCs.Add(senderIFC.Get(0));
		leftRouterIFCs.Add(senderIFC.Get(1));
		//Increment the network number and reset the IP address counter 
		//to the base value provided in the SetBase method.
		senderIP.NewNetwork();

		NetDeviceContainer receiverDevice;
		receiverDevice.Add(receiverDevices.Get(i));
		receiverDevice.Add(rightRouterDevices.Get(i));
		Ipv4InterfaceContainer receiverIFC = receiverIP.Assign(receiverDevice);
		receiverIFCs.Add(receiverIFC.Get(0));
		rightRouterIFCs.Add(receiverIFC.Get(1));
		receiverIP.NewNetwork();
	}

	Ipv4GlobalRoutingHelper::PopulateRoutingTables();

	//Ptr<Socket> n3TCPSocket[NUM_FLOWS];

	uint i=0;
	uint port = 911;
	//For Creating BBR sockets
	
	for (; i<NUM_FLOWS_BBR; i++){			
		prepareTCPSocket(receiverIFCs.GetAddress(i), 
											port, "ns3::TcpBbr", senders.Get(i), receivers.Get(i), 
											START_TIME, STOP_TIME,
	 										PACKET_SIZE, 0, START_TIME, STOP_TIME,i);
	
		
	}
	std::cout << "No. of BBR flows created" << i << std::endl;

	for (; i<NUM_FLOWS_BBR + NUM_FLOWS_CUBIC; i++){
		 prepareTCPSocket(receiverIFCs.GetAddress(i), 
											port, "ns3::TcpCubic", senders.Get(i), receivers.Get(i), 
											START_TIME, STOP_TIME,
	 										PACKET_SIZE, 0, START_TIME, STOP_TIME,i); // START_TIME or DELAYED_START can be used

	}
	std::cout << "No. of BBR + CUBIC flows created" << i << std::endl;

	for (; i<NUM_FLOWS; i++){
		 prepareTCPSocket(receiverIFCs.GetAddress(i), 
											port, "ns3::TcpNewReno", senders.Get(i), receivers.Get(i), 
											START_TIME, STOP_TIME,
	 										PACKET_SIZE, 0, START_TIME, STOP_TIME,i);

	}
	std::cout << "No. of BBR + CUBIC+RENO flows created" << i << std::endl;
	NS_LOG_INFO("Running simulation.");
	Simulator::Schedule (Seconds (0.1), &CalculateThroughput);
	//Simulator::Schedule (Seconds (0.1), &CalculateTQ);
	std::string  prefix_file_name="dumble_";
	std::string dirToSave = "mkdir -p " + directory;
  	system (dirToSave.c_str ());
//	std::ofstream ascii;
//    Ptr<OutputStreamWrapper> ascii_wrap;
//    ascii.open ((prefix_file_name + "-ascii").c_str ());
//    ascii_wrap = new OutputStreamWrapper ((directory+prefix_file_name + "-ascii").c_str (),
//                                            std::ios::out);
//    stack.EnableAsciiIpv4All (ascii_wrap);
	NS_LOG_INFO("Enabling pcap files.");
    p2pNR.EnablePcapAll(directory+prefix_file_name+"p2pNR", true);
	//p2pR1R2.EnablePcapAll(directory+prefix_file_name+"p2pR1R2", true);
	
	
/* 	std::string fileNameWithNoExtension = "FlowVSThroughput_Dumble";
    std::string graphicsFileName        = fileNameWithNoExtension + ".png";
    std::string plotFileName            = fileNameWithNoExtension + ".plt";
    std::string plotTitle               = "Flow vs Throughput";
    std::string dataTitle               = "Throughput";

    // Instantiate the plot and set its title.
    Gnuplot gnuplot (graphicsFileName);
    gnuplot.SetTitle (plotTitle);

    // Make the graphics file, which the plot file will be when it
    // is used with Gnuplot, be a PNG file.
    gnuplot.SetTerminal ("png");

    // Set the labels for each axis.
    gnuplot.SetLegend ("Flow", "Throughput");

     
   Gnuplot2dDataset dataset;
   dataset.SetTitle (dataTitle);
   dataset.SetStyle (Gnuplot2dDataset::LINES_POINTS);

  //flowMonitor declaration
  FlowMonitorHelper fmHelper;
  Ptr<FlowMonitor> allMon = fmHelper.InstallAll(); */
  // call the flow monitor function
  //ThroughputMonitor(&fmHelper, allMon, dataset);
	


  Simulator::Stop(Seconds(STOP_TIME));
  NS_LOG_INFO("Simulation time: [" << 
              START_TIME << "," <<
              STOP_TIME << "]");
  NS_LOG_INFO("---------------- Start -----------------------");
  //AnimationInterface anim ("test_dumble.xml");
  Simulator::Run();
  NS_LOG_INFO("---------------- Stop ------------------------");
	// Done.
/* 
	//Gnuplot ...continued
  gnuplot.AddDataset (dataset);
  // Open the plot file.
  std::ofstream plotFile (plotFileName.c_str());
  // Write the plot file.
  gnuplot.GenerateOutput (plotFile);
  // Close the plot file.
  plotFile.close ();

  allMon->SerializeToXmlFile("test_dumble_2_flowmon.xml", true, true); */
  Simulator::Destroy();

	
	return 0;
}  
void ThroughputMonitor (FlowMonitorHelper *fmhelper, Ptr<FlowMonitor> flowMon,Gnuplot2dDataset DataSet)
	{
        double localThrou=0;
		std::map<FlowId, FlowMonitor::FlowStats> flowStats = flowMon->GetFlowStats();
		Ptr<Ipv4FlowClassifier> classing = DynamicCast<Ipv4FlowClassifier> (fmhelper->GetClassifier());
		for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator stats = flowStats.begin (); stats != flowStats.end (); ++stats)
		{
			Ipv4FlowClassifier::FiveTuple fiveTuple = classing->FindFlow (stats->first);
			std::cout<<"Flow ID			: " << stats->first <<" ; "<< fiveTuple.sourceAddress <<" -----> "<<fiveTuple.destinationAddress<<std::endl;
			std::cout<<"Tx Packets = " << stats->second.txPackets<<std::endl;
			std::cout<<"Rx Packets = " << stats->second.rxPackets<<std::endl;
            std::cout<<"Duration		: "<<(stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())<<std::endl;
			std::cout<<"Last Received Packet	: "<< stats->second.timeLastRxPacket.GetSeconds()<<" Seconds"<<std::endl;
			std::cout<<"Throughput: " << stats->second.rxBytes * 8.0 / (stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())/1024/1024  << " Mbps"<<std::endl;
            localThrou=(stats->second.rxBytes * 8.0 / (stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())/1024/1024);
			// updata gnuplot data
            DataSet.Add((double)Simulator::Now().GetSeconds(),(double) localThrou);
			std::cout<<"---------------------------------------------------------------------------"<<std::endl;
		}
			Simulator::Schedule(Seconds(1),&ThroughputMonitor, fmhelper, flowMon,DataSet);
   //if(flowToXml)
      {
	flowMon->SerializeToXmlFile ("ThroughputMonitorDumble.xml", true, true);
      }

	}
