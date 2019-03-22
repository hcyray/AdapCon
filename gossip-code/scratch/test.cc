#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/pointer.h"
#include <string>

using namespace ns3;

int main(){

	Config::SetDefault("ns3::TcpSocket::SegmentSize", StringValue("4096"));

	std::string LinkRate ("1kBps");
	std::string LinkDelay ("0ms");

	NodeContainer Nodes;
	Nodes.Create(2);

	PointToPointHelper p2p;
	p2p.SetDeviceAttribute ("DataRate", StringValue (LinkRate));
	p2p.SetChannelAttribute ("Delay", StringValue (LinkDelay));

	NetDeviceContainer p2pdevice = p2p.Install(Nodes);


	InternetStackHelper stack;
	stack.Install(Nodes);

	Ipv4AddressHelper address;
	address.SetBase ("10.1.1.0", "255.255.255.0");
	Ipv4InterfaceContainer interface;
	interface = address.Assign(p2pdevice);

	// UdpEchoServerHelper echoServer (19);
	// ApplicationContainer serverApps = echoServer.Install (Nodes.Get(0));
	// serverApps.Start (Seconds (1.0));
	// serverApps.Stop (Seconds (10.0));

	// UdpEchoClientHelper echoClient (interface.GetAddress(0), 19);
	// echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
	// echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
	// echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

	// ApplicationContainer clientApps = echoClient.Install (Nodes.Get(1));
	// clientApps.Start (Seconds (2.0));
	// clientApps.Stop (Seconds (10.0));

	// ServerTrafficHelper serverTraffic (109);
	// ApplicationContainer serverTrafficApp = serverTraffic.Install (Nodes.Get(0));
	// serverTrafficApp.Start (Seconds (0.0));
	// serverTrafficApp.Stop (Seconds (23.0));

	// UserTrafficHelper userTraffic (interface.GetAddress(0), 109);
	// ApplicationContainer userTrafficApp = userTraffic.Install (Nodes.Get(1));
	// userTrafficApp.Start (Seconds (1.0));
	// userTrafficApp.Stop (Seconds (30.0));

	// UserTrafficHelper userTraffic (interface.GetAddress(1), 109);
	// ApplicationContainer userTrafficApp = userTraffic.Install (Nodes.Get(1));
	// userTrafficApp.Start (Seconds (1.0));
	// userTrafficApp.Stop (Seconds (20.0));

	// double x=7.33332;
	// x = int(x*100)/100.;
	// std::cout<<x<<"*****"<<std::endl;
	// std::string sy = std::to_string(x);
	// std::cout<<sy<<"****"<<std::endl;
	// std::cout<<std::stod(sy)<<"*****"<<std::endl;
	
	Simulator::Stop(Seconds(220.0));
	Simulator::Run ();
    Simulator::Destroy ();
	return 0;
}


