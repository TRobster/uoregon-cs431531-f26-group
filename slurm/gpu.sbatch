#!/bin/bash
#SBATCH --account=cis431_531
#SBATCH --partition=gpu
#SBATCH --job-name=ts_gpu
#SBATCH --output=slurm-%j.out
#SBATCH --time=0-00:10:00
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --gpus=1

module load gcc/13.1.0 cuda/12.4.1

srun ./build/ts_gpu
