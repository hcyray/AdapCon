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

#include "user-traffic.h"

#include <stdlib.h>
#include <time.h>
#include <algorithm>
#include <fstream>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("UserTrafficApplication");

NS_OBJECT_ENSURE_REGISTERED (UserTraffic);



TypeId
UserTraffic::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::UserTraffic")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<UserTraffic> ()
    // .AddAttribute ("NodeId", "Node on which the application runs.",
    //                UintegerValue (0),
    //                MakeUintegerAccessor (&UserTraffic::m_node_id),
    //                MakeUintegerChecker<uint8_t> ())
    .AddAttribute ("Port", "Port on which we listen for incoming packets.",
                   UintegerValue (27),
                   MakeUintegerAccessor (&UserTraffic::m_peerPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("RemoteAddress",
                   "The destination address of the outbound packets",
                   AddressValue (),
                   MakeAddressAccessor (&UserTraffic::m_peerAddress),
                   MakeAddressChecker ())
    // .AddTraceSource ("Rx", "A packet has been received",
    //                  MakeTraceSourceAccessor (&UserTraffic::m_rxTrace),
    //                  "ns3::Packet::TracedCallback")
    // .AddTraceSource ("RxWithAddresses", "A packet has been received",
    //                  MakeTraceSourceAccessor (&UserTraffic::m_rxTraceWithAddresses),
    //                  "ns3::Packet::TwoAddressTracedCallback")
  ;
  return tid;
}

UserTraffic::UserTraffic ()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;

}



UserTraffic::~UserTraffic()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  
}



std::vector<std::string> UserTraffic::SplitMessage(const std::string& str, const char pattern)
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
UserTraffic::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void 
UserTraffic::StartApplication (void)
{
  NS_LOG_FUNCTION (this);
  

  if (m_socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      InetSocketAddress remote = InetSocketAddress (Ipv4Address::ConvertFrom (m_peerAddress), m_peerPort);
      m_socket->Bind();
      if(m_socket->Connect(remote)!=0)
        std::cout<<"user socket connection failed!"<<std::endl;
    }
  m_socket->SetRecvCallback (MakeCallback (&UserTraffic::HandleTraffic, this));

  Simulator::Schedule(Seconds(1.), &UserTraffic::ScheduleTransmit, this);
}

void 
UserTraffic::StopApplication ()
{
  NS_LOG_FUNCTION (this);
  // printf("%s\n", "Server stops!");
  if (m_socket != 0) 
    {
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }
}

void UserTraffic::ScheduleTransmit()
{
  double time = Simulator::Now().GetSeconds();
  if(time<200)
  {
    double traffic = TrafficData(time);
    SendTraffic(traffic);
  }
  Simulator::Schedule(Seconds(5.), &UserTraffic::ScheduleTransmit, this);
}

double UserTraffic::TrafficData(double time)
{
  if(time>0)
    return 1024*32;
  return 1024*32;
}

void UserTraffic::SendTraffic(double traffic)
{
  Ptr<Packet> p;
  double x = 1024*32;
  double loop_number = traffic/x;
  // uint8_t const buffer[x] = "Hello world";
  p = Create<Packet> (x);
  for(int i=0; i<loop_number; i++)
  {
    if(m_socket->Send(p)==-1)
      std::cout<<"User fail to send a packet to server!"<<std::endl;
    std::cout<<"user send a packet to server at "<<Simulator::Now().GetSeconds()<<"s"<<std::endl;
  }
  
}



void 
UserTraffic::HandleTraffic (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from)))
    {
      // uint8_t content_[20];
      // packet->CopyData(content_, 20);
      // Ipv4Address from_addr = InetSocketAddress::ConvertFrom (from).GetIpv4 ();
      // int from_node = (int)map_addr_node[from_addr];
      // std::cout<<"node "<<(int)GetNodeId()<<" received a "<<content_<<" "<<packet->GetSize()
      //   <<" bytes from node "<<from_node<<" at "<<Simulator::Now().GetSeconds()<<" s"<<std::endl;
      std::cout<<"user received "<<packet->GetSize()<<" bytes from "<<InetSocketAddress::ConvertFrom (from).GetIpv4 ()
        <<" at "<<Simulator::Now().GetSeconds()<<" s"<<std::endl;

      // std::string str_of_content(content_, content_+20);
      // std::vector<std::string> res = SplitMessage(str_of_content, '+');
      // const char* time_of_recived_message = res[0].c_str();
    }
}

} // Namespace ns3
