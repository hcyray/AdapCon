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

#include <stdlib.h>
#include <time.h>
#include <algorithm>
#include <fstream>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("GossipAppApplication");

NS_OBJECT_ENSURE_REGISTERED (GossipApp);



TypeId
GossipApp::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::GossipApp")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<GossipApp> ()
    .AddAttribute ("NodeId", "Node on which the application runs.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&GossipApp::m_node_id),
                   MakeUintegerChecker<uint8_t> ())
    .AddAttribute ("Port", "Port on which we listen for incoming packets.",
                   UintegerValue (9),
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
  // m_sendEvent = EventId ();
  // m_sent = 0;
  // m_count = 1;
  m_epoch = 0;
  m_epoch_beginning = 0.;

  len_phase1 = 120.0;
  len_phase2 = 30.0;
  waitting_time = len_phase1*2/4.;


  const int LINE_LENGTH = 100;
  char str1[LINE_LENGTH];
  std::ifstream infile1;
  infile1.open("scratch/subdir/topologydata/node-address.txt");
  while(infile1.getline(str1,LINE_LENGTH))
  {
      if(strcmp(str1, "Node and Ip address")==0)
          continue;
      std::string str2(str1);
      std::vector<std::string> res = SplitMessage(str2, ' ');
      if(res.size()==2)
      {
          int x = (atoi)(res[0].c_str());
          char* ipaddress = (char*)res[1].data();
          Ipv4Address y = Ipv4Address(ipaddress);
          map_node_addr[x] = y;
      }
  }
  infile1.close();

  std::ifstream infile2;
  infile2.open("scratch/subdir/topologydata/address-node.txt");
  while(infile2.getline(str1,LINE_LENGTH))
  {
      if(strcmp(str1, "Node and Ip address")==0)
          continue;
      std::string str2(str1);
      std::vector<std::string> res = SplitMessage(str2, ' ');
      if(res.size()==2)
      {
          
          int x = (atoi)(res[1].c_str());
          char* ipaddress = (char*)res[0].data();
          Ipv4Address y = Ipv4Address(ipaddress);
          map_addr_node[y] = x;
      }
  }
  infile2.close();
  // std::cout<<"read ip address"<<std::endl;
}



GossipApp::~GossipApp()
{
  NS_LOG_FUNCTION (this);
  m_socket_receive = 0;
}

void GossipApp::ConsensProcess()
{
  // if(m_epoch>=1)
  //   GossipReputationMessage();
    // std::pair<int, int> p = ReputationComputation();
    // len_phase1 = p.first;
    // len_phase2 = p.second;
    // waitting_time = len_phase1*2/4.;
  
  m_epoch++;
  m_epoch_beginning = Simulator::Now().GetSeconds();
  InitializeReputationMessage();
  for(int i=0; i<NODE_NUMBER; i++)
  {
    map_blockpiece_received[i] = 0;
    map_node_PREPARE[i] = 0;
    map_node_COMMIT[i] = 0;
  }

  block_got = false;
  

  if_leader();
  GetNeighbor(OUT_GOSSIP_ROUND, out_neighbor_choosed);
  GetNeighbor(IN_GOSSIP_ROUND, in_neighbor_choosed);
  if(m_leader)
  {
    std::cout<<"*****************"<<"epoch "<<(int)m_epoch<<" starts"<<"**********"<<std::endl;
    std::cout<<"node "<<(int)GetNodeId()<<" is the leader"<<std::endl;
    std::cout<<"time now: "<<m_epoch_beginning<<std::endl;
    GossipBlockOut();
  }
  else
  {
    Simulator::Schedule(Seconds(waitting_time), &GossipApp::SolicitMessageFromOthers, this);
  }

  
  Simulator::Schedule(Seconds(len_phase1), &GossipApp::GossipVotingMessageOut, this, 3);
  Simulator::Schedule(Seconds(len_phase1), &GossipApp::DetermineCommit, this);
  Simulator::Schedule(Seconds(len_phase1+len_phase2-0.01), &GossipApp::GossipReputationMessage, this);
  if(m_epoch<TOTAL_EPOCH_FOR_SIMULATION)
    Simulator::Schedule(Seconds(len_phase1+len_phase2), &GossipApp::ConsensProcess, this);
}

