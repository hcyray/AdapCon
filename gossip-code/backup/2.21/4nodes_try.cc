#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/pointer.h"
#include <vector>
#include <set>
#include <tuple>
#include <string>
// #include "variable-init.h"


using namespace ns3;


NS_LOG_COMPONENT_DEFINE("4NodesTry");

// class ContainerOfData{
// public:
// 	std::set<Ptr<Packet>> Data_Received;
// 	std::vector<std::tuple<Ptr<Packet>, Ptr<Node>>> Data_to_Send;

// 	// ContainerOfData();
// 	// ~ContainerOfData();
// };

// std::set<Ptr<Packet>> Data_Received;
// std::vector<std::tuple<Ptr<Packet>, Ptr<Node>>> Data_to_Send;


int main()
{
	Time::SetResolution(Time::NS);
	LogComponentEnable("GossipAppApplication", LOG_LEVEL_INFO);
	LogComponentEnable("GossipAppApplication", LOG_LEVEL_INFO);

	NodeContainer nodeslist;
	nodeslist.Create(4);
	NodeContainer router;
	router.Create(1);

	
//////////////////////////////////////////////////////


//////////////////////////////////////////////////////

	NodeContainer subnet1;
	subnet1.Add(nodeslist.Get(0));
	subnet1.Add(router.Get(0));

	PointToPointHelper pointToPoint1;
	pointToPoint1.SetDeviceAttribute("DataRate", StringValue("100kBps"));
	pointToPoint1.SetChannelAttribute("Delay", StringValue("0ms"));
	NetDeviceContainer p2pDevice1;
	p2pDevice1 = pointToPoint1.Install(subnet1);


	NodeContainer subnet2;
	subnet2.Add(nodeslist.Get(1));
	subnet2.Add(router.Get(0));

	PointToPointHelper pointToPoint2;
	pointToPoint2.SetDeviceAttribute("DataRate", StringValue("100KBps"));
	pointToPoint2.SetChannelAttribute("Delay", StringValue("0ms"));
	NetDeviceContainer p2pDevice2;
	p2pDevice2 = pointToPoint2.Install(subnet2);


	NodeContainer subnet3;
	subnet3.Add(nodeslist.Get(2));
	subnet3.Add(router.Get(0));

	PointToPointHelper pointToPoint3;
	pointToPoint3.SetDeviceAttribute("DataRate", StringValue("100KBps"));
	pointToPoint3.SetChannelAttribute("Delay", StringValue("0ms"));
	NetDeviceContainer p2pDevice3;
	p2pDevice3 = pointToPoint3.Install(subnet3);


	NodeContainer subnet4;
	subnet4.Add(nodeslist.Get(3));
	subnet4.Add(router.Get(0));

	PointToPointHelper pointToPoint4;
	pointToPoint4.SetDeviceAttribute("DataRate", StringValue("200KBps"));
	pointToPoint4.SetChannelAttribute("Delay", StringValue("0ms"));
	NetDeviceContainer p2pDevice4;
	p2pDevice4 = pointToPoint4.Install(subnet4);

//////////////////////////////////////////////////////

	InternetStackHelper stack;
	stack.Install(nodeslist);
	stack.Install(router);

	Ipv4AddressHelper address;
	address.SetBase("10.1.1.0", "255.255.255.0");
	Ipv4InterfaceContainer p2pInterface1 = address.Assign(p2pDevice1); 
	address.SetBase("10.1.2.0", "255.255.255.0");
	Ipv4InterfaceContainer p2pInterface2 = address.Assign(p2pDevice2);
	address.SetBase("10.1.3.0", "255.255.255.0");
	Ipv4InterfaceContainer p2pInterface3 = address.Assign(p2pDevice3);
 	address.SetBase("10.1.4.0", "255.255.255.0");
 	Ipv4InterfaceContainer p2pInterface4 = address.Assign(p2pDevice4);


	
//////////////////////////////////////////////////////

	GossipAppHelper gossipApp1 (0, 17);
    ApplicationContainer serverApps1 = gossipApp1.Install (nodeslist.Get (0));
    serverApps1.Start (Seconds (1.0));
    serverApps1.Stop (Seconds (10000.0));

    // UdpCommunicateClientHelper communicateClient1 (p2pInterface1.GetAddress (0), 17);
    // communicateClient1.SetAttribute ("MaxPackets", UintegerValue (1));
    // communicateClient1.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
    // communicateClient1.SetAttribute ("PacketSize", UintegerValue (10240));
    // ApplicationContainer clientApps1 = communicateClient1.Install (nodeslist.Get (1));
    // clientApps1.Start (Seconds (20.0));
    // clientApps1.Stop (Seconds (10000.0));



    GossipAppHelper gossipApp2 (1, 17);
    ApplicationContainer serverApps2 = gossipApp2.Install (nodeslist.Get (1));
    serverApps2.Start (Seconds (1.0));
    serverApps2.Stop (Seconds (10000.0));

    // UdpCommunicateClientHelper communicateClient2 (p2pInterface1.GetAddress (0), 17);
    // std::cout<<p2pInterface2.GetAddress(0)<<std::endl;
    // communicateClient2.SetAttribute ("MaxPackets", UintegerValue (3));
    // communicateClient2.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
    // communicateClient2.SetAttribute ("PacketSize", UintegerValue (10240));
    // ApplicationContainer clientApps2 = communicateClient2.Install (nodeslist.Get (1));
    // clientApps2.Start (Seconds (15.0));
    // clientApps2.Stop (Seconds (20000.0));



    GossipAppHelper gossipApp3 (2, 17);
    ApplicationContainer serverApps3 = gossipApp3.Install (nodeslist.Get (2));
    serverApps3.Start (Seconds (1.0));
    serverApps3.Stop (Seconds (10000.0));

    // UdpCommunicateClientHelper communicateClient3 (p2pInterface1.GetAddress (0), 17);
    // communicateClient3.SetAttribute ("MaxPackets", UintegerValue (1));
    // communicateClient3.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
    // communicateClient3.SetAttribute ("PacketSize", UintegerValue (10240));
    // ApplicationContainer clientApps3 = communicateClient3.Install (nodeslist.Get (2));
    // clientApps3.Start (Seconds (20.0));
    // clientApps3.Stop (Seconds (20000.0));

    // UdpCommunicateClientHelper communicateClient4 (p2pInterface1.GetAddress (0), 17);
    // communicateClient4.SetAttribute ("MaxPackets", UintegerValue (1));
    // communicateClient4.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
    // communicateClient4.SetAttribute ("PacketSize", UintegerValue (10240));
    // ApplicationContainer clientApps4 = communicateClient4.Install (nodeslist.Get (3));
    // clientApps4.Start (Seconds (30.0));
    // clientApps4.Stop (Seconds (20000.0));


//////////////////////////////////////////////////////

	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
	Simulator::Run ();
    Simulator::Destroy ();

    return 0;
	
}