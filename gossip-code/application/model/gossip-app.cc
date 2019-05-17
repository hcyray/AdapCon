/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2007 University of Washington
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  010011-1307  USA
 */

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
#include "/root/repos/ns-3-allinone/ns-3-dev/scratch/data-struc.h"

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
      // .AddTraceSource ("Rx", "A packet has been received",
      //                  MakeTraceSourceAccessor (&GossipApp::m_rxTrace),
      //                  "ns3::Packet::TracedCallback")
      // .AddTraceSource ("RxWithAddresses", "A packet has been received",
      //                  MakeTraceSourceAccessor (&GossipApp::m_rxTraceWithAddresses),
      //                  "ns3::Packet::TwoAddressTracedCallback")
      ;
  return tid;
}

GossipApp::GossipApp ()
{
	NS_LOG_FUNCTION (this);
	m_epoch = 0;
	leadership_length = 0;
	// restart point
	m_epoch_beginning = 0.;
	len_phase1 = 120;
	len_phase2 = 30;

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

void
GossipApp::ConsensProcess ()
{
	m_epoch++;
	m_epoch_beginning = Simulator::Now ().GetSeconds ();
	map_epoch_start_time[m_epoch] = m_epoch_beginning;
	UpdateCRGain();

	if(gracecount==1)
	{
		for(int i=0; i<NODE_NUMBER; i++)
		{
			map_node_BR[i] = 1;
			map_epoch_node_BR[m_epoch][i] = 1;
		}
		UpdateCR();
	}
	// if(m_epoch>WINDOW_SIZE+2)
	if(gracecount==0)
	{
		UpdateCR();
		UpdateBR();
	}


	std::pair<float, float> p = InitializeEpoch ();
	if(m_node_id == 0)
	{
		std::cout << "*****************"
		          << "epoch " << (int) m_epoch << " starts"
		          << "**********" << std::endl;
		std::cout << "****block propagation time: " << len_phase1 << "s" <<
		  "  ****voting time: " << len_phase2 <<"s" << "****" << std::endl;
		std::cout << "current view: "<< view <<", grace count: "<<gracecount << std::endl;
		std::cout << "time now: " << Simulator::Now().GetSeconds() << "s" << std::endl;
	}

	if_leader ();
	if (m_leader)
	{
		Block b = BlockPropose ();
		block_received = b;

		if(Silence_Attacker==0)
		{
			LeaderGossipBlockOut (b);
			leadership_length++;
			std::cout<<"node "<<(int)m_node_id<<" is leader, leadership length: "<<leadership_length<<std::endl;
		}
		else
		{
			std::cout << "node " << (int) GetNodeId () << " launches a silence attack! "<< std::endl;
		}
	}
	id1 = Simulator::Schedule (Seconds (p.first), &GossipApp::GossipPrepareOut, this);
	id3 = Simulator::Schedule (Seconds (p.first + p.second - 0.2), &GossipApp::EndSummary, this);
	if (m_epoch < TOTAL_EPOCH_FOR_SIMULATION)
	{  
		id4 = Simulator::Schedule (Seconds (p.first + p.second), &GossipApp::ConsensProcess, this);
	}
}

void
GossipApp::if_leader (void)
{
	uint8_t x = view % NODE_NUMBER;
	// uint8_t x = 1;
	if (m_node_id == x)
	{
		if(m_epoch==1)
// restart point
		{
			m_leader = true;
			block_got = true;
			get_block_or_not = 1;
			get_block_time = 0;
		}
		else
		{
			if(gracecount<WINDOW_SIZE+2 && leadership_length<LEADERSHIP_LIFE)
			{
				m_leader = true;
				block_got = true;
				get_block_or_not = 1;
				get_block_time = 0;
			}
			else
			{
				m_leader = false;
				view++;
        		receive_viewplusplus_time = 1;
				GossipViewplusplusMsg();
				leadership_length = 0;
				std::cout<<"the previous leader "<<(int) m_node_id<< " launched a view change!"<<std::endl;
			}
		}
	}
	else
		m_leader = false;
}

void
GossipApp::if_get_block (void)
{
	int sum = 0;
	for (int i = 0; i < BLOCK_PIECE_NUMBER; i++)
		sum += map_blockpiece_received[i];
	if (sum == BLOCK_PIECE_NUMBER)
		block_got = true;
	else
		block_got = false;
}

std::string
GossipApp::MessagetypeToString (int x)
{
	std::string res;
	switch (x)
    {
    case 0:
		res = "BLOCK";
		break;
    case 1:
		res = "SOLICIT";
		break;
    case 2:
		res = "ACK";
		break;
    case 3:
		res = "PREPARE";
		break;
    case 4:
		res = "COMMIT";
		break;
    case 5:
		res = "REPUTATION";
		break;
    }
	return res;
}

void
GossipApp::GetNeighbor (int node_number, int out_neighbor_choosed[])
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


	// std::string str3 = "scratch/subdir/log_cmjj/node_";
	// str3.append(std::to_string((int)m_node_id));
	// str3.append("_link_log.txt");
	// std::ifstream infile;
	// infile.open(str3);
	// char str4[30];
	// infile.getline(str4, 30);
	// std::string str5(str4);
	// std::vector<std::string> res = SplitString(str5, ' ');
	// for(int i=0; i<NODE_NUMBER; i++)
	// {
	// 	out_neighbor_choosed[i] = res[i+1];
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

void
GossipApp::StartApplication (void)
{
	NS_LOG_FUNCTION (this);
	GetNeighbor (OUT_GOSSIP_ROUND, out_neighbor_choosed);
	// GetNeighbor (IN_GOSSIP_ROUND, in_neighbor_choosed);

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

  // if (m_socket_receive == 0)
  //   {
  //     TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
  //     m_socket_receive = Socket::CreateSocket (GetNode (), tid);
  //     InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_port);
  //     if (m_socket_receive->Bind (local) == -1)
  //     {
  //       NS_FATAL_ERROR ("Failed to bind socket");
  //     }
  //     if(m_socket_receive->Listen()!=0)
  //       std::cout<<"listen failed"<<std::endl;
  //     m_socket_receive->ShutdownSend();
  //   }
  // m_socket_receive->SetAcceptCallback(MakeNullCallback<bool, Ptr<Socket>, const Address &> (), 
  //   MakeCallback (&GossipApp::HandleAccept, this));
  // m_socket_receive->SetRecvCallback (MakeCallback (&GossipApp::HandleRead, this));

	for (int i = 0; i < NODE_NUMBER; i++)
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

	for (int i = 0; i < NODE_NUMBER; i++)
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
	gracecount = WINDOW_SIZE + 2;
	quad.B_root = GENESIS_BLOCK;
	quad.H_root = 0;
	quad.B_pending = EMPTY_BLOCK;
	quad.freshness = 0;
	block_received.name = 0;
	block_received.height = 0;
	total_traffic = 0;
  
	if(m_node_id == 113 or m_node_id == 117)
	{
		Silence_Attacker = 1;
		std::cout<<"node "<<std::setw(2)<<(int)m_node_id<<" is a silence attacker"<<std::endl;
	}
	else
	{
		Silence_Attacker = 0;
	}
  
	int ss[5] = {132, 137, 114, 115, 123};
	std::vector<int> v1(ss, ss+5);
	std::vector<int>::iterator iE1 = find(v1.begin(), v1.end(), (int)m_node_id);
	if(iE1==v1.end())
	{
		Bias_Attacker = 0;
	}
	else
	{
		Bias_Attacker = 1;
		std::cout<<"node "<<std::setw(2)<<(int)m_node_id<<" is a bias attacker"<<std::endl;
	}
	// bias_attacker_induced = false;
	// bias_attacker_prevented = false;

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
	int sum = m_local_ledger.size ();
	std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~"<<(int)m_node_id<<" information~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;
	std::cout << "node " << (int) m_node_id << " got " << sum << " consensed block" << std::endl; 
 	std::cout<< "node " << (int) m_node_id << " got "<< total_traffic<<" Bytes data"<<std::endl;
  // std::string str1 = "scratch/subdir/log_cmjj/node_";
	// str1.append(std::to_string((int)m_node_id));
	// str1.append("_receiving_time_log.txt");
	// std::ofstream outfile1;
	// outfile1.open(str1);
  // for (int j = 1; j <= TOTAL_EPOCH_FOR_SIMULATION; j++)
  // {
  //   outfile1 << "****epoch " << j << " receiving time information****" << std::endl;
  //   outfile1<<"epoch "<<j <<": "<<"starts at "<<map_epoch_start_time[j]<<"s"<<std::endl;
  //   outfile1<<"epoch "<<j<<": "<<"block propagation time: "<<map_epoch_len_phase1[j]<<", voting time: "
  //     <<map_epoch_len_phase2[j]<<std::endl;
  //   for (int n = 0; n < NODE_NUMBER; n++)
  //   {
  //     outfile1 << "node " << n << " got block at " << map_epoch_node_getblocktime[j][n]
  //               << "s" << std::endl;
  //   }
  //   for (int n = 0; n < NODE_NUMBER; n++)
  //   {
  //     outfile1 << "node " << n << " got committed at "
  //               << map_epoch_node_getcommittedtime[j][n] << "s" << std::endl;
  //   }  
  // }

  // outfile1 << "**** phase length information****" << std::endl;
  // for(int n=1; n<TOTAL_EPOCH_FOR_SIMULATION; n++)
  // {
  //   outfile1<<"epoch "<<n <<": "<<"starts at "<<map_epoch_start_time[n]<<"s"<<std::endl;
  //   outfile1<<"epoch "<<n<<": "<<"block propagation time: "<<map_epoch_len_phase1[n]<<", voting time: "
  //     <<map_epoch_len_phase2[n]<<std::endl;
  // }
	for (int j = 1; j <= m_epoch - WINDOW_SIZE; j++)
	{
		if(j>=3)
			log_time_file << "BR in epoch "<< j <<": "<< map_epoch_node_BR[j][(int)m_node_id]<<std::endl;
	}
	log_time_file.close();
	log_rep_file.close();
	for (int i = 0; i < (int) m_local_ledger.size (); i++)
	{
		std::cout << "local ledger height " << std::setw (3)<< i << ": " << std::setw (11) << m_local_ledger[i]
		          << "    ";
		std::cout << "in epoch " << m_ledger_built_epoch[i] << std::endl;
	}
	std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~"<<(int)m_node_id<< " information~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;
}

void
GossipApp::SendBlock (int dest, Block b)
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

void
GossipApp::SendBlockPiece (int dest, int piece, Block b)
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
		std::cout << "Failed to send block piece "<<piece << std::endl;
}

