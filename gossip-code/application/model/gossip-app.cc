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

  for(int i=0; i<NODE_NUMBER; i++)
  {
    std::string ipaddress_string = "10.1.";
    ipaddress_string.append(std::to_string(i+1));
    ipaddress_string.append(".1");
    char* ipaddress = (char*)ipaddress_string.data();
    map_node_addr[i] = Ipv4Address(ipaddress);
  }


}



GossipApp::~GossipApp()
{
  NS_LOG_FUNCTION (this);
  m_socket_receive = 0;
}

void GossipApp::if_leader(void)
{
  uint8_t x = m_epoch % NODE_NUMBER;
  if(m_node_id==x)
    m_leader = true;
  else
    m_leader = false;

}

void GossipApp::ChooseNeighbor(int neighbor_choosed[GOSSIP_ROUND])
{
  srand((unsigned)Simulator::Now().GetSeconds());
  // srand(910.);
  for(int i=0; i<GOSSIP_ROUND; i++)
  {
  int x = (rand() % (NODE_NUMBER));
  neighbor_choosed[i] = x;
  }
  
}

uint8_t GossipApp::GetNodeId(void)
{
  return m_node_id;
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
    if(m_socket_send[i]->Connect (InetSocketAddress (Ipv4Address::ConvertFrom(map_node_addr[i]), 17))==0)
      // std::cout<<"connection to "<<i<<" succeed"<<std::endl;
    m_socket_send[i]->Connect (InetSocketAddress (Ipv4Address::ConvertFrom(map_node_addr[i]), 17));
    
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

  if_leader();
  if(m_leader)
  {
    std::cout<<"node "<<(int)GetNodeId()<<" is the leader"<<std::endl;
    GossipMessageOut();
    // Simulator::Schedule (Seconds(0.), &GossipApp::GossipMessageOut,this);
  }
  else
  {
    // TODO wait for some time then query neighbors
  }
  // ScheduleTransmit (Seconds (0.), 0);

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
  
}



void GossipApp::Send(int dest)
{
  NS_LOG_FUNCTION(this);

  Packet pack1(TYPE_BLOCK, 1024);
  Ptr<Packet> p = &pack1;
  m_socket_send[dest]->Send(p);
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

void GossipApp::ScheduleTransmit(Time dt, int dest)
{
  // NS_LOG_FUNCTION(this << dt);
  // if(m_socket_send == 0)
  // {
  //   TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
  //   m_socket_send = Socket::CreateSocket(GetNode(), tid);
  //   m_socket_send->Connect (InetSocketAddress (Ipv4Address::ConvertFrom(map_node_addr[dest]), 17));
  // }
  
  m_sendEvent = Simulator::Schedule (dt, &GossipApp::Send, this, dest);

  std::cout<<"node "<<(int)GetNodeId()<<" send a packet to node "<<dest<<" at "<<Simulator::Now().GetSeconds()<<"s"<<std::endl;
}

void GossipApp::GossipMessageOut()
{
  int neighbors[GOSSIP_ROUND];
  ChooseNeighbor(neighbors);

  for(int i=0; i<GOSSIP_ROUND; i++)
  {
    // std::cout<<neighbors[i]<<"********"<<std::endl;
    ScheduleTransmit(Seconds (0.), neighbors[i]);
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
      uint8_t content_[10];
      packet->CopyData(content_, 10); 
      std::cout<<"node "<<(int)GetNodeId()<<" received a packet "<<packet->GetSize()<<" bytes at "<<Simulator::Now().GetSeconds()<<" s"<<std::endl;
      std::cout<<"packet content:"<<content_<<std::endl;
    }
}

} // Namespace ns3
