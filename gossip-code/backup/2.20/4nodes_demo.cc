/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU GenLogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
   LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);eral Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("4NodesDemo");

int
main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);
  
  Time::SetResolution (Time::NS);
  LogComponentEnable ("UdpCommunicateClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpCommunicateServerApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

  NodeContainer nodes;
  nodes.Create (2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5KBps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("0ms"));
  NetDeviceContainer devices;
  devices = pointToPoint.Install (nodes);



  InternetStackHelper stack;
  stack.Install (nodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");

  Ipv4InterfaceContainer interfaces = address.Assign (devices);


  UdpCommunicateServerHelper communicateServer1 (17);
  ApplicationContainer serverApps1 = communicateServer1.Install (nodes.Get (0));
  serverApps1.Start (Seconds (1.0));
  serverApps1.Stop (Seconds (10000.0));

  UdpCommunicateClientHelper communicateClient1 (interfaces.GetAddress (0), 17);
  communicateClient1.SetAttribute ("MaxPackets", UintegerValue (1));
  communicateClient1.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  communicateClient1.SetAttribute ("PacketSize", UintegerValue (10000));
  ApplicationContainer clientApps1 = communicateClient1.Install (nodes.Get (1));
  clientApps1.Start (Seconds (20.0));
  clientApps1.Stop (Seconds (10000.0));



  UdpCommunicateServerHelper communicateServer2 (19);
  ApplicationContainer serverApps2 = communicateServer2.Install (nodes.Get (1));
  serverApps2.Start (Seconds (1.0));
  serverApps2.Stop (Seconds (10000.0));

  UdpCommunicateClientHelper communicateClient2 (interfaces.GetAddress (1), 19);
  communicateClient2.SetAttribute ("MaxPackets", UintegerValue (1));
  communicateClient2.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  communicateClient2.SetAttribute ("PacketSize", UintegerValue (10240));
  ApplicationContainer clientApps2 = communicateClient2.Install (nodes.Get (0));
  clientApps2.Start (Seconds (20.0));
  clientApps2.Stop (Seconds (10000.0));



  // UdpEchoServerHelper echoServer (9);
  // ApplicationContainer serverApps1 = echoServer.Install (nodes.Get (1));
  // serverApps1.Start (Seconds (1.0));
  // serverApps1.Stop (Seconds (10.0));

  // UdpEchoClientHelper echoClient (interfaces.GetAddress (1), 9);
  // echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
  // echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  // echoClient.SetAttribute ("PacketSize", UintegerValue (1024));
  // ApplicationContainer clientApps1 = echoClient.Install (nodes.Get (0));
  // clientApps1.Start (Seconds (2.01));
  // clientApps1.Stop (Seconds (10.0));

  



  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
