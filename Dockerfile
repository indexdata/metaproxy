FROM debian:bookworm-slim AS build

# Builds from the workspace root dir
WORKDIR /app

RUN apt-get update && apt-get install -y \
  autoconf automake libtool gcc g++ make bison \
  tclsh xsltproc docbook docbook-xml docbook-xsl librsvg2-bin \
  pkg-config libxslt1-dev libgnutls28-dev libicu-dev \
  libboost-dev libboost-system-dev libboost-thread-dev \
  libboost-test-dev libboost-regex-dev \
  git

RUN git clone https://github.com/indexdata/yaz.git
RUN cd yaz && ./buildconf.sh
RUN cd yaz && ./configure --disable-shared --enable-static
RUN cd yaz && make -j4

RUN git clone https://github.com/indexdata/yazpp.git
RUN cd yazpp && ./buildconf.sh
RUN cd yazpp && ./configure --with-yaz=../yaz --disable-shared --enable-static
RUN cd yazpp && make -j4

COPY . metaproxy
RUN cd metaproxy && ./buildconf.sh
RUN cd metaproxy && ./configure --with-yazpp=../yazpp --disable-shared --enable-static
RUN cd metaproxy && make -j4

# Save list of shared lib deps
RUN ldd metaproxy/src/metaproxy | tr -s '[:blank:]' '\n' | grep '^/' | \
    xargs -I % sh -c 'mkdir -p $(dirname deps%); cp % deps%;'

RUN mkdir -p /etc/metaproxy/filters-enabled
RUN mkdir -p /etc/metaproxy/routes.d
RUN mkdir -p /etc/metaproxy/ports.d

# create runtime user
RUN adduser \
  --disabled-password \
  --gecos "" \
  --home "/nonexistent" \
  --shell "/sbin/nologin" \
  --no-create-home \
  --uid 65532 \
  metaproxy-user

# create small runtime image
FROM scratch

# need to copy SSL certs and runtime use
COPY --from=build /usr/share/zoneinfo /usr/share/zoneinfo
COPY --from=build /etc/ssl/certs/ca-certificates.crt /etc/ssl/certs/
COPY --from=build /etc/passwd /etc/passwd
COPY --from=build /etc/group /etc/group

# copy the binary and deps
COPY --from=build /app/metaproxy/src/metaproxy .
COPY --from=build /app/deps /

COPY --from=build /etc/metaproxy /etc/metaproxy

# Nice root XML file
COPY --from=build /app/metaproxy/debian/metaproxy.xml /etc/metaproxy

EXPOSE 9000

# Run
USER metaproxy-user:metaproxy-user
CMD ["/metaproxy", "-c", "/etc/metaproxy/metaproxy.xml"]
