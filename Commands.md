# Commands

Run everything from the project root as `pixi run <task>`.

## Local (serial and OpenMP)

| Command | What it does |
| --- | --- |
| `pixi run build` | configure and build `ts_serial` and `ts_omp` into `build/` |
| `pixi run serial` | build if needed, run the serial version |
| `pixi run omp` | build if needed, run OpenMP with 4 threads |
| `pixi run hybrid` | refuses, tells you to use `remote hybrid` |
| `pixi run gpu` | refuses, tells you to use `remote gpu` |

## Talapas

Connect to the UO VPN. Setup is once per developer.

| Command | What it does |
| --- | --- |
| `pixi run remote-setup` | create the SSH key, add the `~/.ssh/config` entry, install the key (needs password and DUO). Does nothing if already set up. |
| `pixi run remote <serial\|omp\|hybrid\|gpu>` | fast run, about 3 seconds. Reserves a node on first use, then reuses it. Output streams to your terminal and nothing is saved. |
| `pixi run batch <serial\|omp\|hybrid\|gpu> [flags]` | normal Slurm job. Waits, prints the output, and saves it in `results/<timestamp>/` with a `VERSION` file. |
| `pixi run results` | state of the last batch job, and its output once finished |
| `pixi run session` | state of your reserved node |
| `pixi run session end` | release the reserved node |

## Large runs

Use `batch`. Any flag you add is passed straight to Slurm (`sbatch`), overriding the defaults in `slurm/<target>.sbatch`. Add `--detach` to submit and return immediately, then check with `pixi run results`.

```
pixi run batch hybrid --nodes 4 --ntasks-per-node 8 --cpus-per-task 4 --time 02:00:00
pixi run batch hybrid --nodes 8 --ntasks-per-node 16 --partition computelong --time 3-00:00:00 --detach
pixi run batch omp --cpus-per-task 32 --exclusive
```

- `--nodes`, `--ntasks-per-node` (MPI ranks per node), `--cpus-per-task` (OpenMP threads per rank), `--time` (`D-HH:MM:SS`), `--partition` (`compute` is 1 day, `computelong` is 14 days), `--exclusive` (whole nodes, no neighbors, use it for timing).
- OpenMP thread count is set from `--cpus-per-task` automatically.
- `ts_gpu` currently uses a single GPU. More GPUs need MPI + CUDA code first.
- The hybrid defaults are 1 node, 2 ranks, 4 threads, 10 minutes. Raise `--time` for anything real, jobs are killed at the limit.

## Notes

- Lines starting with `[remote]` come from the tooling. Everything else is the program's own output.
- `remote gpu` reserves a GPU node. If your node has no GPU it is replaced with one.
- The reserved node is on the preempt partition, lasts 4 hours, and can be taken away. `remote` then reserves a new one. Do not take timing numbers from it, use `batch`.
- `TALAPAS_HOST` overrides the login host.