void GossipApp::if_leader(void)
{
  // uint8_t x = m_epoch % NODE_NUMBER;
  uint8_t x = 1;
  if(m_node_id==x)
  {
    m_leader = true;
    block_got = true;
  }
  else
    m_leader = false;

}

void GossipApp::if_get_block(void)
{
  int sum=0;
  for(int i=0; i<32; i++)
    sum += map_blockpiece_received[i];
  if(sum==32)
    block_got = true;
  else
    block_got = false;
}


std::string GossipApp::MessagetypeToString(int x)
{
  std::string res;
  switch(x){
    case 0: res = "BLOCK"; break;
    case 1: res = "SOLICIT"; break;
    case 2: res = "ACK"; break;
    case 3: res = "PREPARE"; break;
    case 4: res = "COMMIT"; break;
    case 5: res = "REPUTATION"; break;
  }
  return res;
}

// void GossipApp::ChooseNeighbor(int number, int neighbor_choosed[])
// {
//   // srand((unsigned)Simulator::Now().GetSeconds());
//   srand(time(NULL)+m_node_id);
//   int i = 0;
//   std::vector<int> node_added;
//   std::vector<int>::iterator it;
//   while(i<number)
//   {
//     int x = rand()%NODE_NUMBER;
//     if(x!=m_node_id)
//     {
//       it=std::find(node_added.begin(), node_added.end(), x);
//       if(it==node_added.end())
//       {
//         neighbor_choosed[i] = x;
//         node_added.push_back(x);
//         i++;
//       }
      
//     }
//   }
// }

// void GossipApp::ChooseNeighbor(int number, int neighbor_choosed[], int node_excluded)
// {
//   // srand((unsigned)Simulator::Now().GetSeconds());
//   srand(time(NULL)+m_node_id);
//   int i = 0;
//   std::vector<int> node_added;
//   std::vector<int>::iterator it;
//   while(i<number)
//   {
//     int x = rand()%NODE_NUMBER;
//     if(x!=m_node_id && x!=node_excluded)
//     {
//       it=std::find(node_added.begin(), node_added.end(), x);
//       if(it==node_added.end())
//       {
//         neighbor_choosed[i] = x;
//         node_added.push_back(x);
//         i++;
//       }
      
//     }
//   }
// }

void GossipApp::GetNeighbor(int node_number, int out_neighbor_choosed[])
{
  srand(time(NULL)+m_node_id);
  int i = 0;
  std::vector<int> node_added;
  std::vector<int>::iterator it;
  while(i<node_number)
  {
    int x = rand()%NODE_NUMBER;
    if(x!=m_node_id)
    {
      it=std::find(node_added.begin(), node_added.end(), x);
      if(it==node_added.end())
      {
        out_neighbor_choosed[i] = x;
        node_added.push_back(x);
        i++;
      }
      
    }
  }
}

uint8_t GossipApp::GetNodeId(void)
{
  return m_node_id;
}

std::vector<std::string> GossipApp::SplitMessage(const std::string& str, const char pattern)
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

  for(int i=0; i<NODE_NUMBER; i++)
  {
    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    Ptr<Socket> socket_send = Socket::CreateSocket(GetNode(), tid);
    m_socket_send.push_back(socket_send);
    // std::cout<<"sending socket created!"<<std::endl;
    if(m_socket_send[i]->Connect (InetSocketAddress (Ipv4Address::ConvertFrom(map_node_addr[i]), 17))==-1)
      NS_FATAL_ERROR("Failer to connect socket");
    
    
  }

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

  

  Simulator::Schedule(Seconds(20.), &GossipApp::ConsensProcess, this); // must wait for some time, for example 2.0s,
  // or no block can be received in epoch 1

}

