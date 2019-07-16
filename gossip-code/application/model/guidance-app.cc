#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "ns3/simulator.h"
#include "guidance-app.h"

#include <time.h>
#include <stdlib.h>
#include <algorithm>
#include <fstream>
#include <regex>

#include <map>
#include <vector>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("GuidanceAppApplication");

NS_OBJECT_ENSURE_REGISTERED (GuidanceApp);

TypeId
GuidanceApp::GetTypeId(void)
{
	static TypeId tid =
 		TypeId ("ns3::GuidanceApp")
			.SetParent<Application> ()
			.SetGroupName ("Applications")
			.AddConstructor<GuidanceApp> ()
			.AddAttribute ("NodeId", "Node on which the application runs.", UintegerValue (0),
							MakeUintegerAccessor (&GuidanceApp::m_node_id),
							MakeUintegerChecker<uint8_t> ())
			
		;
	return tid;
}

GuidanceApp::GuidanceApp()
{
	NS_LOG_FUNCTION (this);
	m_epoch = 0;
}

GuidanceApp::~GuidanceApp()
{
	NS_LOG_FUNCTION (this);
}

void
GuidanceApp::DoDispose (void)
{
	NS_LOG_FUNCTION (this);
	Application::DoDispose ();
}

void GuidanceApp::StartApplication (void)
{
	NS_LOG_FUNCTION (this);

	// TODO
	delta = 0;
	srtt = 1;
	rttvar = 0;
	rtt = 1;
	cur_epoch_len = 160;
	RecordNeighbor();
	Simulator::Schedule(Seconds(600.0), &GuidanceApp::GuidanceProcess, this);

}

void
GuidanceApp::StopApplication ()
{
	NS_LOG_FUNCTION (this);
}

void GuidanceApp::GuidanceProcess()
{
	m_epoch++;
	m_epoch_beginning = Simulator::Now().GetSeconds();
	std::cout<<"EPOCH "<<m_epoch<<"********\n";
	std::cout<<"TIME NOW: "<<Simulator::Now().GetSeconds()<<"s\n";
	consensus_success = 0;
	for(int k=0; k<NODE_NUMBER; k++)
		map_node_vote[k] = 0;
	Simulator::Schedule(Seconds(cur_epoch_len+4*FIXED_EPOCH_LEN-1), &GuidanceApp::EndSummary, this);
	if(m_epoch<TOTAL_EPOCH_FOR_SIMULATION)
		Simulator::Schedule(Seconds(cur_epoch_len+4*FIXED_EPOCH_LEN), &GuidanceApp::GuidanceProcess, this);
}

void GuidanceApp::RecordNeighbor()
{
	srand((unsigned)time(0));
	std::map<int, std::vector<int> > map_node_out_neigh;
	std::map<int, std::vector<int> > map_node_in_neigh;
	for(int i=0; i<NODE_NUMBER; i++)
	{
		while(map_node_out_neigh[i].size()<OUT_NEIGHBOR_NUMBER)
		{
			int x = rand() % NODE_NUMBER;
			if(x!=i)
			{
				std::vector<int>::iterator it;
				it = std::find(map_node_out_neigh[i].begin(), map_node_out_neigh[i].end(), x);
				if(it==map_node_out_neigh[i].end())
				{
					std::vector<int>::iterator ti;
					ti = std::find(map_node_in_neigh[i].begin(), map_node_in_neigh[i].end(), x);
					if(ti==map_node_in_neigh[i].end())
					{
						if((int)map_node_in_neigh[x].size()<MAX_IN_NEIGHBOR_NUMBER)
						{
							map_node_out_neigh[i].push_back(x);
							map_node_in_neigh[x].push_back(i);
						}
					}
					
				}
			}
			
		}

	}

	std::ofstream neigh_recorder;
	neigh_recorder.open("scratch/subdir/log_cmjj/nodelink.txt");
	for(int i=0; i<NODE_NUMBER; i++)
    {
        for(int k=0; k<OUT_NEIGHBOR_NUMBER; k++)
            neigh_recorder<<map_node_out_neigh[i][k]<<"+";
        for(int k=0; k<(int)map_node_in_neigh[i].size(); k++)
            neigh_recorder<<map_node_in_neigh[i][k]<<"+";
        neigh_recorder<<"+"<<std::endl;
    }
    neigh_recorder.close();
}




