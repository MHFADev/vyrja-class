#!/bin/sh
# Vyrja RMM server (PocketBase) - listens on the whole LAN.
# Put the pocketbase binary in this folder, then run: ./start-rmm-server.sh
set -e
cd "$(dirname "$0")"
exec ./pocketbase serve --http=0.0.0.0:8090
