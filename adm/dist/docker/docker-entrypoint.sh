#!/bin/bash
#
# Container entrypoint for the Oxidus image.
#
#   1. Symlinks every runtime-mutable mudlib path into the persistent volume at
#      $OXIDUS_HOME/state, so the baked image stays pristine while game state
#      (players, data, logs, certs, ...) survives container/image upgrades.
#   2. Optionally enables TLS telnet using the FluffOS test certs.
#   3. Runs the driver in a reboot loop, mirroring adm/dist/run but with proper
#      signal handling for PID 1.
#
# The set of persisted paths is derived from the mudlib .gitignore (everything
# the lib writes at runtime). Add to PERSIST_DIRS / PERSIST_FILES if the lib
# starts writing somewhere new.

set -eo pipefail

OXIDUS_HOME="${OXIDUS_HOME:-/oxidus}"
STATE="${OXIDUS_HOME}/state"
DIST="${OXIDUS_HOME}/adm/dist"
DRIVER="${DIST}/bin/driver"
CONFIG="${DIST}/config.mud"

cd "${OXIDUS_HOME}"

# Directories whose *contents* are runtime state. adm/custom is the
# consolidated per-MUD override tree (config, security groups/roles/access,
# alarms, first_user, ...); persisting it as a unit means any override slot
# added under it survives upgrades without touching this list again.
PERSIST_DIRS=(
  data
  open
  home
  log
  tmp
  adm/etc/certs
  adm/etc/secret
  adm/custom
)

# Individual files written at runtime (may not exist in a pristine clone).
# Most per-MUD state now lives under adm/custom (above); mssp.lpml is the
# last hold-out still read from adm/etc.
PERSIST_FILES=(
  adm/etc/mssp.lpml
)

# Move a directory's pristine contents into the volume on first boot, then
# replace it with a symlink into the volume.
link_dir() {
  local rel="$1"
  local src="${OXIDUS_HOME}/${rel}"
  local dst="${STATE}/${rel}"

  if [ ! -e "${dst}" ]; then
    mkdir -p "$(dirname "${dst}")"
    if [ -d "${src}" ] && [ ! -L "${src}" ]; then
      cp -a "${src}" "${dst}"   # seed from the pristine image copy (.keep files etc.)
    else
      mkdir -p "${dst}"
    fi
  fi

  rm -rf "${src}"
  mkdir -p "$(dirname "${src}")"
  ln -s "${dst}" "${src}"
}

# Symlink a single file into the volume. A dangling link is fine: the first
# write the lib makes lands in the volume.
link_file() {
  local rel="$1"
  local src="${OXIDUS_HOME}/${rel}"
  local dst="${STATE}/${rel}"

  mkdir -p "$(dirname "${dst}")"
  if [ ! -e "${dst}" ] && [ -f "${src}" ] && [ ! -L "${src}" ]; then
    cp -a "${src}" "${dst}"
  fi

  rm -f "${src}"
  mkdir -p "$(dirname "${src}")"
  ln -s "${dst}" "${src}"
}

echo "[entrypoint] wiring persistent state into ${STATE}"
for d in "${PERSIST_DIRS[@]}"; do link_dir "${d}"; done
for f in "${PERSIST_FILES[@]}"; do link_file "${f}"; done

# Refresh image-managed scaffolding into the (now symlinked) adm/custom volume.
# README, the .keep skeleton, and *.example templates are baked reference files,
# not editable state - so they flow one-directionally, image -> volume, on every
# boot, and an upgrade always lands the current versions. Live per-MUD data
# (config.lpml, security/*, alarms/*.txt, first_user, ...) is not part of the
# baked stash, so it is left untouched.
#
# Add and overwrite only, never delete. Removing a file from the image does not
# remove it from the volume: pruning downstream would be a bidirectional sync,
# and that is deliberately out of scope.
if [ -d "${DIST}/custom.dist" ]; then
  cp -a "${DIST}/custom.dist/." "${OXIDUS_HOME}/adm/custom/"
  echo "[entrypoint] refreshed adm/custom scaffolding from the image"
fi

# Optional TLS telnet. Default off (plain telnet on 1336 only).
if [ "${OXIDUS_TLS:-0}" = "1" ]; then
  tls_port="${OXIDUS_TLS_PORT:-1338}"
  certdir="${OXIDUS_HOME}/adm/etc/certs"   # symlinked into the volume above
  if [ ! -f "${certdir}/cert.pem" ] || [ ! -f "${certdir}/key.pem" ]; then
    cp -a "${DIST}/certs.dist/cert.pem" "${certdir}/cert.pem"
    cp -a "${DIST}/certs.dist/key.pem"  "${certdir}/key.pem"
    echo "[entrypoint] installed FluffOS test certs into ${certdir}"
  fi
  if ! grep -q "external_port_2_tls" "${CONFIG}"; then
    {
      echo ""
      echo "external_port_2: telnet ${tls_port}"
      echo "external_port_2_tls: cert=adm/etc/certs/cert.pem key=adm/etc/certs/key.pem"
    } >> "${CONFIG}"
    echo "[entrypoint] enabled TLS telnet on port ${tls_port}"
  fi
fi

# Reboot loop with signal handling (mirrors adm/dist/run; exit 0 == reboot).
set +e
child=""
shutdown() {
  echo "[entrypoint] received shutdown signal, stopping driver..."
  [ -n "${child}" ] && kill -TERM "${child}" 2>/dev/null
}
trap shutdown TERM INT

cd "${DIST}"
while true; do
  echo "[entrypoint] starting driver..."
  "${DRIVER}" "${CONFIG}" &
  child=$!
  wait "${child}"
  status=$?
  echo "[entrypoint] driver exited with status ${status}"
  if [ "${status}" -eq 0 ]; then
    echo "[entrypoint] reboot requested, restarting..."
    # No delay between reboots, matching bin/run (sleep commented out there).
    # This also leaves no window for a stop signal to land between drivers.
  else
    break
  fi
done

exit "${status}"