void
GossipApp::SendBlockAck (int dest, Block b)
{
	for (int piece = 0; piece < 1; piece++)
	{
		srand (Simulator::Now ().GetSeconds ());
		float x = rand () % 100;
		x = x / 100;
		Simulator::Schedule (Seconds (x), &GossipApp::SendBlockPiece, this, dest, piece, b);
	}
	std::cout << "node " << (int) GetNodeId () << " reply a block to node " << dest << " at "
	        << Simulator::Now ().GetSeconds () << "s" << std::endl;
}

void
GossipApp::SendBlockAckPiece (int dest, int piece, Block b)
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
	str1.append (std::to_string (b.height));
	str1.append ("+");
	str1.append (std::to_string ((int) piece));
	const uint8_t *str3 = reinterpret_cast<const uint8_t *> (str1.c_str ());
	Packet pack1 (str3, 1024 * 16);
	Ptr<Packet> p = &pack1;
	if (m_socket_send[dest]->Send (p) == -1)
	{
		float x = rand () % 200;
		x = x / 200 + 0.2;
		std::cout << "Failed to send ack block piece, will retransmit in "<<x<<"s" << std::endl;
		Simulator::Schedule (Seconds (x), &GossipApp::SendBlockPiece, this, dest, piece, b);
	}
    
    
}

void
GossipApp::SendPrepare (int dest, Block b)
{
	std::string str1 = "";
	str1.append (std::to_string ((int) m_epoch));
	str1.append ("+");
	str1.append (std::to_string ((int) m_node_id));
	str1.append ("+");
	std::string str2 = "PREPARE";
	str1.append (str2);
	str1.append ("+");
	str1.append (std::to_string (b.name));
	str1.append ("+");
	str1.append (std::to_string (b.height));
	const uint8_t *str3 = reinterpret_cast<const uint8_t *> (str1.c_str ());
	Packet pack1 (str3, 100);
	Ptr<Packet> p = &pack1;
	if(m_socket_send[dest]->Send (p)==-1)
		std::cout<<std::setw(2)<<"node "<<(int)m_node_id<<" send prepare failed, tx buffer: "<<
			m_socket_send[dest]->GetTxAvailable()<<std::endl;
}

void
GossipApp::SendCommit (int dest, Block b)
{
	std::string str1 = "";
	str1.append (std::to_string ((int) m_epoch));
	str1.append ("+");
	str1.append (std::to_string ((int) m_node_id));
	str1.append ("+");
	std::string str2 = "COMMIT";
	str1.append (str2);
	str1.append ("+");
	str1.append (std::to_string (b.name));
	str1.append ("+");
	str1.append (std::to_string (b.height));
	const uint8_t *str3 = reinterpret_cast<const uint8_t *> (str1.c_str ());
	Packet pack1 (str3, 100);
	Ptr<Packet> p = &pack1;
	if(m_socket_send[dest]->Send (p)==-1)
		std::cout<<std::setw(2)<<(int)m_node_id<<" send commit failed, tx buffer: "<<m_socket_send[dest]->GetTxAvailable()<<std::endl;
}

