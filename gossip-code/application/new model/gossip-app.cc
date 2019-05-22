#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/address-utils.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/socket.h"
#include "ns3/tcp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/string.h"
#include "ns3/pointer.h"

#include "gossip-app.h"
#include "/home/hqw/repos/ns-3-allinone/ns-3-dev/scratch/data-struct.h"

#include <stdlib.h>
#include <time.h>
#include <algorithm>
#include <fstream>
#include <iomanip>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("GossipAppApplication");

NS_OBJECT_ENSURE_REGISTERED (GossipApp);

TypeId
GossipApp::GetTypeId (void)
{
 	static TypeId tid =
 		TypeId ("ns3::GossipApp")
			.SetParent<Application> ()
			.SetGroupName ("Applications")
			.AddConstructor<GossipApp> ()
			.AddAttribute ("NodeId", "Node on which the application runs.", UintegerValue (0),
							MakeUintegerAccessor (&GossipApp::m_node_id),
							MakeUintegerChecker<uint8_t> ())
			.AddAttribute ("Port", "Port on which we listen for incoming packets.", UintegerValue (9),
							MakeUintegerAccessor (&GossipApp::m_port),
							MakeUintegerChecker<uint16_t> ())
		;
	return tid;
}

GossipApp::GossipApp()
{
	NS_LOG_FUNCTION (this);
	m_epoch = 0;
	cur_epoch_len = 190.0;

	m_local_ledger.push_back (0xFFFFFFFF);
	m_ledger_built_epoch.push_back (0);

	EMPTY_BLOCK.name = 0;
	EMPTY_BLOCK.height = 0;
	GENESIS_BLOCK.name = 0xFFFFFFFF;
	GENESIS_BLOCK.height = 0;

	const int LINE_LENGTH = 100;
	char str1[LINE_LENGTH];
	std::ifstream infile1;
	infile1.open ("scratch/subdir/topologydata/node-address.txt");
	while (infile1.getline (str1, LINE_LENGTH))
	{
		if (strcmp (str1, "Node and Ip address") == 0)
			continue;
		std::string str2 (str1);
		std::vector<std::string> res = SplitMessage (str2, ' ');
		if (res.size () == 2)
		{
			int x = (atoi) (res[0].c_str ());
			char *ipaddress = (char *) res[1].data ();
			Ipv4Address y = Ipv4Address (ipaddress);
			map_node_addr[x] = y;
		}
	}
	infile1.close ();

	std::ifstream infile2;
	infile2.open ("scratch/subdir/topologydata/address-node.txt");
	while (infile2.getline (str1, LINE_LENGTH))
    {
		if (strcmp (str1, "Node and Ip address") == 0)
			continue;
		std::string str2 (str1);
		std::vector<std::string> res = SplitMessage (str2, ' ');
		if (res.size () == 2)
		{
			int x = (atoi) (res[1].c_str ());
			char *ipaddress = (char *) res[0].data ();
			Ipv4Address y = Ipv4Address (ipaddress);
			map_addr_node[y] = x;
		}
    }
	infile2.close ();

}

GossipApp::~GossipApp ()
{
	NS_LOG_FUNCTION (this);
}

void GossipApp::ConsensProcess()
{
	m_epoch++;
	m_epoch_beginning = Simulator::Now().GetSeconds();

	
	float curr_epoch_len = InitializeEpoch();
	if_leader();
	if(m_node_id == 0)
	{
		std::cout << "*****************"
		          << "epoch " << (int) m_epoch << " starts"
		          << "**********" << std::endl;
		std::cout << "time now: " << Simulator::Now().GetSeconds() << "s" << std::endl;
	}
	if(m_leader)
	{
		Block b = BlockPropose();
		block_received = b;
		block_got = true;
		GossipBlockOut (b);
	}
	id0 = Simulator::Schedule(Seconds(curr_epoch_len+0*FIXED_EPOCH_LEN), &GossipApp::GossipEcho, this);
	id1 = Simulator::Schedule(Seconds(curr_epoch_len+1*FIXED_EPOCH_LEN), &GossipApp::GossipTime, this);
	id2 = Simulator::Schedule(Seconds(curr_epoch_len+2*FIXED_EPOCH_LEN), &GossipApp::GossipTimeEcho, this);
	id3 = Simulator::Schedule(Seconds(curr_epoch_len+3*FIXED_EPOCH_LEN), &GossipApp::GossipVote, this);
	if(m_epoch<TOTAL_EPOCH_FOR_SIMULATION)
		id4 = Simulator::Schedule(Seconds(curr_epoch_len+4*FIXED_EPOCH_LEN), &GossipApp::ConsensProcess, this);
}


void GossipApp::if_leader(void)
{
	uint8_t x = view % NODE_NUMBER;
	if(m_node_id==x)
		m_leader = true;
	else
		m_leader = false;
}

Block GossipApp::BlockPropose()
{
	Block blcok_proposed;
	srand(time(NULL));
	blcok_proposed.name = rand() % 0xFFFFFFFF;
	blcok_proposed.height = m_local_ledger.size();
	return blcok_proposed;
}

