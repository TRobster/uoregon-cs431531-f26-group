# Agent notes

1D electrostatic particle-in-cell code for the two-stream instability, in four versions: serial, OpenMP, OpenMP + MPI, CUDA. The equations in the code comments refer to `ReferencePaper.pdf`.

All commands, including the Slurm flags for large runs, are in `Commands.md`.

## Prerequisites

- pixi is the only thing to install. macOS or Linux: `curl -fsSL https://pixi.sh/install.sh | bash`. Windows: `winget install prefix-dev.pixi`.
- Windows also needs the Visual Studio Build Tools C++ workload for local builds, and the OpenSSH Client (on by default in Windows 10 and 11) for Talapas.

## Running code

- Always use `pixi run <task>`, never call `cmake`, `mpirun` or binaries directly. Pixi provides the toolchain.
- Locally only serial and OpenMP run (`pixi run serial`, `pixi run omp`). `hybrid` (MPI) and `gpu` (CUDA) run on Talapas only. Do not try to build them locally.
- On Talapas, use `pixi run remote <serial|omp|hybrid|gpu>` for fast runs and `pixi run batch <target>` for timing or large runs. Both sync the working tree (uncommitted changes included), build, and run. No commit or push is needed.
- Lines starting with `[remote]` come from the tooling. Everything else is the program's own output. Do not treat `[remote]` lines as program results.

## Talapas rules

- It is the user's real cluster account on a shared class allocation. Keep jobs small and short, and ask before submitting anything long, large, or repeated in a loop.
- Needs the UO VPN. If a command says it cannot ssh without a prompt, stop and tell the user to connect the VPN or run `! pixi run remote-setup` themselves. It needs their password and DUO, which you cannot provide.
- `remote` reserves a node on first use and reuses it. Always `pixi run session end` when you are done, an idle node is held for up to 4 hours.
- The state files `.talapas-session` and `.talapas-last` are shared by everyone using this checkout. Run `pixi run session` first. Never end a session you did not start.
- The reserved node is on the preempt partition and can be killed at any time. `remote` then reserves a new one. Take timing numbers only from `batch`.
- `batch` forwards extra flags to Slurm, for example `--nodes 4 --ntasks-per-node 8 --time 02:00:00`. Add `--detach` to return immediately and read the result later with `pixi run results`.
- Do not run real work on the login node. Short builds are fine, the tooling already does this.

## Output and cleanup

- Only `batch` saves results, in `results/<timestamp>/` with a `VERSION` file (git commit, `-dirty` if uncommitted). Only delete result folders you created.
- Batch runs leave `~/twostream-runs/<timestamp>/` on Talapas and sessions leave `~/twostream-session/<job>/`, which `session end` removes. Remove your own when finished.
- Do not commit unless asked.

## Editing the tooling

- Remote tooling is in `scripts/remote.py` (Python standard library only) and the shell scripts `scripts/remote_build.sh` and `scripts/session_run.sh`, which run on Talapas. Windows developers use it too, so do not add `bash`, `rsync` or other Unix-only tools to the local side.
- Job resources for batch runs are in `slurm/<target>.sbatch`. The class Slurm account is `cis431_531`.
- The remote environment (`pixi run -e remote`) is deliberately small, Python and git only. Do not add compilers to it.
- Keep `.sbatch` and `.sh` files LF only. `.gitattributes` enforces this.
