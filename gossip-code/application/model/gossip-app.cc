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
  m_sendEvent = EventId ();
  m_sent = 0;
  m_count = 1;
  m_epoch = 0;
  m_epoch_beginning = 0.;
  
  len_phase1 = 15.0;
  len_phase2 = 10.0;

  for(int i=0; i<NODE_NUMBER; i++)
  {
    std::string ipaddress_string = "10.1.";
    ipaddress_string.append(std::to_string(i+1));
    ipaddress_string.append(".1");
    char* ipaddress = (char*)ipaddress_string.data();
    map_node_addr[i] = Ipv4Address(ipaddress);    
  }

  for(int i=0; i<NODE_NUMBER; i++)
  {
    std::string ipaddress_string = "10.1.";
    ipaddress_string.append(std::to_string(i+1));
    ipaddress_string.append(".1");
    char* ipaddress = (char*)ipaddress_string.data();
    Ipv4Address key1 = Ipv4Address(ipaddress);
    map_addr_node[key1] = (uint8_t)i;
  }

  


}



GossipApp::~GossipApp()
{
  NS_LOG_FUNCTION (this);
  m_socket_receive = 0;
}

void GossipApp::ConsensProcess()
{
  m_epoch++;
  m_epoch_beginning = Simulator::Now().GetSeconds();
  
  for(int i=0; i<NODE_NUMBER; i++)
  {
    map_node_PREPARE[i] = 0;
    map_node_COMMIT[i] = 0;
  }
  block_got = false;

  if_leader();
  if(m_leader)
  {
    std::cout<<"*****************"<<"epoch "<<(int)m_epoch<<" starts"<<"**********"<<std::endl;
    std::cout<<"node "<<(int)GetNodeId()<<" is the leader"<<std::endl;
    GossipMessageOut();
  }
  else
  {
    Simulator::Schedule(Seconds(WAITTING_TIME), &GossipApp::SolicitMessageFromOthers, this);
    // TODO wait for some time then query neighbors
  }

  Simulator::Schedule(Seconds(len_phase1), &GossipApp::BroadcastMessageOut, this, 3);
  Simulator::Schedule(Seconds(len_phase1), &GossipApp::DetermineCommit, this);

  if(m_epoch<TOTAL_EPOCH_FOR_SIMULATION)
    Simulator::Schedule(Seconds(len_phase1+len_phase2), &GossipApp::ConsensProcess, this);
}

void GossipApp::if_leader(void)
{
  uint8_t x = m_epoch % NODE_NUMBER;
  if(m_node_id==x)
  {
    m_leader = true;
    block_got = true;
  }
  else
    m_leader = false;

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
  }
  return res;
}

void GossipApp::ChooseNeighbor(int number, int neighbor_choosed[])
{
  // srand((unsigned)Simulator::Now().GetSeconds());
  srand(time(NULL)+m_node_id);
  int i = 0;
  std::vector<int> node_added;
  std::vector<int>::iterator it;
  while(i<number)
  {
    int x = rand()%NODE_NUMBER;
    if(x!=m_node_id)
    {
      it=std::find(node_added.begin(), node_added.end(), x);
      if(it==node_added.end())
      {
        neighbor_choosed[i] = x;
        node_added.push_back(x);
        i++;
      }
      
    }
  }
}

