#!/usr/bin/env python3
import argparse
import io
import os
import re
import shlex
import subprocess
import sys
import tarfile
import tempfile
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
HOST = os.environ.get("TALAPAS_HOST", "login.talapas.uoregon.edu")
STATE = ROOT / ".talapas-last"
SESSION = ROOT / ".talapas-session"
ACCOUNT = "cis431_531"
SSH_OPTS = ["-o", "BatchMode=yes", "-o", "ConnectTimeout=15", "-o", "LogLevel=ERROR"]
TERMINAL = {"COMPLETED", "FAILED", "CANCELLED", "TIMEOUT", "OUT_OF_MEMORY",
            "NODE_FAIL", "PREEMPTED", "BOOT_FAIL", "DEADLINE"}
LF_ONLY = {".sh", ".sbatch"}
TARGETS = ("serial", "omp", "hybrid", "gpu")


def say(msg, **kw):
    print(f"[remote] {msg}", **kw)


def die(msg):
    print(f"[remote] error: {msg}", file=sys.stderr)
    sys.exit(1)


def run(cmd, **kw):
    try:
        return subprocess.run(cmd, **kw)
    except FileNotFoundError:
        die(f"'{cmd[0]}' not found. Install it (Windows: Settings > Optional features > OpenSSH Client)")


def ssh(*remote_cmd, stdin_bytes=None, stdin_file=None):
    kw = {"capture_output": True}
    if stdin_bytes is not None:
        kw["input"] = stdin_bytes
    if stdin_file is not None:
        kw["stdin"] = stdin_file
    return run(["ssh", *SSH_OPTS, HOST, *remote_cmd], **kw)


def check_connection():
    if ssh("true").returncode != 0:
        die(f"cannot ssh to {HOST} without a prompt. Connect to the UO VPN, then run: pixi run remote-setup")


def git(*args):
    r = run(["git", *args], cwd=ROOT, capture_output=True)
    if r.returncode != 0:
        die(f"git {' '.join(args)} failed: {r.stderr.decode().strip()}")
    return r.stdout


def git_version():
    version = git("rev-parse", "--short", "HEAD").decode().strip()
    return version + "-dirty" if git("status", "--porcelain").strip() else version


def lf_script(name):
    return (ROOT / "scripts" / name).read_bytes().replace(b"\r\n", b"\n")


def make_archive():
    names = git("ls-files", "-z", "--cached", "--others", "--exclude-standard").decode().split("\0")
    tmp = tempfile.TemporaryFile()
    with tarfile.open(fileobj=tmp, mode="w:gz") as tar:
        for name in filter(None, names):
            path = ROOT / name
            if not path.is_file():
                continue
            if path.suffix in LF_ONLY:
                data = path.read_bytes().replace(b"\r\n", b"\n")
                info = tar.gettarinfo(str(path), arcname=name)
                info.size = len(data)
                tar.addfile(info, io.BytesIO(data))
            else:
                tar.add(str(path), arcname=name, recursive=False)
    tmp.seek(0)
    return tmp


def job_state(job):
    out = ssh(f"sacct -j {job} -X -n -P -o State").stdout.decode().strip().splitlines()
    return out[-1].split()[0] if out else ""


def wait_for_job(job):
    last = ""
    while True:
        state = job_state(job)
        if state and state != last:
            say(f"job {job}: {state}", flush=True)
            last = state
        if state in TERMINAL:
            return state
        time.sleep(10)


def show_output(stamp, job):
    dest = ROOT / "results" / stamp
    dest.mkdir(parents=True, exist_ok=True)
    files = "VERSION build.log slurm-*.out output"
    r = ssh(f"cd twostream-runs/{stamp} && tar czf - $(ls -d {files} 2>/dev/null)")
    if r.returncode != 0:
        die(f"could not fetch results: {r.stderr.decode().strip()}")
    with tarfile.open(fileobj=io.BytesIO(r.stdout), mode="r:gz") as tar:
        try:
            tar.extractall(dest, filter="data")
        except TypeError:
            tar.extractall(dest)
    say(f"results in results/{stamp}/")
    say("program output:")
    out = dest / f"slurm-{job}.out"
    print(out.read_text() if out.exists() else "(no output file)")


