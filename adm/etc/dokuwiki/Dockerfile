FROM php:8.3-apache

ARG HOST_UID=1000
ARG HOST_GID=1000

RUN apt-get update && apt-get install -y \
    libxml2-dev \
    libgd-dev \
    wget \
    tar \
  && docker-php-ext-install gd xml \
  && rm -rf /var/lib/apt/lists/*

RUN a2enmod rewrite
COPY dokuwiki.conf /etc/apache2/conf-enabled/dokuwiki.conf

# Run Apache as the host user so volume files stay owned by you
RUN usermod -u ${HOST_UID} www-data && groupmod -g ${HOST_GID} www-data

ENV DOKUWIKI_VERSION=2024-02-06b

RUN wget -q "https://download.dokuwiki.org/src/dokuwiki/dokuwiki-${DOKUWIKI_VERSION}.tgz" -O /tmp/dokuwiki.tgz \
  && mkdir -p /opt/dokuwiki \
  && tar xzf /tmp/dokuwiki.tgz --strip-components=1 -C /opt/dokuwiki \
  && rm /tmp/dokuwiki.tgz

COPY entrypoint.sh /usr/local/bin/entrypoint.sh
RUN chmod +x /usr/local/bin/entrypoint.sh

EXPOSE 80

ENTRYPOINT ["entrypoint.sh"]
CMD ["apache2-foreground"]
