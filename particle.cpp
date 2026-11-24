#include "particle.hpp"

// 1D periodic mesh over x in [0, L). There are n cells of size h = L / n.
// Cell i spans [x_i, x_i+1) where x_i = i * h. Node n is the same point as node 0.


Particle::Particle()
    {
        x = 0;
        v = 0;
    }

Particle::Particle(double x1, const Grid &rf)
    {
        x = rf.wrap(x1);
        v = 0;
    }

Particle::Particle(double x1, double v1, const Grid &rf)
    {
        x = rf.wrap(x1);
        v = v1;
    }

// Which cell of rf is this particle sitting in?
// Distance from the origin measured in cells: x / h, rounded down.
int Particle::cellIndex(const Grid &rf) const
    {
        int i = floor(this->x / rf.retSpacing());

        // x just below L can round up to cell n, which doesn't exist
        if (i >= rf.retCells()) i = rf.retCells() - 1;
        return i;
    }

// Eqs. (9) and (10), move by dx then wrap back into the domain.
void Particle::updatePOS(double dx, const Grid &rf)
    {
        this->x = rf.wrap(this->x + dx);
    }

void Particle::updateVEL(double dv)
    {
        this->v += dv;
    }

double Particle::retPos() const
    {
        return this->x;
    }

double Particle::retVel() const
    {
        return this->v;
    }

double Grid::wrap(double x) const
    {
        x = fmod(x, L);
        if (x < 0.0) x += L;   // fmod keeps the sign of x
        if (x >= L) x = 0.0;   // a tiny negative x can round up to exactly L
        return x;
    }

int Grid::index(int i) const 
    { 
    return (i % n + n) % n; 
    }
    
Grid::Grid(double length, int cells)
    {
        L = length;
        n = cells;
        h = L / n;
        E.assign(n, 0.0);
        phi.assign(n, 0.0);
        rho.assign(n, 0.0);
    }

// Eq. (4), rebuild rho from the particles plus the fixed ion background.
// Each particle splits its charge between its two nearest nodes, using the
// same linear weights as eq. (7), so the nearer node gets the larger share.
void Grid::clearRho()
    {
        rho.assign(n, 0.0);
    }

// density one particle adds to a node if it gives that node all its charge
double Grid::chargeWeight(size_t total) const
    {
        return ELECTRON_Q * N0 * L / (total * h);
    }

void Grid::scatter(double *target, const Particle &p, double w) const
    {
        int i = p.cellIndex(*this);
        double f = (p.retPos() - nodePos(i)) / h;

        target[i] += w * (1.0 - f);
        target[index(i + 1)] += w * f;
    }

void Grid::depositCharge(const vector<Particle> &particles, size_t total)
    {
        double w = chargeWeight(total);

        clearRho();
        for (size_t p = 0; p < particles.size(); p++)
        {
            scatter(rho.data(), particles[p], w);
        }
    }

void Grid::addBackground()
    {
        for (int i = 0; i < n; i++)
        {
            rho[i] -= ELECTRON_Q * N0;
        }
    }

void Grid::deposit(const vector<Particle> &particles)
    {
        depositCharge(particles, particles.size());
        addBackground();
    }

// Eq. (11) for phi, then eq. (12) for E. Call deposit first.
//
// Periodic phi is only defined up to a constant, so we pin phi[0] = 0. Node n
// is node 0, so phi[n] = 0 too, and nodes 1..n-1 become an ordinary
// tridiagonal system (1, -2, 1) solved with the Thomas algorithm. The i = 0
// equation is left out, which is safe because sum(rho) = 0 makes it redundant.
void Grid::fieldSolve()
    {
        vector<double> c(n, 0.0); // Thomas scratch, the modified upper diagonal

        // forward sweep, phi[i] temporarily holds the modified right-hand side
        phi[0] = 0.0;
        for (int i = 1; i < n; i++)
        {
            double d = -h * h * rho[i] / EPS0;
            double m = -2.0 - c[i - 1];

            c[i] = 1.0 / m;
            phi[i] = (d - phi[i - 1]) / m;
        }

        // back substitution, phi[n - 1] is already solved since phi[n] = 0
        for (int i = n - 2; i >= 1; i--)
        {
            phi[i] -= c[i] * phi[i + 1];
        }

        for (int i = 0; i < n; i++)
        {
            E[i] = -(phi[index(i + 1)] - phi[index(i - 1)]) / (2.0 * h);
        }
    }

double Grid::interpE(const Particle &p) const
    {
        int i = p.cellIndex(*this);
        double f = (p.retPos() - nodePos(i)) / h;
        return E[i] + f * (E[index(i + 1)] - E[i]);
    }

double Grid::fieldEnergy() const
    {
        double sum = 0.0;
        for (int i = 0; i < n; i++)
        {
            sum += E[i] * E[i];
        }
        return 0.5 * EPS0 * sum * h;
    }