void 
GossipApp::StopApplication ()
{
  NS_LOG_FUNCTION (this);
  // printf("%s\n", "Server stops!");
  if (m_socket_receive != 0) 
    {
      m_socket_receive->Close ();
      m_socket_receive->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }
  // if(block_got)
  //   std::cout<<"node "<<(int)m_node_id<<" has received the block"<<std::endl;
  int sum=0;
  for(int i=1; i<=TOTAL_EPOCH_FOR_SIMULATION; i++)
  {
    sum += map_epoch_consensed[i];
  }
  // std::cout<<"temporal epoch: "<<(int)m_epoch<<"**********"<<std::endl;
  std::cout<<"node "<<(int)m_node_id<<" got "<<sum<<" consensed block"<<std::endl;
  // std::cout<<"node "<<(int)m_node_id<<" get block at "<<get_block_time<<"s"<<std::endl;
  // std::cout<<"node "<<(int)m_node_id<<" get prepared at "<<get_prepared_time<<"s"<<std::endl;
  // std::cout<<"node "<<(int)m_node_id<<" get committed at "<<get_committed_time<<"s"<<std::endl;
  
  if(m_node_id==4)
  {
    std::cout<<"******************************"<<std::endl;
    for(int n=1; n<NODE_NUMBER; n++)
    {
      std::cout<<"node "<<n<<" got block at "<<map_node_getblocktime[n]<<"s"<<std::endl;
    }
    for(int n=1; n<NODE_NUMBER; n++)
    {
      std::cout<<"node "<<n<<" got prepared at "<<map_node_getpreparedtime[n]<<"s"<<std::endl;
    }
    for(int n=1; n<NODE_NUMBER; n++)
    {
      std::cout<<"node "<<n<<" got committed at "<<map_node_getcommittedtime[n]<<"s"<<std::endl;
    }
    std::cout<<"******************************"<<std::endl;
  }
  
}



void GossipApp::Send(int dest, MESSAGE_TYPE message_type)
{
  NS_LOG_FUNCTION(this);
  switch(message_type){
    case BLOCK:{
      std::string str1 = "";
      str1.append(std::to_string((int)m_epoch));
      str1.append("+");
      str1.append(std::to_string((int)m_node_id));
      str1.append("+");
      std::string str2(TYPE_BLOCK, TYPE_BLOCK+10);
      str1.append(str2);
      const uint8_t *str3 = reinterpret_cast<const uint8_t*>(str1.c_str());
      Packet pack1(str3, 1024*16);
      Ptr<Packet> p = &pack1;
      m_socket_send[dest]->Send(p);
      std::cout<<"node "<<(int)GetNodeId()<<" send a "<<str3<<" to node "<<dest
        <<" at "<<Simulator::Now().GetSeconds()<<"s"<<std::endl;
      break;
    } 
    case SOLICIT:
    {
      // Packet pack1(TYPE_SOLICIT, 80);
      std::string str1 = "";
      str1.append(std::to_string((int)m_epoch));
      str1.append("+");
      str1.append(std::to_string((int)m_node_id));
      str1.append("+");
      std::string str2(TYPE_SOLICIT, TYPE_SOLICIT+10);
      str1.append(str2);
      const uint8_t *str3 = reinterpret_cast<const uint8_t*>(str1.c_str());
      Packet pack1(str3, 80);
      Ptr<Packet> p = &pack1;
      m_socket_send[dest]->Send(p);
      // std::cout<<"node "<<(int)GetNodeId()<<" send a "<<str3<<" to node "<<dest
      //   <<" at "<<Simulator::Now().GetSeconds()<<"s"<<std::endl;
      break;
    }
    case ACK:
    {
      // Packet pack1(TYPE_ACK, 80);
      std::string str1 = "";
      str1.append(std::to_string((int)m_epoch));
      str1.append("+");
      str1.append(std::to_string((int)m_node_id));
      str1.append("+");
      std::string str2(TYPE_ACK, TYPE_ACK+10);
      str1.append(str2);
      const uint8_t *str3 = reinterpret_cast<const uint8_t*>(str1.c_str());
      Packet pack1(str3, 80);
      Ptr<Packet> p = &pack1;
      m_socket_send[dest]->Send(p);
      std::cout<<"node "<<(int)GetNodeId()<<" send a "<<str3<<" to node "<<dest
        <<" at "<<Simulator::Now().GetSeconds()<<"s"<<std::endl;
      break;
    }
    case PREPARE:
    {
      // Packet pack1(TYPE_PREPARE, 80);
      std::string str1 = "";
      str1.append(std::to_string((int)m_epoch));
      str1.append("+");
      str1.append(std::to_string((int)m_node_id));
      str1.append("+");
      std::string str2(TYPE_PREPARE, TYPE_PREPARE+10);
      str1.append(str2);
      const uint8_t *str3 = reinterpret_cast<const uint8_t*>(str1.c_str());
      Packet pack1(str3, 80);
      Ptr<Packet> p = &pack1;
      m_socket_send[dest]->Send(p);
      // std::cout<<"node "<<(int)GetNodeId()<<" send a "<<str3<<" to node "<<dest
      //   <<" at "<<Simulator::Now().GetSeconds()<<"s"<<std::endl;
      break;
    }
    case COMMIT:
    {
      // Packet pack1(TYPE_COMMIT, 80);
      std::string str1 = "";
      str1.append(std::to_string((int)m_epoch));
      str1.append("+");
      str1.append(std::to_string((int)m_node_id));
      str1.append("+");
      std::string str2(TYPE_COMMIT, TYPE_COMMIT+10);
      str1.append(str2);
      const uint8_t *str3 = reinterpret_cast<const uint8_t*>(str1.c_str());
      Packet pack1(str3, 80);
      Ptr<Packet> p = &pack1;
      m_socket_send[dest]->Send(p);
      // std::cout<<"node "<<(int)GetNodeId()<<" send a "<<str3<<" to node "<<dest
      //   <<" at "<<Simulator::Now().GetSeconds()<<"s"<<std::endl;
      break;
    }
    case REPUTATION:
    {
      std::string str1 = "";
      str1.append(std::to_string((int)m_epoch));
      str1.append("+");
      str1.append(std::to_string((int)m_node_id));
      str1.append("+");
      std::string str2(TYPE_REPUTATION, TYPE_REPUTATION+12);
      str1.append(str2);
      str1.append("+");
      str1.append(std::to_string(get_block_or_not));
      str1.append("+");
      str1.append(std::to_string(get_committed_or_not));
      str1.append("+");
      str1.append(std::to_string(get_block_time));
      str1.append("+");
      str1.append(std::to_string(get_prepared_time));
      str1.append("+");
      str1.append(std::to_string(get_committed_time));
      // std::cout<<"***********************"<<str1<<std::endl;
      const uint8_t *str3 = reinterpret_cast<const uint8_t*>(str1.c_str());
      Packet pack1(str3, 100);
      Ptr<Packet> p = &pack1;
      if(m_socket_send[dest]->Send(p)==-1)
        std::cout<<"fatal error"<<std::endl;
      // else
      //   std::cout<<"node "<<(int)GetNodeId()<<" send a "<<str3<<" to node "<<dest
      //     <<" at "<<Simulator::Now().GetSeconds()<<"s"<<std::endl;
      //   std::cout<<"node "<<(int)GetNodeId()<<" send a "<<str1<<" to node "<<dest
      //     <<" at "<<Simulator::Now().GetSeconds()<<"s"<<std::endl;
      break;
    }
  }
    // Packet pack1(TYPE_BLOCK, 1000);

  // Ptr<Packet> p = &pack1;
  // m_socket_send[dest]->Send(p);
  // if(m_socket_send[dest]->Send(p));
  //   {
  //     std::cout<<"packet sent successfully!"<<std::endl;
      
  //   }
  // ++m_sent;
  // if (m_sent < m_count) 
  //   {
  //     ScheduleTransmit (m_interval, i);
  //   }

}

