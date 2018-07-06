#!/bin/bash

ulimit -c unlimited

./stop.sh

cd src/tcpsvr/bin
./tcpsvr -c deploy_auth.cfg -d
./tcpsvr -c deploy_hall.cfg -d
./tcpsvr -c deploy_game.cfg -d
cd -

cd src/proxysvr/bin
./proxysvr -c deploy.cfg -d
cd -

cd src/authsvr/bin
./authsvr -i 0 -c deploy_0.cfg -d
cd -

cd src/hallsvr/bin
./hallsvr -i 0 -c deploy_0.cfg -d
cd -

cd src/gamesvr/bin
./gamesvr -i 0 -c deploy_0.cfg -d
./gamesvr -i 1 -c deploy_1.cfg -d
cd -

cd src/dbproxy/bin
./dbproxy -i 0 -c deploy_0.cfg -d
cd -


./status.sh
