#ifndef DATA_STRUCT_H
#define DATA_STRUCT_H

#include <stdint.h>

class State{

public:
    uint32_t B_root;
    int H_root;
    uint32_t B_pending;
    int freshness;

};

#endif