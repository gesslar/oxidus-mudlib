# Oxidus in Docker

A self-contained image of the [Oxidus](https://oxidus.online/) mudlib plus the
FluffOS driver. The image clones a **pristine** copy of the mudlib and compiles
the driver from a fresh clone of FluffOS using the canonical `adm/dist/rebuild`
script — so a fresh container is a brand-new mudlib, ready to play. The first
character to log in becomes Oxidus's owner with the highest privileges.

## Quick start (build locally)

```bash
cd adm/dist/docker
docker compose up -d --build
docker compose logs -f          # watch it boot
```

Then connect:

```bash
telnet localhost 1336
```

Create a character — the first one to log in is made the owner.

Stop it (game state is preserved in the state mount on your host):

```bash
docker compose down
```

Your world persists on the host at `$HOME/oxidus-state`. To put it elsewhere,
set `OXIDUS_STATE_PARENT` in a `.env` file beside the compose file (see
[Configuration knobs](#configuration-knobs) and `.env.example`).

## Run the published image

```bash
docker run -d --name oxidus --init \
  -p 1336:1336 \
  -e OXIDUS_UID=$(id -u) -e OXIDUS_GID=$(id -g) \
  -v "${OXIDUS_STATE_PARENT:-$HOME}/oxidus-state:/oxidus/state" \
  ghcr.io/gesslar/oxidus:latest
```

State lives on the host at `$HOME/oxidus-state` (override the parent dir with
`OXIDUS_STATE_PARENT`; the `oxidus-state` leaf is always appended). The
`OXIDUS_UID`/`OXIDUS_GID` flags make that directory owned by you, so you can
edit config/certs directly and tools like certbot can write into it. On Docker
Desktop (mac/Windows) the uid flags are unnecessary — it handles ownership.

## What persists

Code is baked into the image (pristine); only runtime state lives in the
`/oxidus/state` mount (a host directory — see above). On first boot the
entrypoint symlinks every mutable mudlib path into that mount, so it survives
restarts **and** image upgrades:

| Path                  | Contents                                  |
| --------------------- | ----------------------------------------- |
| `data/`               | accounts, areas, db, users, storage       |
| `home/`               | player / wizard home directories          |
| `log/`                | driver + mudlib logs                       |
| `open/`, `tmp/`       | scratch / open data                       |
| `adm/etc/secret/`     | secrets                                    |
| `adm/custom/`         | per-MUD overrides: config, security (groups/roles/access), alarms, certs, first_user |
| `adm/etc/mssp.lpml`   | MSSP config                                |

To start completely fresh, delete the host state directory:
`rm -rf "${OXIDUS_STATE_PARENT:-$HOME}/oxidus-state"`.

## Upgrading

Pull/rebuild the image and recreate the container. Because the lib is baked in
and state is in the volume, you get the new code on top of your existing world:

```bash
docker compose pull   # or: docker compose build --no-cache
docker compose up -d
```

**Running is stable; rebuilding is when you pull the latest.** A built image is
a frozen snapshot — restarting a container never changes the driver or lib, only
the volume state moves. To pick up a newer FluffOS or newer base packages you
must *rebuild*, and because Docker's layer cache reuses the clone/compile layers,
use `--no-cache` to force a genuinely fresh pull:

```bash
docker compose build --no-cache && docker compose up -d
```

(CI builds are always fresh: each push builds at its exact commit, busting the
cache and recompiling against the latest FluffOS `master`.)

## TLS (optional)

Off by default. To enable TLS telnet using the FluffOS test certificates
(self-signed — fine for testing, replace for production), set `OXIDUS_TLS=1`
and publish the port:

```yaml
environment:
  OXIDUS_TLS: "1"
  OXIDUS_TLS_PORT: "1338"
ports:
  - "1336:1336"
  - "1338:1338"
```

To use your own certificates instead, drop `cert.pem` / `key.pem` into
`adm/custom/certs/` in the state mount (e.g. certbot renews there). The HTTP
server reads the same pair via the `TLS_CERT` / `TLS_KEY` config keys.

## Configuration knobs

Build args (Dockerfile):

| Arg            | Default                                            | Purpose                          |
| -------------- | -------------------------------------------------- | -------------------------------- |
| `OXIDUS_REPO`  | `https://github.com/gesslar/oxidus-mudlib.git`     | mudlib repo to clone             |
| `OXIDUS_REF`   | `main`                                             | branch / tag / commit to build   |
| `OXIDUS_HOME`  | `/oxidus`                                          | install path inside the image    |

Runtime env (entrypoint):

| Env              | Default | Purpose                                            |
| ---------------- | ------- | -------------------------------------------------- |
| `OXIDUS_UID`     | `1000`  | host uid that owns the state mount (chowned at boot) |
| `OXIDUS_GID`     | `1000`  | host gid that owns the state mount                 |
| `OXIDUS_TLS`     | `0`     | `1` enables TLS telnet                             |
| `OXIDUS_TLS_PORT`| `1338`  | TLS telnet port                                    |

Host-side (compose interpolation / `docker run` — resolved on the host, **not**
passed into the container):

| Var                  | Default | Purpose                                                |
| -------------------- | ------- | ------------------------------------------------------ |
| `OXIDUS_STATE_PARENT`| `$HOME` | parent dir of the `oxidus-state` state dir on the host |

Set this (and `OXIDUS_UID`/`GID`) in a `.env` file beside the compose file (see
`.env.example`) — that's the cross-platform way, since shell export syntax
differs per OS.

## Notes

- Per the canonical `adm/dist/rebuild`, the **driver tracks fluffos `master`**:
  rebuilding the image picks up the latest FluffOS. The mudlib itself is pinned
  to whatever `OXIDUS_REF` you build (CI builds the exact pushed commit).
- FluffOS is **not a submodule** — `rebuild` clones it fresh from
  `github.com/fluffos/fluffos` (when the `fluffos/` dir is absent) and does
  `git reset --hard origin/master` before compiling, so every rebuild rides
  master HEAD. There's no pinned SHA to advance and nothing to keep in sync.
  (fluffos updates are sporadic — a year quiet, then a flurry — so if you're ever
  unsure whether you "need to update" anything: you don't. Just rerun `rebuild`.)
- The container **starts as root** to set up the state mount (chown a host bind
  mount, rewire the baked tree), then drops to `OXIDUS_UID:OXIDUS_GID` (default
  `1000`) via `gosu` and runs the driver unprivileged.
- `--init` (compose: `init: true`) is recommended so signals/zombies are handled
  cleanly and `docker stop` lets the driver shut down gracefully.
