# Oxidus

## Setting up and documentation

Information on setting up and further documentation of the systems and
functions of Oxidus can be found on the [Oxidus documentation site](https://oxidus.online/).

## Running with Docker

A fresh, self-contained Oxidus MUD runs entirely in Docker. The image bundles
the mudlib **and** a freshly-compiled FluffOS driver, so the only thing you need
on the host is Docker.

There are two ways in: just run the published image (no clone), or build it
yourself from a clone of this repo.

### Option 1 — Run the published image (lowest friction, no clone)

You need nothing but Docker. The image is pulled automatically on first run:

```bash
docker run -d --name oxidus --init \
  -p 1336:1336 \
  -v oxidus-state:/oxidus/state \
  ghcr.io/gesslar/oxidus:latest
```

- `--init` — clean signal handling, so `docker stop` shuts the driver down promptly
- `-p 1336:1336` — exposes the telnet port on the host
- `-v oxidus-state:/oxidus/state` — named volume holding all game state

Watch it boot until you see `Accepting telnet connections...`:

```bash
docker logs -f oxidus
```

Connect and create a character — **the first character to log in becomes the
superuser**:

```bash
telnet localhost 1336
```

Manage it:

```bash
docker stop oxidus       # stop (your world is kept in the volume)
docker start oxidus      # start again
docker pull ghcr.io/gesslar/oxidus:latest   # upgrade: then `docker rm -f oxidus` and re-run
```

**Reset to a brand-new MUD** (wipes the world — new accounts, superuser on first
login again):

```bash
docker rm -f oxidus
docker volume rm oxidus-state
# then re-run the `docker run` command above
```

### Option 2 — Build it yourself (clone, then Docker)

Clone the repo and build the image locally with Compose. This compiles the
driver from source, so a rebuild always picks up the latest FluffOS:

```bash
git clone https://github.com/gesslar/oxidus-mudlib.git
cd oxidus-mudlib/adm/dist/docker
docker compose up -d --build      # build the driver + start the MUD
docker compose logs -f            # watch it boot ("Accepting telnet connections...")
```

Connect (first login is superuser, as above):

```bash
telnet localhost 1336
```

Manage and upgrade:

```bash
docker compose down               # stop (world kept)
docker compose up -d              # start
docker compose build --no-cache   # rebuild against latest source, then `up -d`
```

**Reset to a brand-new MUD:**

```bash
docker compose down -v            # -v also drops the oxidus-state volume
docker compose up -d
```

### What's actually happening

- **The image is built in two stages.** The first stage clones a *pristine*
  copy of this repository over HTTPS and compiles the driver using the canonical
  [`adm/dist/rebuild`](adm/dist/rebuild) script (which also pins the mudlib paths
  in `config.mud`). The second, slim runtime stage ships only the built tree.
  (With Option 1 this has already been done for you on the published image.)
- **Code is baked into the image; game state lives in a volume.** On first boot
  the container's entrypoint moves every runtime-mutable path
  (`data/`, `home/`, `log/`, `open/`, `tmp/`, `adm/etc/certs/`,
  `adm/etc/secret/`, `adm/etc/alarms/`, and the `first_user` / `config.lpml` /
  `mssp.lpml` files) into the `oxidus-state` volume and symlinks them back in.
  This keeps the mudlib code pristine while your players, data and logs survive
  restarts **and** image upgrades. Resetting the MUD is simply removing that
  volume.
- **The driver runs in a reboot loop**, mirroring `adm/dist/run`: an in-game
  reboot restarts it automatically, while a real shutdown stops the container.

### Running vs. rebuilding (stable at run, fresh at build)

There are two clocks here, and they behave differently on purpose:

- **Running is deterministic.** A built image is a frozen snapshot — a fixed
  driver binary with the mudlib baked in. `docker run` / `docker start` /
  `docker compose up` on the same image always behaves identically; all your
  changing state lives in the `oxidus-state` volume, not the image.
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

(CI doesn't have this problem: each push builds at its exact commit, which busts
the cache and always recompiles against the latest FluffOS.)

For TLS, build args, and the full option list, see
[`adm/dist/docker/README.md`](adm/dist/docker/README.md).

## AI Assistant Guidelines

Be aware of skills documented in .claude/skills
