#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/global-route-manager.h"
#include "ns3/pointer.h"
#include "ns3/gossip-app.h"
#include <vector>
#include <set>
#include <tuple>
#include <string>
#include<time.h>

using namespace ns3;


// **************************************************　preprocessing functions
std::vector<std::string> SplitString(const std::string& str, const char pattern)
{

	

    std::vector<std::string> res;
    std::stringstream input(str);   
    std::string temp;
    while(getline(input, temp, pattern))
    {
        res.push_back(temp);
    }
    return res;
}

int Get_Topology(std::map<int, int>& map_AP_devicenumber, 
	std::vector<std::pair<int, int> >& vecotr_edge)
{	
	int totalIotdevice = 0;
    std::ifstream infile;
    infile.open("scratch/subdir/topologydata/topology.txt");
    const int LINE_LENGTH = 100; 
    char str1[LINE_LENGTH];
    while(infile.getline(str1,LINE_LENGTH))
    {
        if(strcmp(str1, "AP and attached device number")==0)
            continue;
        if(strcmp(str1, "edge between AP")==0)
            break;
        std::string str2(str1);
        std::vector<std::string> res = SplitString(str2, ' ');
        if(res.size()==2)
        {
            int x = (atoi)(res[0].c_str());
            int y = (atoi)(res[1].c_str());
            map_AP_devicenumber[x] = y;
            totalIotdevice += y;
        }
        else
            continue;        
    }
    
    while(infile.getline(str1,LINE_LENGTH))
    {
        std::string str2(str1);
        std::vector<std::string> res = SplitString(str2, ' ');
        if(res.size()==2)
        {
            std::pair<int, int> p;
            p.first = (atoi)(res[0].c_str());
            p.second = (atoi)(res[1].c_str());
            vecotr_edge.push_back(p);
        }
        else
            continue;
    }

    return totalIotdevice;    
}

std::map<int, std::pair<int, int> > NodeNumberToApNodeNumber(int IotNodeNumber, std::map<int, int> map_AP_devicenumber)
{
	std::map<int, std::pair<int, int> > res;
	int n=0;
	int completed_n=0;
	int i=0;
	int j=0;
	while(n < IotNodeNumber)
	{
		if(n-completed_n<map_AP_devicenumber[i])
		{
			std::pair<int, int> p;
			p.first = i;
			p.second = j;
			res[n] = p;
			j++;
			n++;
		}
		else
		{
			completed_n += j;
			i++;
			j = 0;
		}
	}
	return res;
}


void bandwidth_vary()
{
	DataRate rate("0.5MBps");
	
	for(int i=0; i<AP_NUMBER*2; i++)
	{
		std::string str = "/NodeList/";
		str.append(std::to_string(i));
		str.append("/DeviceList/0/$ns3::PointToPointNetDevice/DataRate");
		Config::Set(str, DataRateValue(rate));
	}
	std::cout<<"bandwidth changed at "<<Simulator::Now().GetSeconds()<<"s"<<std::endl;
}



NS_LOG_COMPONENT_DEFINE("HundredNodesTry");