void GossipApp::if_get_block(int from_node)
{
	int sum = 0;
	for (int i = 0; i < BLOCK_PIECE_NUMBER; i++)
		sum += map_node_blockpiece_received[from_node][i];
	if (sum == BLOCK_PIECE_NUMBER)
		block_got = true;
	else
		block_got = false;
}

void GossipApp::GetNeighbor (int node_number, int out_neighbor_choosed[])
{
	log_link_file<<(int)m_node_id<<":\n";

	srand((unsigned)time(NULL)+m_node_id);
	int i = 0;
	std::vector<int> node_added;
	std::vector<int>::iterator it;
	while (i < node_number)
	{
		int x = rand () % NODE_NUMBER;
		if (x != m_node_id)
		{
			it = std::find (node_added.begin (), node_added.end (), x);
			if (it == node_added.end ())
			{
				out_neighbor_choosed[i] = x;
				node_added.push_back (x);
				log_link_file<<x<<" ";
				i++;
			}
		}
	}

	log_link_file<<"\n";
	log_link_file.close();

	// for(int i=0; i<NODE_NUMBER; i++)
	// {
	// 	if(i<(int)m_node_id)
	// 		out_neighbor_choosed[i] = i;
	// 	else if(i>(int)m_node_id)
	// 		out_neighbor_choosed[i-1] = i;
	// }
}

uint8_t
GossipApp::GetNodeId (void)
{
	return m_node_id;
}

std::vector<std::string>
GossipApp::SplitMessage (const std::string &str, const char pattern)
{
	std::vector<std::string> res;
	std::stringstream input (str);
	std::string temp;
	while (getline (input, temp, pattern))
	{
		res.push_back (temp);
	}
	return res;
}

void
GossipApp::DoDispose (void)
{
	NS_LOG_FUNCTION (this);
	Application::DoDispose ();
}

void GossipApp::StartApplication (void)
{
	NS_LOG_FUNCTION (this);
	GetNeighbor (OUT_GOSSIP_ROUND, out_neighbor_choosed);

	std::string str1 = "scratch/subdir/log_cmjj/node_";
	str1.append(std::to_string((int)m_node_id));
	str1.append("_receiving_time_log.txt");
	log_time_file.open(str1);

	std::string str2 = "scratch/subdir/log_cmjj/node_";
	str2.append(std::to_string((int)m_node_id));
	str2.append("_rep_log.txt");
	log_rep_file.open(str2);

	std::string str3 = "scratch/subdir/log_cmjj/node_";
	str3.append(std::to_string((int)m_node_id));
	str3.append("_link_log.txt");
	log_link_file.open(str3);

	for (int i=0; i<NODE_NUMBER; i++)
	{
		TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
		Ptr<Socket> socket_receive = Socket::CreateSocket (GetNode (), tid);
		m_socket_receive.push_back(socket_receive);
		InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 100+i);
		if(m_socket_receive[i]->Bind(local) == -1)
			NS_FATAL_ERROR ("Failed to bind socket");
		if(m_socket_receive[i]->Listen()!=0)
			std::cout<<"listen failed"<<std::endl;
		m_socket_receive[i]->ShutdownSend();
		m_socket_receive[i]->SetAcceptCallback(MakeNullCallback<bool, Ptr<Socket>, const Address &> (), 
			MakeCallback (&GossipApp::HandleAccept, this));
	}
	for (int i=0; i<NODE_NUMBER; i++)
	{
		TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
		Ptr<Socket> socket_send = Socket::CreateSocket (GetNode (), tid);
		m_socket_send.push_back (socket_send);
	// std::cout<<"sending socket created!"<<std::endl;
		if (m_socket_send[i]->Connect (InetSocketAddress (Ipv4Address::ConvertFrom (map_node_addr[i]), 100+(int)m_node_id)) == -1)
			NS_FATAL_ERROR ("Fail to connect socket");
		m_socket_send[i]->ShutdownRecv();
	}

	view = 1;
	block_received.name = 0;
	block_received.height = 0;

	Simulator::Schedule (Seconds (600.), &GossipApp::ConsensProcess, this);
}

void
GossipApp::StopApplication ()
{
	NS_LOG_FUNCTION (this);
	for(int i = 0; i < NODE_NUMBER; i++)
	{
		if(m_socket_receive[i]!=0)
			m_socket_receive[i]->Close();
		if(m_socket_send[i]!=0)
			m_socket_send[i]->Close();
	}

}

float GossipApp::InitializeEpoch()
{
	for(int i=0; i<NODE_NUMBER; i++)
	{
		for(int j=0; j<BLOCK_PIECE_NUMBER; j++)
			map_node_blockpiece_received[i][j] = 0;
		map_node_vote[i] = 0;
	}
	block_got = false;
	return 190;
}

void GossipApp::GossipBlockOut (Block b)
{
	srand (Simulator::Now ().GetSeconds () + m_node_id);
  	for (int i = 0; i < OUT_GOSSIP_ROUND; i++)
    {
      
    	float x = rand () % 1000;
    	x = x / 1000 + 0.1 * i;
    	Simulator::Schedule (Seconds (x), &GossipApp::SendBlock, this, out_neighbor_choosed[i], b);
    }
}

