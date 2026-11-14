#include "particle.hpp"
#include <mpi.h>
#include <omp.h>

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    cout << "rank " << rank << " of " << size << ", omp threads " << omp_get_max_threads() << endl;

    MPI_Finalize();
}
