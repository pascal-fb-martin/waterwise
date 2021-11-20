FROM debian:stable-slim
COPY . /
RUN apt-get update && apt-get --yes install ca-certificates openssl
ENTRYPOINT [ "/usr/local/bin/waterwise" ]