int main()
{
	clock_t start,finish;
    double totaltime;
    start=clock();

	Time::SetResolution(Time::NS);
	LogComponentEnable("GossipAppApplication", LOG_LEVEL_INFO);
	LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
	LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
	Config::SetDefault("ns3::TcpSocket::SegmentSize", StringValue("4096"));
	Config::SetDefault("ns3::TcpSocket::SndBufSize", StringValue("1048576"));
	// **************************************************read topology from txt file
	std::map<int, int> map_AP_devicenumber;
    std::vector<std::pair<int, int> > vecotr_edge;

    int IotNodeNumber = Get_Topology(map_AP_devicenumber, vecotr_edge);
	int ApNumber = map_AP_devicenumber.size();
	std::cout<<"total Aps are: "<<ApNumber<<std::endl;
    std::cout<<"total IoT devices are: "<<IotNodeNumber<<std::endl;
	std::map<int, std::pair<int, int> > map_NodeNumberToApNodeNumber = NodeNumberToApNodeNumber(IotNodeNumber, map_AP_devicenumber);
    // for(int i=0; i<IotNodeNumber; i++)
	// {
	// 	std::pair<int, int> p = map_NodeNumberToApNodeNumber[i];
	// 	std::cout<<i<<"<----"<<p.first<<" "<<p.second<<std::endl;
	// }
	// for(int i=0; i<40; i++)
    //     std::cout<<map_AP_devicenumber[i]<<"  ";
    // std::cout<<std::endl;
    // for(int i=0; i<(int)vecotr_edge.size(); i++)
    // {
    //     std::cout<<vecotr_edge[i].first<<" "<<vecotr_edge[i].second<<std::endl;
    // }
// ************************************************** create nodes

    NodeContainer AP;
    NodeContainer Router;
	NodeContainer UserNode;
    NodeContainer IotNode[AP_NUMBER];

    AP.Create(AP_NUMBER);
    Router.Create(AP_NUMBER);
    UserNode.Create(AP_NUMBER);
    for(int i=0; i<AP_NUMBER; i++)
    {
    	IotNode[i].Create(map_AP_devicenumber[i]);
    }


// ************************************************** set p2p link between router and AP
    NodeContainer subnetap2rlist[AP_NUMBER];
    PointToPointHelper p2phelper;
    NetDeviceContainer p2pdevicelist1[AP_NUMBER];
	srand(time(NULL));
    for(int i=0; i<AP_NUMBER; i++)
	{
		subnetap2rlist[i].Add(Router.Get(i));
		subnetap2rlist[i].Add(AP.Get(i));
		// int x = rand() % 4;
		int x = i % 4;
		std::cout<<"Ap "<<i<<" class: "<<x<<std::endl;
		if(x==0)
			p2phelper.SetDeviceAttribute("DataRate", StringValue("1MBps"));
		else if(x==1)
			p2phelper.SetDeviceAttribute("DataRate", StringValue("30Mbps"));
		else if(x==2)
			p2phelper.SetDeviceAttribute("DataRate", StringValue("50Mbps"));
		else
			p2phelper.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
		p2phelper.SetChannelAttribute("Delay", StringValue("0ms"));
		p2pdevicelist1[i] = p2phelper.Install(subnetap2rlist[i]);

	}




// ************************************************** set p2p link between routers according to the topology

	int edgenumber = (int)vecotr_edge.size();
    NodeContainer subnetr2rlist[edgenumber];
    NetDeviceContainer p2pdevicelist2[edgenumber];
    for(int i=0; i<edgenumber; i++)
	{
		subnetr2rlist[i].Add(Router.Get(vecotr_edge[i].first));
		subnetr2rlist[i].Add(Router.Get(vecotr_edge[i].second));

		p2phelper.SetDeviceAttribute("DataRate", StringValue("1GBps"));
		p2phelper.SetChannelAttribute("Delay", StringValue("0ms"));
		p2pdevicelist2[i] = p2phelper.Install(subnetr2rlist[i]);
	}

// ************************************************** set p2p link between AP and attatched Iotdevices
	// NodeContainer subnetap2dlist[AP_NUMBER];
	// NetDeviceContainer p2pdevicelist3[AP_NUMBER];
	// do not want to change to this at present


// ************************************************** set wifi link between AP and attatched Iotdevices


	NodeContainer subnetwifistalist[AP_NUMBER];
	NetDeviceContainer stadevices[AP_NUMBER];
	NetDeviceContainer apdevices[AP_NUMBER];
	for(int i=0; i<AP_NUMBER; i++)
	{
		subnetwifistalist[i] = IotNode[i];
		subnetwifistalist[i].Add(UserNode.Get(i));

		std::string wifiname = "AP-";
		wifiname.append(std::to_string(i));
		wifiname.append("-ssid");

		Ssid ssid = Ssid(wifiname);
		WifiHelper wifi;
		wifi.SetRemoteStationManager("ns3::AarfWifiManager");

		YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
		YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
		phy.SetChannel (channel.Create ());

		WifiMacHelper mac;
		mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid),
			"ActiveProbing", BooleanValue(false));;
		stadevices[i] = wifi.Install(phy, mac, subnetwifistalist[i]);
		mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
		apdevices[i] = wifi.Install(phy, mac, AP.Get(i));

		MobilityHelper mobility;
		mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
		mobility.Install(subnetwifistalist[i]);
		mobility.Install(AP.Get(i));

	}





// ************************************************** internet stack and ip address
	InternetStackHelper stack;
	stack.Install(AP);
	stack.Install(Router);
	for(int i=0; i<AP_NUMBER; i++)
		stack.Install(IotNode[i]);
	stack.Install(UserNode);

	Ipv4AddressHelper address;

	Ipv4InterfaceContainer ap2rInterface[AP_NUMBER];
	for(int i=0; i<AP_NUMBER; i++)
	{
		std::string ipaddress_string = "10.1.";
		ipaddress_string.append(std::to_string(i+1));
		ipaddress_string.append(".0");
		char* ipaddress = (char*)ipaddress_string.data();
		address.SetBase(Ipv4Address(ipaddress), "255.255.255.0");
		ap2rInterface[i] = address.Assign(p2pdevicelist1[i]);
	}

	Ipv4InterfaceContainer r2rInterface[edgenumber];
	for(int i=0; i<edgenumber; i++)
	{
		std::string ipaddress_string = "10.2.";
		ipaddress_string.append(std::to_string(i+1));
		ipaddress_string.append(".0");
		char* ipaddress = (char*)ipaddress_string.data();
		address.SetBase(Ipv4Address(ipaddress), "255.255.255.0");
		r2rInterface[i] = address.Assign(p2pdevicelist2[i]);
	}

	Ipv4InterfaceContainer wifiapInterface[AP_NUMBER];
	Ipv4InterfaceContainer wifistaInterface[AP_NUMBER];
	for(int i=0; i<AP_NUMBER; i++)
	{
		std::string ipaddress_string = "10.3.";
		ipaddress_string.append(std::to_string(i+1));
		ipaddress_string.append(".0");
		char* ipaddress = (char*)ipaddress_string.data();
		address.SetBase(Ipv4Address(ipaddress), "255.255.255.0");
		wifistaInterface[i] = address.Assign(stadevices[i]);
		wifiapInterface[i] = address.Assign(apdevices[i]);
	}


