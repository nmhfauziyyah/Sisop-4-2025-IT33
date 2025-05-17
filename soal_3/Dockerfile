FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    gcc make libfuse3-dev pkg-config \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY antink.c .

RUN gcc -Wall -D_FILE_OFFSET_BITS=64 antink.c -o antink -lfuse3 -pthread

CMD ["sh", "-c", "mkdir -p /it24_host /antink_mount /var/log && touch /var/log/it24.log && chmod 666 /var/log/it24.log && /app/antink /antink_mount"]
