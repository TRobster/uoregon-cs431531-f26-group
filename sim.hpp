#pragma once
#include "particle.hpp"

// Table 1 of the paper
const double DT = 0.025;

// the paper does not give these, chosen so v_max * dt stays well below h
struct Setup
{
    double A = 0.0;         // perturbation amplitude in eq. (13)
    int mode = 16;          // k = 2 pi mode / L in eq. (13)
    double vb = 1.0;        // beam speed, beams are at +vb and -vb
    double vth = 0.2;       // thermal velocity of each beam
    int perCell = 100;      // macro-particles per cell
    unsigned seed = 431;
};

struct Energy
{
    double kinetic;
    double field;
    double momentum;
};

// eq. (5) with the density modulation of eq. (13)
// builds particles first to last-1 of the full set, so ranks can each build their own slice
vector<Particle> initParticles(const Grid &g, const Setup &s, size_t first, size_t last);
vector<Particle> initParticles(const Grid &g, const Setup &s);

// eqs. (7) to (10) for one particle, with the current E
void pushParticle(const Grid &g, Particle &p);

// push every particle, then eqs. (4), (11), (12) for the new E
void advance(Grid &g, vector<Particle> &plasma);

// kinetic and momentum are sums over the given particles only, total is the count over all ranks
Energy measure(const Grid &g, const vector<Particle> &plasma, size_t total);
