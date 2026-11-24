#include <stdlib.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include <math.h>
#include <type_traits>
#pragma once

using namespace std;

// Spatial setup from Table 1 of Gonzalez et al., Pramana 100:7 (2026)
const double PAPER_L = 32.0 * M_PI; // domain length
const int PAPER_N = 512;            // number of cells

// Normalised units, chosen so that the plasma frequency is 1 as in Table 1
const double ELECTRON_Q = -1.0;     // electron charge
const double ELECTRON_M = 1.0;      // electron mass
const double N0 = 1.0;              // background electron density
const double EPS0 = 1.0;            // vacuum permittivity

class Grid;

class Particle
{
    double x; // always in [0, L), every write goes through Grid::wrap
    double v;
    public:
        void updatePOS(double dx, const Grid &rf);
        void updateVEL(double dv);

        Particle();
        Particle(double x1, const Grid &rf);
        Particle(double x1, double v1, const Grid &rf);

        double retPos() const;
        double retVel() const;

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
        double retPhi(int i) const { return phi[i]; }
        double retE(int i) const { return E[i]; }

        // x_i, position of node i
        double nodePos(int i) const { return i * h; }
        int index(int i) const;

        // Eq. (10), x mod L. The result is always in [0, L).
        double wrap(double x) const;

        // total is the particle count over all ranks, it sets the charge per particle
        void depositCharge(const vector<Particle> &particles, size_t total);
        void addBackground();
        void deposit(const vector<Particle> &particles);
        void fieldSolve();

        // for threads, MPI reductions and device copies
        void clearRho();
        double chargeWeight(size_t total) const;
        void scatter(double *target, const Particle &p, double w) const;
        double *rhoData() { return rho.data(); }
        const double *EData() const { return E.data(); }

        // eq. (7), E at the particle position
        double interpE(const Particle &p) const;
        double fieldEnergy() const;
};

// kept copyable with memcpy so a vector of particles can go straight to a device or an MPI buffer
static_assert(is_trivially_copyable<Particle>::value, "Particle must stay trivially copyable");

struct vel
{
    float vx;

};
