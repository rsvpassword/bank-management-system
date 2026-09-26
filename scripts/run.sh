#!/usr/bin/env bash
set -e
PORT="${1:-8888}"

echo "[1/2] 正在编译..."
g++ -O2 -std=c++17 -pthread bank_server.cpp -o bank_server

echo "[2/2] 启动服务器，端口 $PORT ..."
./bank_server "$PORT"