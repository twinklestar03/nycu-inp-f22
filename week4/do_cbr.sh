#!/bin/sh
timeout 20 ./tcpcbr 1;   sleep 5  # send at 1 MBps
timeout 20 ./tcpcbr 1.5; sleep 5  # send at 1.5 MBps
timeout 20 ./tcpcbr 2;   sleep 5  # send at 2 MBps
timeout 20 ./tcpcbr 3