void GossipApp::ChooseNeighbor(int number, int neighbor_choosed[], int node_excluded)
{
  // srand((unsigned)Simulator::Now().GetSeconds());
  srand(time(NULL)+m_node_id);
  int i = 0;
  std::vector<int> node_added;
  std::vector<int>::iterator it;
  while(i<number)
  {
    int x = rand()%NODE_NUMBER;
    if(x!=m_node_id && x!=node_excluded)
    {
      it=std::find(node_added.begin(), node_added.end(), x);
      if(it==node_added.end())
      {
        neighbor_choosed[i] = x;
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

  

  Simulator::Schedule(Seconds(0.), &GossipApp::ConsensProcess, this);

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
  std::cout<<"temporal epoch: "<<(int)m_epoch<<"**********"<<std::endl;
  std::cout<<"node "<<(int)m_node_id<<" got "<<sum<<" consensed block"<<std::endl;
}



void GossipApp::Send(int dest, MESSAGE_TYPE message_type)
{
  NS_LOG_FUNCTION(this);
  switch(message_type){
    case BLOCK:{
      std::string str1 = "";
      str1.append(std::to_string((int)m_epoch));
      str1.append("+");
      std::string str2(TYPE_BLOCK, TYPE_BLOCK+10);
      str1.append(str2);
      const uint8_t *str3 = reinterpret_cast<const uint8_t*>(str1.c_str());
      Packet pack1(str3, 1000);
      Ptr<Packet> p = &pack1;
      m_socket_send[dest]->Send(p);
      break;
    } 
    case SOLICIT:
    {
      // Packet pack1(TYPE_SOLICIT, 80);
      std::string str1 = "";
      str1.append(std::to_string((int)m_epoch));
      str1.append("+");
      std::string str2(TYPE_SOLICIT, TYPE_SOLICIT+10);
      str1.append(str2);
      const uint8_t *str3 = reinterpret_cast<const uint8_t*>(str1.c_str());
      Packet pack1(str3, 1000);
      Ptr<Packet> p = &pack1;
      m_socket_send[dest]->Send(p);
      break;
    }
    case ACK:
    {
      // Packet pack1(TYPE_ACK, 80);
      std::string str1 = "";
      str1.append(std::to_string((int)m_epoch));
      str1.append("+");
      std::string str2(TYPE_ACK, TYPE_ACK+10);
      str1.append(str2);
      const uint8_t *str3 = reinterpret_cast<const uint8_t*>(str1.c_str());
      Packet pack1(str3, 1000);
      Ptr<Packet> p = &pack1;
      m_socket_send[dest]->Send(p);
      break;
    }
    case PREPARE:
    {
      // Packet pack1(TYPE_PREPARE, 80);
      std::string str1 = "";
      str1.append(std::to_string((int)m_epoch));
      str1.append("+");
      std::string str2(TYPE_PREPARE, TYPE_PREPARE+10);
      str1.append(str2);
      const uint8_t *str3 = reinterpret_cast<const uint8_t*>(str1.c_str());
      Packet pack1(str3, 1000);
      Ptr<Packet> p = &pack1;
      m_socket_send[dest]->Send(p);
      break;
    }
    case COMMIT:
    {
      // Packet pack1(TYPE_COMMIT, 80);
      std::string str1 = "";
      str1.append(std::to_string((int)m_epoch));
      str1.append("+");
      std::string str2(TYPE_COMMIT, TYPE_COMMIT+10);
      str1.append(str2);
      const uint8_t *str3 = reinterpret_cast<const uint8_t*>(str1.c_str());
      Packet pack1(str3, 1000);
      Ptr<Packet> p = &pack1;
      m_socket_send[dest]->Send(p);
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

  std::cout<<"node "<<(int)GetNodeId()<<" send a "<<MessagetypeToString(((int)message_type))<<" to node "<<dest<<" at "<<Simulator::Now().GetSeconds()<<"s"<<std::endl;
}

void GossipApp::GossipMessageOut()
{
  int neighbors[GOSSIP_ROUND];
  ChooseNeighbor(GOSSIP_ROUND, neighbors);

  for(int i=0; i<GOSSIP_ROUND; i++)
  {
    ScheduleTransmit(Seconds (0.), neighbors[i], 0);
  }
}

void GossipApp::GossipMessageAfterReceive(int from_node)
{
  int neighbors[GOSSIP_ROUND];
  ChooseNeighbor(GOSSIP_ROUND, neighbors, from_node);

  for(int i=0; i<GOSSIP_ROUND; i++)
  {
    ScheduleTransmit(Seconds (0.), neighbors[i], 0);
  }
}

void GossipApp::BroadcastMessageOut(int type)
{
  // TODO some logic ambiguity
  if(type==3)
  {
    if(block_got)
    {
      for(int i=0; i<NODE_NUMBER; i++)
      { 
        if(i!=m_node_id)
          ScheduleTransmit(Seconds(0.), i, 3);
      }
    }
  }
  else if(type==4)
  {
    for(int i=0; i<NODE_NUMBER; i++)
      { 
        if(i!=m_node_id)
          ScheduleTransmit(Seconds(0.), i, 4);
      }
  }
    
}

void GossipApp::DetermineCommit()
{
  int sum = 0;
  for(int i=0; i<NODE_NUMBER; i++)
  {
    sum += map_node_PREPARE[i];
  }
  if(sum>=(2*NODE_NUMBER/3.0+1))
  {
    BroadcastMessageOut(4);
    Simulator::Schedule(Seconds(DETERMINECONSENS_INTERVAL), &GossipApp::DetermineConsens, this);
  }
  else
    Simulator::Schedule(Seconds(DETERMINECOMMIT_INTERVAL), &GossipApp::DetermineCommit, this);
}


void GossipApp::DetermineConsens()
{
  int sum=0;
  for(int i=0; i<NODE_NUMBER; i++)
  {
    sum += map_node_COMMIT[i];
  }
  if(sum>=(2*NODE_NUMBER/3.0+1))
  {
    map_epoch_consensed[m_epoch] = 1;
    // std::cout<<(int)m_node_id<<" has consensed"<<std::endl;
    // std::cout<<"temporal epoch: "<<(int)m_epoch<<"**********"<<std::endl;
    // std::cout<<map_epoch_consensed[m_epoch]<<std::endl;
  }
  else
    Simulator::Schedule(Seconds(DETERMINECONSENS_INTERVAL), &GossipApp::DetermineConsens, this);
}

void GossipApp::SolicitMessageFromOthers()
{
  if((Simulator::Now().GetSeconds() - m_epoch_beginning) >= WAITTING_TIME)
  {
    
    if(block_got==false)
    {
      int neighbors[SOLICIT_ROUND];
      ChooseNeighbor(SOLICIT_ROUND, neighbors);
      for(int i=0; i<SOLICIT_ROUND; i++)
      {
        ScheduleTransmit(Seconds (0.), neighbors[i], 1);
      }
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
      uint8_t content_[20];
      packet->CopyData(content_, 20);
      Ipv4Address from_addr = InetSocketAddress::ConvertFrom (from).GetIpv4 ();
      int from_node = (int)map_addr_node[from_addr];
      // std::cout<<"node "<<(int)GetNodeId()<<" received a "<<content_<<" "<<packet->GetSize()
      //   <<" bytes from node "<<from_node<<" at "<<Simulator::Now().GetSeconds()<<" s"<<std::endl;

      std::string str_of_content(content_, content_+20);
      std::vector<std::string> res = SplitMessage(str_of_content, '+');
      const char* time_of_recived_message = res[0].c_str();

      if(strcmp(time_of_recived_message, (std::to_string(m_epoch)).c_str())==0)
      {
        const char* type_of_received_message = res[1].c_str();
        if(strcmp(type_of_received_message, "BLOCK")==0)
        {
          if(block_got==false)
          {
            block_got = true;
            std::cout<<"node "<<(int)GetNodeId()<<" received a "<<content_<<" for the first time "<<packet->GetSize()
              <<" bytes from node "<<from_node<<" at "<<Simulator::Now().GetSeconds()<<" s"<<std::endl;
            GossipMessageAfterReceive(from_node);
            
          }
          else
            std::cout<<"node "<<(int)GetNodeId()<<" received a "<<content_<<" "<<packet->GetSize()
              <<" bytes from node "<<from_node<<" at "<<Simulator::Now().GetSeconds()<<" s"<<std::endl;
        }
        else if(strcmp(type_of_received_message, "SOLICIT")==0)
        {
          if(block_got==true)
          {
            ScheduleTransmit(Seconds(0.), (int)map_addr_node[from_addr], 0);
            std::cout<<"node "<<(int)GetNodeId()<<" responds node "<<from_node<<" and send him a block at "
              <<Simulator::Now().GetSeconds()<<" s"<<std::endl;

          }
          else{
            std::cout<<"node "<<(int)GetNodeId()<<" can't respond node "<<from_node<<" because until "<<
              Simulator::Now().GetSeconds()<<" s he has not received a block yet"<<std::endl;
          }
          
        }
        else if(strcmp(type_of_received_message, "PREPARE")==0)
        { 
          map_node_PREPARE[from_node] = 1;
          std::cout<<"node "<<(int)GetNodeId()<<" received a "<<content_<<" "<<packet->GetSize()
            <<" bytes from node "<<from_node<<" at "<<Simulator::Now().GetSeconds()<<" s"<<std::endl;
        }
        else if(strcmp(type_of_received_message, "COMMIT")==0)
        {
          map_node_COMMIT[from_node] = 1;
          std::cout<<"node "<<(int)GetNodeId()<<" received a "<<content_<<" "<<packet->GetSize()
            <<" bytes from node "<<from_node<<" at "<<Simulator::Now().GetSeconds()<<" s"<<std::endl;
        }
      }


      


    }
}

} // Namespace ns3
