# Oxidus in Docker

A self-contained image of the [Oxidus](https://oxidus.online/) mudlib plus the
FluffOS driver. The image clones a **pristine** copy of the mudlib and compiles
the driver from the bundled `fluffos` submodule using the canonical
`adm/dist/rebuild` script — so a fresh container is a brand-new mudlib, ready
to play. The first character to log in becomes Oxidus's owner with the highest
privileges.

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

Create a character — the first one to log in is made an admin.

Stop it (game state is preserved in the `oxidus-state` volume):

```bash
docker compose down
```

## Run the published image

```bash
docker run -d --name oxidus --init \
  -p 1336:1336 \
  -v oxidus-state:/oxidus/state \
  ghcr.io/gesslar/oxidus:latest
```

## What persists

Code is baked into the image (pristine); only runtime state lives in the
`/oxidus/state` volume. On first boot the entrypoint symlinks every mutable
mudlib path into that volume, so it survives restarts **and** image upgrades:

| Path                  | Contents                                  |
| --------------------- | ----------------------------------------- |
| `data/`               | accounts, areas, db, users, storage       |
| `home/`               | player / wizard home directories          |
| `log/`                | driver + mudlib logs                       |
| `open/`, `tmp/`       | scratch / open data                       |
| `adm/etc/certs/`      | TLS certs                                  |
| `adm/etc/secret/`     | secrets                                    |
| `adm/etc/alarms/`     | alarm files                               |
| `adm/etc/first_user`  | owner-assigned marker                      |
| `adm/etc/config.lpml` | customised config                          |
| `adm/etc/mssp.lpml`   | MSSP config                                |

To start completely fresh, remove the volume: `docker volume rm oxidus-state`.

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

To use your own certificates instead, drop `cert.pem` / `key.pem` into the
volume at `adm/etc/certs/` before first boot.

## Configuration knobs

Build args (Dockerfile):

| Arg            | Default                                            | Purpose                          |
| -------------- | -------------------------------------------------- | -------------------------------- |
| `OXIDUS_REPO`  | `https://github.com/gesslar/oxidus-mudlib.git`     | mudlib repo to clone             |
| `OXIDUS_REF`   | `main`                                             | branch / tag / commit to build   |
| `OXIDUS_HOME`  | `/oxidus`                                          | install path inside the image    |

Runtime env (entrypoint):

| Env              | Default | Purpose                          |
| ---------------- | ------- | -------------------------------- |
| `OXIDUS_TLS`     | `0`     | `1` enables TLS telnet           |
| `OXIDUS_TLS_PORT`| `1338`  | TLS telnet port                  |

## Notes

- Per the canonical `adm/dist/rebuild`, the **driver tracks fluffos `master`**:
  rebuilding the image picks up the latest FluffOS. The mudlib itself is pinned
  to whatever `OXIDUS_REF` you build (CI builds the exact pushed commit).
- The fluffos submodule is **effectively unpinned**: `rebuild` does
  `git reset --hard origin/master` before compiling, so the committed submodule
  SHA never decides what's built — every rebuild rides master HEAD. The
  submodule's *position* (its `.gitmodules` entry + gitlink path) is required so
  the directory gets populated to compile from; its recorded *version* is
  cosmetic. Advancing the pointer is optional housekeeping, never a build step.
  (fluffos updates are sporadic — a year quiet, then a flurry — so if you're ever
  unsure whether you "need to update" anything: you don't. Just rerun `rebuild`.)
- The container runs as a non-root `oxidus` user.
- `--init` (compose: `init: true`) is recommended so signals/zombies are handled
  cleanly and `docker stop` lets the driver shut down gracefully.
