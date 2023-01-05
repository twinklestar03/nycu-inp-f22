#!/bin/sh

test() {
	sudo tc qdisc del dev lo root netem
	sudo tc qdisc add dev lo root netem delay $1 rate $2 #loss $3
	echo "\033[1;33mCONFIG: delay $1 rate $2\033[m"
	iperf3 -c localhost -p 9997
	sleep 2
}

test 2ms  100Mbit
test 4ms  200Mbit
test 6ms  300Mbit
test 8ms  400Mbit
test 10ms 500Mbit
test 12ms 600Mbit
test 14ms 700Mbit
test 16ms 800Mbit
test 18ms 900Mbit
test 20ms 1000Mbit

# cleanup
sudo tc qdisc del dev lo root netem
