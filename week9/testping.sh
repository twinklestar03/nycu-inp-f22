#!/bin/sh

test() {
	sudo tc qdisc del dev lo root netem
	sudo tc qdisc add dev lo root netem delay $1 rate $2 #loss $3
	echo "\033[1;33mCONFIG: delay $1 rate $2\033[m"
	ping -c 3 localhost 2>/dev/null | grep ttl
}

test 5ms  100Mbit
test 10ms 200Mbit
test 15ms 300Mbit
test 20ms 400Mbit
test 25ms 500Mbit
test 30ms 600Mbit
test 35ms 700Mbit
test 40ms 800Mbit
test 45ms 900Mbit
test 50ms 1000Mbit

# cleanup
sudo tc qdisc del dev lo root netem
