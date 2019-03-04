#include <iostream>
#include <fstream>
#include <time.h>
#include <stdlib.h>
#include <map>
#include "topology-generator.h"

using namespace std;

bool topology()
{	
	bool quality_of_node = false;
	bool quality_of_edge = true;

	ofstream outfile;
	outfile.open("topology.txt");
	outfile<<"AP and attached device number\n";
	int sum=0;
	map<int, int> map_ap_edge;
	for(int i=0; i<AP_NUMBER; i++)
	{
		map_ap_edge[i] = 0;
	}

	srand((unsigned)time(NULL));
	for(int i=0; i<AP_NUMBER; i++)
	{
		int device_of_AP = rand() % MAX_DEVICE_OF_AP;
		device_of_AP = device_of_AP>0?device_of_AP:(device_of_AP+5);
		sum += device_of_AP;
		outfile<<i<<" "<<device_of_AP<<"\n";

	}
	outfile<<"\n\n";
	outfile<<"edge between AP\n";

	srand((unsigned)time(NULL)+sum);
	for(int i=0; i<AP_NUMBER; i++)
	{
		for(int j=i+1; j<AP_NUMBER; j++)
		{
			int x=rand()%1000;
			if(x<300)
			{
				outfile<<i<<" "<<j<<"\n";
				map_ap_edge[i]++;
				map_ap_edge[j]++;
			}
		}
	}

	for(int i=0; i<AP_NUMBER; i++)  //every ap should at least have 2 edges.
	{
		if(map_ap_edge[i]<2)
			quality_of_edge = false;
	}
	outfile.close();
	

	float total_device = (MAX_DEVICE_OF_AP + 1.)/2 * AP_NUMBER;
	if(sum>=total_device*0.9 && sum<=total_device*1.1)  // control device number.
		quality_of_node = true;
	else 
		quality_of_node = false;
	if(quality_of_node && quality_of_edge)
		return true;
	else
		return false;
}

int main()
{
	bool res=false;
	while(res==false)
		res = topology();
	return 0;
}