void GuidanceApp::Read_data()
{

	map_node_onehoptime.clear();
	map_node_blockrcvtime.clear();
	vec_one_hop_time.clear();
	vec_block_rcv_time.clear();

	
	for(int i=0; i<NODE_NUMBER; i++)
	{
		std::string str2 = "scratch/subdir/log_cmjj/node_";
		str2.append(std::to_string(i));
		str2.append("_self_report.txt");
		std::ifstream fi;
		fi.open(str2);
		if (!fi)
			std::cout<<"fail to open "<<str2<<std::endl;
		std::string line;
		int v = 0;
		while(std::getline(fi, line))
		{
			const char *tms = line.c_str();
			if(strcmp("--------", tms)==0)
				v++;
			if(v==(int)m_epoch)
				break;
		}
		for(int n=0; n<3; n++)
			std::getline(fi, line);
		std::smatch match;
		// std::cout<<"node "<<i<<"--"<<line<<"\n";
		std::regex rx1("State: ([0-9]) ([0-9]*\\.?[0-9]*)");
    	if(regex_match(line, match, rx1))
    	{
    		std::string ss1 = match[1].str();
    		int tmp1 = (atoi)(ss1.c_str());
    		map_node_vote[i] = tmp1;
    		if(tmp1)
    		{
    			std::string ss2 = match[2].str();
    			float tmp2 = (atof) (ss2.c_str());
    			map_node_blockrcvtime[i] = tmp2;
    			vec_block_rcv_time.push_back(tmp2 - m_epoch_beginning);
    			std::regex rx2("SYN number: ([0-9]+)");
    			while(std::getline(fi, line))
    			{
    				if(regex_match(line, match, rx2))
    				{
    					std::string ss3 = match[1].str();
    					int tmp3 = (atoi) (ss3.c_str());
    					for(int i=0; i<tmp3; i++)
    						getline(fi, line);
    					break;
    				}
    			}
    			std::regex rx3("([0-9]+) ([0-9]*\\.?[0-9]*) sig ([0-9]*\\.?[0-9]*)");
    			if(regex_match(line, match, rx3))
    			{    				
    				std::string ss4 = match[3].str();
    				float tmp4 = (atof) (ss4.c_str());
    				map_node_onehoptime[i] = tmp2 - tmp4;
    				vec_one_hop_time.push_back(tmp2 - tmp4);
    			}
    		}
    		else
    		{
    			map_node_blockrcvtime[i] = -1;
    			std::regex rx2("SYN number: ([0-9]+)");
    			int tmp3;
    			while(getline(fi, line))
    			{
    				if(regex_match(line, match, rx2))
    				{

    					std::string ss3 = match[1].str();
    					tmp3 = (atoi) (ss3.c_str());
    					break;
    				}
    			}
    			for(int i=1; i<tmp3; i++)
    			{
    				getline(fi, line);
    				map_node_failedSYN[i].push_back(watchdog_timeout);
    			}
    			getline(fi, line);
    			std::regex rx4("([0-9]+) ([0-9]*\\.?[0-9]*) sig ([0-9]*\\.?[0-9]*)");
    			if(regex_match(line, match, rx4))
    			{
    				std::string ss4 = match[2].str();
    				float tmp6 = (atof) (ss4.c_str());
    				float tmp7 = cur_epoch_len - (tmp6 - m_epoch_beginning);
    				map_node_failedSYN[i].push_back(tmp7);
    			}
    			
    			map_node_onehoptime[i] = -1;
    		}

    	}
		// locate current epoch;
		// locate block rev time, map_node_blockrcvtime, vec_block_rcv_time;
		// locate the last SYN, fill map_node_onehoptime, vec_one_hop_time;
	
    	fi.close();

	}
	int tmp8 = vec_one_hop_time.size();
	std::cout<<"ONE HOP TIME: ("<<tmp8<<") ";
	for(int k=0; k<NODE_NUMBER; k++)
	{
		if(map_node_vote[k] && map_node_onehoptime[k]>0)
			std::cout<<map_node_onehoptime[k]<<" ";
		else
			std::cout<<"("<<k<<") ";
	}
	std::cout<<"\n";
	
}


