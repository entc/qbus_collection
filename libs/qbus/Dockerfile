FROM bitnami/minideb:jessie

RUN apt-get update && apt-get install -y \
  cmake \
  gcc \
  g++ \
  libmysqlclient-dev \
  libsqlite3-dev \
  libpcre3-dev \
  libcurl4-openssl-dev \
  libssl-dev \
  uuid-dev \
  && rm -rf /var/lib/apt/lists/*

