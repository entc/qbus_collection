FROM qore/qbus:jessie

COPY docker /qore
RUN mkdir /build && cd /build && cmake ../qore/flow && make install

FROM bitnami/minideb:jessie

RUN apt-get update && apt-get install -y \
  libmysqlclient-dev \
  && rm -rf /var/lib/apt/lists/*
  
COPY --from=0 /opt .

WORKDIR /modules

ENTRYPOINT ["/modules/qbus_mod_flow"]
CMD ["-d", "socket:127.0.0.1:33340"]
