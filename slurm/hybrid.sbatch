#!/bin/bash
#SBATCH --account=cis431_531
#SBATCH --partition=compute
#SBATCH --job-name=ts_hybrid
#SBATCH --output=slurm-%j.out
#SBATCH --time=0-00:10:00
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=2
#SBATCH --cpus-per-task=4

module load gcc/13.1.0 openmpi/4.1.5
export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK

srun ./build/ts_hybrid
