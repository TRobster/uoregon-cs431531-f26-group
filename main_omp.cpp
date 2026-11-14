#include "particle.hpp"
#include <omp.h>

int main()
{
    cout << "omp threads " << omp_get_max_threads() << endl;
}