void GossipApp::SendBlock(int dest)
{
  for(int piece=0; piece<32; piece++)
  {
    std::string str1 = "";
    str1.append(std::to_string((int)m_epoch));
    str1.append("+");
    str1.append(std::to_string((int)m_node_id));
    str1.append("+");
    std::string str2(TYPE_BLOCK, TYPE_BLOCK+10);
    str1.append(str2);
    str1.append("+");
    str1.append(std::to_string((int)piece));
    const uint8_t *str3 = reinterpret_cast<const uint8_t*>(str1.c_str());
    Packet pack1(str3, 1024*32);
    Ptr<Packet> p = &pack1;
    m_socket_send[dest]->Send(p);
    // std::cout<<"node "<<(int)GetNodeId()<<" send "<<str1<<" to node "<<dest
    //     <<" at "<<Simulator::Now().GetSeconds()<<"s"<<std::endl;
    // std::cout<<"node "<<(int)GetNodeId()<<" send "<<str3<<" to node "<<dest
    //   <<" at "<<Simulator::Now().GetSeconds()<<"s"<<std::endl;
  }
  // std::cout<<"node "<<(int)GetNodeId()<<" send a block to node "<<dest
  //       <<" at "<<Simulator::Now().GetSeconds()<<"s"<<std::endl;
}

