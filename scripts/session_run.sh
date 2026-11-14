#!/usr/bin/env bash
set -euo pipefail
job=$1; target=$2; gpu=$3
cd "$HOME/twostream-session/$job/src"
flags=(-DTS_ENABLE_MPI=ON)
[ "$gpu" = 1 ] && flags+=(-DTS_ENABLE_GPU=ON)

# loading modules is slow, so the environment is saved once per session
env_cache="$HOME/twostream-session/$job/env.sh"
if [ -f "$env_cache" ]; then
  . "$env_cache"
else
  set +eu
  . /etc/profile
  module load gcc/13.1.0 openmpi/4.1.5
  [ "$gpu" = 1 ] && module load cuda/12.4.1
  set -eu
  export -p | grep -v -E '^declare -x (SSH_[A-Z_]*|LC_[A-Z_]*|LANG|LANGUAGE|PWD|OLDPWD|SHLVL|_)=' > "$env_cache"
fi
step() { srun --jobid="$job" --overlap --input=none "$@"; }

if ! step -n1 -c8 bash -c 'cmake -B build "$@" && cmake --build build -j8' _ "${flags[@]}" > build.log 2>&1; then
  echo "[remote] build failed:" >&2
  tail -n 40 build.log >&2
  exit 1
fi

case $target in
  serial) step -n1 -c1 ./build/ts_serial ;;
  omp)    export OMP_NUM_THREADS=8; step -n1 -c8 ./build/ts_omp ;;
  hybrid) export OMP_NUM_THREADS=4; step -n2 -c4 ./build/ts_hybrid ;;
  gpu)    step -n1 -c1 --gpus=1 ./build/ts_gpu ;;
esac
