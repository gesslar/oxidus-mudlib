# Running Oxidus in Docker

A fresh, self-contained Oxidus MUD runs entirely in Docker. The image bundles
the mudlib **and** a freshly-compiled FluffOS driver, so the only thing you need
on the host is Docker.

Which way in depends on what you're after:

- **If your goal is to try Oxidus** — poke around, or keep a demo running whose
  world survives updates — run the published image
  ([Option 1](#option-1--run-the-published-image)). No clone, no toolchain.
- **If your goal is a newer FluffOS than the published image, or an image of a
  specific lib ref or your own fork** — build the image yourself
  ([Option 2](#option-2--build-the-image-yourself)).

> [!IMPORTANT]
> **The Docker image is not provided or intended for running a live game.** If
> your goal is to develop the lib, or run your own MUD with code changes that
> last, Docker isn't the tool. Fork the repo and use the native build-and-run
> pipeline in [README.md](README.md).

## Option 1 — Run the published image

You need nothing but Docker. The image is pulled automatically on first run:

```bash
docker run -d --name oxidus --init \
  -p 1336:1336 \
  -e OXIDUS_UID=$(id -u) -e OXIDUS_GID=$(id -g) \
  -v "${OXIDUS_STATE_PARENT:-$HOME}/oxidus-state:/oxidus/state" \
  ghcr.io/gesslar/oxidus:latest
```

- `--name oxidus` — names the container `oxidus`; **every command below refers
  to it by that name**
- `--init` — clean signal handling, so `docker stop` shuts the driver down promptly
- `-p 1336:1336` — exposes the telnet port on the host
- `-e OXIDUS_UID/GID` — makes the state directory owned by you, so you can edit
  it without sudo (omit on Docker Desktop, which handles ownership itself)
- `-v …/oxidus-state:/oxidus/state` — a directory on your host holding all game
  state, at `$HOME/oxidus-state` (set `OXIDUS_STATE_PARENT` to put it elsewhere;
  the `oxidus-state` leaf is always appended)

That `docker run -d` command **has already started the MUD** — it boots in the
background the moment the command returns. You see no output because `-d`
(detached) runs it silently and hands your prompt straight back.

To *watch* it, attach to the logs. `docker logs` replays everything the
container has printed since it started, and `-f` then follows live output —
so you'll see the whole boot scroll past, ending at `Initializations complete.`
once it's listening:

```bash
docker logs -f oxidus    # a viewer onto recorded + live output
                         # Ctrl-C detaches the viewer; the MUD keeps running
```

You can run the above `docker logs` statement at any time to be re-plugged into
the currently running docker to see any debug messages that the game has
output. Doing so right after a boot is useful for checking for any issues with
the boot process.

`Ctrl+C` will exit you from tailing the driver output, but will not stop the
game or the container.

Connect and create a character — **the first character to log in becomes
Oxidus's owner with the highest privileges**:

```bash
telnet localhost 1336
```

### Editing the lib (heads-up: changes are temporary)

The mudlib code is **baked into the image**, not the state directory — so every
`docker pull` gives you a clean, current Oxidus. The trade-off: **any edit to
the shipped lib is wiped when you recreate the container on a newer image.**
That's intended — you always land on fresh, stock Oxidus. Only *state* survives
an update: players, data, logs, and your wizard home directory (`/home/...`).
So tinker freely; a refresh just resets the lib.

**From inside the game:** the shipped lib is **read-only to the game**. The
driver runs as your `OXIDUS_UID`, and the baked code belongs to the image, so
in-game tools can't write it. They can write everything in the state directory,
such as your wizard home directory (`/home/...`), and `update <path>` reloads
whatever you change there.

**From a shell:** `docker exec` puts you in as root, which *can* edit the shipped
lib — these are the temporary edits described above. The image ships `nano`,
`nvim`, and `rg` (ripgrep), so hop in and edit directly:

```bash
docker exec -it oxidus bash
#  nano /oxidus/std/file.lpc     # /oxidus is the mudlib root
#  rg "some_function" /oxidus    # search the lib
#  then, in the game: update /std/file
```

Prefer your own editor on the host? Copy out, edit, copy back:

```bash
docker cp oxidus:/oxidus/std/file.lpc ./file.lpc
docker cp ./file.lpc oxidus:/oxidus/std/file.lpc
```

**If your goal is edits that last**, Docker isn't the tool: fork the repo and
run it natively (see [README.md](README.md)), or build an image from your fork
(see [Option 2](#building-from-your-own-fork)).

For a single area of your own, you can bind-mount a host folder as a **new**
directory in the lib by adding it to the `docker run` command above:

```bash
-v "$PWD/myarea:/oxidus/d/myarea"
```

Mount a new directory, never an existing one like `d/` or `d/village`. A bind
mount *replaces* whatever the image has at that path rather than merging with
it, so mounting over `d/` hides the entire shipped world.

Create the folder on the host first (`mkdir myarea`). It's then owned by you,
and with `OXIDUS_UID` set to your `id -u` the game can write to it — so unlike
the shipped lib, that area is editable in-game. If `-v` has to create the folder
itself, it comes up empty and owned by root, and the game can't write to it.

### Start, stop, upgrade, reset

**Start / stop the MUD.** These control the container's lifecycle — whether the
driver is actually running. Your world is preserved in the state directory
either way:

```bash
docker stop oxidus       # halt the running container
docker start oxidus      # boot it back up (in the background again)
docker restart oxidus    # stop + start in one step
```

Again, after starting or restarting Oxidus, you may opt to review the driver
output to ensure a clean boot using `docker logs -f oxidus`. Keep it going to
persist watching driver output, or Ctrl-C to return to the operating system
prompt.

**Upgrade** to a newer published image:

```bash
docker pull ghcr.io/gesslar/oxidus:latest   # fetch the newest image
docker rm -f oxidus                         # remove the old container
# then re-run the `docker run` command above to recreate it on the new image
```

**Reset to a brand-new MUD** (wipes the world — new accounts, and the first
login becomes Oxidus's owner again):

```bash
docker rm -f oxidus
rm -rf "${OXIDUS_STATE_PARENT:-$HOME}/oxidus-state"
# then re-run the `docker run` command above
```

**Remove it entirely** (uninstall — reclaim the disk; a later `docker run` just
re-pulls the image, so nothing here is permanent):

```bash
docker rm -f oxidus                                    # the container
rm -rf "${OXIDUS_STATE_PARENT:-$HOME}/oxidus-state"    # your world
docker rmi ghcr.io/gesslar/oxidus:latest               # the image itself
```

The container must go before the image — `docker rmi` refuses to remove an image
something is still using. (On the build-it-yourself path, `docker compose down
--rmi all` clears the locally-built image the same way.)

## Option 2 — Build the image yourself

If your goal is a newer FluffOS than the published image, or an image built from
a specific lib ref or your own fork, build the image locally with Compose. This
compiles the driver from source against the current FluffOS `master`.

**The build does not use the lib in your checkout.** It clones `OXIDUS_REPO` at
`OXIDUS_REF` — by default, this repository's `main` — over HTTPS inside the
image, so edits in your working directory never reach it. The clone below only
gives you the Dockerfile and compose file.

```bash
git clone https://github.com/gesslar/oxidus-mudlib.git
cd oxidus-mudlib/adm/dist/docker
docker compose up -d --build      # build the driver + start the MUD
docker compose logs -f            # watch it boot ("Accepting telnet connections...")
```

Connect (first login becomes Oxidus's owner, as above):

```bash
telnet localhost 1336
```

Manage and upgrade:

```bash
docker compose down               # stop (world kept)
docker compose up -d              # start
docker compose build --no-cache   # rebuild against latest source, then `up -d`
```

**Reset to a brand-new MUD** (state lives in a host directory, so remove it
directly — `down -v` only drops named volumes, not bind mounts):

```bash
docker compose down
rm -rf "${OXIDUS_STATE_PARENT:-$HOME}/oxidus-state"
docker compose up -d
```

### Building from your own fork

Push your changes to your fork, then point the build at it. The fork must be
cloneable over public HTTPS:

```bash
docker compose build --no-cache \
  --build-arg OXIDUS_REPO=https://github.com/<you>/<your-fork>.git \
  --build-arg OXIDUS_REF=main
docker compose up -d
```

Your lib changes are baked into that image, so they survive upgrades the same
way stock Oxidus does. Rebuild after each push to pick up new commits.

### Running vs. rebuilding (stable at run, fresh at build)

There are two clocks here, and they behave differently on purpose:

- **Running is deterministic.** A built image is a frozen snapshot — a fixed
  driver binary with the mudlib baked in. `docker run` / `docker start` /
  `docker compose up` on the same image always behaves identically; all your
  changing state lives in the `oxidus-state` directory on the host, not the image.
- **Rebuilding is when you pull the latest.** A rebuild re-clones the mudlib and,
  via `adm/dist/rebuild`, checks out and compiles the **current** FluffOS
  `master`. So the way to pick up a newer driver (or newer base packages) is to
  rebuild — not to restart a container you already have.

One gotcha for **local** builds: Docker's layer cache will happily reuse the
clone/compile layers, so a plain `docker compose build` can hand you a *stale*
rebuild (old FluffOS, old packages). To actually pull the latest, force a clean
build:

```bash
docker compose build --no-cache && docker compose up -d
```

(The published image doesn't have this problem: CI builds it on each published
release, at that release's exact commit and with no layer cache, so it always
compiles against the FluffOS `master` of the day.)

## What's actually happening

- **The image is built in two stages.** The first stage clones a *pristine*
  copy of this repository over HTTPS and compiles the driver using the canonical
  [`adm/dist/rebuild`](adm/dist/rebuild) script (which also pins the mudlib paths
  in `config.mud`). The second, slim runtime stage ships only the built tree.
  (With Option 1 this has already been done for you on the published image.)
- **Code is baked into the image; game state lives in a host directory.** On
  first boot the container's entrypoint moves every runtime-mutable path
  (`data/`, `home/`, `log/`, `open/`, `tmp/`, `adm/etc/secret/`, the
  `adm/custom/` override tree — config, security, alarms, certs, `first_user`,
  `mssp` — plus the seeded-but-editable `adm/dist/config.mud`) into the
  `/oxidus/state` mount and symlinks them back
  in. This keeps the mudlib code pristine while your players, data and logs
  survive restarts **and** image upgrades. Resetting the MUD is simply deleting
  that state directory.
- **The driver runs in a reboot loop**, mirroring `adm/dist/run`: an in-game
  reboot restarts it automatically, while a real shutdown stops the container.

For TLS, build args, and the full option list, see
[`adm/dist/docker/README.md`](adm/dist/docker/README.md).
