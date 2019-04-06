#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/global-route-manager.h"
#include "ns3/pointer.h"
#include "ns3/gossip-app.h"
#include <vector>
#include <set>
#include <tuple>
#include <string>
#include <time.h>
#include <math.h>
#include <random>

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
    infile.open("scratch/subdir/topologydata/topology-50-single.txt");
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


void bandwidth_vary(float ratio)
{	
	for(int i=0; i<AP_NUMBER; i++)
	{
		// int n = i % 4;
		// float x;
		// if(n==0)
		// {
		// 	x = 1 * ratio;
		// }
		// else if(n==1)
		// {
		// 	x = 2 * ratio;
		// }
		// else if(n==2)
		// {
		// 	x = 2 * ratio;
		// }
		// else
		// {
		// 	x = 2 * ratio;
		// }
		float x;
		x = 1 * ratio;
		std::string str1 = std::to_string(x);
		str1.append("Mbps");
		DataRate rate(str1);
		std::string str2 = "/NodeList/";
		str2.append(std::to_string(i));
		str2.append("/DeviceList/0/$ns3::PointToPointNetDevice/DataRate");
		Config::Set(str2, DataRateValue(rate));

		std::string str3 = "/NodeList/";
		str3.append(std::to_string(i+AP_NUMBER));
		str3.append("/DeviceList/0/$ns3::PointToPointNetDevice/DataRate");
		Config::Set(str3, DataRateValue(rate));
	}
	std::cout<<"bandwidth changed at "<<Simulator::Now().GetSeconds()<<"s available ratio: "<<ratio<<std::endl;
}



NS_LOG_COMPONENT_DEFINE("HundredNodesTry");