void GossipApp::SendBlockAck(int dest)
{
  for(int piece=0; piece<32; piece++)
  {
    std::string str1 = "";
    str1.append(std::to_string((int)m_epoch));
    str1.append("+");
    str1.append(std::to_string((int)m_node_id));
    str1.append("+");
    std::string str2(TYPE_ACK, TYPE_ACK+10);
    str1.append(str2);
    str1.append("+");
    str1.append(std::to_string((int)piece));
    const uint8_t *str3 = reinterpret_cast<const uint8_t*>(str1.c_str());
    Packet pack1(str3, 1024*32);
    Ptr<Packet> p = &pack1;
    m_socket_send[dest]->Send(p);
    // std::cout<<"node "<<(int)GetNodeId()<<" send "<<str1<<" to node "<<dest
    //     <<" at "<<Simulator::Now().GetSeconds()<<"s"<<std::endl;
  }
  // std::cout<<"node "<<(int)GetNodeId()<<" reply a block to node "<<dest
  //       <<" at "<<Simulator::Now().GetSeconds()<<"s"<<std::endl;
}

void GossipApp::RelayVotingMessage(Ptr<Packet> p)
{
  for(int i=0; i<OUT_GOSSIP_ROUND; i++)
  {
    int dest = out_neighbor_choosed[i];
    m_socket_send[dest]->Send(p);
  }
}

void GossipApp::RelayReputationMessage(Ptr<Packet> p)
{
  for(int i=0; i<OUT_GOSSIP_ROUND; i++)
  {
    int dest = out_neighbor_choosed[i];
    m_socket_send[dest]->Send(p);
  }
}


void GossipApp::ScheduleTransmit(Time dt, int dest, int type)
{
  // NS_LOG_FUNCTION(this << dt);
  // if(m_socket_send == 0)
  // {
  //   TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
  //   m_socket_send = Socket::CreateSocket(GetNode(), tid);
  //   m_socket_send->Connect (InetSocketAddress (Ipv4Address::ConvertFrom(map_node_addr[dest]), 17));
  // }
  MESSAGE_TYPE message_type;
  message_type = (MESSAGE_TYPE)type;
  // m_sendEvent = Simulator::Schedule (dt, &GossipApp::Send, this, dest, message_type);
  Simulator::Schedule (dt, &GossipApp::Send, this, dest, message_type);

  // std::cout<<"node "<<(int)GetNodeId()<<" send a "<<MessagetypeToString(((int)message_type))<<" to node "<<dest<<" at "<<Simulator::Now().GetSeconds()<<"s"<<std::endl;
}

void GossipApp::InitializeReputationMessage()
{
  for(int i=0; i<NODE_NUMBER; i++)
  {
    map_node_getblockornot[i] = 0;
    map_node_getcommitedornot[i] = 0;
    map_node_getblocktime[i] = -1;
    map_node_getpreparedtime[i] = -1;
    map_node_getcommittedtime[i] = -1;
  }
  get_block_or_not = 0;
  get_committed_or_not = 0;
  get_block_time = -1;
  get_prepared_time = -1;
  get_committed_time = -1;
}

// std::pair<int, int> GossipApp::ReputationComputation()
// {
  
// }
// TODO, compute reputation value and phase lenth accordingly

void GossipApp::GossipBlockOut()
{
  for(int i=0; i<OUT_GOSSIP_ROUND; i++)
  {
    SendBlock(out_neighbor_choosed[i]);
  }
}

void GossipApp::GossipBlockAfterReceive(int from_node)
{

  for(int i=0; i<OUT_GOSSIP_ROUND; i++)
  {
    if(out_neighbor_choosed[i]!=from_node)
      SendBlock(out_neighbor_choosed[i]);
  }
}

void GossipApp::GossipVotingMessageOut(int type)
{
  // TODO some logic ambiguity
  if(type==3)
  {
    if(block_got)
    {
      for(int i=0; i<OUT_GOSSIP_ROUND; i++)
      { 
        
        ScheduleTransmit(Seconds(0.), out_neighbor_choosed[i], 3);
      }
    }
  }
  else if(type==4)
  {
    for(int i=0; i<OUT_GOSSIP_ROUND; i++)
      { 
        
        ScheduleTransmit(Seconds(0.), out_neighbor_choosed[i], 4);
      }
  }
    
}