void GossipApp::GossipBlockAfterReceive (int from_node, Block b)
{
	for (int i = 0; i < OUT_GOSSIP_ROUND; i++)
    {
		if (out_neighbor_choosed[i] != from_node)
        {
			srand (Simulator::Now ().GetSeconds () + m_node_id);
			float x = rand () % 1000;
			x = x / 1000 + 0.1 * i;
			Simulator::Schedule (Seconds (x), &GossipApp::SendBlock, this, out_neighbor_choosed[i], b);
        }
    }
}

void GossipApp::GossipEcho()
{
	// TODO	
}

void GossipApp::GossipTime()
{
	// TODO
}

void GossipApp::GossipTimeEcho()
{
	// TODO
}

void GossipApp::GossipVote()
{
	// TODO 
}

void GossipApp::SendBlock (int dest, Block b)
{
	for (int piece = 0; piece < BLOCK_PIECE_NUMBER; piece++)
	{
		srand (Simulator::Now ().GetSeconds ());
		float x = rand () % 100;
		x = x / 100 + 0.1*piece;
		Simulator::Schedule (Seconds (x), &GossipApp::SendBlockPiece, this, dest, piece, b);
	}
	if((int)m_node_id==view)
		std::cout << "node " << (int) GetNodeId () << " send a block to node " << dest << " at "
	          << Simulator::Now ().GetSeconds () << "s" << std::endl;
}


void GossipApp::SendBlockPiece (int dest, int piece, Block b)
{
	std::string str1 = "";
	str1.append (std::to_string ((int) m_epoch));
	str1.append ("+");
	str1.append (std::to_string ((int) m_node_id));
	str1.append ("+");
	std::string str2 = "BLOCK";
	str1.append (str2);
	str1.append ("+");
	str1.append (std::to_string (b.name));
	str1.append ("+");
	int u = m_local_ledger.size() - 1;
	str1.append (std::to_string (m_local_ledger[u]));
	str1.append ("+");
	str1.append (std::to_string (b.height));
	str1.append ("+");
	str1.append (std::to_string ((int) piece));
	const uint8_t *str3 = reinterpret_cast<const uint8_t *> (str1.c_str ());
	Packet pack1 (str3, 1024 * 16);
	Ptr<Packet> p = &pack1;
	if (m_socket_send[dest]->Send (p) == -1)
		std::cout << "node "<< (int)m_node_id << " failed to send block piece "<<piece <<" to node "<< dest <<
		" , send buffer　: "<< m_socket_receive[dest]->GetTxAvailable()<< std::endl;
}

void GossipApp::HandleAccept(Ptr<Socket> s, const Address& from)
{
	s->SetRecvCallback(MakeCallback(&GossipApp::HandleRead, this));
}

void GossipApp::HandleRead(Ptr<Socket> socket)
{
	NS_LOG_FUNCTION(this << socket);
	Ptr<Packet> packet;
	Address from;
	while(packet=socket->RecvFrom(from))
	{
		uint8_t content_[200];
		packet->CopyData(content_, 200);
		Ipv4Address from_addr = InetSocketAddress::ConvertFrom(from).GetIpv4();
		int from_node = (int) map_addr_node[from_addr];
		std::string str_of_content(content_, content_+200);
		std::vector<std::string> res = SplitMessage(str_of_content, '+');
		const char *time_of_received_message = res[0].c_str();
		if(strcmp (time_of_received_message, (std::to_string (m_epoch)).c_str ()) == 0)
		{
			const char*type_of_received_message = res[2].c_str();
			if(strcmp(type_of_received_message, "BLOCK")==0)
			{
				uint32_t received_former_block = (atoi)(res[4].c_str());
				uint32_t local_former_block = m_local_ledger[m_local_ledger.size()-1];
				if(received_former_block==local_former_block)
				{
					if(block_got==false)
					{
						int u = (atoi) (res[6].c_str ());
						map_node_blockpiece_received[from_node][u] = 1;
						if_get_block(from_node);
						if(block_got==true)
						{
							uint32_t tmp1 = atoi (res[3].c_str ());
							int tmp2 = (atoi) (res[5].c_str ());
							block_received.name = tmp1;
							block_received.height = tmp2;
							std::cout << "node " <<std::setw(2)<< (int) GetNodeId () << " received a " << content_
							    << " from node " << from_node << " at "
							    << Simulator::Now ().GetSeconds () << " s, rcv buffer : "
							    << m_socket_receive[from_node]->GetTxAvailable()<< std::endl;
							block_got_time = ((int) (block_got_time * 1000)) / 1000.;
							map_node_recvtime[from_node] = block_got_time;
							if(Simulator::Now().GetSeconds()-m_epoch_beginning<cur_epoch_len)
								GossipBlockAfterReceive (from_node, block_received);
						}
					}
				}
			}
		}
	}
}

} // namespace ns3