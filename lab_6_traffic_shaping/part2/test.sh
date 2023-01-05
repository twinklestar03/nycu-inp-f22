#!/bin/sh

test() {
	sudo tc qdisc del dev lo root netem
	sudo tc qdisc add dev lo root netem delay $1 rate $2 #loss $3
	echo "\033[1;33mCONFIG: delay $1 rate $2\033[m"
	ping -c 1 localhost 2>/dev/null | grep ttl
	timeout 10 ./client 127.0.0.1 9998 $3
}

test 2ms  100Mbit	$1
test 4ms  200Mbit	$1
test 6ms  300Mbit	$1
test 8ms  400Mbit	$1
test 10ms 500Mbit	$1
test 12ms 600Mbit	$1
test 14ms 700Mbit	$1
test 16ms 800Mbit	$1
test 18ms 900Mbit	$1
test 20ms 1000Mbit	$1

# cleanup
sudo tc qdisc del dev lo root netem
