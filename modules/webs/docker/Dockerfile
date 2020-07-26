FROM qore/qbus:jessie

COPY docker /qore
RUN mkdir /build && cd /build && cmake ../qore && make install

FROM bitnami/minideb:jessie

RUN apt-get update && apt-get install -y \
  libssl-dev \
  && rm -rf /var/lib/apt/lists/*
  
COPY --from=0 /opt .

CMD ["/modules/qbus_mod_webs"]

