/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
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
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "communicate-helper.h"
#include "ns3/communicate-server.h"
#include "ns3/communicate-client.h"
#include "ns3/uinteger.h"
#include "ns3/names.h"
#include "ns3/string.h"
#include "ns3/pointer.h"

namespace ns3 {


UdpCommunicateServerHelper::UdpCommunicateServerHelper (
  uint8_t nodeid, uint16_t port)
{
  m_factory.SetTypeId (UdpCommunicateServer::GetTypeId ());
  SetAttribute("NodeId", UintegerValue(nodeid));
  SetAttribute ("Port", UintegerValue (port));
}

void 
UdpCommunicateServerHelper::SetAttribute (
  std::string name, 
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
UdpCommunicateServerHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
UdpCommunicateServerHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
UdpCommunicateServerHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
UdpCommunicateServerHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<UdpCommunicateServer> ();
  node->AddApplication (app);

  return app;
}

UdpCommunicateClientHelper::UdpCommunicateClientHelper (Address address, uint16_t port)
{
  m_factory.SetTypeId (UdpCommunicateClient::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (address));
  SetAttribute ("RemotePort", UintegerValue (port));
}

UdpCommunicateClientHelper::UdpCommunicateClientHelper (Address address)
{
  m_factory.SetTypeId (UdpCommunicateClient::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (address));
}

void 
UdpCommunicateClientHelper::SetAttribute (
  std::string name, 
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

void
UdpCommunicateClientHelper::SetFill (Ptr<Application> app, std::string fill)
{
  app->GetObject<UdpCommunicateClient>()->SetFill (fill);
}

void
UdpCommunicateClientHelper::SetFill (Ptr<Application> app, uint8_t fill, uint32_t dataLength)
{
  app->GetObject<UdpCommunicateClient>()->SetFill (fill, dataLength);
}

void
UdpCommunicateClientHelper::SetFill (Ptr<Application> app, uint8_t *fill, uint32_t fillLength, uint32_t dataLength)
{
  app->GetObject<UdpCommunicateClient>()->SetFill (fill, fillLength, dataLength);
}

ApplicationContainer
UdpCommunicateClientHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
UdpCommunicateClientHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
UdpCommunicateClientHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
UdpCommunicateClientHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<UdpCommunicateClient> ();
  node->AddApplication (app);

  return app;
}

} // namespace ns3
