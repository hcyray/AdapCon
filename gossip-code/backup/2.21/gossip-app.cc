#include "ns3/socket.h"
#include "ns3/packet.h"
#include "gossip-app.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("GossipAppApplication");
// NS_OBJECT_ENSURE_REGISTERED(GossipApp);


TypeId
GossipApp::GetTypeId(void)
{
	static TypeId tid = TypeId("ns3::GossipApp")
		.SetParent<Application>()
		.SetGroupName("Applications")
		.AddConstructor<GossipApp>()
		;
	return tid;
}

GossipApp::GossipApp()
{
	NS_LOG_FUNCTION(this);

}

GossipApp::~GossipApp()
{
	NS_LOG_FUNCTION(this);
	m_socket = 0;
}

void GossipApp::SetGossipInterval(Time val)
{
	NS_LOG_FUNCTION(this << val);
	gossip_delta_t = val;
}

void GossipApp::SetSolicitInterval(Time val)
{
	NS_LOG_FUNCTION(this << val);
	solicit_delta_t = val;
}

void GossipApp::SetCurrentValue(int val)
{
	NS_LOG_FUNCTION(this << val);
	CurrentValue = val;
	NS_LOG_INFO("Value of node set to" << CurrentValue);
	
}

void GossipApp::HandleSolicit(Ipv4Address src, Ipv4Address dest)
{
	NS_LOG_INFO("HandleSolicit");
	NS_LOG_INFO("Time: "<< Simulator::Now().GetSeconds()<<"s");
	if(CurrentValue!=0)
	{
		// TODO reply for solicit
	}
}

void GossipApp::HandleAck()
{
	NS_LOG_INFO("HandleAck");
	NS_LOG_INFO("Time: "<<Simulator::Now().GetSeconds()<<"s");

	halt = true;
}

void 
GossipApp::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  Ptr<Packet> packet;
  Address from;
  // Address localAddress;
  while ((packet = socket->RecvFrom (from)))
    {
      printf("%s\n", "server received a packet");
    }
}

void GossipApp::DoDispose(void)
{
	NS_LOG_FUNCTION(this);
	Application::DoDispose();
}


void GossipApp::SendMessage(Ipv4Address src, Ipv4Address dest, int type)
{
	SentMessage++;
	Ipv4Header header = Ipv4Header();
	header.SetDestination(dest);
	header.SetPayloadSize(0);
	header.SetSource(src);

	// TODO using Ipv4Header to send message out
}

void GossipApp::SendPayload(Ipv4Address src, Ipv4Address dest)
{
	SentMessage++;
	Ipv4Header header = Ipv4Header();
	header.SetDestination(dest);
	header.SetPayloadSize(0);
	header.SetSource(src);

	// TODO using 
}

void GossipApp::GossipProcess(void)
{
	if(!halt)
	{
		Simulator::Schedule(gossip_delta_t, &GossipApp::GossipProcess, this);
		if(CurrentValue!=0)
		{
			// TODO
			// send current value to a random neighbor
		}
	}
}

void GossipApp::Solicit(void)
{
	if(CurrentValue==0)
	{
		Simulator::Schedule(solicit_delta_t, &GossipApp::Solicit, this);
		// TODO
		// send a solicit message to a random neighbor
	}
}

void GossipApp::StartApplication(void)
{
	NS_LOG_FUNCTION(this);
	// TODO
	if (m_socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 16);
      if (m_socket->Bind (local) == -1)
        {
          NS_FATAL_ERROR ("Failed to bind socket");
        }
    }


  	m_socket->SetRecvCallback (MakeCallback (&GossipApp::HandleRead, this));

	Simulator::Schedule(gossip_delta_t, &GossipApp::GossipProcess, this);
	Simulator::Schedule(solicit_delta_t, &GossipApp::Solicit, this);
}

void GossipApp::StopApplication(void)
{
	NS_LOG_FUNCTION(this);
	if (m_socket != 0) 
    {
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }
}

} // Namespace ns3