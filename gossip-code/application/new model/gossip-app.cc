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
	cur_epoch_len = 190.0;  //  main target variable

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
	id1 = Simulator::Schedule(Seconds(curr_epoch_len+1*FIXED_EPOCH_LEN), &GossipApp::GossipVote, this);
	id2 = Simulator::Schedule(Seconds(curr_epoch_len+2*FIXED_EPOCH_LEN), &GossipApp::GossipTime, this);
	id3 = Simulator::Schedule(Seconds(curr_epoch_len+3*FIXED_EPOCH_LEN), &GossipApp::GossipTimeEcho, this);
	id_end = Simulator::Schedule(Seconds(curr_epoch_len+3*FIXED_EPOCH_LEN-1), &GossipApp::EndSummary, this);
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


void GossipApp::record_block_piece(int from_node)
{
	int sum = 0;
	for (int i = 0; i < BLOCK_PIECE_NUMBER; i++)
		sum += map_node_blockpiece_received[from_node][i];
	if (sum == BLOCK_PIECE_NUMBER)
	{
		map_node_blockrcv_state[from_node] = 1;
		block_got = true;
		float x = Simulator::Now ().GetSeconds ();
		x = ((int) (x * 1000)) / 1000.;
		map_node_blockrcvtime[from_node] = x;
		block_got_time = x;
	}
}


int GossipApp::GetOutNeighbor ()
{
	// hope that a vector without initialization has size 0
	if(out_neighbor_choosed.size()<OUT_NEIGHBOR_NUMBER)
	{
		srand((unsigned)time(NULL)+m_node_id+Simulator::Now ().GetSeconds ());
		int x = rand () % NODE_NUMBER;
		std::vector<int>::iterator it;
		it = std::find(out_neighbor_choosed.begin(), out_neighbor_choosed.end(), x);
		if(it==out_neighbor_choosed.end())
		{
			try_tcp_connect(x);
			float y = x/(NODE_NUMBER+1.0)+3;
			Simulator.Schedule(Seconds (y), &GossipApp::GetOutNeighbor, this);
		}
	}
}


int GossipApp::GetAllNeighbor ()
{
	neighbor_number = in_neighbor_number + OUT_NEIGHBOR_NUMBER;
	neighbor_choosed.insert(neighbor_choosed.end(), out_neighbor_choosed.begin(), out_neighbor_choosed.end());
	neighbor_choosed.insert(neighbor_choosed.end(), in_neighbor_choosed.begin(), in_neighbor_choosed.end());
	// need to test if this way to merge works
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
	// log_link_file<<"The weather is good\n";


	for (int i=0; i<NODE_NUMBER; i++)
	{
		TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
		Ptr<Socket> socket_receive = Socket::CreateSocket (GetNode (), tid);
		m_socket_receive.push_back(socket_receive);
		InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 109);
		if(m_socket_receive[i]->Bind(local) == -1)
			NS_FATAL_ERROR ("Failed to bind socket");
		if(m_socket_receive[i]->Listen()!=0)
			std::cout<<"listen failed"<<std::endl;
		// m_socket_receive[i]->ShutdownSend();
		m_socket_receive[i]->SetAcceptCallback(MakeNullCallback<bool, Ptr<Socket>, const Address &> (), 
			MakeCallback (&GossipApp::HandleAccept, this));
	}


	for (int i=0; i<NODE_NUMBER; i++)
	{
		TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
		Ptr<Socket> socket_send = Socket::CreateSocket (GetNode (), tid);
		m_socket_send.push_back (socket_send);
	// std::cout<<"sending socket created!"<<std::endl;
		if (m_socket_send[i]->Connect (InetSocketAddress (Ipv4Address::ConvertFrom (map_node_addr[i]), 109)) == -1)
			NS_FATAL_ERROR ("Fail to connect socket");
		m_socket_send[i]->ShutdownRecv();
	}

	GetOutNeighbor (in_neighbor_choosed, out_neighbor_choosed);
	Simulator::Schedule (Seconds (590.), &GossipApp::GetAllNeighbor, this);

	view = 1;
	block_received.name = 0;
	block_received.height = 0;
	attacker_pretend_lost = false;
	attacker_do_not_send = false;

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
	RecordNeighbor();
	log_link_file.close();
	log_time_file.close();
	log_rep_file.close();
}


float GossipApp::InitializeEpoch()
{
	for(int i=0; i<NODE_NUMBER; i++)
	{
		for(int j=0; j<BLOCK_PIECE_NUMBER; j++)
			map_node_blockpiece_received[i][j] = 0;
		map_node_vote[i] = 0;
		// map_node_blockrcv_state[i] = 0;
		// map_node_blockrcvtime[i] = -1;
	}
	block_got = false;
	time_query_for_block = 0;
	gossip_action = false;

	return 190;
}


