#!/bin/bash

pkill tcpsvr
pkill authsvr
pkill hallsvr
pkill gamesvr
pkill dbproxy
pkill proxysvr
ipcrm -a
./status.sh