int main()
{
	clock_t start,finish;
    double totaltime;
    start=clock();

	Time::SetResolution(Time::NS);
	LogComponentEnable("GossipAppApplication", LOG_LEVEL_INFO);
	Config::SetDefault("ns3::TcpSocket::SegmentSize", StringValue("16384"));
	Config::SetDefault("ns3::TcpSocket::SndBufSize", StringValue("51428800"));
	Config::SetDefault("ns3::TcpSocket::RcvBufSize", StringValue("2097152"));
	// **************************************************read topology from txt file
	std::map<int, int> map_AP_devicenumber;
    std::vector<std::pair<int, int> > vecotr_edge;

    int IotNodeNumber = Get_Topology(map_AP_devicenumber, vecotr_edge);
	int ApNumber = map_AP_devicenumber.size();
	std::cout<<"total Aps are: "<<ApNumber<<std::endl;
    std::cout<<"total IoT devices are: "<<IotNodeNumber<<std::endl;
	std::map<int, std::pair<int, int> > map_NodeNumberToApNodeNumber = NodeNumberToApNodeNumber(IotNodeNumber, map_AP_devicenumber);
    int edgenumber = (int)vecotr_edge.size();
	std::cout<<"edge number: "<<edgenumber<<std::endl;
// ************************************************** create nodes

    NodeContainer AP;
    NodeContainer Router;
    NodeContainer IotNode[AP_NUMBER];

    AP.Create(AP_NUMBER);
    Router.Create(AP_NUMBER);
    for(int i=0; i<AP_NUMBER; i++)
    {
    	IotNode[i].Create(map_AP_devicenumber[i]);
    }


// ************************************************** set delay of links
// 
// 
    std::default_random_engine generator;
	std::exponential_distribution<float> distribution(10./3);
	std::vector<std::string> delay_data;
	for(int i=0; i<edgenumber+AP_NUMBER; i++)
	{
		float de = distribution(generator);
		de = de * 1000;
		std::string str4 = "";
		str4.append(std::to_string(de));
		str4.append("ms");
		delay_data.push_back(str4);
	}


// ************************************************** set p2p link between router and AP
    NodeContainer subnetap2rlist[AP_NUMBER];
    PointToPointHelper p2phelper;
    NetDeviceContainer p2pdevicelist1[AP_NUMBER];
	srand(time(NULL));
	int cc = 0;
    for(int i=0; i<AP_NUMBER; i++)
	{
		subnetap2rlist[i].Add(Router.Get(i));
		subnetap2rlist[i].Add(AP.Get(i));
		int x = i % 4;
		if(x==0)
			p2phelper.SetDeviceAttribute("DataRate", StringValue("0.2Mbps"));
		else if(x==1)
			p2phelper.SetDeviceAttribute("DataRate", StringValue("0.2Mbps"));
		else if(x==2)
			p2phelper.SetDeviceAttribute("DataRate", StringValue("0.5Mbps"));
		else if(x==3)
			p2phelper.SetDeviceAttribute("DataRate", StringValue("1Mbps"));
		// p2phelper.SetChannelAttribute("Delay", StringValue("5ms"));
		p2phelper.SetChannelAttribute("Delay", StringValue("25ms"));
		p2pdevicelist1[i] = p2phelper.Install(subnetap2rlist[i]);

	}




// ************************************************** set p2p link between routers according to the topology

	
    NodeContainer subnetr2rlist[edgenumber];
    NetDeviceContainer p2pdevicelist2[edgenumber];
    for(int i=0; i<edgenumber; i++)
	{
		subnetr2rlist[i].Add(Router.Get(vecotr_edge[i].first));
		subnetr2rlist[i].Add(Router.Get(vecotr_edge[i].second));

		p2phelper.SetDeviceAttribute("DataRate", StringValue("10GBps"));
		// p2phelper.SetChannelAttribute("Delay", StringValue(delay_data[cc]));
		// p2phelper.SetChannelAttribute("Delay", StringValue("5ms"));
		p2phelper.SetChannelAttribute("Delay", StringValue("25ms"));
		cc++;
		p2pdevicelist2[i] = p2phelper.Install(subnetr2rlist[i]);
	}

// ************************************************** set p2p link between AP and attatched Iotdevices
	NodeContainer subnetd2aplist[IotNodeNumber];
	NetDeviceContainer p2pdevicelist3[IotNodeNumber];
	for(int i=0; i<IotNodeNumber; i++)
	{
		std::pair<int, int> p = map_NodeNumberToApNodeNumber[i];
		subnetd2aplist[i].Add(IotNode[p.first].Get(p.second));
		subnetd2aplist[i].Add(AP.Get(p.first));

		p2phelper.SetDeviceAttribute("DataRate", StringValue("20Mbps"));
		p2phelper.SetChannelAttribute("Delay", StringValue("5ms"));
		p2pdevicelist3[i] = p2phelper.Install(subnetd2aplist[i]);
	}



// ************************************************** internet stack and ip address
	InternetStackHelper stack;
	stack.Install(AP);
	stack.Install(Router);
	for(int i=0; i<AP_NUMBER; i++)
		stack.Install(IotNode[i]);


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

	Ipv4InterfaceContainer dev2apInterface[IotNodeNumber];
	for(int i=0; i<IotNodeNumber; i++)
	{
		std::string ipaddress_string = "10.2.";
		ipaddress_string.append(std::to_string(i+1));
		ipaddress_string.append(".0");
		char* ipaddress = (char*)ipaddress_string.data();
		address.SetBase(Ipv4Address(ipaddress), "255.255.255.0");
		dev2apInterface[i] = address.Assign(p2pdevicelist3[i]);
	}


	Ipv4InterfaceContainer r2rInterface[edgenumber];
	for(int i=0; i<edgenumber; i++)
	{
		int cc = i / 200 ;
		int uu = i % 200;
		std::string ipaddress_string = "10.";
		ipaddress_string.append(std::to_string(cc+3));
		ipaddress_string.append(".");
		ipaddress_string.append(std::to_string(uu+1));
		ipaddress_string.append(".0");
		char* ipaddress = (char*)ipaddress_string.data();
		address.SetBase(Ipv4Address(ipaddress), "255.255.255.0");
		r2rInterface[i] = address.Assign(p2pdevicelist2[i]);
	}

	


// ************************************************** extract IoT device's ip address and save them
	std::map<uint8_t, Ipv4Address> map_node_addr;
	for(int i=0; i<IotNodeNumber; i++)
	{
		map_node_addr[i] = dev2apInterface[i].GetAddress(0);
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
	ApplicationContainer gossipApplist[IotNodeNumber];
	for(int i=0; i<IotNodeNumber; i++)
	{
		GossipAppHelper gossipApp1(i, 17);
		std::pair<int, int> p = map_NodeNumberToApNodeNumber[i];
		gossipApplist[i] = gossipApp1.Install(IotNode[p.first].Get(p.second));
		// float x = 0.2*i;
		gossipApplist[i].Start(Seconds(12253.8));
		gossipApplist[i].Stop(Seconds(39630.));
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
	std::vector<float> remaining_datarate;
	std::ifstream infile1;
    infile1.open("scratch/subdir/topologydata/datarate.txt");
    const int LINE_LENGTH = 100; 
    char str1[LINE_LENGTH];
    while(infile1.getline(str1,LINE_LENGTH))
    {
        std::string str2(str1);
        std::vector<std::string> res = SplitString(str2, '+');
		float x = (atof)(res[1].c_str());
		remaining_datarate.push_back(x);
    }
	for(int i=0; i<66; i++)
	{
		float time1 = 600 * i;
		Simulator::Schedule(Seconds(time1), &bandwidth_vary, remaining_datarate[i]);
	}
	// Simulator::Schedule(Seconds(10.0), &bandwidth_vary);
	
// **************************************************  run simulation

	Simulator::Stop (Seconds (40000.0));
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
	Simulator::Run ();
    Simulator::Destroy ();

	finish=clock();
    totaltime=(double)(finish-start)/CLOCKS_PER_SEC;
    std::cout<<"\n run time: "<<totaltime<<"s！"<<std::endl;
	
    return 0;
}
