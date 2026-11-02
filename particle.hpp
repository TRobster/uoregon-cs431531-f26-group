#include <stdlib.h>
#include <vector>
#include <iostream>
#include <math.h>
#pragma once

using namespace std;

// Spatial setup from Table 1 of Gonzalez et al., Pramana 100:7 (2026)
const double PAPER_L = 32.0 * M_PI; // domain length
const int PAPER_N = 512;            // number of cells

class Particle
{
    double x; // always in [0, L), every write goes through Grid::wrap
    //vel v; 
    //float q; // charge
    //float m; // mass
    public:
        void updatePOS(double dx, const Grid &rf);

        Particle()
        {
            x = 0;
            // fl vel = 0;
            // fl force = 0;
            // fl dens = 0;
        }
        Particle(double x1, const Grid &rf)
        {
            x = rf.wrap(x1);
        }

        double retPos() const
        {
            return this->x;
        }

        int cellIndex(const Grid &rf) const;

};


class Grid
{
    double L;   // domain length
    double h;   // cell size, always derived from L and n
    int n;      // number of cells, which is also the number of unique nodes
    // field array, E[i] sits on node x_i
    vector<double> E;

    vector<double> phi;

    public:
        Grid(double length, int cells)
        {
            L = length;
            n = cells;
            h = L / n;
            E.assign(n, 0.0);
        }

        double retLength() const { return L; }
        int retCells() const { return n; }
        double retSpacing() const { return h; }

        // x_i, position of node i
        double nodePos(int i) const { return i * h; }
        int index(int i) const { return (i % n + n) % n; }

        // Eq. (10), x mod L. The result is always in [0, L).
        double wrap(double x) const

        void fieldSolve()
    }
struct vel
{
    float vx;

};