void
GossipApp::SendTimeMessage (int dest, int t, int a, int b, int c, int d)
{

	std::string str1 = "";
	str1.append (std::to_string (t));
	str1.append ("+");
	str1.append (std::to_string ((int) m_node_id));
	str1.append ("+");
	std::string str2 = "TIMEMESSAGE";
	str1.append (str2);
	str1.append ("+");
	str1.append (std::to_string (a));
	str1.append ("+");
	str1.append (std::to_string (b));
	str1.append ("+");
	str1.append (std::to_string (c));
	str1.append ("+");
	str1.append (std::to_string (d));
  
	const uint8_t *str3 = reinterpret_cast<const uint8_t *> (str1.c_str ());
	Packet pack1 (str3, 100);
	Ptr<Packet> p = &pack1;
	if (m_socket_send[dest]->Send (p) == -1)
		std::cout <<std::setw(2)<<(int)m_node_id<< " fail to send time msg, tx available:"<<m_socket_send[dest]->GetTxAvailable() << std::endl;
	// else
	// {
	// 	if(m_node_id==0)
	// 		std::cout<<std::setw(2)<<(int)m_node_id<<" send "<<str1<<std::endl;
	// }
}

void GossipApp::SendViewplusplus(int dest)
{
	std::string str1 = "";
	str1.append (std::to_string ((int) m_epoch));
	str1.append ("+");
	str1.append (std::to_string ((int) m_node_id));
	str1.append ("+");
	std::string str2 = "VIEWPLUSPLUS";
	str1.append (str2);
	const uint8_t *str3 = reinterpret_cast<const uint8_t *> (str1.c_str ());
	Packet pack1 (str3, 100);
	Ptr<Packet> p = &pack1;
	if(m_socket_send[dest]->Send (p)==-1)
		std::cout <<(int)m_node_id<< " send view++ failed, tx available:"<<m_socket_send[dest]->GetTxAvailable() << std::endl;
	else
		std::cout << "node " << (int) GetNodeId () << " send "<<str1<<" to node " << dest << " at "
			<< Simulator::Now ().GetSeconds () << "s, tx buffer available: "<< m_socket_send[dest]->GetTxAvailable() << std::endl;
}

void GossipApp::RelayViewpluplusMessage(int dest, Ptr<Packet> p)
{
	if(m_socket_send[dest]->Send(p)==-1)
		std::cout<<(int)m_node_id<<"error view change msg relay, tx available:"<<m_socket_send[dest]->GetTxAvailable()<<std::endl;
	else
		std::cout<<"node "<<(int)m_node_id<<" relay view change msg to node "<<dest<<" at "<<Simulator::Now().GetSeconds()
		  <<"s in epoch "<<(int) m_epoch<<" tx buffer: "<<m_socket_send[dest]->GetTxAvailable()<<std::endl;
}

void
GossipApp::RelayVotingMessage (int dest, Ptr<Packet> p)
{
	if(m_socket_send[dest]->Send(p)==-1)
		std::cout<<(int)m_node_id<<"error voting msg relay, tx available:"<<m_socket_send[dest]->GetTxAvailable()<<std::endl;
}

void
GossipApp::RelayTimeMessage (int dest, Ptr<Packet> p)
{
	if(m_socket_send[dest]->Send(p)==-1)
		std::cout<<(int)m_node_id<<"error time msg relay, tx available:"<<m_socket_send[dest]->GetTxAvailable()<<std::endl;
}


std::pair<float, float>
GossipApp::InitializeEpoch ()
{
	for (int i = 0; i < NODE_NUMBER; i++)
	{
		map_blockpiece_received[i] = 0;
		map_node_PREPARE[i] = 0;
		map_node_COMMIT[i] = 0;
	}
	receive_viewplusplus_time = 0;
	query_time = 0;
	map_epoch_viewchange_happen[m_epoch] = 0;
	receive_history = 0;
	block_got = false;
	map_epoch_consensed[m_epoch] = 0;


	m_len = NewLen();
	if(m_epoch==1)
// restart point
	{
		len_phase1 *= 1;
		len_phase2 *= 1;
	}
	else if(gracecount==WINDOW_SIZE+2)
	{
   //  len_phase1 *= 2;
  	// len_phase2 *= 2;
// restart point
    len_phase1 *= 1;
    len_phase2 *= 1;
	}
	else if(gracecount>0)
	{
		len_phase1 *= 1;
    	len_phase2 *= 1;
	}
	else
	{
		len_phase1 = m_len.first;
		len_phase2 = m_len.second;
	}
	map_epoch_len_phase1[m_epoch] = len_phase1;
	map_epoch_len_phase2[m_epoch] = len_phase2;
	state = 0;
	for (int i = 0; i < NODE_NUMBER; i++)
	{
		map_epoch_node_getblockornot[m_epoch][i] = -1;
		map_epoch_node_getcommitedornot[m_epoch][i] = -1;
		map_epoch_node_getblocktime[m_epoch][i] = -1;
		map_epoch_node_getcommittedtime[m_epoch][i] = -1;
	}
	// if (m_epoch > 1)
	// 	GossipTimeMessage (m_epoch - 1, get_block_or_not, get_committed_or_not, get_block_time, get_committed_time);
	get_block_or_not = 0;
	get_committed_or_not = 0;
	get_block_time = -1;
	get_committed_time = -1;


	std::pair<float, float> p;
	p.first = len_phase1;
	p.second = len_phase2;
	return p;
}

void
GossipApp::EndSummary ()
{
	int n = m_epoch;
	for (int i = 0; i < NODE_NUMBER; i++)
	{
		if (map_epoch_node_getblockornot[n - 1][i] == -1)
		  map_epoch_node_getblockornot[n - 1][i] = 0;
		if (map_epoch_node_getcommitedornot[n - 1][i] == -1)
		  map_epoch_node_getcommitedornot[n - 1][i] = 0;
		if (map_epoch_node_getblocktime[n - 1][i] == -1 or 
		  map_epoch_node_getblocktime[n - 1][i]>map_epoch_len_phase1[n-1])
		  map_epoch_node_getblocktime[n - 1][i] = map_epoch_len_phase1[n-1];
		if (map_epoch_node_getcommittedtime[n - 1][i] == -1 or
		  map_epoch_node_getcommittedtime[n - 1][i]>map_epoch_len_phase1[n-1] + map_epoch_len_phase2[n-1])
		  map_epoch_node_getcommittedtime[n - 1][i] = map_epoch_len_phase1[n-1] + map_epoch_len_phase2[n-1];
	}

	log_time_file<<"epoch "<<n <<": "<<"starts at "<<map_epoch_start_time[n]<<"s"<<std::endl;
	log_time_file<<"epoch "<<n <<": "<<"block propagation time: "<<map_epoch_len_phase1[n]<<", voting time: "
		<<map_epoch_len_phase2[n]<<std::endl;
	int ind = (int)m_node_id;
	log_time_file<<"epoch "<<n-1<<" get block time: "<<map_epoch_node_getblocktime[n-1][ind]<<std::endl;
	log_time_file<<"epoch "<<n-1<<" get consensed time: "<<map_epoch_node_getcommittedtime[n - 1][ind]<<std::endl;
	log_rep_file<<"CR in epoch "<<n <<": "<<map_epoch_node_CR[n][ind]<<std::endl;
	log_rep_file<<"BR in epoch "<<n <<": "<<map_epoch_node_BR[n][ind]<<std::endl;

	if(map_epoch_consensed[n]==1)
	{
		if(gracecount>0)
		{
			gracecount_backup = gracecount;
			gracecount--;
		}
	}
	else
	{
		gracecount_backup = gracecount;
		gracecount = WINDOW_SIZE + 2;
	}
	log_rep_file<<"gracecount value in epoch "<<n <<": "<<gracecount<<std::endl;

}

