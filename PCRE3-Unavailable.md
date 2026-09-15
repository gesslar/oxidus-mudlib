# PCRE3 unavailable (Debian 13+, Ubuntu 26.04+)

FluffOS's `pcre` package links against **classic PCRE** — the 8.x series, which
Debian and Ubuntu package as `pcre3` (`libpcre3-dev`). Newer releases of both
distributions have dropped it in favour of PCRE2, which the driver can't use in
its place. On those releases, the dependency install in [README.md](README.md)
stops with:

```text
E: Package 'libpcre3-dev' has no installation candidate
```

| Distribution | Last release with `libpcre3-dev` | First release without it |
| ------------ | -------------------------------- | ------------------------ |
| Debian       | 12 "bookworm"                    | 13 "trixie"              |
| Ubuntu       | 25.10 "questing"                 | 26.04 LTS "resolute"     |

Not sure which you have? `apt-cache policy libpcre3-dev` answers it — a
`Candidate: (none)` line means you need this page.

The fix is to build PCRE 8.45, the final classic PCRE release, from source. It
builds as a static library, so the compiled driver carries PCRE inside it and
doesn't need anything installed at runtime.

> [!NOTE]
> PCRE 8.45 is the end of the line: upstream no longer maintains classic PCRE
> and won't release fixes for it. It's still what FluffOS links, so it's what
> the driver needs.

## Option 1 — Let the script do it

Install the build dependencies **without** `libpcre3-dev`, plus `curl` for the
download:

```bash
sudo apt-get install build-essential bison cmake git curl pkg-config \
  libssl-dev libz-dev libsqlite3-dev libpq-dev libffi-dev \
  libjemalloc-dev libicu-dev default-libmysqlclient-dev
```

Then, from the root of your checkout:

```bash
npm run pcre            # or: adm/dist/scripts/build-pcre
```

The script downloads PCRE 8.45, checks it against its known SHA-256, builds it,
and installs the libraries and `pcre.h` into `/usr/local`. It asks for `sudo` only
for the install step, and only if `/usr/local` isn't writable by you. Running it
again once PCRE is installed does nothing; set `FORCE=1` to rebuild anyway.

Once it finishes, carry on with the driver build as normal:

```bash
adm/dist/rebuild
```

### Installing somewhere other than `/usr/local`

Set `PREFIX` to install elsewhere — handy if you'd rather not touch system
directories or don't have `sudo`:

```bash
PREFIX="$HOME/.local/pcre" npm run pcre
```

cmake doesn't search a custom prefix on its own, so point the driver build at
it:

```bash
OXIDUS_CMAKE_EXTRA="-DCMAKE_PREFIX_PATH=$HOME/.local/pcre" adm/dist/rebuild
```

## Option 2 — Build it by hand

If you'd rather see every step, this is exactly what the script does:

```bash
url=https://downloads.sourceforge.net/project/pcre/pcre/8.45
curl -fsSLO "$url/pcre-8.45.tar.gz"
sha=4e6ce03e0336e8b4a3d6c2b70b1c5e18590a5673a98186da90d4f33c23defc09
echo "$sha  pcre-8.45.tar.gz" | sha256sum -c -
tar xzf pcre-8.45.tar.gz
cd pcre-8.45

./configure --prefix=/usr/local \
  --enable-static --disable-shared --with-pic \
  --disable-cpp --enable-utf --enable-unicode-properties
make -j "$(nproc)" libpcre.la
sudo make install-libLTLIBRARIES install-nodist_includeHEADERS
```

What the options are for:

- `--enable-static --disable-shared` — a static library only, so the driver
  links PCRE in and needs no `libpcre` at runtime
- `--with-pic` — the driver links as a position-independent executable, so the
  library has to be built position-independent too
- `--enable-utf --enable-unicode-properties` — required; the driver compiles
  every pattern in UTF-8 mode
- `--disable-cpp` — skips the C++ wrapper, which FluffOS doesn't use

The last two targets install just the libraries (`libpcre` and its POSIX
wrapper `libpcreposix`) and `pcre.h`, leaving out the `pcregrep` / `pcretest`
tools and man pages. Then build the driver with
`adm/dist/rebuild`.

## Removing it

Everything installed is five files:

```bash
sudo rm /usr/local/include/pcre.h \
  /usr/local/lib/libpcre.a /usr/local/lib/libpcre.la \
  /usr/local/lib/libpcreposix.a /usr/local/lib/libpcreposix.la
```

The driver you've already built keeps working — PCRE is compiled into it.
