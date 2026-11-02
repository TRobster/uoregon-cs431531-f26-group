#include "particle.hpp"

// 1D periodic mesh over x in [0, L). There are n cells of size h = L / n.
// Cell i spans [x_i, x_i+1) where x_i = i * h. Node n is the same point as node 0.


Particle::Particle()
    {
        x = 0;
        // fl vel = 0;
        // fl force = 0;
        // fl dens = 0;
    }

Particle::Particle(double x1, const Grid &rf)
    {
        x = rf.wrap(x1);
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

double Particle::retPos() const
    {
        return this->x;
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
    }

int main()
{
    Grid gnarl(PAPER_L, PAPER_N);

    cout << gnarl.retLength() << " " << gnarl.retCells() << " " << gnarl.retSpacing() << endl;

    Particle test(10.0, gnarl);
    cout << test.retPos() << " is in cell " << test.cellIndex(gnarl) << endl;

    // leaving through the right edge comes back in on the left
    test.updatePOS(PAPER_L - 9.0, gnarl);
    cout << test.retPos() << " is in cell " << test.cellIndex(gnarl) << endl;
}