void GossipApp::UpdateCRGain()
{
	if(m_epoch>=3)
	{
		for(int i=0; i<NODE_NUMBER; i++)
		{
			float CR_gain = map_epoch_node_getblockornot[m_epoch-2][i]*(map_epoch_len_phase1[m_epoch-2] - 
			map_epoch_node_getblocktime[m_epoch-2][i])/map_epoch_len_phase1[m_epoch-2] +
			map_epoch_node_getcommitedornot[m_epoch-2][i]*(map_epoch_len_phase1[m_epoch-2] + map_epoch_len_phase2[m_epoch-2] - 
			map_epoch_node_getcommittedtime[m_epoch-2][i])/map_epoch_len_phase2[m_epoch-2];
			map_epoch_node_CR_gain[m_epoch][i] = CR_gain;
		}
	}
	// else
	// 	std::cout<<"too early, not enough message"<<std::endl;
}

void GossipApp::UpdateCR()
{
	for(int i=0; i<NODE_NUMBER; i++)
	{
		float sum=0;
		for(int j=0; j<WINDOW_SIZE; j++)
		{
			sum += map_epoch_node_CR_gain[m_epoch-j][i];
		}
		map_epoch_node_CR[m_epoch][i] = sum/WINDOW_SIZE;
	}
  // if(map_epoch_node_CR.size()>WINDOW_SIZE)
  // {
  //   int x = m_epoch -(WINDOW_SIZE + 2);
    // map_epoch_node_CR.erase(x);
  // }
}

void GossipApp::UpdateBR()
{
	for(int i=0; i<NODE_NUMBER; i++)
	{
		std::vector<float> v1;
		std::vector<float> v2;
		for(int i=0; i<NODE_NUMBER; i++)
		{
			v1.push_back(map_epoch_node_CR[m_epoch-1][i]);
			v2.push_back(map_epoch_node_CR[m_epoch][i]);
		}
		float res = DistanceOfPermu(i, v1, v2);
		map_node_BR[i] *= exp(-1*res);
		map_epoch_node_BR[m_epoch][i] = map_node_BR[i];
	}
	log_rep_file<<"node "<<std::setw(2)<<(int)m_node_id<<" BR in epoch "<<(int)(m_epoch)<<": "
		<<map_node_BR[(int)m_node_id]<<std::endl;
}


float GossipApp::DistanceOfPermu(int i, std::vector<float> v1, std::vector<float> v2)
{
	std::vector<float> x(v1);
	std::vector<float> y(v2);
	sort(x.begin(), x.end());
	sort(y.begin(), y.end());
	std::vector<float>::iterator iElement1 = find(x.begin(), x.end(), map_epoch_node_CR[m_epoch-1][i]);
	std::vector<float>::iterator iElement2 = find(y.begin(), y.end(), map_epoch_node_CR[m_epoch][i]);
	int pos_1 = distance(x.begin(), iElement1);
	int pos_2 = distance(y.begin(), iElement2);
	float res = float(std::abs(pos_1 - pos_2)) / float(NODE_NUMBER);
	if(i==(int)m_node_id)
		log_rep_file<<res<<" in epoch "<<m_epoch<<"~~~~~~~~"<<pos_1<<"~~~~~~~~"<<pos_2<<"~~~~~~~~"<<std::endl;
  if(res>=0.66)  // problem about parameter 0.7 may exist
    return res;
  else
    return 0;
}

float GossipApp::AvgByCR(int node, std::vector<float> vec_CR, std::map<int, float> map_node_time)
{
	std::vector<float> tmp1;
	for(int i=0; i<NODE_NUMBER; i++)
	{
		tmp1.push_back(abs(vec_CR[i] - vec_CR[node]));
	}
	std::vector<float> tmp2(tmp1);
	std::vector<float> nearest_one;
	sort(tmp2.begin(), tmp2.end());
	for(int i=0; i<PATCH; i++)
	{
		std::vector<float>::iterator iE1 = find(tmp1.begin(), tmp1.end(), tmp2[i]);
		int pos = distance(tmp1.begin(), iE1);
		nearest_one.push_back(pos);
	}
	float res = 0;
	for(int i=0; i<PATCH; i++)
		res += map_node_time[nearest_one[i]];
	return res/PATCH;
}

// void GossipApp::if_bias_attack_induction_timing()
// {
// 	int x = m_epoch;
// 	if(map_epoch_consensed[x-1]==1 && map_epoch_consensed[x-2]==1 && map_epoch_consensed[x-3]==1)
// 	{
// 		bias_attacker_induced = true;
// 	}
// }

// void GossipApp::if_bias_attack_prevent_timing()
// {
// 	int x = m_epoch;
// 	if(map_epoch_consensed[x-2]==0 && map_epoch_consensed[x-1]==1 && bias_attacker_induced)
// 	{
// 		bias_attacker_induced = false;
// 		bias_attacker_prevented = true;
// 	}
  
// }

std::pair<float, float> GossipApp::NewLen()
{

	// if(m_epoch<WINDOW_SIZE+3)
// restart point
	if(m_epoch<WINDOW_SIZE+300000)
	{
		std::pair<float, float> p;
		p.first = len_phase1;
		p.second = len_phase2;
		return p;
	}
	else
	{
		std::vector<float> time_block_prediction;
		std::vector<float> time_vote_prediction;
    	//******** time prediction for nodes
		for(int i=0; i<NODE_NUMBER; i++)
		{
			std::vector<float> v1; // contains CR 
			for(int j=0; j<NODE_NUMBER; j++)
			{
				v1.push_back(map_epoch_node_CR[m_epoch][j]);
			}
			float tmp1 = AvgByCR(i, v1, map_epoch_node_getblocktime[m_epoch-2]);
			std::map<int, float> v2;  // contains voting time
			for(int k=0; k<NODE_NUMBER; k++)
			{
				v2[k] = map_epoch_node_getcommittedtime[m_epoch-2][k]-map_epoch_len_phase1[m_epoch-2];
			}
			float tmp2 = AvgByCR(i, v1, v2);
			if(map_node_BR[i]>0.95) // some problem with this parameter 0.9
			{
				time_block_prediction.push_back(tmp1);
				time_vote_prediction.push_back(tmp2);
			}
			// time_block_prediction.push_back(tmp1);
			// time_vote_prediction.push_back(tmp2);
		}
    	//******** get len according to predicted time 
		auto maxPosition1 = max_element(time_block_prediction.begin(), time_block_prediction.end());
		float len1 = time_block_prediction[maxPosition1 - time_block_prediction.begin()];
		auto maxPosition2 = max_element(time_vote_prediction.begin(), time_vote_prediction.end());
		float len2 = time_vote_prediction[maxPosition2 - time_vote_prediction.begin()];

		std::pair<float, float> p;
		p.first = len1 + EPSILON1;
		p.second = len2 + EPSILON2;
		return p;
	}
}