// ************************************************** extract IoT device's ip address and save them
	std::map<uint8_t, Ipv4Address> map_node_addr;
	for(int i=0; i<IotNodeNumber; i++)
	{
		std::pair<int, int> p = map_NodeNumberToApNodeNumber[i];
		map_node_addr[i] = wifistaInterface[p.first].GetAddress(p.second);
	}

	std::ofstream outfile1;
	outfile1.open("scratch/subdir/topologydata/node-address.txt");
	outfile1<<"Node and Ip address\n";
	for(int i=0; i<IotNodeNumber; i++)
	{
		outfile1<<i<<" "<<map_node_addr[i]<<"\n";
	}
	outfile1.close();


	std::ofstream outfile2;
	outfile2.open("scratch/subdir/topologydata/address-node.txt");
	outfile2<<"Ip address and Node\n";
	for(int i=0; i<IotNodeNumber; i++)
	{
		outfile2<<map_node_addr[i]<<" "<<i<<"\n";
	}
	outfile2.close();

// **************************************************　install app
	// ApplicationContainer gossipApplist[IotNodeNumber];
	// for(int i=0; i<IotNodeNumber; i++)
	// {
	// 	GossipAppHelper gossipApp1(i, 17);
	// 	std::pair<int, int> p = map_NodeNumberToApNodeNumber[i];
	// 	gossipApplist[i] = gossipApp1.Install(IotNode[p.first].Get(p.second));
	// 	// float x = 0.2*i;
	// 	gossipApplist[i].Start(Seconds(0.));
	// 	gossipApplist[i].Stop(Seconds(800.));
	// }
	
	ApplicationContainer serverTrafficlist[ApNumber];
	ApplicationContainer userTrafficlist[ApNumber];
	for(int i=0; i<ApNumber; i++)
	{
		if(i==0)
		{
		ServerTrafficHelper ServerTraffic(109);
		serverTrafficlist[i] = ServerTraffic.Install(Router.Get(i));
		serverTrafficlist[i].Start(Seconds(0.0));
		serverTrafficlist[i].Stop(Seconds(35.0));

		UserTrafficHelper UserTraffic(ap2rInterface[i].GetAddress(0), 109);
		userTrafficlist[i] = UserTraffic.Install(UserNode.Get(i));

		// UserTrafficHelper UserTraffic(ap2rInterface[i].GetAddress(0), 109);
		// userTrafficlist[i] = UserTraffic.Install(AP.Get(i));
		userTrafficlist[i].Start(Seconds(2.0));
		userTrafficlist[i].Stop(Seconds(35.0));
		}
	}

	// int client_;
	// int server_ = 1;
	// std::pair<int, int> p_server = map_NodeNumberToApNodeNumber[server_];
	
	// UdpEchoServerHelper echoServer (229);
	// ApplicationContainer serverApps = echoServer.Install (IotNode[p_server.first].Get(p_server.second));
	// // ApplicationContainer serverApps = echoServer.Install (Router.Get(9));
	// serverApps.Start (Seconds (1.0));
	// serverApps.Stop (Seconds (10.0));

	// UdpEchoClientHelper echoClient (map_node_addr[server_], 229);
	// echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
	// echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
	// echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

	// for(client_ = 0; client_<16; client_++)
	// {
	// 	if(client_!=1)
	// 	{
	// 		std::pair<int, int> p_client = map_NodeNumberToApNodeNumber[client_];
	// 		ApplicationContainer clientApps = 
	// 			echoClient.Install (IotNode[p_client.first].Get(p_client.second));
	// 		float x = 2+0.01*client_;
	// 		clientApps.Start (Seconds (x));
	// 		clientApps.Stop (Seconds (10.0));
	// 	}
		
	// }
	
// ************************************************** change bandwidth dynamically

	Simulator::Schedule(Seconds(10.0), &bandwidth_vary);
	
// **************************************************  run simulation

	Simulator::Stop (Seconds (1020.0));
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
	Simulator::Run ();
    Simulator::Destroy ();

	finish=clock();
    totaltime=(double)(finish-start)/CLOCKS_PER_SEC;
    std::cout<<"\n run time: "<<totaltime<<"s！"<<std::endl;
	
    return 0;
}
