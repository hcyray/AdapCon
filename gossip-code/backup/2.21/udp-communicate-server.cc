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

#include "udp-communicate-server.h"
// #include "/home/hqw/repos/ns-3-allinone/ns-3-dev/scratch/variable-init.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("UdpCommunicateServerApplication");

NS_OBJECT_ENSURE_REGISTERED (UdpCommunicateServer);



TypeId
UdpCommunicateServer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::UdpCommunicateServer")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<UdpCommunicateServer> ()
    .AddAttribute ("NodeId", "Node on which the application runs.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&UdpCommunicateServer::node_id),
                   MakeUintegerChecker<uint8_t> ())
    .AddAttribute ("Port", "Port on which we listen for incoming packets.",
                   UintegerValue (9),
                   MakeUintegerAccessor (&UdpCommunicateServer::m_port),
                   MakeUintegerChecker<uint16_t> ())
    // .AddTraceSource ("Rx", "A packet has been received",
    //                  MakeTraceSourceAccessor (&UdpCommunicateServer::m_rxTrace),
    //                  "ns3::Packet::TracedCallback")
    // .AddTraceSource ("RxWithAddresses", "A packet has been received",
    //                  MakeTraceSourceAccessor (&UdpCommunicateServer::m_rxTraceWithAddresses),
    //                  "ns3::Packet::TwoAddressTracedCallback")
  ;
  return tid;
}

UdpCommunicateServer::UdpCommunicateServer ()
{
  NS_LOG_FUNCTION (this);
  m_sendEvent = EventId ();
  m_sent = 0;

  for(int i=0; i<100; i++)
  {
    std::string ipaddress_string = "10.1.";
    ipaddress_string.append(std::to_string(i+1));
    ipaddress_string.append(".1");
    char* ipaddress = (char*)ipaddress_string.data();
    map_node_addr[i] = Ipv4Address(ipaddress);
  }
}



UdpCommunicateServer::~UdpCommunicateServer()
{
  NS_LOG_FUNCTION (this);
  m_socket_receive = 0;
}

uint8_t UdpCommunicateServer::GetNodeId(void)
{
  return node_id;
}

void
UdpCommunicateServer::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void 
UdpCommunicateServer::StartApplication (void)
{
  NS_LOG_FUNCTION (this);
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
  m_socket_receive->SetRecvCallback (MakeCallback (&UdpCommunicateServer::HandleRead, this));


  

  ScheduleTransmit (Seconds (0.));

}

void 
UdpCommunicateServer::StopApplication ()
{
  NS_LOG_FUNCTION (this);
  // printf("%s\n", "Server stops!");
  if (m_socket_receive != 0) 
    {
      m_socket_receive->Close ();
      m_socket_receive->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }
  
}

void UdpCommunicateServer::Send(void)
{
  NS_LOG_FUNCTION(this);

  


  // uint32_t m_dataSize=1;
  // uint8_t *m_data=0;
  // std::cout<<m_data<<" "<<m_dataSize<<std::endl;
  // uint8_t const* raw = NULL;
  // p = Create<Packet>(raw, 200);
  Packet pack1(TYPE_BLOCK, 10240);
  Ptr<Packet> p = &pack1;
  m_socket_send->Send(p);
  std::cout<<"server "<<(int)GetNodeId()<<" send a packet out at "<<Simulator::Now().GetSeconds()<<"s"<<std::endl;
  ++m_sent;
  if (m_sent < m_count) 
    {
      ScheduleTransmit (m_interval);
    }

}

void UdpCommunicateServer::ScheduleTransmit(Time dt)
{
  NS_LOG_FUNCTION(this << dt);
  if(m_socket_send == 0)
  {
    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    m_socket_send = Socket::CreateSocket(GetNode(), tid);
    m_socket_send->Connect (InetSocketAddress (Ipv4Address::ConvertFrom(map_node_addr[0]), 17));
  }
  m_sendEvent = Simulator::Schedule (dt, &UdpCommunicateServer::Send, this);
}

void 
UdpCommunicateServer::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  Ptr<Packet> packet;
  Address from;
  // Address localAddress;
  while ((packet = socket->RecvFrom (from)))
    {
      uint8_t content_[10];
      packet->CopyData(content_, 10); 
      std::cout<<"server "<<(int)GetNodeId()<<" received a packet "<<packet->GetSize()<<" bytes at "<<Simulator::Now().GetSeconds()<<" s"<<std::endl;
      std::cout<<"packet content:"<<content_<<std::endl;
    }
}

} // Namespace ns3