Block
GossipApp::BlockPropose ()
{
  Block block_proposed;
  if (quad.B_pending.name == 0)
    {
      srand (time (NULL));
      block_proposed.name = rand () % 0xFFFFFFFF;
      block_proposed.height = m_local_ledger.size ();
      std::cout << "leader "<<(int)m_node_id<<" build a new block " << block_proposed.name
                << " because he has no pending block" << std::endl;
    }
  else
    {
      block_proposed.name = quad.B_pending.name;
      block_proposed.height = quad.B_pending.height;
      std::cout << "leader repropose his pending block " << block_proposed.name << std::endl;
    }
  return block_proposed;
}

void
GossipApp::LeaderGossipBlockOut (Block b)
{
	srand (Simulator::Now ().GetSeconds () + m_node_id);
  	for (int i = 0; i < OUT_GOSSIP_ROUND; i++)
    {
      
    	float x = rand () % 1000;
    	x = x / 1000 + 0.1 * i;
    	Simulator::Schedule (Seconds (x), &GossipApp::SendBlock, this, out_neighbor_choosed[i], b);
    }
}

void GossipApp::GossipViewplusplusMsg()
{
	for (int i = 0; i < OUT_GOSSIP_ROUND; i++)
	{
		srand (Simulator::Now ().GetSeconds () + m_node_id);
		int x1 = rand () % 100;
		float x = x1 / 100.0 + 0.1*i;
		Simulator::Schedule(Seconds(x), &GossipApp::SendViewplusplus, this , out_neighbor_choosed[i]);
	}
}

void
GossipApp::GossipPrepareOut ()
{
  
	if (block_got)
	{
		if (block_received.height == (int) m_local_ledger.size ())
        {
			if (quad.B_pending.name == EMPTY_BLOCK.name)
            {
				id2 = Simulator::Schedule (Seconds (DETERMINECONSENS_INTERVAL), &GossipApp::GossipCommitOut, this);
				state = 1;
				quad.B_pending = EMPTY_BLOCK;
				quad.freshness = 0;
				std::cout << "node " <<std::setw(2)<< (int) m_node_id << " send prepare msg for block "<< block_received.name 
				<<" at "<<Simulator::Now().GetSeconds()<<"s in epoch "<<m_epoch<<" tx buffer: ";
				  
				for (int i = 0; i < OUT_GOSSIP_ROUND; i++)
				{
					srand (Simulator::Now ().GetSeconds () + m_node_id);
					float x = rand () % 100;
					x = x / 100 + 0.1 * i;
					Simulator::Schedule(Seconds(x), &GossipApp::SendPrepare, this, out_neighbor_choosed[i], block_received);
					// SendPrepare (out_neighbor_choosed[i], block_received);
					int dest = out_neighbor_choosed[i];
					std::cout<<m_socket_send[dest]->GetTxAvailable()<<" ";
				}
				std::cout<<std::endl;
            }
			else if (m_epoch > quad.freshness)
            {
				state = 1;
				quad.B_pending = EMPTY_BLOCK;
				quad.freshness = 0;
				for (int i = 0; i < OUT_GOSSIP_ROUND; i++)
                {
					srand (Simulator::Now ().GetSeconds () + m_node_id);
					float x = rand () % 100;
					x = x / 100 + 0.1 * i;
					Simulator::Schedule(Seconds(x), &GossipApp::SendPrepare, this, out_neighbor_choosed[i], block_received);
                }
            }
        }
    }
}

void
GossipApp::GossipBlockAfterReceive (int from_node, Block b)
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

void GossipApp::GossipTimeMessage (int t, int a, int b, int c, int d)
{
	for (int i = 0; i < OUT_GOSSIP_ROUND; i++)
	{
		srand (Simulator::Now ().GetSeconds () + m_node_id);
		// SendTimeMessage(out_neighbor_choosed[i], t, a, b, c, d);
		float x = rand () % 100;
		x = x / 100 + 0.1 * i;
		Simulator::Schedule(Seconds(x), &GossipApp::SendTimeMessage, this, out_neighbor_choosed[i], t, a, b, c, d);
    }
}

void GossipApp::GossipCommitOut () // send commit msg out
{
  if ((Simulator::Now ().GetSeconds () - m_epoch_beginning) >= len_phase1)
    {
      int sum = 0;
      for (int i = 0; i < NODE_NUMBER; i++)
        {
          sum += map_node_PREPARE[i];
        }
      // if(m_node_id==2)
      //   std::cout<<"######################node "<<2<<" has "<<sum<<" prepare msg at "
      //     <<Simulator::Now().GetSeconds()<<"s ######################"<<std::endl;
      if (sum >= (2 * NODE_NUMBER / 3.0 + 1))
        {
          state = 2;
          quad.B_pending = block_received;
          quad.freshness = m_epoch;
          get_prepared_time = Simulator::Now ().GetSeconds () - m_epoch_beginning;
          get_prepared_time = ((int) (get_prepared_time * 1000)) / 1000.;
          // std::cout << "node " << (int) m_node_id << " send commit msg for block "<< block_received.name 
          //   <<"at "<<Simulator::Now().GetSeconds()<<"s in epoch "<<m_epoch << std::endl;
          for (int i = 0; i < OUT_GOSSIP_ROUND; i++)
            {
              srand (Simulator::Now ().GetSeconds () + m_node_id);
              float x = rand () % 1000;
              x = x / 1000 + 0.1 * i;
              Simulator::Schedule (Seconds (x), &GossipApp::SendCommit, this,
                                   out_neighbor_choosed[i], block_received);
            }
          Simulator::Schedule (Seconds (DETERMINECONSENS_INTERVAL), &GossipApp::DetermineConsens,
                               this);
        }
      else
        Simulator::Schedule (Seconds (DETERMINECOMMIT_INTERVAL), &GossipApp::GossipCommitOut, this);
    }
}