void GossipApp::GossipReputationMessage()
{
  for(int i=0; i<OUT_GOSSIP_ROUND; i++)
  {
    MESSAGE_TYPE message_type = (MESSAGE_TYPE)5;
    Send(out_neighbor_choosed[i], message_type);
  }
}

void GossipApp::DetermineCommit()
{
  if((Simulator::Now().GetSeconds() - m_epoch_beginning) >= len_phase1)
  {
    int sum = 0;
    for(int i=0; i<NODE_NUMBER; i++)
    {
      sum += map_node_PREPARE[i];
    }
    if(sum>=(2*NODE_NUMBER/3.0+1))
    {
      get_prepared_time = Simulator::Now().GetSeconds();
      get_prepared_time = ((int)(get_prepared_time*1000))/1000.;
      GossipVotingMessageOut(4);
      Simulator::Schedule(Seconds(DETERMINECONSENS_INTERVAL), &GossipApp::DetermineConsens, this);
    }
    else
      Simulator::Schedule(Seconds(DETERMINECOMMIT_INTERVAL), &GossipApp::DetermineCommit, this);
  }
}


void GossipApp::DetermineConsens()
{
  if((Simulator::Now().GetSeconds() - m_epoch_beginning) >= len_phase1)
  {
    int sum=0;
    for(int i=0; i<NODE_NUMBER; i++)
    {
      sum += map_node_COMMIT[i];
    }
    if(sum>=(2*NODE_NUMBER/3.0+1))
    {
      get_committed_time = Simulator::Now().GetSeconds();
      get_committed_time = (int(get_committed_time*1000))/1000.;
      get_committed_or_not = 1;
      map_epoch_consensed[m_epoch] = 1;
    }
    else
      Simulator::Schedule(Seconds(DETERMINECONSENS_INTERVAL), &GossipApp::DetermineConsens, this);
  }
}

void GossipApp::SolicitMessageFromOthers()
{
  if((Simulator::Now().GetSeconds() - m_epoch_beginning) >= waitting_time)
  {
    
    if(block_got==false)
    {
      // int neighbors[SOLICIT_ROUND];
      // ChooseNeighbor(SOLICIT_ROUND, neighbors);
      // for(int i=0; i<SOLICIT_ROUND; i++)
      // {
      //   ScheduleTransmit(Seconds (0.), neighbors[i], 1);
      // }
      srand(Simulator::Now().GetSeconds() +m_node_id);
      int i = rand() % IN_GOSSIP_ROUND;
      ScheduleTransmit(Seconds (0.), in_neighbor_choosed[i], 1);
      if((Simulator::Now().GetSeconds() - m_epoch_beginning + SOLICIT_INTERVAL)<len_phase1 + len_phase2)
        Simulator::Schedule(Seconds(SOLICIT_INTERVAL), &GossipApp::SolicitMessageFromOthers, this);
    }
  }
  


}

