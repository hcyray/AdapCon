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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/address-utils.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/string.h"
#include "ns3/pointer.h"

#include "gossip-app.h"
#include "/home/hqw/repos/ns-3-allinone/ns-3-dev/scratch/data-struc.h"

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
  m_epoch_beginning = 0.;
  len_phase1 = 10.0;
  len_phase2 = 2.5;
  waitting_time = len_phase1 * 2 / 4.;

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
  // std::cout<<"read ip address"<<std::endl;
}

GossipApp::~GossipApp ()
{
  NS_LOG_FUNCTION (this);
  m_socket_receive = 0;
}

void
GossipApp::ConsensProcess ()
{
  // if(m_epoch>=1)
  //   GossipReputationMessage();
  // std::pair<int, int> p = NewLenComputation();
  // len_phase1 = p.first;
  // len_phase2 = p.second;
  // map_epoch_len_phase1[m_epoch] = len_phase1;
  // map_epoch_len_phase2[m_epoch] = len_phase2;
  // waitting_time = len_phase1*2/4.;

  m_epoch++;
  m_epoch_beginning = Simulator::Now ().GetSeconds ();
  UpdateCRGain();
  // if(m_epoch==WINDOW_SIZE+2)
  // {
  //   if(m_node_id==2)
  //     std::cout<<"it's time"<<std::endl;
  //   for(int i=0; i<NODE_NUMBER; i++)
  //   {
  //     map_node_BR[i] = 1;
  //   }
  //   // UpdateCR();
  // }
  // if(m_epoch>WINDOW_SIZE+2)
  // {
  //   UpdateCR();
  //   // UpdateBR();
  // }
  InitializeTimeMessage ();
  InitializeEpoch ();
  InitializeState ();
  if_leader ();
  if (m_leader)
    {
      std::cout << "*****************"
                << "epoch " << (int) m_epoch << " starts"
                << "**********" << std::endl;
      std::cout << "****block propagation time: " << len_phase1 << "s" <<
        "  ****voting time: " << len_phase2 <<"s" << "****" << std::endl;
      std::cout << "node " << (int) GetNodeId () << " is the leader, current view: "<< view << std::endl;
      std::cout << "time now: " << m_epoch_beginning << "s" << std::endl;
      Block b = BlockPropose ();
      block_received = b;
      LeaderGossipBlockOut (b);
    }
  // else
  // {
  //   Simulator::Schedule(Seconds(waitting_time), &GossipApp::SolicitBlockFromOthers, this);
  // }
  id1 = Simulator::Schedule (Seconds (len_phase1), &GossipApp::GossipPrepareOut, this);
  id2 = Simulator::Schedule (Seconds (len_phase1), &GossipApp::GossipCommitOut, this);
  id3 = Simulator::Schedule (Seconds (len_phase1 + len_phase2 - 0.1), &GossipApp::EndSummary, this);
  if (m_epoch < TOTAL_EPOCH_FOR_SIMULATION)
  {  
    id4 = Simulator::Schedule (Seconds (len_phase1 + len_phase2), &GossipApp::ConsensProcess, this);
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
    {
      m_leader = true;
      block_got = true;
      get_block_or_not = 1;
      get_block_time = 0;
    }
    else
    {
      if(consensed_this_epoch==true)
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
        GossipViewplusplusMsg();
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
  srand (time (NULL) + m_node_id);
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
              i++;
            }
        }
    }
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
  // std::cout<<"node "<<(int)GetNodeId()<<" starts!"<<std::endl;

  if (m_socket_receive == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket_receive = Socket::CreateSocket (GetNode (), tid);
      InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_port);
      if (m_socket_receive->Bind (local) == -1)
        {
          NS_FATAL_ERROR ("Failed to bind socket");
        }
    }
  m_socket_receive->SetRecvCallback (MakeCallback (&GossipApp::HandleRead, this));

  GetNeighbor (OUT_GOSSIP_ROUND, out_neighbor_choosed);
  GetNeighbor (IN_GOSSIP_ROUND, in_neighbor_choosed);
  for (int i = 0; i < NODE_NUMBER; i++)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      Ptr<Socket> socket_send = Socket::CreateSocket (GetNode (), tid);
      m_socket_send.push_back (socket_send);
      // std::cout<<"sending socket created!"<<std::endl;
      if (m_socket_send[i]->Connect (
              InetSocketAddress (Ipv4Address::ConvertFrom (map_node_addr[i]), 17)) == -1)
        NS_FATAL_ERROR ("Failer to connect socket");
    }

  view = 1;
  quad.B_root = GENESIS_BLOCK;
  quad.H_root = 0;
  quad.B_pending = EMPTY_BLOCK;
  quad.freshness = 0;
  block_received.name = 0;
  block_received.height = 0;
  consensed_this_epoch=false;
  Simulator::Schedule (Seconds (0.), &GossipApp::ConsensProcess, this);
}

