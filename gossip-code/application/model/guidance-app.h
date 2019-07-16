#ifndef GUIDANCE_APP_H
#define GUIDANCE_APP_H

#include "ns3/application.h"
#include "ns3/gossip-app.h"
#include <map>
#include <vector>

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
	void GuidanceProcess();
	void EndSummary();
	void Read_data();
	void Kickout_malicious();
	float Set_timeout();
	float Epoch_len_computation();
	void Res_quorum_update();
	void Trust_val_update();
	float failrcv_probability(float x);
	float timeout_probability(float x);


protected:
	virtual void DoDispose(void);

private:
	virtual void StartApplication(void);
	virtual void StopApplication (void);

	


	int m_node_id;
	int m_epoch;
	float m_epoch_beginning;
	int consensus_success;

	float cur_epoch_len;
	float watchdog_timeout;

	float delta;
	float srtt;
	float rttvar;
	float rtt;

	std::map<int, int> map_node_vote;
	std::map<int, float> map_node_blockrcvtime;
	std::vector<float> vec_block_rcv_time;
	std::map<int, float> map_node_onehoptime;
	std::vector<float> vec_one_hop_time;
	std::map<int, std::vector<float> > map_node_trustval;
	std::map<int, std::vector<float> > map_node_failedSYN;
	std::vector<int> formal_res_quorum;
	std::vector<int> curr_res_quorum;

};


} // namespace ns3

#endif