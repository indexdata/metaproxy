FROM debian:trixie-slim AS build

ARG YAZ_VERSION=5.37.1
ARG YAZPP_VERSION=1.9.1

# Builds from the workspace root dir
WORKDIR /app

RUN apt update && apt-get install -y \
  apt-transport-https ca-certificates curl \
  autoconf automake libtool gcc g++ make \
  tclsh xsltproc docbook docbook-xml docbook-xsl librsvg2-bin \
  pkg-config libxslt1-dev libgnutls28-dev libicu-dev \
  libboost-dev libboost-system-dev libboost-thread-dev \
  libboost-test-dev libboost-regex-dev \
  git

RUN curl https://ftp.indexdata.com/pub/yaz/yaz-${YAZ_VERSION}.tar.gz -o yaz-${YAZ_VERSION}.tar.gz
RUN tar zxf yaz-${YAZ_VERSION}.tar.gz
WORKDIR /app/yaz-${YAZ_VERSION}
RUN ./configure --disable-shared --enable-static
RUN make -j4

WORKDIR /app

RUN curl https://ftp.indexdata.com/pub/yazpp/yazpp-${YAZPP_VERSION}.tar.gz -o yazpp-${YAZPP_VERSION}.tar.gz
RUN tar zxf yazpp-${YAZPP_VERSION}.tar.gz
WORKDIR /app/yazpp-${YAZPP_VERSION}
RUN ./configure --disable-shared --enable-static
RUN make -j4

WORKDIR /app

COPY . metaproxy
RUN cd metaproxy && ./buildconf.sh
RUN cd metaproxy && ./configure --disable-shared --enable-static
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
