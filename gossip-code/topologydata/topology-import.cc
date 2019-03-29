#include <iostream>
#include <fstream>
#include <sstream>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <utility>
#include <vector>


using namespace std;

vector<string> SplitString(const string& str, const char pattern)
{
    vector<string> res;
    stringstream input(str);   
    string temp;
    while(getline(input, temp, pattern))
    {
        res.push_back(temp);
    }
    return res;
}



void Get_Topology(map<int, int>& map_AP_devicenumber, vector<pair<int, int> >& vecotr_edge)
{
	ifstream infile;
	infile.open("topology.txt");
	const int LINE_LENGTH = 100; 
    char str1[LINE_LENGTH];
	int sum = 0;
    while(infile.getline(str1,LINE_LENGTH))
    {
        if(strcmp(str1, "AP and attached device number")==0)
            continue;
        if(strcmp(str1, "edge between AP")==0)
            break;
        string str2(str1);
        vector<string> res = SplitString(str2, ' ');
        if(res.size()==2)
        {
            int x = (atoi)(res[0].c_str());
            int y = (atoi)(res[1].c_str());
            cout<<x<<" "<<y<<endl;
            map_AP_devicenumber[x] = y;
			sum += y;
			
        }
        else
            continue;        
    }
    cout<<"total device: "<< sum<<endl;

    while(infile.getline(str1,LINE_LENGTH))
    {
        string str2(str1);
        vector<string> res = SplitString(str2, ' ');
        if(res.size()==2)
        {
            pair<int, int> p;
            p.first = (atoi)(res[0].c_str());
            p.second = (atoi)(res[1].c_str());
            vecotr_edge.push_back(p);
        }
        else
            continue;
    }
    cout<<"total edge: "<<vecotr_edge.size()<<endl;

    infile.close();
	
}

int main()
{
    map<int, int> map_AP_devicenumber;
    vector<pair<int, int> > vecotr_edge;

    Get_Topology(map_AP_devicenumber, vecotr_edge);
/*
    for(int i=0; i<30; i++)
        cout<<map_AP_devicenumber[i]<<"  ";
    cout<<endl;

    for(int i=0; i<vecotr_edge.size(); i++)
    {
        cout<<vecotr_edge[i].first<<" "<<vecotr_edge[i].second<<endl;
    }
*/
    return 0;
}
