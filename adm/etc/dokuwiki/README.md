# DokuWiki Development Container

Local DokuWiki instance for iterating on site design and content.
The wiki docroot is `doc/wiki/` at the project root — everything there
is tracked in git and can be rsync'd to the production host.

## Prerequisites

Install Docker Engine (includes `docker compose`).

### Fedora

```bash
sudo dnf install dnf-plugins-core
sudo dnf-3 config-manager --add-repo https://download.docker.com/linux/fedora/docker-ce.repo
sudo dnf install docker-ce docker-ce-cli containerd.io docker-compose-plugin
sudo systemctl enable --now docker
sudo usermod -aG docker $USER
# Log out and back in, or run `newgrp docker` in your current shell.
```

### Ubuntu / Debian

```bash
sudo apt-get update
sudo apt-get install ca-certificates curl
sudo install -m 0755 -d /etc/apt/keyrings
sudo curl -fsSL https://download.docker.com/linux/ubuntu/gpg \
  -o /etc/apt/keyrings/docker.asc
echo "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.asc] \
  https://download.docker.com/linux/ubuntu $(. /etc/os-release && echo "$VERSION_CODENAME") stable" \
  | sudo tee /etc/apt/sources.list.d/docker.list > /dev/null
sudo apt-get update
sudo apt-get install docker-ce docker-ce-cli containerd.io docker-compose-plugin
sudo usermod -aG docker $USER
# Log out and back in, or run `newgrp docker` in your current shell.
```

### macOS

Install [Docker Desktop for Mac](https://docs.docker.com/desktop/install/mac-install/).
`docker compose` is included.

### Windows

Install [Docker Desktop for Windows](https://docs.docker.com/desktop/install/windows-install/).
Enable the WSL 2 backend during setup. `docker compose` is included.
Run the commands below from a WSL 2 terminal or Git Bash.

## Setup (first time)

```bash
cd adm/etc/dokuwiki
docker compose up --build
```

Visit <http://localhost:8080/install.php> to run the DokuWiki installer.
Configure the wiki (superuser, title, ACL, etc.), then the site is live
at <http://localhost:8080/>.

## Start / Stop

```bash
# start (from adm/etc/dokuwiki/)
docker compose up -d

# stop
docker compose down

# rebuild after Dockerfile changes
docker compose up --build -d
```

## Deploy

rsync config and content to your web host (the host has its own DokuWiki
core install — you only need to push pages, media, meta, and config):

```bash
rsync -avz --delete doc/wiki/conf/ you@host:/path/to/dokuwiki/conf/
rsync -avz --delete doc/wiki/data/pages/ you@host:/path/to/dokuwiki/data/pages/
rsync -avz --delete doc/wiki/data/media/ you@host:/path/to/dokuwiki/data/media/
rsync -avz --delete doc/wiki/data/meta/ you@host:/path/to/dokuwiki/data/meta/
```

## Notes

- DokuWiki is file-based — no database required.
- On first run the entrypoint copies DokuWiki into `doc/wiki/`.
  Subsequent runs reuse what's already there.
- Port 8080 is configurable in `docker-compose.yml`.