def read_session():
    job, gpu = SESSION.read_text().split()
    return job, gpu == "1"


def end_session():
    job, _ = read_session()
    ssh(f"scancel {job}; rm -rf twostream-session/{job}")
    SESSION.unlink()
    return job


def start_session(gpu):
    check_connection()
    cmd = ["salloc", "--no-shell", "--immediate=300", "--job-name=ts_session",
           f"--account={ACCOUNT}", "--partition=preempt", "--time=04:00:00",
           "--nodes=1", "--ntasks=1", "--cpus-per-task=8"]
    if gpu:
        cmd.append("--gpus=1")
    say("reserving a node" + (" with a GPU" if gpu else "") + ", this can take a minute the first time", flush=True)
    r = ssh(*cmd)
    m = re.search(r"job allocation (\d+)", r.stdout.decode() + r.stderr.decode())
    if not m:
        die(f"could not get a node: {r.stderr.decode().strip()}. Try: pixi run batch <target>")
    SESSION.write_text(f"{m.group(1)} {1 if gpu else 0}\n")
    return m.group(1)


def run_in_session(target, job, gpu):
    r = ssh(f"squeue -j {job} -h -o %T | grep -qx RUNNING || exit 3; "
            f"mkdir -p twostream-session/{job}/src && tar xzf - -C twostream-session/{job}/src",
            stdin_file=make_archive())
    if r.returncode == 3:
        SESSION.unlink()
        return False
    if r.returncode != 0:
        die(f"sync failed: {r.stderr.decode().strip()}. Is the UO VPN connected?")

    r = run(["ssh", *SSH_OPTS, HOST, "bash", "-s", "--", job, target, "1" if gpu else "0"],
            input=lf_script("session_run.sh"))
    if r.returncode != 0:
        die(f"run failed (exit {r.returncode})")
    return True


def cmd_remote(target):
    gpu = target == "gpu"
    if SESSION.exists():
        job, has_gpu = read_session()
        if gpu and not has_gpu:
            say("current session has no GPU, replacing it")
            end_session()
        elif run_in_session(target, job, has_gpu):
            return
        else:
            say("your session ended (expired or preempted), starting a new one")
    run_in_session(target, start_session(gpu), gpu)


def cmd_batch(target, detach, extra):
    check_connection()
    stamp = time.strftime("%Y%m%d-%H%M%S")
    version = git_version()

    say(f"syncing to {HOST}:twostream-runs/{stamp} ({version})", flush=True)
    r = ssh(f"mkdir -p twostream-runs/{stamp} && tar xzf - -C twostream-runs/{stamp}", stdin_file=make_archive())
    if r.returncode != 0:
        die(f"sync failed: {r.stderr.decode().strip()}")

    say(f"building and submitting {target}", flush=True)
    r = ssh("bash", "-l", "-s", "--", stamp, target, version, *map(shlex.quote, extra),
            stdin_bytes=lf_script("remote_build.sh"))
    if r.returncode != 0:
        say("build output:", file=sys.stderr)
        print(r.stderr.decode(), file=sys.stderr)
        die("remote build or submit failed")
    job = r.stdout.decode().strip().splitlines()[-1].split(";")[0]
    if not job.isdigit():
        die(f"unexpected sbatch output: {r.stdout.decode()!r}")

    STATE.write_text(f"{stamp} {job} {target}\n")
    if detach:
        say(f"submitted job {job}. Check on it with: pixi run results")
        return
    say(f"submitted job {job} (Ctrl-C is safe, the job keeps running; check it with: pixi run results)", flush=True)
    final = wait_for_job(job)
    show_output(stamp, job)
    if final != "COMPLETED":
        die(f"job ended as {final}")


