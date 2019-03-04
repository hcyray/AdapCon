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






NS_LOG_COMPONENT_DEFINE("HundredNodesTry");

int main()
{
	Time::SetResolution(Time::NS);
	LogComponentEnable("GossipAppApplication", LOG_LEVEL_INFO);
	LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
	LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
	// **************************************************read topology from txt file
	std::map<int, int> map_AP_devicenumber;
    std::vector<std::pair<int, int> > vecotr_edge;

    int IotNodeNumber = Get_Topology(map_AP_devicenumber, vecotr_edge);
    std::cout<<"total IoT devices are: "<<IotNodeNumber<<std::endl;
	std::map<int, std::pair<int, int> > map_NodeNumberToApNodeNumber = NodeNumberToApNodeNumber(IotNodeNumber, map_AP_devicenumber);
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
    NodeContainer IotNode[AP_NUMBER];
    NodeContainer UserNode;

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
    for(int i=0; i<AP_NUMBER; i++)
	{
		subnetap2rlist[i].Add(Router.Get(i));
		subnetap2rlist[i].Add(AP.Get(i));

		p2phelper.SetDeviceAttribute("DataRate", StringValue("100kBps"));
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

		p2phelper.SetDeviceAttribute("DataRate", StringValue("100kBps"));
		p2phelper.SetChannelAttribute("Delay", StringValue("0ms"));
		p2pdevicelist2[i] = p2phelper.Install(subnetr2rlist[i]);
	}


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
		wifiapInterface[i] = address.Assign(apdevices[i]);
		wifistaInterface[i] = address.Assign(stadevices[i]);
	}


// ************************************************** extract IoT device's ip address 



// **************************************************　install app
	// ApplicationContainer gossipApplist[IotNodeNumber];
	// for(int i=0; i++; i<IotNodeNumber)
	// {
	// 	GossipAppHelper gossipApp1(i, 17);
	// 	gossipApplist[i] = gossipApp1.Install()
	// 	gossipApplist[i].Start(Seconds(0.));
	// 	gossipApplist[i].Stop(Seconds(1000.));
	// }

    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
	Simulator::Run ();
    Simulator::Destroy ();
    return 0;
}
