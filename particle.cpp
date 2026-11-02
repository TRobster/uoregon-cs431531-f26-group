#include "particle.hpp"

class Grid
{
    dim Lx, Ly, Lz;   // each carries its own st (origin) and l (length)
    float h;          // uniform spacing, derived or passed in
    int nx, ny, nz;
    // field array 
    vector<coord> E;

    public:
        Grid()
        {
            Lx = Ly = Lz = {0.0f, 0.0f};
            h = 1.0f;
            nx = ny = nz = 0;
        }
        // assuming rectilinear grid, uniform in size
        Grid(dim x, dim y, dim z, float spacing)
        {
            Lx = x; Ly = y; Lz = z;
            h = spacing;
            nx = Lx.l / h;
            ny = Ly.l / h;
            nz = Lz.l / h;
        }

        dim retDimX() { return Lx; }
        dim retDimY() { return Ly; }
        dim retDimZ() { return Lz; }

        int retCellX() { return nx; }
        int retCellY() { return ny; }
        int retCellZ() { return nz; }

        float retSpacing() { return h; }
        
        int index(cell C) {C.i + C.j * nx + C.k * nx * ny; }

};

class Particle
{
    coord pos; 
    //vel v; 
    //float q; // charge
    //float m; // mass
    public:
        void updatePOS(coord pos1);

        Particle()
        {
            pos.x = 0;
            pos.y = 0;
            pos.z = 0;
            //fl vel = 0;
            // fl force = 0;
            // fl dens = 0;
        }
        Particle(coord x1)
        {
            pos.x = x1.x;
            pos.y = x1.y;
            pos.z = x1.z;
        }

        coord returnCart()
        {
            return this->pos;
        }

        void gridPlacement(Grid rf);

        cell cellIndex(Grid rf);

};


void Particle::gridPlacement(Grid rf)
{   
    dim X, Y, Z;
    X = rf.retDimX();
    Y = rf.retDimY();
    Z = rf.retDimZ();
    this->pos.x = X.st + this->pos.x;
    this->pos.y = Y.st + this->pos.y;
    this->pos.z = Z.st + this->pos.z;

}

// Which cell of rf is this particle sitting in?
// Distance from the grid origin, measured in cells: (pos - st) / h, rounded down.
// k is the z-slice, so a 2D slice is every particle sharing one k.
cell Particle::cellIndex(Grid rf)
{
    float h = rf.retSpacing();

    cell c;
    c.i = floor((this->pos.x - rf.retDimX().st) / h);
    c.j = floor((this->pos.y - rf.retDimY().st) / h);
    c.k = floor((this->pos.z - rf.retDimZ().st) / h);

    return c;
}

void Particle::updatePOS(coord pos1)
{
    this->pos.x += pos1.x;
    this->pos.y += pos1.y;
    this->pos.z += pos1.z;
}

int main()
{
    coord begin = {10.0, 10.0, 10.0};

    Particle test(begin);

    coord end = test.returnCart();
    cout << end.x << endl;

    dim up = {0.0, 10.0};

    Grid gnarl(up, up, up, 2);

    cout << gnarl.retCellX() << endl;

    // cell of c(n)
    cell here = test.cellIndex(gnarl);
    cout << here.i << " " << here.j << " " << here.k << endl;
}