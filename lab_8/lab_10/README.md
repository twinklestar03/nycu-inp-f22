# INP111 Lab10 Week #15 (2022-12-22)

Date: 2022-12-22

[TOC]

# Broadcast XDP File Transfer

This lab aims to practice implementing a broadcast and raw socket program. The objective is to send 1000 files from a client to a server, just like the scenario we used in the [Robust UDP Challenge](https://md.zoolab.org/s/zniK2CzAA). The link quality between the client and the server is good. However, the network settings reject all packets delivered using well-known transport protocols, including TCP, UDP, and SCTP. Packets sent to unicast addresses are dropped as well. You have to implement both the server and the client. We do not have a spec for the protocol. You can decide how to transmit the files by yourself. Once you have completed your implementations, you can upload your compiled server and client to our challenge server for evaluation. Good luck, and have fun!

## The Challenge Server

The challenge server can be accessed via ``nc`` using the command:
```
nc inp111.zoolab.org 10012
```
You have to solve the Proof-of-Work challenge first. The server then allows you to upload two binary files encoded in base64. The first one is for the server, and the second one is for the client. You ***must*** compile your programs on a Linux machine (both x86_64 and arm64 dockers are fine) and link the program with `-static` option. This is because your binary will be invoked on the challenge server, but no standard dynamic libraries are available on the challenge server.

We recommend you interact with our challenge server using `pwntools`. If you do not have it, install it by following the instructions [here](https://md.zoolab.org/s/EleTCdAQ5).

Once your installation is successful, you can submit your binaries using our prepared script `submit.py` ([view](https://inp111.zoolab.org/code.html?file=lab10/submit.py) | [download](https://inp111.zoolab.org/lab10/submit.py)). The usage of this script is as follows:
```
python3 submit.py /path/to/your/server /path/to/your/client`
```
The script solves the PoW challenge from the challenge server, uploads the two binaries to the server, runs the server and the client on the challenge server, and reports results from the server.

Note that the challenge server runs your program by passing several arguments to your programs. For the server, it is
```
/server <path-to-store-files> <total-number-of-files> <broadcast-address>
^^^^^^^
Your program
```
For the client, it is
```
/client <path-to-read-files> <total-number-of-files> <broadcast-address>
^^^^^^^
Your program
```
Suppose the files are stored in the `/files` directory (only on the client side), and there is a total of *N* files. Each filename is a six-digit numeric starting from zero to *N-1*. The default setting of the challenge server generates 1000 files (named from `000000` to `000999`) of different sizes randomly.

Your client program should read files from the `<path-to-read-files>` and send the files to your server using a customized transport protocol. Your server program should store received files in the `<path-to-store-files>` directory. You can consider implementing your customized transport protocol using `raw socket`.

The challenge server checks the transmitted files right after your client program terminates. It then reports how many files have been correctly transmitted from the client to the server.

:::danger
Note that the MTU of the network interface is 1500 bytes.
:::

## Demonstration

To better illustrate how challenge server works, we implement a **XDP echo server** and a **XDP ping client** to demonstrate how the challenge server works.

:::warning
You can upload any executables you like to the server and see how it behaves. Here we simply use our implemented sample files for an illustration.
:::

- **XDP echo server**:
   On receipt of an XDP packet, the XDP echo server broadcasts it back to the network.

- **XDP ping client**:
   The XDP ping client broadcasts a packet to a target network. A sequence number and a timestamp are embedded into a packet. The echo-backed packet from the echo server can then be used to measure the round-trip time. 

Similiar to the robust UDP challenge, you can submit two binary executables to the challenge server using the `submit.py` script: (Suppose all the files are placed in the same directory)
```
python3 submit.py ./xdpechosrv ./xdpping
```
In our demonstration, you can see messages like those in the below screenshot. Here are some interesting observations.

- The screenshot shows that the `submit.py` script spent about 13s to solve the PoW.

- It then uploads the two files to the challenge server. The local md5 values are computed by `submit.py`, and the remote md5 values are returned from the challenge server.

- The screenshot shows that the measured round-trip network quality is pretty good.

- After the client is terminated, the challenge server checks the files on the server side. Since we did not perform any file transmission, all the checks failed, and the final success rate was zero.

![submit-xdpecho-ping](https://inp111.zoolab.org/lab10/submit-xdpecho-ping.png)

## Hints

- You have to specify your selected protocl number when creating a raw socket. In our sample code, we use protocl number 161. The sample code is `socket(AF_INET, SOCK_RAW, proto_number)`.

- You have to ***fill the IP header by yourself***, because our runtime evaluation environment run both your server and client on the same host. In this case, broadcasting packets sent by process A on the host cannot be received by another process B on the same host (and vice versa). As we explained in the class, you can solve this problem by filling a non-local IP address, e.g., the broadcast address, in the source IP address field.

- To fill the IP header by yourself, you can enable the IP_HDRINCL option for a created raw socket. Don't forget to ensure that your IP packet has a correct checksum.

- The data structure of the IP header can be found in `/usr/include/netinet/ip.h`. Please `#include <netinet/ip.h>` and then you can use the structure `ip` declared in the header file. Here is an online version [ip.h](https://inp111.zoolab.org/code.html?file=lab10/ip.h) for your convenience. 

- One final note: You have to enable SO_BROADCAST option for your raw socket.

## Grading

1. [100pts] You can successfully transfer all the 1000 files from the client to the server, each one worth 0.1 pts.