void 
GossipApp::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from)))
    {
      uint8_t content_[80];
      packet->CopyData(content_, 80);
      Ipv4Address from_addr = InetSocketAddress::ConvertFrom (from).GetIpv4 ();
      int from_node = (int)map_addr_node[from_addr];
      // std::cout<<"node "<<(int)GetNodeId()<<" received a "<<content_<<" "<<packet->GetSize()
      //   <<" bytes from node "<<from_node<<" at "<<Simulator::Now().GetSeconds()<<" s"<<std::endl;

      std::string str_of_content(content_, content_+80);
      std::vector<std::string> res = SplitMessage(str_of_content, '+');
      const char* time_of_recived_message = res[0].c_str();

      if(strcmp(time_of_recived_message, (std::to_string(m_epoch)).c_str())==0)
      {
        const char* type_of_received_message = res[2].c_str();
        if(strcmp(type_of_received_message, "BLOCK")==0)
        {
          if(block_got==false)
          {
            int u = (atoi)(res[3].c_str());
            // std::cout<<"****************************************"<<u<<"*********"<<std::endl;
            map_blockpiece_received[u] = 1;
            if_get_block();
            if(block_got == true)
            {
              // std::cout<<"node "<<(int)GetNodeId()<<" received a "<<content_<<" for the first time "<<packet->GetSize()
              // <<" bytes from node "<<from_node<<" at "<<Simulator::Now().GetSeconds()<<" s"<<std::endl;
              get_block_time = Simulator::Now().GetSeconds();
              get_block_time = ((int)(get_block_time*1000))/1000.;
              get_block_or_not = 1;
              std::cout<<"node "<<(int)GetNodeId()<<" received a "<<content_<<" for the first time from node "
                <<from_node<<" at "<<get_block_time<<"s"<<std::endl;
              GossipBlockAfterReceive(from_node);
            }
          }
          // else
          //   std::cout<<"node "<<(int)GetNodeId()<<" received a "<<content_<<" "<<packet->GetSize()
          //     <<" bytes from node "<<from_node<<" at "<<Simulator::Now().GetSeconds()<<" s"<<std::endl;
        }
        else if(strcmp(type_of_received_message, "ACK")==0)
        {
          if(block_got==false)
          {
            int u = (atoi)(res[3].c_str());
            // std::cout<<"****************************************"<<u<<"*********"<<std::endl;
            map_blockpiece_received[u] = 1;
            if_get_block();
            if(block_got == true)
            {
              // std::cout<<"node "<<(int)GetNodeId()<<" received a "<<content_<<" for the first time "<<packet->GetSize()
              // <<" bytes from node "<<from_node<<" at "<<Simulator::Now().GetSeconds()<<" s"<<std::endl;
              get_block_time = Simulator::Now().GetSeconds();
              get_block_time = ((int)(get_block_time*1000))/1000.;
              get_block_or_not = 1;
              std::cout<<"node "<<(int)GetNodeId()<<" received a "<<content_<<" for the first time from node "
                <<from_node<<" at "<<get_block_time<<"s"<<std::endl;
            }
          }
        }
        else if(strcmp(type_of_received_message, "SOLICIT")==0)
        {
          if(block_got==true)
          {
            SendBlockAck(from_node);
            std::cout<<"node "<<(int)GetNodeId()<<" responds node "<<from_node<<" and send him a block at "
              <<Simulator::Now().GetSeconds()<<"s"<<std::endl;

          }
          // else{
            // std::cout<<"node "<<(int)GetNodeId()<<" can't respond node "<<from_node<<" because until "<<
            //   Simulator::Now().GetSeconds()<<" s he has not received a block yet"<<std::endl;
          // }
          
        }
        else if(strcmp(type_of_received_message, "PREPARE")==0)
        { 
          int x = (atoi)(res[1].c_str());
          if(map_node_PREPARE[x]==0)
          {
            map_node_PREPARE[x] = 1;
            // std::cout<<"node "<<(int)GetNodeId()<<" received a "<<content_<<" "<<packet->GetSize()
            //   <<" bytes from node "<<from_node<<" at "<<Simulator::Now().GetSeconds()<<" s"<<std::endl;
            RelayVotingMessage(packet);
          }
        }
        else if(strcmp(type_of_received_message, "COMMIT")==0)
        {
          int x = (atoi)(res[1].c_str());
          if(map_node_COMMIT[x]==0)
          {
            map_node_COMMIT[x] = 1;
            // std::cout<<"node "<<(int)GetNodeId()<<" received a "<<content_<<" "<<packet->GetSize()
            //   <<" bytes from node "<<from_node<<" at "<<Simulator::Now().GetSeconds()<<" s"<<std::endl;
            RelayVotingMessage(packet);
          }
        }
        else if(strcmp(type_of_received_message, "REPUTATION")==0)
        {
          // std::cout<<"*************************************************"<<std::endl;
          int x = (atoi)(res[1].c_str());
          if(map_node_getblockornot[x]==0)
          {
            map_node_getblockornot[x] = (atoi)(res[3].c_str());
            map_node_getcommitedornot[x] = (atoi)(res[4].c_str());
            map_node_getblocktime[x] = std::stod(res[5].c_str());
            map_node_getpreparedtime[x] = std::stod(res[6].c_str());
            map_node_getcommittedtime[x] = std::stod(res[7].c_str());
            std::cout<<"node "<<(int)GetNodeId()<<" received a "<<content_<<" "<<packet->GetSize()
                <<" bytes from node "<<from_node<<" at "<<Simulator::Now().GetSeconds()<<"s"<<std::endl;
            RelayReputationMessage(packet);
          }
          
        }
      }
      else if(strcmp(time_of_recived_message, (std::to_string(m_epoch-1)).c_str())==0)
      {
        if(strcmp(type_of_received_message, "REPUTATION")==0)
        {
          
        }
      }


      


    }
}

} // Namespace ns3