void
GossipApp::StopApplication ()
{
  NS_LOG_FUNCTION (this);
  if (m_socket_receive != 0)
    {
      m_socket_receive->Close ();
      m_socket_receive->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }
  int sum = m_local_ledger.size ();
  std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~"<<(int)m_node_id<<" information~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;
  std::cout << "node " << (int) m_node_id << " got " << sum << " consensed block" << std::endl; 
  
  std::string str1 = "scratch/subdir/log_cmjj/node_";
	str1.append(std::to_string((int)m_node_id));
	str1.append("_receiving_time_log.txt");
	std::ofstream outfile1;
	outfile1.open(str1);
  for (int j = 1; j < TOTAL_EPOCH_FOR_SIMULATION; j++)
  {
    outfile1 << "****epoch " << j << " receiving time information****" << std::endl;
    for (int n = 1; n < NODE_NUMBER; n++)
    {
      outfile1 << "node " << n << " got block at " << map_epoch_node_getblocktime[j][n]
                << "s" << std::endl;
    }
    for (int n = 1; n < NODE_NUMBER; n++)
    {
      outfile1 << "node " << n << " got committed at "
                << map_epoch_node_getcommittedtime[j][n] << "s" << std::endl;
    }
  }
  outfile1.close();

  for (int i = 0; i < (int) m_local_ledger.size (); i++)
  {
    std::cout << "local ledger height " << std::setw (3)<< i << ": " << std::setw (12) << m_local_ledger[i]
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
    float x = rand () % 1000;
    x = x / 1000;
    Simulator::Schedule (Seconds (x), &GossipApp::SendBlockPiece, this, dest, piece, b);
  }
  // std::cout << "node " << (int) GetNodeId () << " send a block to node " << dest << " at "
  //           << Simulator::Now ().GetSeconds () << "s" << std::endl;
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
  Packet pack1 (str3, 1024 * 32);
  Ptr<Packet> p = &pack1;
  if (m_socket_send[dest]->Send (p) == -1)
    std::cout << "Failed to send packet" << std::endl;
}

