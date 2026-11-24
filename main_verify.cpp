#include "sim.hpp"
#include <iomanip>
#include <sstream>

static int failures = 0;

static void check(const string &name, bool ok, const string &detail)
{
    cout << (ok ? "PASS " : "FAIL ") << name << ": " << detail << endl;
    if (!ok) failures++;
}

static vector<Particle> start(Grid &g, const Setup &s)
{
    vector<Particle> plasma = initParticles(g, s);
    g.deposit(plasma);
    g.fieldSolve();
    return plasma;
}

// electrons at rest, only the density modulation of eq. (13)
static Setup coldSetup(double A, int mode)
{
    Setup s;
    s.A = A;
    s.mode = mode;
    s.vb = 0.0;
    s.vth = 0.0;
    return s;
}

// nearly cold beams with a small seed in one mode
static Setup seededBeams(int mode)
{
    Setup s;
    s.A = 1e-3;
    s.mode = mode;
    s.vth = 0.01;
    return s;
}

// a neutral plasma has no charge and no field
static void neutralPlasma()
{
    Grid g(PAPER_L, PAPER_N);
    start(g, coldSetup(0.0, 1));

    double total = 0.0, maxE = 0.0;
    for (int i = 0; i < g.retCells(); i++)
    {
        total += g.retRho(i);
        maxE = max(maxE, fabs(g.retE(i)));
    }
    check("neutral plasma", fabs(total) < 1e-9 && maxE < 1e-9,
          "sum rho " + to_string(total) + ", max|E| " + to_string(maxE));
}

// density 1 + A sin(kx) has E = (A/k) cos(kx) exactly, error must fall as h^2
static double poissonError(int cells)
{
    Grid g(PAPER_L, cells);
    Setup s = coldSetup(0.05, 2);
    start(g, s);

    double k = 2.0 * M_PI * s.mode / g.retLength();
    double err = 0.0;
    for (int i = 0; i < cells; i++)
    {
        err = max(err, fabs(g.retE(i) - (s.A / k) * cos(k * g.nodePos(i))));
    }
    return err / (s.A / k);
}

static void poissonConvergence()
{
    int sizes[] = { 64, 128, 256, 512 };
    double prev = 0.0;
    bool ok = true;
    ostringstream detail;
    for (int n : sizes)
    {
        double err = poissonError(n);
        detail << "N=" << n << " err " << scientific << setprecision(2) << err;
        if (prev > 0.0)
        {
            double ratio = prev / err;
            detail << " (ratio " << fixed << setprecision(2) << ratio << ")";
            if (ratio < 3.5 || ratio > 4.5) ok = false;
        }
        detail << "  ";
        prev = err;
    }
    check("deposit + Poisson converge as h^2", ok, detail.str());
}

// eq. (7) is exact at nodes and gives the average at midpoints
static void interpolation()
{
    Grid g(PAPER_L, PAPER_N);
    start(g, coldSetup(0.3, 3));

    double worst = 0.0;
    for (int i = 0; i < g.retCells(); i++)
    {
        Particle atNode(g.nodePos(i), g);
        Particle midway(g.nodePos(i) + 0.5 * g.retSpacing(), g);
        double mid = 0.5 * (g.retE(i) + g.retE(g.index(i + 1)));
        worst = max(worst, fabs(g.interpE(atNode) - g.retE(i)));
        worst = max(worst, fabs(g.interpE(midway) - mid));
    }
    check("E interpolation, eq. (7)", worst < 1e-9, "worst error " + to_string(worst));
}

// particles built and deposited in slices must match the whole set, as with MPI ranks or OpenMP threads
static void splitMatchesWhole()
{
    Grid g(PAPER_L, PAPER_N);
    Setup s;
    s.A = 0.1;
    vector<Particle> whole = initParticles(g, s);
    size_t total = whole.size();
    size_t cuts[] = { 0, 1000, 20001, 40000, total };

    vector<double> summed(g.retCells(), 0.0);
    double w = g.chargeWeight(total);
    double worstX = 0.0;
    for (int part = 0; part < 4; part++)
    {
        vector<Particle> slice = initParticles(g, s, cuts[part], cuts[part + 1]);
        for (size_t p = 0; p < slice.size(); p++)
        {
            const Particle &ref = whole[cuts[part] + p];
            worstX = max(worstX, fabs(slice[p].retPos() - ref.retPos()) + fabs(slice[p].retVel() - ref.retVel()));
            g.scatter(summed.data(), slice[p], w);
        }
    }

    g.depositCharge(whole, total);
    double worstRho = 0.0;
    for (int i = 0; i < g.retCells(); i++)
    {
        worstRho = max(worstRho, fabs(summed[i] - g.retRho(i)));
    }
    check("sliced init and deposit match the whole", worstX == 0.0 && worstRho < 1e-12,
          "particle diff " + to_string(worstX) + ", rho diff " + to_string(worstRho));
}

// complex amplitude of Fourier mode m of E
static double modeAmp(const Grid &g, int m)
{
    double k = 2.0 * M_PI * m / g.retLength();
    double c = 0.0, s = 0.0;
    for (int i = 0; i < g.retCells(); i++)
    {
        c += g.retE(i) * cos(k * g.nodePos(i));
        s += g.retE(i) * sin(k * g.nodePos(i));
    }
    return 2.0 * sqrt(c * c + s * s) / g.retCells();
}