void
GossipApp::DetermineConsens ()
{
	if ((Simulator::Now ().GetSeconds () - m_epoch_beginning) >= len_phase1)
	{
		int sum = 0;
		for (int i = 0; i < NODE_NUMBER; i++)
		{
			sum += map_node_COMMIT[i];
		}
		if (sum >= (2 * NODE_NUMBER / 3.0 + 1))
		{
			std::cout << "node " <<std::setw(2)<< (int) m_node_id << " consensed block " << block_received.name
			        << " to its local ledger at height "<<m_local_ledger.size()<<" at "<<Simulator::Now().GetSeconds()<<"s in epoch "<<m_epoch<< std::endl;
			state = 3;
			if(Bias_Attacker==1)
				get_committed_time = len_phase1 + 2.0;
			else
				get_committed_time = Simulator::Now ().GetSeconds () - m_epoch_beginning;
			get_committed_time = (int(get_committed_time * 1000)) / 1000.;
			get_committed_or_not = 1;
			map_epoch_consensed[m_epoch] = 1;
			quad.B_root = block_received;
			quad.H_root = block_received.height;
			quad.B_pending = EMPTY_BLOCK;
			quad.freshness = 0;
			m_local_ledger.push_back (block_received.name);
			m_ledger_built_epoch.push_back (m_epoch);
		}
		else
			Simulator::Schedule (Seconds (DETERMINECONSENS_INTERVAL), &GossipApp::DetermineConsens, this);
	}
}

// void
// GossipApp::SolicitBlockFromOthers ()
// {
//   if ((Simulator::Now ().GetSeconds () - m_epoch_beginning) >= waitting_time)
//     {

//       if (block_got == false)
//         {
//           // int neighbors[SOLICIT_ROUND];
//           // ChooseNeighbor(SOLICIT_ROUND, neighbors);
//           // for(int i=0; i<SOLICIT_ROUND; i++)
//           // {
//           //   ScheduleTransmit(Seconds (0.), neighbors[i], 1);
//           // }
//           srand (Simulator::Now ().GetSeconds () + m_node_id);
//           int i = rand () % IN_GOSSIP_ROUND;
//           ScheduleTransmit (Seconds (0.), in_neighbor_choosed[i], 1);
//           if ((Simulator::Now ().GetSeconds () - m_epoch_beginning + SOLICIT_INTERVAL) <
//               len_phase1 + len_phase2)
//             Simulator::Schedule (Seconds (SOLICIT_INTERVAL), &GossipApp::SolicitBlockFromOthers,
//                                  this);
//         }
//     }
// }

void GossipApp::SolicitBlock(int dest)
{
  std::string str1 = "";
  str1.append(std::to_string(m_epoch));
  str1.append("+");
  str1.append(std::to_string(m_node_id));
  str1.append("+");
  std::string str2 = "SOLICITBLOCK";
  str1.append(str2);
  const uint8_t *str3 = reinterpret_cast<const uint8_t *> (str1.c_str ());
  Packet pack1 (str3, 100);
  Ptr<Packet> p = &pack1;
  if (m_socket_send[dest]->Send (p) == -1)
    std::cout<<(int)m_node_id<<" Failed to solicit blokc, tx available:"<<m_socket_send[dest]->GetTxAvailable()<<std::endl;
    
}

void GossipApp::SolicitHistory(int dest, int h)
{
  std::string str1 = "";
  str1.append(std::to_string(m_epoch));
  str1.append("+");
  str1.append(std::to_string(m_node_id));
  str1.append("+");
  std::string str2 = "SOLICITHISTORY";
  str1.append(str2);
  str1.append("+");
  str1.append(std::to_string(h));
  const uint8_t *str3 = reinterpret_cast<const uint8_t *> (str1.c_str ());
  Packet pack1 (str3, 100);
  Ptr<Packet> p = &pack1;
  if (m_socket_send[dest]->Send (p) == -1)
    std::cout<<(int)m_node_id<<" Failed to solicit history, tx available:"<<m_socket_send[dest]->GetTxAvailable()<<std::endl;
  std::cout<<"node "<<std::setw(2)<<(int)m_node_id<<" query node "<<dest<<" for block history at height "<<h<<" at "
    <<Simulator::Now().GetSeconds()<<"s"<<std::endl;

}

void GossipApp::ReplyHistorySolicit(int dest, int h)
{
  std::string str1 = "";
  str1.append(std::to_string(m_epoch));
  str1.append("+");
  str1.append(std::to_string(m_node_id));
  str1.append("+");
  std::string str2 = "REPLYHISTORY";
  str1.append(str2);
  int len = m_local_ledger.size();
  for(int i=h; i<len; i++)
  {
    str1.append("+");
    str1.append(std::to_string(m_local_ledger[i]));
    str1.append("+");
    str1.append(std::to_string(m_ledger_built_epoch[i]));
  }
  str1.append("+");
  str1.append(str2);
  str1.append("+");
  str1.append(str2);
  const uint8_t *str3 = reinterpret_cast<const uint8_t *> (str1.c_str ());
  Packet pack1 (str3, 100);
  // Packet pack1 (str3, (int)str1.size());
  Ptr<Packet> p = &pack1;
  if (m_socket_send[dest]->Send (p) == -1)
    std::cout<<(int)m_node_id<<" Failed to reply history solicit , tx available:"<<m_socket_send[dest]->GetTxAvailable()<<std::endl;
  std::cout<<"node "<<std::setw(2)<<(int)m_node_id<<" reply node "<<dest<<" for query at "
    <<Simulator::Now().GetSeconds()<<"s and send him "<<str1<<std::endl;
}

