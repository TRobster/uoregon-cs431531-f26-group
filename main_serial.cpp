#include "particle.hpp"

int main()
{
    Grid gnarl(PAPER_L, PAPER_N);

    cout << gnarl.retLength() << " " << gnarl.retCells() << " " << gnarl.retSpacing() << endl;

    Particle test(10.0, gnarl);
    cout << test.retPos() << " is in cell " << test.cellIndex(gnarl) << endl;

    // leaving through the right edge comes back in on the left
    test.updatePOS(PAPER_L - 9.0, gnarl);
    cout << test.retPos() << " is in cell " << test.cellIndex(gnarl) << endl;

    // evenly spaced electrons should cancel the ion background
    int Np = 100 * PAPER_N;
    vector<Particle> plasma;
    for (int p = 0; p < Np; p++)
    {
        plasma.push_back(Particle((p + 0.5) * PAPER_L / Np, gnarl));
    }
    gnarl.deposit(plasma);

    double total = 0.0;
    for (int i = 0; i < PAPER_N; i++)
    {
        total += gnarl.retRho(i);
    }
    cout << "total charge density " << total << endl;

    // a neutral plasma should have no field
    gnarl.fieldSolve();

    double maxE = 0.0;
    for (int i = 0; i < PAPER_N; i++)
    {
        maxE = max(maxE, fabs(gnarl.retE(i)));
    }
    cout << "largest |E| " << maxE << endl;
}