// a cold plasma must ring at omega_p = 1, period 2 pi
static void plasmaOscillation()
{
    Grid g(PAPER_L, PAPER_N);
    int mode = 4;
    double k = 2.0 * M_PI * mode / g.retLength();
    double delta = 0.01;
    size_t Np = 100 * (size_t)g.retCells();

    vector<Particle> plasma;
    for (size_t p = 0; p < Np; p++)
    {
        double x0 = (p + 0.5) * g.retLength() / Np;
        plasma.push_back(Particle(x0 + delta * sin(k * x0), g));
    }
    g.deposit(plasma);
    g.fieldSolve();

    // projection on sin(kx) changes sign every half period
    auto sinAmp = [&]() {
        double s = 0.0;
        for (int i = 0; i < g.retCells(); i++) s += g.retE(i) * sin(k * g.nodePos(i));
        return 2.0 * s / g.retCells();
    };

    vector<double> crossings;
    double prev = sinAmp(), peak0 = fabs(prev), peakMax = 0.0;
    int steps = (int)round(60.0 / DT);
    for (int step = 1; step <= steps; step++)
    {
        advance(g, plasma);
        double cur = sinAmp();
        peakMax = max(peakMax, fabs(cur));
        if (prev * cur < 0.0)
        {
            crossings.push_back((step - 1 + prev / (prev - cur)) * DT);
        }
        prev = cur;
    }

    double period = 2.0 * (crossings.back() - crossings.front()) / (crossings.size() - 1);
    double omega = 2.0 * M_PI / period;
    check("cold plasma oscillation at omega_p = 1", crossings.size() >= 15 && fabs(omega - 1.0) < 0.01,
          "omega " + to_string(omega) + " from " + to_string(crossings.size()) + " zero crossings");
    check("oscillation amplitude stays constant", fabs(peakMax / peak0 - 1.0) < 0.02,
          "peak ratio " + to_string(peakMax / peak0));
}

// total energy and momentum over the full paper run
static void conservation()
{
    double amplitudes[] = { 0.0, 0.1, 0.5 };
    for (double A : amplitudes)
    {
        Setup s;
        s.A = A;
        Grid g(PAPER_L, PAPER_N);
        vector<Particle> plasma = start(g, s);

        Energy e0 = measure(g, plasma, plasma.size());
        double E0 = e0.kinetic + e0.field;
        double drift = 0.0, momDrift = 0.0;
        for (int step = 1; step <= 2000; step++)
        {
            advance(g, plasma);
            Energy e = measure(g, plasma, plasma.size());
            drift = max(drift, fabs(e.kinetic + e.field - E0) / E0);
            momDrift = max(momDrift, fabs(e.momentum - e0.momentum));
        }
        check("conservation A=" + to_string(A).substr(0, 3), drift < 0.005 && momDrift < 1e-8,
              "energy drift " + to_string(drift) + ", momentum drift " + to_string(momDrift));
    }
}

// unstable root for cold symmetric beams with total omega_p = 1
static double coldGrowth(double k, double vb)
{
    double u = k * k * vb * vb;
    double w2 = u + 0.5 - sqrt(2.0 * u + 0.25);
    return w2 < 0.0 ? sqrt(-w2) : 0.0;
}

// linear growth rate of one mode against the cold two-stream dispersion relation
static void growthRate(int mode)
{
    Setup s = seededBeams(mode);
    Grid g(PAPER_L, PAPER_N);
    vector<Particle> plasma = start(g, s);

    vector<double> t, amp;
    for (int step = 0; step <= 2000; step++)
    {
        if (step > 0) advance(g, plasma);
        t.push_back(step * DT);
        amp.push_back(modeAmp(g, mode));
    }

    // slope of ln(amp) over the last stretch above the noise, up to the first crossing of 0.1
    double sx = 0, sy = 0, sxx = 0, sxy = 0;
    int cnt = 0;
    for (size_t i = 0; i < t.size(); i++)
    {
        if (amp[i] > 0.1) break;
        if (amp[i] < 0.01) { sx = sy = sxx = sxy = 0; cnt = 0; continue; }
        double y = log(amp[i]);
        sx += t[i]; sy += y; sxx += t[i] * t[i]; sxy += t[i] * y;
        cnt++;
    }
    double k = 2.0 * M_PI * mode / g.retLength();
    double expect = coldGrowth(k, s.vb);
    double slope = (cnt * sxy - sx * sy) / (cnt * sxx - sx * sx);
    check("growth rate mode " + to_string(mode), cnt >= 10 && fabs(slope - expect) < 0.1 * expect,
          "measured " + to_string(slope) + ", cold theory " + to_string(expect) + ", " + to_string(cnt) + " samples");
}

// modes with k * vb > omega_p are stable, so the seed must not grow
static void stableMode(int mode)
{
    Setup s = seededBeams(mode);
    Grid g(PAPER_L, PAPER_N);
    vector<Particle> plasma = start(g, s);

    double peak = 0.0;
    for (int step = 1; step <= 800; step++)
    {
        advance(g, plasma);
        peak = max(peak, modeAmp(g, mode));
    }
    check("stable mode " + to_string(mode) + " does not grow", peak < 0.01, "peak amplitude " + to_string(peak));
}

int main()
{
    neutralPlasma();
    poissonConvergence();
    interpolation();
    splitMatchesWhole();
    plasmaOscillation();
    conservation();
    int modes[] = { 6, 8, 10, 12, 14 };
    for (int m : modes) growthRate(m);
    stableMode(18);
    cout << (failures ? "FAILED " : "all passed ") << failures << endl;
    return failures ? 1 : 0;
}
