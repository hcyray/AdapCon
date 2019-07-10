#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "guidance-app.h"

#include <time.h>
#include <stdlib.h>
#include <algorithm>
#include <fstream>

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
	RecordNeighbor();

}

void
GuidanceApp::StopApplication ()
{
	NS_LOG_FUNCTION (this);
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
			std::vector<int>::iterator it;
			it = std::find(map_node_out_neigh[i].begin(), map_node_out_neigh[i].end(), x);
			if(it==map_node_out_neigh[i].end())
			{
				if(map_node_in_neigh[x].size()<MAX_IN_NEIGHBOR_NUMBER)
				{
					map_node_out_neigh[i].push_back(x);
					map_node_in_neigh[x].push_back(i);
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


} // namespace ns3