void GossipApp::RecoverHistory(std::vector<uint32_t> b, std::vector<int> b_epo, int dest)
{
	map_epoch_consensed[m_epoch-1] = 1;
	int i=0;
	for(; i<(int)b.size(); i++)
	{
		m_local_ledger.push_back(b[i]);
		m_ledger_built_epoch.push_back(b_epo[i]);
		i++;
	}

	quad.B_root.name = b[i-1];
	quad.B_root.height = 0;
	quad.H_root = m_local_ledger.size();
	quad.B_pending = EMPTY_BLOCK;
	quad.freshness = 0;


	Simulator::Cancel(id1);
	Simulator::Cancel(id3);
	Simulator::Cancel(id4);

	gracecount = gracecount_backup;
	if(gracecount>0)
	{
		gracecount--;
	}
	gracecount_backup = gracecount;
	if(gracecount==1)
	{
		for(int i=0; i<NODE_NUMBER; i++)
		{
			map_node_BR[i] = 1;
			map_epoch_node_BR[m_epoch][i] = 1;
		}
		UpdateCR();
	}
	if(gracecount==0)
	{
		UpdateCR();
		UpdateBR();
	}
	if(gracecount==0)
	{
      std::pair<float, float> p = NewLen();
      len_phase1 = m_len.first;
      len_phase2 = m_len.second;
      std::cout<<"node "<<std::setw(2)<<(int)m_node_id<<" recoverd his local ledger by computation at "<<Simulator::Now().GetSeconds()<<
      	"s and set his epoch length to block propagation: "<<len_phase1<<", voting: "<<len_phase2<<std::endl;
	}
	else
	{
		len_phase1 /= 2;
    	len_phase2 /= 2;

    	std::cout<<"node "<<std::setw(2)<<(int)m_node_id<<" recoverd his local ledger by halving at "<<Simulator::Now().GetSeconds()<<
			 "s and set his epoch length to block propagation: "<<len_phase1<<", voting: "<<len_phase2<<std::endl;
	}
// change for synchronous point


	map_epoch_len_phase1[m_epoch] = len_phase1;
	map_epoch_len_phase2[m_epoch] = len_phase2;
	float x1 = Simulator::Now().GetSeconds() - m_epoch_beginning;
	if(x1<len_phase1)
	{
		id1 = Simulator::Schedule (Seconds (len_phase1 - x1), &GossipApp::GossipPrepareOut, this);
	}
	else
	{
		if(x1 + 0.1 < len_phase1 + len_phase2)
			id1 = Simulator::Schedule (Seconds(0.), &GossipApp::GossipPrepareOut, this);
		// std::cout<<std::setw(2)<<(int)m_node_id<<" will not participate in voting because of time issue"<<std::endl;
	}
	if(x1 + 0.1 < len_phase1 + len_phase2)
	{
		id3 = Simulator::Schedule (Seconds (len_phase1 + len_phase2 - 0.1 - x1), &GossipApp::EndSummary, this);
		if (m_epoch < TOTAL_EPOCH_FOR_SIMULATION)
		{ 
			id4 = Simulator::Schedule (Seconds (len_phase1 + len_phase2 - x1), &GossipApp::ConsensProcess, this);
		}
	}
	else
	{
		std::cout<<std::setw(2)<<(int)m_node_id<<"haults because of time issue"<<std::endl;
	}
  
  
  
  // std::cout<<"node "<<(int)m_node_id<<"'s local ledger: "<<std::endl;
  // for(int i=0; i<(int)m_local_ledger.size(); i++)
  // {
  //   std::cout<<m_local_ledger[i]<<"-->";
  // }
  // std::cout<<std::endl;
  SolicitBlock(dest);
}

void GossipApp::HandleAccept(Ptr<Socket> s, const Address& from)
{
  // std::cout<<"node "<<(int)m_node_id<<" handles accept at: "<<Simulator::Now().GetSeconds()<<"s"<<std::endl;
  s->SetRecvCallback (MakeCallback (&GossipApp::HandleRead, this));
}

