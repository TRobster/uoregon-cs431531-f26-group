#!/usr/bin/env bash
set -euo pipefail
stamp=$1; target=$2; version=$3; shift 3
cd "$HOME/twostream-runs/$stamp"
echo "$version" > VERSION
module load gcc/13.1.0
module load openmpi/4.1.5
module load cuda/12.4.1
if ! { cmake -B build -DTS_ENABLE_MPI=ON -DTS_ENABLE_GPU=ON && cmake --build build -j4; } > build.log 2>&1; then
  tail -n 40 build.log >&2
  exit 1
fi
sbatch --parsable "$@" "slurm/$target.sbatch"
