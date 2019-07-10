#ifndef GUIDANCE_APP_H
#define GUIDANCE_APP_H

#include "ns3/application.h"
#include "ns3/gossip-app.h"
#include <map>


namespace ns3 {

class Socket;
class Packet;

// const int NODE_NUMBER = 52;
// const int OUT_NEIGHBOR_NUMBER = 2;
// const int MAX_IN_NEIGHBOR_NUMBER = 4;

class GuidanceApp: public Application
{
public:
	static TypeId GetTypeId(void);
	GuidanceApp();
	virtual ~GuidanceApp();

	void RecordNeighbor();

protected:
	virtual void DoDispose(void);

private:
	virtual void StartApplication(void);
	virtual void StopApplication (void);

	int m_node_id;
	int m_epoch;

};


} // namespace ns3

#endif