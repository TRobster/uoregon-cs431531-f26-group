#!/bin/bash
#SBATCH --account=cis431_531
#SBATCH --partition=compute
#SBATCH --job-name=ts_omp
#SBATCH --output=slurm-%j.out
#SBATCH --time=0-00:10:00
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=8

module load gcc/13.1.0
export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK

srun ./build/ts_omp