void GossipApp::GossipBlockOut (Block b)
{
	srand (Simulator::Now ().GetSeconds () + m_node_id);
  	for (int i = 0; i < neighbor_number; i++)
    {
      
    	float x = rand () % 1000;
    	x = x / 1000 + 0.1 * i;
    	Simulator::Schedule (Seconds (x), &GossipApp::SendBlockInv, this, neighbor_choosed[i]);
    }
    gossip_action = true;
}


void GossipApp::GossipBlockAfterReceive (int from_node, Block b)
{
	for (int i = 0; i < neighbor_number; i++)
    {
		if (neighbor_choosed[i] != from_node)
        {
			srand (Simulator::Now ().GetSeconds () + m_node_id);
			float x = rand () % 1000;
			x = x / 1000 + 0.1 * i;
			Simulator::Schedule (Seconds (x), &GossipApp::SendBlockInv, this, neighbor_choosed[i]);
        }
    }
}


void GossipApp::GossipEcho()
{
	srand (Simulator::Now ().GetSeconds () + m_node_id);
  	for (int i = 0; i < neighbor_number; i++)
    {
      
    	float x = rand () % 1000;
    	x = x / 1000 + 0.1 * i;
    	Simulator::Schedule (Seconds (x), &GossipApp::SendEcho, this, neighbor_choosed[i]);
    }
    //  TODO
}


void GossipApp::GossipVote()
{
	if(block_got==true)
	{
		srand (Simulator::Now ().GetSeconds () + m_node_id);
	  	for (int i = 0; i < neighbor_number; i++)
	    {
	      
	    	float x = rand () % 1000;
	    	x = x / 1000 + 0.1 * i;
	    	Simulator::Schedule (Seconds (x), &GossipApp::SendVote, this, neighbor_choosed[i]);
	    }
	}
	// TODO 
}


void GossipApp::GossipTime()
{
	srand (Simulator::Now ().GetSeconds () + m_node_id);
  	for (int i = 0; i < neighbor_number; i++)
    {
      
    	float x = rand () % 1000;
    	x = x / 1000 + 0.1 * i;
    	Simulator::Schedule (Seconds (x), &GossipApp::SendTime, this, neighbor_choosed[i]);
    }
	// TODO
}


void GossipApp::GossipTimeEcho()
{
	srand (Simulator::Now ().GetSeconds () + m_node_id);
  	for (int i = 0; i < neighbor_number; i++)
    {
      
    	float x = rand () % 1000;
    	x = x / 1000 + 0.1 * i;
    	Simulator::Schedule (Seconds (x), &GossipApp::SendTimeEcho, this, neighbor_choosed[i]);
    }
	// TODO
}





void GossipApp::EndSummary()
{
	// TODO
}


void GossipApp::RecordNeighbor()
{
	log_link_file<<(int)m_node_id<<":\n";
	for(int i=0; i<OUT_NEIGHBOR_NUMBER; i++)
		log_link_file<<out_neighbor_choosed[i]<<" ";
	log_link_file<<"\n";
}


void GossipApp::RecordTime()
{
	log_time_file << "Epoch "<< (int)m_epoch<<" :\n";
	for(int i=0; i<NODE_NUMBER; i++)
	{
		if(i!=(int)m_node_id)
		{
			if(map_node_blockrcv_state[i]!=0)
			{
				log_time_file<<i<<" "<<map_node_blockrcvtime[i]-m_epoch_beginning<<"\n";
			}
		}
	}
	log_time_file<<"\n";
}


void GossipApp::SendBlockInv(int dest)
{
	std::string str1 = "";
	str1.append (std::to_string ((int) m_epoch));
	str1.append ("+");
	str1.append (std::to_string ((int) m_node_id));
	str1.append ("+");
	std::string str2 = "BLOCKINV";
	str1.append (str2);
	str1.append ("+");
	str1.append (std::to_string (b.height));
	const uint8_t *str3 = reinterpret_cast<const uint8_t *> (str1.c_str ());
	Packet pack1 (str3, 100);
	Ptr<Packet> p = &pack1;
	if (m_socket_send[dest]->Send (p) == -1)
		std::cout << "node "<< (int)m_node_id << " failed to send block inv to node "<< dest <<
		" , send buffer　: "<< m_socket_receive[dest]->GetTxAvailable()<< std::endl;
}


