#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/pointer.h"
#include "ns3/gossip-app.h"
#include <vector>
#include <set>
#include <tuple>
#include <string>

using namespace ns3;



NS_LOG_COMPONENT_DEFINE("HundredNodesTry");

int main()
{
	Time::SetResolution(Time::NS);
	LogComponentEnable("GossipAppApplication", LOG_LEVEL_INFO);

	NodeContainer nodeslist;
	nodeslist.Create(NODE_NUMBER);
	NodeContainer router;
	router.Create(1);

	NodeContainer subnetlist[NODE_NUMBER];
	PointToPointHelper pointToPointlist[NODE_NUMBER];
	NetDeviceContainer p2pDevicelist[NODE_NUMBER];
	Ipv4AddressHelper addresslist[NODE_NUMBER];
	Ipv4InterfaceContainer p2pInterfacelist[NODE_NUMBER];
	ApplicationContainer applicationlist[NODE_NUMBER];

	for(int i=0; i<NODE_NUMBER; i++)
	{
		
		subnetlist[i].Add(nodeslist.Get(i));
		subnetlist[i].Add(router.Get(0));

		pointToPointlist[i].SetDeviceAttribute("DataRate", StringValue("10kBps"));
		pointToPointlist[i].SetChannelAttribute("Delay", StringValue("0ms"));
		p2pDevicelist[i] = pointToPointlist[i].Install(subnetlist[i]);

	}

	InternetStackHelper stack;
	stack.Install(nodeslist);
	stack.Install(router);

	for(int i=0; i<NODE_NUMBER; i++)
	{
		std::string ipaddress_string = "10.1.";
		ipaddress_string.append(std::to_string(i+1));
		ipaddress_string.append(".0");
		char* ipaddress = (char*)ipaddress_string.data();
		addresslist[i].SetBase(Ipv4Address(ipaddress), "255.255.255.0");
		p2pInterfacelist[i] = addresslist[i].Assign(p2pDevicelist[i]);

	}

	for(int i=0; i<NODE_NUMBER; i++)
	{
		
		GossipAppHelper gossipApp1(i, 17);
		applicationlist[i] = gossipApp1.Install (nodeslist.Get (i));
	    applicationlist[i].Start (Seconds (0.));
	    applicationlist[i].Stop (Seconds (10000.0));
	}

	// GossipAppHelper gossipApp1 (0, 17);
 //    ApplicationContainer serverApps1 = gossipApp1.Install (nodeslist.Get (0));
 //    serverApps1.Start (Seconds (1.0));
 //    serverApps1.Stop (Seconds (20.0));

 //    GossipAppHelper gossipApp2 (1, 17);
 //    ApplicationContainer serverApps2 = gossipApp2.Install (nodeslist.Get (1));
 //    serverApps2.Start (Seconds (1.0));
 //    serverApps2.Stop (Seconds (20.0));


    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
	Simulator::Run ();
    Simulator::Destroy ();

    





    return 0;
}
