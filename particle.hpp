#include <stdlib.h>
#include <vector>
#include <iostream>
#include <math.h>
#pragma once

using namespace std;

// Spatial setup from Table 1 of Gonzalez et al., Pramana 100:7 (2026)
const double PAPER_L = 32.0 * M_PI; // domain length
const int PAPER_N = 512;            // number of cells

// Normalised units, chosen so that the plasma frequency is 1 as in Table 1
const double ELECTRON_Q = -1.0;     // electron charge
const double N0 = 1.0;              // background electron density

class Grid;

class Particle
{
    double x; // always in [0, L), every write goes through Grid::wrap
    //vel v;
    //float q; // charge
    //float m; // mass
    public:
        void updatePOS(double dx, const Grid &rf);

        Particle();
        Particle(double x1, const Grid &rf);

        double retPos() const;

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

    // charge density, rho[i] sits on node x_i
    vector<double> rho;

    public:
        Grid(double length, int cells);

        double retLength() const { return L; }
        int retCells() const { return n; }
        double retSpacing() const { return h; }
        double retRho(int i) const { return rho[i]; }

        // x_i, position of node i
        double nodePos(int i) const { return i * h; }
        int index(int i) const;

        // Eq. (10), x mod L. The result is always in [0, L).
        double wrap(double x) const;

        void deposit(const vector<Particle> &particles);
        void fieldSolve();
};

struct vel
{
    float vx;

};