void
GossipApp::HandleRead (Ptr<Socket> socket)
{
	NS_LOG_FUNCTION (this << socket);
	Ptr<Packet> packet;
	Address from;
	while ((packet = socket->RecvFrom (from)))
	{
	    total_traffic += packet->GetSize();
	    uint8_t content_[200];
	    packet->CopyData (content_, 200);
	    Ipv4Address from_addr = InetSocketAddress::ConvertFrom (from).GetIpv4 ();
	    int from_node = (int) map_addr_node[from_addr];
	    std::string str_of_content (content_, content_ + 200);
	    std::vector<std::string> res = SplitMessage (str_of_content, '+');
	    const char *time_of_recived_message = res[0].c_str ();

	    // std::cout<<"node "<<std::setw(2)<<(int)GetNodeId()<<" received a "<<content_<<" "<<packet->GetSize()
	    //   <<" bytes from node "<<from_node<<" at "<<Simulator::Now().GetSeconds()<<" s"<<std::endl;

	    if (strcmp (time_of_recived_message, (std::to_string (m_epoch)).c_str ()) == 0)
		{
			const char *type_of_received_message = res[2].c_str ();
			if (strcmp (type_of_received_message, "BLOCK") == 0)
			{
				uint32_t received_former_block = (atoi) (res[4].c_str ());
				uint32_t local_former_block = m_local_ledger[m_local_ledger.size()-1];
				if(received_former_block == local_former_block)
				{
					if (block_got == false)
					{
						int u = (atoi) (res[6].c_str ());
						map_blockpiece_received[u] = 1;
						if_get_block ();
						if (block_got == true)
						{
							uint32_t tmp1 = atoi (res[3].c_str ());
							int tmp2 = (atoi) (res[5].c_str ());
							block_received.name = tmp1;
							block_received.height = tmp2;
							std::cout << "node " <<std::setw(2)<< (int) GetNodeId () << " received a " << content_
							    << " from node " << from_node << " at "
							    << Simulator::Now ().GetSeconds () << " s, rcv buffer : "
							    << m_socket_receive[from_node]->GetTxAvailable()<< std::endl;
							if(Bias_Attacker==1)
								get_block_time = 5.0;
							else
								get_block_time = Simulator::Now ().GetSeconds () - m_epoch_beginning;
							get_block_time = ((int) (get_block_time * 1000)) / 1000.;
							get_block_or_not = 1;
							if(Simulator::Now().GetSeconds()-m_epoch_beginning < map_epoch_len_phase1[m_epoch])
								GossipBlockAfterReceive (from_node, block_received);
						}
					}
				}
				else
				{
					int received_block_height = (atoi) (res[5].c_str());
					if(received_block_height==((int)m_local_ledger.size()+1))
					{
						int local_ledger_height = (int) (m_local_ledger.size());
					// if(Simulator::Now().GetSeconds()<m_epoch_beginning+len_phase1)
					if(query_time==0)
					{
						SolicitHistory(from_node, local_ledger_height);
						query_time++;
					}
					  
					}
					else if(received_block_height>((int)m_local_ledger.size()+1))
					{
						std::cout<<std::setw(2)<< (int) GetNodeId ()<<"lost forever!"<<std::endl;
						Simulator::Cancel(id1);
						Simulator::Cancel(id3);
						Simulator::Cancel(id4);
					}
				  
				}

			}
			else if (strcmp (type_of_received_message, "PREPARE") == 0)
			{
				if (state == 1)
				{
				    uint32_t block_prepared = atoi (res[3].c_str ());
				    if (block_prepared == block_received.name)
					{
				        int x = (atoi) (res[1].c_str ());
				        if (map_node_PREPARE[x] == 0)
						{
				            map_node_PREPARE[x] = 1;
				            // if(m_epoch>=WINDOW_SIZE+2)
				            //   if(m_node_id==2)
				            //     std::cout<<"node "<<(int)GetNodeId()<<" received a "<<content_<<" "<<packet->GetSize()
				            //       <<" bytes from node "<<from_node<<" at "<<Simulator::Now().GetSeconds()<<" s"<<std::endl;
				            for (int i = 0; i < OUT_GOSSIP_ROUND; i++)
				            {
								srand (Simulator::Now ().GetSeconds ());
								float x = rand () % 1000;
								x = x / 1000;
								int dest = out_neighbor_choosed[i];
								Simulator::Schedule (Seconds (x), &GossipApp::RelayVotingMessage, this, dest, packet);
							}
						}
					}
				}
			}
			else if (strcmp (type_of_received_message, "COMMIT") == 0)
			{
				if (state == 2)
				{
				    uint32_t block_committed = atoi (res[3].c_str ());
				    if (block_committed == block_received.name)
					{
						int x = (atoi) (res[1].c_str ());
						if (map_node_COMMIT[x] == 0)
						{
						    map_node_COMMIT[x] = 1;
						    // if(m_epoch>=WINDOW_SIZE+2)
						    //   std::cout<<"node "<<(int)GetNodeId()<<" received a "<<content_<<" "<<packet->GetSize()
						    //     <<" bytes from node "<<from_node<<" at "<<Simulator::Now().GetSeconds()<<" s"<<std::endl;
						    for (int i = 0; i < OUT_GOSSIP_ROUND; i++)
							{
								srand (Simulator::Now ().GetSeconds ());
								float x = rand () % 1000;
								x = x / 1000;
								int dest = out_neighbor_choosed[i];
								Simulator::Schedule (Seconds (x), &GossipApp::RelayVotingMessage, this, dest, packet);
							}
						}
					}
				}
			}
			else if(strcmp (type_of_received_message, "SOLICITHISTORY") == 0)
			{
				int solicit_height = (atoi) (res[3].c_str());
				Simulator::Schedule(Seconds(0.3), &GossipApp::ReplyHistorySolicit, this, from_node, solicit_height);
			}
			else if(strcmp (type_of_received_message, "SOLICITBLOCK") == 0)
			{
				SendBlock(from_node, block_received);
			}
			else if(strcmp (type_of_received_message, "REPLYHISTORY") == 0)
			{
				if(receive_history==0)
				{
					receive_history++;
					std::cout<<"node "<<std::setw(2)<<(int)m_node_id<<" get REPLYHISTORY~~~~"<<std::endl;
					// std::cout<<"~~~~~~~~~~~~~~"<<(int)res.size()<<std::endl;
					// for(int i=0; i<(int)res.size(); i++)
					//   std::cout<<i<<"~~"<<res[i]<<std::endl;
					// std::cout<<"~~~~~~~~~~~~~~"<<std::endl;
					std::vector<uint32_t> history;
					std::vector<int> history_built_height;
					int i=3;
					const char* next_str = res[i].c_str();
					while(strcmp (next_str, "REPLYHISTORY") != 0)
					{
						uint32_t x = (atoi) (res[i].c_str());
						history.push_back(x);
						i++;
						int y = (atoi) (res[i].c_str());
						history_built_height.push_back(y);
						i++;
						next_str = res[i].c_str();
						std::cout<<x<<"    "<<y<<std::endl;
					}
					RecoverHistory(history, history_built_height, from_node);
				}
			}
			else if(strcmp (type_of_received_message, "VIEWPLUSPLUS") == 0)
			{
			// std::cout<<"node "<<(int)m_node_id<<" receive a view change msg at "<<Simulator::Now().GetSeconds()<<"s"<<std::endl;
				if(receive_viewplusplus_time==0)
				{
					view++;
					receive_viewplusplus_time++;
					std::cout<<"node "<<(int)m_node_id<<" receive view change msg from node "<<from_node<<" at "<< Simulator::Now().GetSeconds()<<
					" s, in epoch "<<(int) m_epoch<<" current view "<<view<<" rcv buffer "<<m_socket_receive[from_node]->GetTxAvailable()<< std::endl;
					map_epoch_viewchange_happen[m_epoch] = 1;

					for (int i = 0; i < OUT_GOSSIP_ROUND; i++)
					{
						srand (Simulator::Now ().GetSeconds ());
						float x = rand () % 1000;
						x = x / 1000;
						int dest = out_neighbor_choosed[i];
						Simulator::Schedule (Seconds (x), &GossipApp::RelayViewpluplusMessage, this, dest, packet);
					}

					uint8_t x = view % NODE_NUMBER;
					if (m_node_id == x)
					{
					    m_leader = true;
					    block_got = true;
					    get_block_or_not = 1;
					    get_block_time = Simulator::Now().GetSeconds();
					    get_block_time = ((int) get_block_time * 1000) / 1000;
					    // std::cout << "*****************"
					    //           << "epoch " << (int) m_epoch << " starts"
					    //           << "**********" << std::endl;
					    // std::cout << "****block propagation time: " << len_phase1 << "s" <<
					    //   "  ****voting time: " << len_phase2 <<"s" << "****" << std::endl;
					    // std::cout << "node " << (int) GetNodeId () << " is the leader, current view: "<< view << std::endl;
					    // std::cout << "time now: " << Simulator::Now().GetSeconds() << "s" << std::endl;
					    Block b = BlockPropose ();
					    block_received = b;
					    if(Silence_Attacker==0)
					    {
							LeaderGossipBlockOut (b);
							leadership_length++;
							std::cout<<"node "<<(int)m_node_id<<" is leader, leadership length: "<<leadership_length<<std::endl;
							// std::cout<<"node "<<(int)m_node_id<<" don't wanna gossip block"<<std::endl;
					    }
					    else
					    {
							std::cout<<"node "<<std::setw(2)<<(int)m_node_id<<" launches a silence attack"<<std::endl;
					    }
					}
				  
				}
			}
		}	
	    else if (strcmp (time_of_recived_message, (std::to_string (m_epoch - 1)).c_str ()) == 0 &&
	              m_epoch > 1)
	    {
	      // std::cout<<"time message received!"<<std::endl;
			const char *type_of_received_message = res[2].c_str ();
			if (strcmp (type_of_received_message, "TIMEMESSAGE") == 0)
			{
				int x = (atoi) (res[1].c_str ());
				if (map_epoch_node_getblockornot[m_epoch - 1][x] == -1)
				{
					map_epoch_node_getblockornot[m_epoch - 1][x] = (atoi) (res[3].c_str ());
					map_epoch_node_getcommitedornot[m_epoch - 1][x] = (atoi) (res[4].c_str ());
					map_epoch_node_getblocktime[m_epoch - 1][x] = std::stod (res[5].c_str ());
					map_epoch_node_getcommittedtime[m_epoch - 1][x] = std::stod (res[6].c_str ());
					// if(m_node_id==0)
					// 	std::cout<<"node "<<(int)GetNodeId()<<" received a "<<content_<<" "<<packet->GetSize()
					// 	    <<" bytes from node "<<from_node<<" at "<<Simulator::Now().GetSeconds()<<"s"<<std::endl;
					for (int i = 0; i < OUT_GOSSIP_ROUND; i++)
					{
						srand (Simulator::Now ().GetSeconds ());
						float x = rand () % 1000;
						x = x / 1000;
						int dest = out_neighbor_choosed[i];
						Simulator::Schedule (Seconds (x), &GossipApp::RelayTimeMessage, this, dest, packet);
					}
				}
			}
	    }
	}
}

} // Namespace ns3