void GossipApp::SendGetdata(int dest)
{
	std::string str1 = "";
	str1.append (std::to_string ((int) m_epoch));
	str1.append ("+");
	str1.append (std::to_string ((int) m_node_id));
	str1.append ("+");
	std::string str2 = "GETDATA";
	str1.append (str2);
	str1.append ("+");
	str1.append (std::to_string (b.height));
	const uint8_t *str3 = reinterpret_cast<const uint8_t *> (str1.c_str ());
	Packet pack1 (str3, 100);
	Ptr<Packet> p = &pack1;
	if (m_socket_send[dest]->Send (p) == -1)
		std::cout << "node "<< (int)m_node_id << " failed to send getdata request to node "<< dest <<
		" , send buffer　: "<< m_socket_receive[dest]->GetTxAvailable()<< std::endl;
}


void GossipApp::SendBlockAck(int dest)
{
	std::string str1 = "";
	str1.append (std::to_string ((int) m_epoch));
	str1.append ("+");
	str1.append (std::to_string ((int) m_node_id));
	str1.append ("+");
	std::string str2 = "BLOCKACK";
	str1.append (str2);
	str1.append ("+");
	str1.append (std::to_string (b.height));
	const uint8_t *str3 = reinterpret_cast<const uint8_t *> (str1.c_str ());
	Packet pack1 (str3, 100);
	Ptr<Packet> p = &pack1;
	if (m_socket_send[dest]->Send (p) == -1)
		std::cout << "node "<< (int)m_node_id << " failed to send receive block ack to node "<< dest <<
		" , send buffer　: "<< m_socket_receive[dest]->GetTxAvailable()<< std::endl;
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


void GossipApp::SendEcho (int dest, Block b)
{
	std::string str1 = "";
	str1.append (std::to_string ((int) m_epoch));
	str1.append ("+");
	str1.append (std::to_string ((int) m_node_id));
	str1.append ("+");
	std::string str2 = "BLOCKECHO";
	str1.append (str2);
	str1.append ("+");
	str1.append (std::to_string (b.name));
	const uint8_t *str3 = reinterpret_cast<const uint8_t *> (str1.c_str ());
	Packet pack1 (str3, 100);
	Ptr<Packet> p = &pack1;
	if (m_socket_send[dest]->Send (p) == -1)
		std::cout << "node "<< (int)m_node_id << " failed to send block echo "<<std::to_string (b.name) <<" to node "<< dest <<
		" , send buffer　: "<< m_socket_receive[dest]->GetTxAvailable()<< std::endl;
}


void GossipApp::SendVote (int dest, Block b)
{
	std::string str1 = "";
	str1.append (std::to_string ((int) m_epoch));
	str1.append ("+");
	str1.append (std::to_string ((int) m_node_id));
	str1.append ("+");
	std::string str2 = "BLOCKVOTE";
	str1.append (str2);
	str1.append ("+");
	str1.append (std::to_string (b.name));
	const uint8_t *str3 = reinterpret_cast<const uint8_t *> (str1.c_str ());
	Packet pack1 (str3, 100);
	Ptr<Packet> p = &pack1;
	if (m_socket_send[dest]->Send (p) == -1)
		std::cout << "node "<< (int)m_node_id << " failed to send vote for block "<<std::to_string (b.name) <<" to node "<< dest <<
		" , send buffer　: "<< m_socket_receive[dest]->GetTxAvailable()<< std::endl;
}


void GossipApp::SendTime (int dest)
{
	// TODO
}


void GossipApp::SendTimeEcho(int dest)
{
	std::string str1 = "";
	str1.append (std::to_string ((int) m_epoch));
	str1.append ("+");
	str1.append (std::to_string ((int) m_node_id));
	str1.append ("+");
	std::string str2 = "TIMEECHO";
	str1.append (str2);
	const uint8_t *str3 = reinterpret_cast<const uint8_t *> (str1.c_str ());
	Packet pack1 (str3, 6000);
	Ptr<Packet> p = &pack1;
	if (m_socket_send[dest]->Send (p) == -1)
		std::cout << "node "<< (int)m_node_id << " failed to send vote for block "<<std::to_string (b.name) <<" to node "<< dest <<
		" , send buffer　: "<< m_socket_receive[dest]->GetTxAvailable()<< std::endl;
}


bool GossipApp::try_tcp_connect(int dest)
{
	remote_state = false;

	std::string str1 = "";
	str1.append (std::to_string ((int) m_epoch));
	str1.append ("+");
	str1.append (std::to_string ((int) m_node_id));
	str1.append ("+");
	std::string str2 = "CONNECT?SYN";
	str1.append (str2);
	const uint8_t *str3 = reinterpret_cast<const uint8_t *> (str1.c_str ());
	Packet pack1 (str3, 100);
	Ptr<Packet> p = &pack1;
	if (m_socket_send[dest]->Send (p) == -1)
		std::cout << "node "<< (int)m_node_id << " failed to send connection request "<<std::to_string (b.name) <<" to node "<< dest <<
		" , send buffer　: "<< m_socket_receive[dest]->GetTxAvailable()<< std::endl;

}


bool GossipApp::reply_tcp_connect(int dest)
{
	std::string str1 = "";
	str1.append (std::to_string ((int) m_epoch));
	str1.append ("+");
	str1.append (std::to_string ((int) m_node_id));
	str1.append ("+");
	std::string str2 = "CONNECT?SYNACK";
	str1.append (str2);
	const uint8_t *str3 = reinterpret_cast<const uint8_t *> (str1.c_str ());
	Packet pack1 (str3, 100);
	Ptr<Packet> p = &pack1;
	if (m_socket_send[dest]->Send (p) == -1)
		std::cout << "node "<< (int)m_node_id << " failed to reply connection request "<<std::to_string (b.name) <<" to node "<< dest <<
		" , send buffer　: "<< m_socket_receive[dest]->GetTxAvailable()<< std::endl;
}


void GossipApp::Send_data_watchdog(int n)
{
	if(map_node_ack_time[n] - map_node_getdata_time[n] >= timeout)
		// TODO
		// the timeout paremeter should be set carefully
	else
		// TODO

}


void GossipApp::Receive_data_watchdog()
{
	// TODO
	// don't know if i need to record those who sends me blocks slowly on purpose
	if(block_got==false)
	{
		if(vec_nodes_sendme_inv.size()>0)
		{
			int source = vec_nodes_sendme_inv.front();
			SendGetdata(source);
			time_start_rcv_block = Simulator::Now().GetSeconds();
			vec_nodes_sendme_inv.erase(vec_nodes_sendme_inv.begin());
		}
		if(Simulator::Now().GetSeconds()-m_epoch_beginning<cur_epoch_len)
			Simulator.Schedule(Seconds(2.0), &GossipApp::Receive_data_watchdog, this)
		// the timeout should be set very carefully
	}
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
					
					int u = (atoi) (res[6].c_str ());
					map_node_blockpiece_received[from_node][u] = 1;
					record_block_piece(from_node);
					time_end_rcv_block = Simulator::Now().GetSeconds();
					if(block_got==true)
					{
						if(attacker_pretend_lost==false)
						{
							SendBlockAck(from_node);
							time_block = Simulator::Now().GetSeconds();

							uint32_t tmp1 = atoi (res[3].c_str ());
							int tmp2 = (atoi) (res[5].c_str ());
							block_received.name = tmp1;
							block_received.height = tmp2;
							if(Simulator::Now().GetSeconds()-m_epoch_beginning<cur_epoch_len && gossip_action==false)
							{
								std::cout << "node " <<std::setw(2)<< (int) GetNodeId () << " received a " << content_
								    << " from node " << from_node << " at "
								    << Simulator::Now ().GetSeconds () << " s, rcv buffer : "
								    << m_socket_receive[from_node]->GetTxAvailable()<< std::endl;
								GossipBlockAfterReceive (from_node, block_received);
								gossip_action = true;
								
							}
						}
						
					}
				}
			}
			else if(strcmp(type_of_received_message, "BLOCKINV")==0)
			{
				vec_nodes_sendme_inv.push_back(from_node);
				time_query_for_block++;
				if(block_got==false && time_query_for_block==1)
				{
					// if has no block and the first time receive query, start receive data
					Receive_data_watchdog();
				}
			}
			else if(strcmp(type_of_received_message, "GETDATA")==0)
			{
				if(attacker_do_not_send==false)
				{
					SendBlock(from_node, block_received);
					Simulator.Schedule(Seconds(20.0), &Send_data_watchdog, this, from_node);
					// check for receiving ack after timeout
					// the timeout parameter should be set carefully
					map_node_getdata_time[from_node] = Simulator::Now().GetSeconds();
				}
			}
			else if(strcmp(type_of_received_message, "BLOCKACK")==0)
			{
				map_node_ack_time[from_node] = Simulator::Now().GetSeconds();
			}
		}
		if(strcmp(type_of_received_message, "CONNECT?SYN")==0)
		{
			if(in_neighbor_number<=MAX_IN_NEIGHBOR_NUMBER)
			{
				in_neighbor_choosed.push_back(from_node);
				in_neighbor_number++;
				reply_tcp_connect(from_node);
			}
		}
		if(strcmp(type_of_received_message, "CONNECT?SYNACK")==0)
		{
			out_neighbor_choosed.push_back(from_node);
		}
	}
}

} // namespace ns3