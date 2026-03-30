#!/bin/bash
set -e

# Populate docroot from bundled DokuWiki on first run
if [ ! -f /var/www/html/doku.php ]; then
  echo "First run: installing DokuWiki into /var/www/html ..."
  cp -a /opt/dokuwiki/. /var/www/html/
  echo "DokuWiki installed. Visit http://localhost:8080/install.php to configure."
fi

exec "$@"