void GuidanceApp::EndSummary()
{
	Read_data();
	int sum=0;
	for(int i=0; i<NODE_NUMBER; i++)
		sum += map_node_vote[i];
	if(sum>=2./3*NODE_NUMBER)
		consensus_success = 1;



	formal_res_quorum.clear();
	formal_res_quorum.insert(formal_res_quorum.begin(), curr_res_quorum.begin(), curr_res_quorum.end());
	Res_quorum_update(); // new res_quorum
	Trust_val_update();
	// Kickout_malicious();
	float x;
	float y;
	if(consensus_success)
	{
		// watchdog_timeout = Set_timeout();
		x = Set_timeout();
		// cur_epoch_len = Epoch_len_computation();
		y = Epoch_len_computation();
	}
	else
	{
		watchdog_timeout *= 2;
		cur_epoch_len *= 2;
	}

	std::cout<<"watchdog timeout: "<<x<<"\n";
	std::cout<<"next epoch length: "<<y<<"\n";
	std::cout<<"\n";
		

}



void GuidanceApp::Kickout_malicious()
{
	// TODO

}


float GuidanceApp::Set_timeout()
{
	for(int i=0; i<(int)vec_one_hop_time.size(); i++)
	{
		float s = vec_one_hop_time[i];
		delta = s - srtt;
		rttvar = rttvar + 1/4 * (abs(delta) - rttvar);
		rtt = srtt + 4 * rttvar;
	}
	return rtt;
}


float GuidanceApp::Epoch_len_computation()
{
	std::vector<float> tmp;
	tmp.insert(tmp.begin(), vec_block_rcv_time.begin(), vec_block_rcv_time.end());
	std::sort(tmp.begin(), tmp.end());
	int x = NODE_NUMBER*(1-BYZANTINE_P);
	float y = tmp[x] + CONSERVE_LEN;
	return y;
}


void GuidanceApp::Res_quorum_update()
{
	curr_res_quorum.clear();
	for(int i=0; i<NODE_NUMBER; i++)
	{
		if(map_node_vote[i]==1)
			curr_res_quorum.push_back(i);
	}
}


void GuidanceApp::Trust_val_update()
{
	for(int i=0; i<NODE_NUMBER; i++)
	{
		std::vector<int>::iterator it;
		it = std::find(curr_res_quorum.begin(), curr_res_quorum.end(), i);
		if(it!=curr_res_quorum.end())
			map_node_trustval[i].push_back(1);
		else
		{
			float x = 1;
			for(int j=0; j<(int)map_node_failedSYN[i].size(); j++)
			{
				float tmp1 = map_node_failedSYN[i][j];
				x *= failrcv_probability(tmp1);
			}
			map_node_trustval[i].push_back(x);
		}
	}
}


float GuidanceApp::failrcv_probability(float x)
{
	// TODO given SYN, the probability that node n can't receive blocks in time
	float y = timeout_probability(x)*(1-BYZANTINE_P) + BYZANTINE_P;
	return y;
}


float GuidanceApp::timeout_probability(float x)
{
	int sum=0;
	int s = vec_one_hop_time.size();
	for(int i=0; i<s; i++)
	{
		if(x>=vec_one_hop_time[i])
			sum++;
	}
	return (sum+0.1)/(s+0.1);
}



} // namespace ns3