def cmd_results():
    if not STATE.exists():
        die("no batch job recorded, start one with: pixi run batch <target>")
    stamp, job, target = STATE.read_text().split()
    check_connection()
    state = job_state(job)
    say(f"job {job} ({target}): {state}")
    if state in TERMINAL:
        show_output(stamp, job)
        return
    r = ssh(f"tail -n 40 twostream-runs/{stamp}/slurm-{job}.out")
    print(r.stdout.decode() if r.returncode == 0 else "(no output yet)")


def cmd_session(action):
    check_connection()
    if not SESSION.exists():
        say("no session. One is reserved automatically by: pixi run remote <target>")
        return
    if action == "end":
        say(f"session {end_session()} ended")
        return
    job, _ = read_session()
    out = ssh(f"squeue -j {job} -h -o '%T on %N, %L left'").stdout.decode().strip()
    say(f"session {job}: {out}" if out else "session ended")


def cmd_setup():
    if ssh("true").returncode == 0:
        say(f"already set up, {HOST} accepts your key")
        return

    ssh_dir = Path.home() / ".ssh"
    ssh_dir.mkdir(exist_ok=True)
    key = ssh_dir / "talapas_ed25519"
    config = ssh_dir / "config"

    if not key.exists():
        say(f"creating {key}")
        if run(["ssh-keygen", "-t", "ed25519", "-N", "", "-f", str(key)]).returncode != 0:
            die("ssh-keygen failed")

    text = config.read_text() if config.exists() else ""
    if f"Host {HOST}" not in text:
        duck = input("UO DuckID: ").strip()
        block = f"\nHost {HOST}\n  User {duck}\n  IdentityFile ~/.ssh/talapas_ed25519\n"
        config.write_text(text + block)
        say(f"added {HOST} to {config}")

    pub = Path(str(key) + ".pub").read_text().strip()
    script = (
        "mkdir -p ~/.ssh && chmod 700 ~/.ssh\n"
        f"grep -qxF '{pub}' ~/.ssh/authorized_keys 2>/dev/null || echo '{pub}' >> ~/.ssh/authorized_keys\n"
        "chmod 600 ~/.ssh/authorized_keys\n"
    )
    say("installing your key on Talapas (enter your password and approve DUO)")
    r = run(["ssh", "-o", "ConnectTimeout=15", HOST, "sh", "-s"], input=script.encode())
    if r.returncode != 0:
        die("could not install the key. Are you on the UO VPN, and did you type your password and DUO correctly?")

    if ssh("true").returncode != 0:
        die("key installed but login still prompts. Check the 'Host' entry in ~/.ssh/config")
    say("done, you can now use pixi run remote <serial|omp|hybrid|gpu>")


def main():
    parser = argparse.ArgumentParser(prog="remote.py")
    sub = parser.add_subparsers(dest="cmd", required=True)
    sub.add_parser("remote").add_argument("target", choices=TARGETS)
    batch = sub.add_parser("batch")
    batch.add_argument("target", choices=TARGETS)
    batch.add_argument("--detach", action="store_true")
    sub.add_parser("session").add_argument("action", nargs="?", default="status", choices=["status", "end"])
    sub.add_parser("results")
    sub.add_parser("setup")
    args, extra = parser.parse_known_args()

    if extra and args.cmd != "batch":
        parser.error(f"unrecognized arguments: {' '.join(extra)}")
    if args.cmd == "remote":
        cmd_remote(args.target)
    elif args.cmd == "batch":
        cmd_batch(args.target, args.detach, extra)
    elif args.cmd == "session":
        cmd_session(args.action)
    elif args.cmd == "results":
        cmd_results()
    else:
        cmd_setup()


if __name__ == "__main__":
    main()
