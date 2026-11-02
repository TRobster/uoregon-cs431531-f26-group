#include <stdlib.h>
#include <vector>
#include <iostream>
#include <math.h>
#pragma once

using namespace std;

struct dim
{
    float st; 
    float l;
};

struct cell
{
    int i;
    int j;
    int k; // the z-slice this particle sits in
};

struct coord
{
    float x;
    float y; 
    float z; // slice operator, fixed at
};

struct vel
{
    float vx;
    float vy;
    float vz;
};