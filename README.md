# Implementation of two-stream instability

Heres a step by step guide so you can easily run the project.

## Setup

Install pixi:

- Windows: `winget install prefix-dev.pixi`
- macOS or Linux: `curl -fsSL https://pixi.sh/install.sh | bash`

For first time setup you need to run: `pixi run remote-setup` which will asks for your DuckID, password and DUO, this is so when you run remote jobs later they dont have to ask again.
I promise it just makes sure you have an ssh key setup like the talapas guide already asks.

## Run locally
Only accepts serial and omp since its your own machine and I felt like limiting it to that but is the fastest option!

```
pixi run serial
pixi run omp
```

## Run on Talapas

Connect to the UO VPN.
You should use `pixi run remote *` requests a partition the first time used every session so that future calls are very fast. 

```
pixi run remote <serial|omp|hybrid|gpu>    # fast runs, about 3 seconds (reccomended for developing)
pixi run batch <serial|omp|hybrid|gpu>     # large runs, submits actual jobs
pixi run session end                       # when you are done for the day (will close automatically after 4 hours)
```

The lines you see with `[remote]` in the front come from this tooling, ignore it.
When you run a batch job the output will be saved to a results/ folder.

All commands are listed in [Commands.md](Commands.md).

## Troubleshooting

`cannot ssh ... without a prompt`: the VPN is off or `pixi run remote-setup` has not been run.
