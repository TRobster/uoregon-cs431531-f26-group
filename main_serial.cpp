#include "sim.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>

const double T_END = 50.0;
const int PRINT_EVERY = 200;

string tag(double A, int step)
{
    ostringstream s;
    s << "A" << fixed << setprecision(1) << A << "_t" << setprecision(0) << step * DT;
    return s.str();
}

void writePhase(const string &name, const vector<Particle> &plasma)
{
    ofstream f(name);
    f << "x,v\n";
    for (size_t p = 0; p < plasma.size(); p++)
    {
        f << plasma[p].retPos() << "," << plasma[p].retVel() << "\n";
    }
}

void writeField(const string &name, const Grid &g)
{
    ofstream f(name);
    f << "x,E,phi,rho\n";
    for (int i = 0; i < g.retCells(); i++)
    {
        f << g.nodePos(i) << "," << g.retE(i) << "," << g.retPhi(i) << "," << g.retRho(i) << "\n";
    }
}

void run(const Setup &setup)
{
    double A = setup.A;
    Grid gnarl(PAPER_L, PAPER_N);
    vector<Particle> plasma = initParticles(gnarl, setup);

    int steps = (int)round(T_END / DT);
    int snapA = (int)round(25.0 / DT);
    int fieldStep = (int)round(10.0 / DT);

    cout << "A = " << A << ", " << plasma.size() << " particles, " << steps << " steps" << endl;
    cout << "step       t         KE           FE           total        momentum     max|E|" << endl;

    gnarl.deposit(plasma);
    gnarl.fieldSolve();
    writePhase("phase_" + tag(A, 0) + ".csv", plasma);

    Energy e0 = measure(gnarl, plasma, plasma.size());
    double worstDrift = 0.0;

    for (int step = 1; step <= steps; step++)
    {
        advance(gnarl, plasma);

        Energy e = measure(gnarl, plasma, plasma.size());
        double total = e.kinetic + e.field;
        worstDrift = max(worstDrift, fabs(total - (e0.kinetic + e0.field)) / (e0.kinetic + e0.field));

        if (step % PRINT_EVERY == 0 || step == steps)
        {
            double maxE = 0.0;
            for (int i = 0; i < PAPER_N; i++)
            {
                maxE = max(maxE, fabs(gnarl.retE(i)));
            }
            cout << setw(5) << step << setw(8) << step * DT << setw(13) << e.kinetic << setw(13) << e.field
                 << setw(13) << total << setw(13) << e.momentum << setw(13) << maxE << endl;
        }

        if (step == fieldStep) writeField("field_" + tag(A, step) + ".csv", gnarl);
        if (step == snapA || step == steps) writePhase("phase_" + tag(A, step) + ".csv", plasma);
    }

    cout << "worst relative energy drift " << worstDrift << "\n" << endl;
}

int main()
{
    double amplitudes[] = { 0.0, 0.1, 0.5 };
    for (double A : amplitudes)
    {
        Setup setup;
        setup.A = A;
        run(setup);
    }
}
