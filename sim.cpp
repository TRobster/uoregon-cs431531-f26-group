#include "sim.hpp"
#include <stdint.h>

// mass of one macro-particle, so that the macro charge to mass ratio is the electron one
static double macroMass(size_t Np)
{
    return ELECTRON_M * N0 * PAPER_L / Np;
}

// quiet start, inverts the cumulative density x + (A/k)(1 - cos kx) = s with Newton
static double perturbedPos(double s, double A, double k)
{
    double x = s;
    for (int it = 0; it < 50; it++)
    {
        double f = x + (A / k) * (1.0 - cos(k * x)) - s;
        double df = 1.0 + A * sin(k * x);
        double dx = f / df;
        x -= dx;
        if (fabs(dx) < 1e-14) break;
    }
    return x;
}

static uint64_t splitmix(uint64_t z)
{
    z += 0x9e3779b97f4a7c15ULL;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

// standard normal that depends only on the seed and the particle index, so it does not depend on how particles are split
static double gaussian(unsigned seed, uint64_t p)
{
    uint64_t a = splitmix(((uint64_t)seed << 32) ^ (2 * p));
    uint64_t b = splitmix(((uint64_t)seed << 32) ^ (2 * p + 1));
    double u1 = ((a >> 11) + 0.5) / 9007199254740992.0;
    double u2 = (b >> 11) / 9007199254740992.0;
    return sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
}

vector<Particle> initParticles(const Grid &g, const Setup &s, size_t first, size_t last)
{
    size_t Np = (size_t)s.perCell * g.retCells();
    double k = 2.0 * M_PI * s.mode / g.retLength();

    vector<Particle> plasma;
    plasma.reserve(last - first);
    for (size_t p = first; p < last; p++)
    {
        double pos = (p + 0.5) * g.retLength() / Np;
        double x = perturbedPos(pos, s.A, k);
        double v = ((p % 2 == 0) ? s.vb : -s.vb) + s.vth * gaussian(s.seed, p);
        plasma.push_back(Particle(x, v, g));
    }
    return plasma;
}

vector<Particle> initParticles(const Grid &g, const Setup &s)
{
    return initParticles(g, s, 0, (size_t)s.perCell * g.retCells());
}

void pushParticle(const Grid &g, Particle &p)
{
    p.updateVEL(ELECTRON_Q / ELECTRON_M * g.interpE(p) * DT);
    p.updatePOS(p.retVel() * DT, g);
}

void advance(Grid &g, vector<Particle> &plasma)
{
    for (size_t p = 0; p < plasma.size(); p++)
    {
        pushParticle(g, plasma[p]);
    }
    g.deposit(plasma);
    g.fieldSolve();
}

Energy measure(const Grid &g, const vector<Particle> &plasma, size_t total)
{
    double m = macroMass(total);
    double ke = 0.0, mom = 0.0;
    for (size_t p = 0; p < plasma.size(); p++)
    {
        double v = plasma[p].retVel();
        ke += v * v;
        mom += v;
    }
    return { 0.5 * m * ke, g.fieldEnergy(), m * mom };
}
