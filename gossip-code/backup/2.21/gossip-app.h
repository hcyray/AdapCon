#ifndef GOSSIP_APP_H
#define GOSSIP_APP_H

#include "ns3/core-module.h"
#include "ns3/application.h"
#include "ns3/nstime.h"
#include "ns3/internet-module.h"
#include "ns3/pointer.h"

namespace ns3{



class GossipApp: public Application
{
public:
	static TypeId GetTypeId(void);
	GossipApp();
	virtual ~GossipApp();

	void SetGossipInterval(Time val);
	void SetSolicitInterval(Time val);
	void SetCurrentValue(int val);

	void HandleSolicit(Ipv4Address src, Ipv4Address dest); // TODO why need parameters?
	void HandleAck(void);
	void HandleRead (Ptr<Socket> socket);

protected:
	virtual void DoDispose(void);

	void SendMessage(Ipv4Address src, Ipv4Address dest, int type); // TODO when this function work?
	void SendPayload(Ipv4Address src, Ipv4Address dest);
private:

	bool halt;
	Time gossip_delta_t;
	Time solicit_delta_t;
	int CurrentValue;
	int SentMessage;

	Ptr<Socket> m_socket;

	void GossipProcess();
	void Solicit(void);

	virtual void StartApplication();
	virtual void StopApplication();
};	

} // Namespace ns3

#endif // GOSSIP_APP_H