void
GossipApp::SendBlockAck (int dest, Block b)
{
  for (int piece = 0; piece < 1; piece++)
    {
      srand (Simulator::Now ().GetSeconds ());
      float x = rand () % 1000;
      x = x / 1000;
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
  Packet pack1 (str3, 1024 * 2);
  Ptr<Packet> p = &pack1;
  if (m_socket_send[dest]->Send (p) == -1)
    std::cout << "Failed to send ack packet" << std::endl;
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
  Packet pack1 (str3, 120);
  Ptr<Packet> p = &pack1;
  m_socket_send[dest]->Send (p);
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
  Packet pack1 (str3, 120);
  Ptr<Packet> p = &pack1;
  m_socket_send[dest]->Send (p);
}

void
GossipApp::SendTimeMessage (int dest, int t)
{
  std::string str1 = "";
  str1.append (std::to_string (t));
  str1.append ("+");
  str1.append (std::to_string ((int) m_node_id));
  str1.append ("+");
  std::string str2 = "TIMEMESSAGE";
  str1.append (str2);
  str1.append ("+");
  str1.append (std::to_string (get_block_or_not));
  str1.append ("+");
  str1.append (std::to_string (get_committed_or_not));
  str1.append ("+");
  str1.append (std::to_string (get_block_time));
  str1.append ("+");
  str1.append (std::to_string (get_committed_time));
  const uint8_t *str3 = reinterpret_cast<const uint8_t *> (str1.c_str ());
  Packet pack1 (str3, 120);
  Ptr<Packet> p = &pack1;
  if (m_socket_send[dest]->Send (p) == -1)
    std::cout << "fatal error" << std::endl;
  // std::cout<<"node "<<(int)m_node_id<<" send "<<str1<<" to node "<<dest<<" at "<<
  //   Simulator::Now().GetSeconds()<<"s"<<std::endl;
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
  Packet pack1 (str3, 120);
  Ptr<Packet> p = &pack1;
  m_socket_send[dest]->Send (p);
  std::cout << "node " << (int) GetNodeId () << " send a view change msg to node " << dest << " at "
            << Simulator::Now ().GetSeconds () << "s" << std::endl;

}

void GossipApp::RelayViewpluplusMessage(int dest, Ptr<Packet> p)
{
  m_socket_send[dest]->Send(p);
}

void
GossipApp::RelayVotingMessage (int dest, Ptr<Packet> p)
{
  m_socket_send[dest]->Send (p);
}

void
GossipApp::RelayTimeMessage (int dest, Ptr<Packet> p)
{
  m_socket_send[dest]->Send (p);
}

// void
// GossipApp::ScheduleTransmit (Time dt, int dest, int type)
// {
//   // NS_LOG_FUNCTION(this << dt);
//   // if(m_socket_send == 0)
//   // {
//   //   TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
//   //   m_socket_send = Socket::CreateSocket(GetNode(), tid);
//   //   m_socket_send->Connect (InetSocketAddress (Ipv4Address::ConvertFrom(map_node_addr[dest]), 17));
//   // }
//   MESSAGE_TYPE message_type;
//   message_type = (MESSAGE_TYPE) type;
//   // m_sendEvent = Simulator::Schedule (dt, &GossipApp::Send, this, dest, message_type);
//   Simulator::Schedule (dt, &GossipApp::Send, this, dest, message_type);

//   // std::cout<<"node "<<(int)GetNodeId()<<" send a "<<MessagetypeToString(((int)message_type))<<" to node "<<dest<<" at "<<Simulator::Now().GetSeconds()<<"s"<<std::endl;
// }

void
GossipApp::InitializeTimeMessage ()
{
  for (int i = 0; i < NODE_NUMBER; i++)
    {
      map_epoch_node_getblockornot[m_epoch][i] = -1;
      map_epoch_node_getcommitedornot[m_epoch][i] = -1;
      map_epoch_node_getblocktime[m_epoch][i] = -1;
      map_epoch_node_getcommittedtime[m_epoch][i] = -1;
    }
  if (m_epoch > 1)
    GossipTimeMessage (m_epoch - 1);
  get_block_or_not = 0;
  get_committed_or_not = 0;
  get_block_time = len_phase1;
  get_committed_time = len_phase1 + len_phase2;
}

void
GossipApp::InitializeState ()
{
  state = 0;
}

void
GossipApp::InitializeEpoch ()
{
  for (int i = 0; i < NODE_NUMBER; i++)
  {
    map_blockpiece_received[i] = 0;
    map_node_PREPARE[i] = 0;
    map_node_COMMIT[i] = 0;
  }
  receive_viewplusplus_time = 0;
  receive_history = 0;
  block_got = false;

  // std::pair<int, int> p = NewLenComputation();
  if(consensed_this_epoch==false && m_epoch>1)
  {
    len_phase1 *= 2;
    len_phase2 *= 2;
    std::cout<<"node "<<(int)m_node_id<<" doubles his epoch length as block propagation: "<<len_phase1
      <<", voting: "<<len_phase2<<std::endl;
  }
  // else
  // {
  //   len_phase1 = p.fist;
  //   len_phase2 = p.second;
  // }
  map_epoch_len_phase1[m_epoch] = len_phase1;
  map_epoch_len_phase2[m_epoch] = len_phase2;
  
}

void
GossipApp::EndSummary ()
{
  for (int i = 0; i < NODE_NUMBER; i++)
    {
      if (map_epoch_node_getblockornot[m_epoch - 1][i] == -1)
        map_epoch_node_getblockornot[m_epoch - 1][i] = 0;
      if (map_epoch_node_getcommitedornot[m_epoch - 1][i] == -1)
        map_epoch_node_getcommitedornot[m_epoch - 1][i] = 0;
      if (map_epoch_node_getblocktime[m_epoch - 1][i] == -1)
        map_epoch_node_getblocktime[m_epoch - 1][i] = len_phase1;
      if (map_epoch_node_getcommittedtime[m_epoch - 1][i] == -1)
        map_epoch_node_getcommittedtime[m_epoch - 1][i] = len_phase1 + len_phase2;
    }
}

void GossipApp::UpdateCRGain()
{
  if(m_epoch>=3)
  {
    for(int i=0; i<NODE_NUMBER; i++)
    {
      float CR_gain = map_epoch_node_getblockornot[m_epoch-2][i]*(map_epoch_len_phase1[m_epoch-2] - 
        map_epoch_node_getblocktime[m_epoch-2][i])/map_epoch_len_phase1[m_epoch-2] +
        map_epoch_node_getcommitedornot[m_epoch-2][i]*(map_epoch_len_phase2[m_epoch-2] - 
        map_epoch_node_getcommittedtime[m_epoch-2][i])/map_epoch_len_phase2[m_epoch-2];
      map_epoch_node_CR_gain[m_epoch-2][i] = CR_gain;
    }
    // if(map_epoch_node_CR_gain.size()>WINDOW_SIZE)
    // {
    //   int x = m_epoch-(WINDOW_SIZE+2);
    //   map_epoch_node_CR_gain.erase(x);
    // }
  }
  std::cout<<"node "<<(int)m_node_id<<"updates his cr gain"<<std::endl;
}

void GossipApp::UpdateCR()
{
  for(int i=0; i<NODE_NUMBER; i++)
  {
    float sum=0;
    for(int j=0; j<WINDOW_SIZE; i++)
    {
      sum += map_epoch_node_CR_gain[m_epoch-2-j][i];
    }
    map_epoch_node_CR[m_epoch-2][i] = sum/WINDOW_SIZE;

    if(m_node_id==15)
    {
      std::cout<<"finish for node "<<i<<std::endl;
    }
  }
  // if(map_epoch_node_CR.size()>WINDOW_SIZE)
  // {
  //   int x = m_epoch -(WINDOW_SIZE + 2);
    // map_epoch_node_CR.erase(x);
  // }
}



// std::pair<int, int> GossipApp::NewLenComputation()
// {
//   for(int i=0; i<NODE_NUMBER; i++)
//   {
//     if(map_node_getblocktime[i] == -1)
//       map_node_getblocktime[i] = len_phase1;
//     if(map_node_getcommittedtime[i] == -1)
//       map_node_getcommittedtime[i] = len_phase2;
//   }

//   for(int i=0; i<NODE_NUMBER; i++)
//   {
//     float CR_gain = map_node_getblockornot[i]*(len_phase1 - map_node_getblocktime[i])/len_phase1 +
//       map_node_getcommitedornot[i]*(len_phase2 - map_node_getcommittedtime[i]);
//     if(map_node_CR_gain_queue[i].size()>=WINDOW_SIZE)
//     {
//       map_node_CR_gain_queue[i].pop();
//     }
//     map_node_CR_gain_queue[i].push(CR_gain);
//     std::queue<int> tmp(map_node_CR_gain_queue[i]);
//     float sum = 0;
//     while(!tmp.empty())
//     {
//       sum += tmp.front();
//       tmp.pop();
//     }
//     map_node_CR[i] = sum/WINDOW_SIZE;
//   }

//   for(int i=0; i<NODE_NUMBER; i++)
//   {
//     map_node_BR[i] *= exp(-1*DistanceOfPermu(map_node_CR_previous[i], map_node_CR[i]));  // TODO distance function
//   }

//   map_node_CR_previous[i] = map_node_CR[i];

//   std::vector<float> time_block_predicted;
//   std::vector<float> time_vote_predicted;
//   for(int i=0; i<NODE_NUMBER; i++)
//   {
//     if(map_node_CR[i]<1)
//     {
//       time_block_predicted.push_back((EPSILON + map_node_getblocktime[i]));
//       time_vote_predicted.push_back((EPSILON + map_node_getcommittedtime[i]));
//     }
//   } // TODO prediction time is the avg of 5 time value of nodes who's reputation value is near to node i;

//   auto maxPosition = max_element(time_block_predicted.begin(), time_block_predicted.end());
//   len1 = time_block_predicted[maxPosition - time_block_predicted.begin()];
//   auto maxPosition = max_element(time_vote_predicted.begin(), time_vote_predicted.end());
//   len2 = time_vote_predicted[maxPosition - time_vote_predicted.begin()];

//   std::pair<float, float> p;
//   p.first = len1;
//   p.second = len2;
//   return p;
// }

Block
GossipApp::BlockPropose ()
{
  Block block_proposed;
  if (quad.B_pending.name == 0)
    {
      srand (time (NULL));
      block_proposed.name = rand () % 0xFFFFFFFF;
      block_proposed.height = m_local_ledger.size () + 1;
      std::cout << "leader build a new block " << block_proposed.name
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
  std::cout << b.name << "*****" << std::endl;
  for (int i = 0; i < OUT_GOSSIP_ROUND; i++)
    {
      srand (Simulator::Now ().GetSeconds () + m_node_id);
      float x = rand () % 2000;
      x = x / 2000;
      Simulator::Schedule (Seconds (x), &GossipApp::SendBlock, this, out_neighbor_choosed[i], b);
      // SendBlock(out_neighbor_choosed[i]);
    }
}

void GossipApp::GossipViewplusplusMsg()
{
  for (int i = 0; i < OUT_GOSSIP_ROUND; i++)
  {
    srand (Simulator::Now ().GetSeconds () + m_node_id);
    float x = rand () % 1000;
    x = x / 1000;
    SendViewplusplus (out_neighbor_choosed[i]);
  }
}

void
GossipApp::GossipPrepareOut ()
{
  consensed_this_epoch = false;
  if (block_got)
    {
      if (block_received.height == (int) m_local_ledger.size () + 1)
        {
          if (quad.B_pending.name == EMPTY_BLOCK.name)
            {
              state = 1;
              quad.B_pending = EMPTY_BLOCK;
              quad.freshness = 0;
              std::cout << "node " << (int) m_node_id << " send prepare msg for block "<< block_received.name 
                <<" at "<<Simulator::Now().GetSeconds()<<"s in epoch "<<m_epoch << std::endl;
              for (int i = 0; i < OUT_GOSSIP_ROUND; i++)
              {
                srand (Simulator::Now ().GetSeconds () + m_node_id);
                float x = rand () % 1000;
                x = x / 1000;
                SendPrepare (out_neighbor_choosed[i], block_received);
              }
            }
          else if (m_epoch > quad.freshness)
            {
              state = 1;
              quad.B_pending = EMPTY_BLOCK;
              quad.freshness = 0;
              for (int i = 0; i < OUT_GOSSIP_ROUND; i++)
                {
                  srand (Simulator::Now ().GetSeconds () + m_node_id);
                  float x = rand () % 1000;
                  x = x / 1000;
                  SendPrepare (out_neighbor_choosed[i], block_received);
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
          x = x / 1000;
          Simulator::Schedule (Seconds (x), &GossipApp::SendBlock, this, out_neighbor_choosed[i],
                               b);
        }
    }
}

void GossipApp::GossipTimeMessage (int t)
{
  for (int i = 0; i < OUT_GOSSIP_ROUND; i++)
    {
      // srand (Simulator::Now ().GetSeconds () + m_node_id);
      // float x = rand () % 100;
      // x = x / 100;
      // Simulator::Schedule(Seconds(x), &GossipApp::SendTimeMessage, this, out_neighbor_choosed[i], t);
      SendTimeMessage (out_neighbor_choosed[i], t);
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
      if (sum >= (2 * NODE_NUMBER / 3.0 + 1))
        {
          state = 2;
          quad.B_pending = block_received;
          quad.freshness = m_epoch;
          get_prepared_time = Simulator::Now ().GetSeconds () - m_epoch_beginning;
          get_prepared_time = ((int) (get_prepared_time * 1000)) / 1000.;
          std::cout << "node " << (int) m_node_id << " send commit msg for block "<< block_received.name 
            <<"at "<<Simulator::Now().GetSeconds()<<"s in epoch "<<m_epoch << std::endl;
          for (int i = 0; i < OUT_GOSSIP_ROUND; i++)
            {
              srand (Simulator::Now ().GetSeconds () + m_node_id);
              float x = rand () % 1000;
              x = x / 1000;
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
          std::cout << "node " << (int) m_node_id << " consensed block " << block_received.name
                    << " to its local ledger at "<<Simulator::Now().GetSeconds()<<"s in epoch "<<m_epoch<< std::endl;
          state = 3;
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
          consensed_this_epoch = true;
        }
      else
        Simulator::Schedule (Seconds (DETERMINECONSENS_INTERVAL), &GossipApp::DetermineConsens,
                             this);
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

// void GossipApp::SolicitConsensusMessageFromOthers()
// {

// }
// TODO to ask other about last epoch consensus or not

void
GossipApp::SilenceAttack ()
{
  m_leader = false;

  // TODO
}

// void Gossip::InductionAttack()
// {

// }
// TODO launch an inductionattack

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
  Packet pack1 (str3, 80);
  Ptr<Packet> p = &pack1;
  if (m_socket_send[dest]->Send (p) == -1)
    std::cout << "Failed to send packet" << std::endl;
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
  Packet pack1 (str3, 200);
  Ptr<Packet> p = &pack1;
  if (m_socket_send[dest]->Send (p) == -1)
    std::cout << "Failed to send packet" << std::endl;
  std::cout<<"node "<<(int)m_node_id<<" query node "<<dest<<" for block history at "
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
  const uint8_t *str3 = reinterpret_cast<const uint8_t *> (str1.c_str ());
  Packet pack1 (str3, 100);
  Ptr<Packet> p = &pack1;
  if (m_socket_send[dest]->Send (p) == -1)
    std::cout << "Failed to send packet" << std::endl;
  std::cout<<"node "<<(int)m_node_id<<" reply node "<<dest<<" for query at "
    <<Simulator::Now().GetSeconds()<<"s and send him "<<str1<<std::endl;
}

void GossipApp::RecoverHistory(std::vector<uint32_t> b, std::vector<int> b_epo, int dest)
{
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
  Simulator::Cancel(id2);
  Simulator::Cancel(id3);
  Simulator::Cancel(id4);
  len_phase1 /= 2;
  len_phase2 /= 2;
  float x1 = Simulator::Now().GetSeconds() - m_epoch_beginning;
  EventId id1 = Simulator::Schedule (Seconds (len_phase1 - x1), &GossipApp::GossipPrepareOut, this);
  EventId id2 = Simulator::Schedule (Seconds (len_phase1 - x1), &GossipApp::GossipCommitOut, this);
  EventId id3 = Simulator::Schedule (Seconds (len_phase1 + len_phase2 - 0.1 - x1), &GossipApp::EndSummary, this);
  if (m_epoch < TOTAL_EPOCH_FOR_SIMULATION)
  {  
    id4 = Simulator::Schedule (Seconds (len_phase1 + len_phase2 - x1), &GossipApp::ConsensProcess, this);
  }
  std::cout<<"node "<<(int)m_node_id<<" recoverd his local ledger and set his epoch length to block propagation: "
    <<len_phase1<<", voting: "<<len_phase2<<std::endl;
  // std::cout<<"node "<<(int)m_node_id<<"'s local ledger: "<<std::endl;
  // for(int i=0; i<(int)m_local_ledger.size(); i++)
  // {
  //   std::cout<<m_local_ledger[i]<<"-->";
  // }
  // std::cout<<std::endl;
  SolicitBlock(dest);
}

void
GossipApp::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from)))
  {
    uint8_t content_[200];
    packet->CopyData (content_, 200);
    Ipv4Address from_addr = InetSocketAddress::ConvertFrom (from).GetIpv4 ();
    int from_node = (int) map_addr_node[from_addr];
    std::string str_of_content (content_, content_ + 200);
    std::vector<std::string> res = SplitMessage (str_of_content, '+');
    const char *time_of_recived_message = res[0].c_str ();

    // std::cout<<"node "<<(int)GetNodeId()<<" received a "<<content_<<" "<<packet->GetSize()
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
              std::cout << "node " << (int) GetNodeId () << " received a " << content_
                        << " for the first time " << packet->GetSize ()
                        << " bytes from node " << from_node << " at "
                        << Simulator::Now ().GetSeconds () << " s" << std::endl;
              get_block_time = Simulator::Now ().GetSeconds () - m_epoch_beginning;
              get_block_time = ((int) (get_block_time * 1000)) / 1000.;
              get_block_or_not = 1;
              // std::cout << "node " << (int) GetNodeId () << " received a " << content_
              //           << " for the first time from node " << from_node << " at "
              //           << get_block_time << "s" << std::endl;
              GossipBlockAfterReceive (from_node, block_received);
            }
          }
        }
        else
        {
          int received_block_height = (atoi) (res[5].c_str());
          if(received_block_height>((int)m_local_ledger.size()+1))
          {
            int local_ledger_height = (int) (m_local_ledger.size());
            int piece = (atoi) (res[6].c_str());
            if(piece==0)
              SolicitHistory(from_node, local_ledger_height);
          }
          // TODO query the node who give me this block
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
                    // std::cout<<"node "<<(int)GetNodeId()<<" received a "<<content_<<" "<<packet->GetSize()
                    //   <<" bytes from node "<<from_node<<" at "<<Simulator::Now().GetSeconds()<<" s"<<std::endl;
                    for (int i = 0; i < OUT_GOSSIP_ROUND; i++)
                    {
                      srand (Simulator::Now ().GetSeconds ());
                      float x = rand () % 1000;
                      x = x / 1000;
                      int dest = out_neighbor_choosed[i];
                      Simulator::Schedule (Seconds (x), &GossipApp::RelayVotingMessage,
                                            this, dest, packet);
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
                    // std::cout<<"node "<<(int)GetNodeId()<<" received a "<<content_<<" "<<packet->GetSize()
                    //   <<" bytes from node "<<from_node<<" at "<<Simulator::Now().GetSeconds()<<" s"<<std::endl;
                    for (int i = 0; i < OUT_GOSSIP_ROUND; i++)
                      {
                        srand (Simulator::Now ().GetSeconds ());
                        float x = rand () % 1000;
                        x = x / 1000;
                        int dest = out_neighbor_choosed[i];
                        Simulator::Schedule (Seconds (x), &GossipApp::RelayVotingMessage,
                                              this, dest, packet);
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
          std::cout<<"node "<<(int)m_node_id<<" get REPLYHISTORY~~~~"<<std::endl;
          // for(int i=0; i<(int)res.size(); i++)
          //   std::cout<<res[i].c_str()<<"~~~~";
          // std::cout<<std::endl;
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
            std::cout<<std::setw(12)<<x<<std::setw(4)<<y<<std::endl;
          }
          RecoverHistory(history, history_built_height, from_node);
        }
      }
      else if(strcmp (type_of_received_message, "VIEWPLUSPLUS") == 0)
      {
        if(receive_viewplusplus_time==0)
        {
          view++;
          receive_viewplusplus_time++;
          uint8_t x = view % NODE_NUMBER;
          if (m_node_id == x)
          {
            m_leader = true;
            block_got = true;
            get_block_or_not = 1;
            get_block_time = Simulator::Now().GetSeconds();
            get_block_time = ((int) get_block_time * 1000) / 1000;
            std::cout << "*****************"
                      << "epoch " << (int) m_epoch << " starts"
                      << "**********" << std::endl;
            std::cout << "****block propagation time: " << len_phase1 << "s" <<
              "  ****voting time: " << len_phase2 <<"s" << "****" << std::endl;
            std::cout << "node " << (int) GetNodeId () << " is the leader, current view: "<< view << std::endl;
            std::cout << "time now: " << m_epoch_beginning << "s" << std::endl;
            Block b = BlockPropose ();
            block_received = b;
            LeaderGossipBlockOut (b);
          }
          for (int i = 0; i < OUT_GOSSIP_ROUND; i++)
          {
            srand (Simulator::Now ().GetSeconds ());
            float x = rand () % 1000;
            x = x / 1000;
            int dest = out_neighbor_choosed[i];
            Simulator::Schedule (Seconds (x), &GossipApp::RelayVotingMessage,
                                  this, dest, packet);
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
              // std::cout<<"node "<<(int)GetNodeId()<<" received a "<<content_<<" "<<packet->GetSize()
              //     <<" bytes from node "<<from_node<<" at "<<Simulator::Now().GetSeconds()<<"s"<<std::endl;
              for (int i = 0; i < OUT_GOSSIP_ROUND; i++)
                {
                  srand (Simulator::Now ().GetSeconds ());
                  float x = rand () % 1000;
                  x = x / 1000;
                  int dest = out_neighbor_choosed[i];
                  Simulator::Schedule (Seconds (x), &GossipApp::RelayTimeMessage, this, dest,
                                        packet);
                }
            }
        }
    }
  }
}

} // Namespace ns3

