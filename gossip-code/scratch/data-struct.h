#ifndef DATA_STRUCT_H
#define DATA_STRUCT_H

#include <stdint.h>

class Block{
public:
    uint32_t name;
    int height;
};



class State_Quad{
public:
    Block B_root;
    int H_root;
    Block B_pending;
    int freshness;

    // State_Quad();
    // State_Quad(uint32_t b1, int h1, uint32_t b2, int h2);
    
};

class Msg_INV{
public:
	int node;
	float create_time;
	float receive_time;

};


class Msg_SYN{
public:
	int node;
	float create_time;
	float receive_time;
};


class Msg_ACK{
public:
	int node;
	float create_time;
	float receive_time;
};

#endif