# INP111 Lab01 Week #2 (2022-09-22)

Date: 2022-09-22

:::warning
PC Classroom login information:
Username: class-1
Password: vjex

Or you could also login by your own account.
:::

[TOC]

# #1 Play with Docker

This lab aims to be familiar with ``docker`` and ``docker-compose`` commands. Please follow the instructions to complete this lab. Once you have completed everything, please demo your implementation to a TA.

:::info
You can do everything either on your own laptop or on the classroom desktop PC.
:::

## Description

Please follow the steps listed below to complete this lab.

1. Prepare your own docker environment. You can install [Docker Desktop](https://www.docker.com/products/docker-desktop/) in your laptop or simply use the ``docker.io" package pre-installed in the classroom desktop PC.

1. Download the [docker-compose.yml](https://people.cs.nctu.edu.tw/~chuang/courses/netprog/resources/ubuntu/docker-compose.yml) and [Dockerfile](https://people.cs.nctu.edu.tw/~chuang/courses/netprog/resources/ubuntu/Dockerfile) from the course website.

:::danger
***For Apple Chip Users (M1/M2)***: You have to enable "Use Docker Compose V2" in your Docker Desktop options, and use the alternative Dockerfile file [here](https://people.cs.nctu.edu.tw/~chuang/courses/netprog/resources/ubuntu/m1/Dockerfile).
:::

3. Build your docker container environment. Ensure that your have correctly setup your username and created the home directory for the user.

1. Follow the instructions in the introduction slide, compile textbook sampels, and run in your container instance. 

## Demonstration

1. [20%] Show your user name and the user's home directory

1. [20%] Rebuild the docker by removing the built one with ``docker-compose rm`` followed by ``docker-compose up -d --build``. You don't have to remove existing docker images and caches so the rebuild process should be very fast.

1. [20%] Download the executable file ``testenv`` from [here](https://inp111.zoolab.org/lab01.1/testenv) and place it into ``/tmp`` directory inside your built container.

1. [20%] Run ``/tmp/testenv`` and show the result.

:::info
***For Apple Chip Users (M1/M2)*** If you encounter ``segmentation fault`` error when running the ``testenv`` program, you may use the following commands to decode the binary first:
```
upx -d testenv
```
You have to install the ``upx`` package first.
:::

5. [20%] Run the ``daytimetcpsrv`` and ``daytimetcpcli`` samples in your docker instance.

:::info
***For Apple Chip Users (M1/M2)*** You need to pass an additional argument ``--build=arm-linux`` to the ``configure`` command to ensure it can correctly detect your build environment.
:::

# #2 Simple Packet Analysis

This lab aims to practice simple packet analysis using ``tcpdump`` (or ``wireshark``).

:::warning
Most UNIX operating systems have the packages for ``tcpdump`` and ``wireshark``. Nevertheless, wireshark can be downloaded from its [official site](https://www.wireshark.org/download.html).
:::

## Description

Please follow the steps listed below to complete this lab.

1. Run ``tcpdump`` (or ``wireshark``) in your preferred environment.

:::danger
Please notice that you ***should not*** capture packets inside a docker because the docker could strip TOS and TTL values required by this challenge. Instead, please capture packets and then save captured packets in your host machine. 
:::

2. Connect to the challenge server by running ``nc inp111.zoolab.org 10001`` also from your preferred environment. This step can be done either inside or outside a docker.

1. There is one magic packet that has a unique TOS/TTL value different from other packets in the same TCP flow. Try your best to identify the TOS/TTL value for the magic packet. 

1. Find out that magic packet and decode the TCP payload encoded in BASE64.

:::danger
You may know that TCP may send successive data requested by ``send`` system calls in the same packet. Once it happens, you may be unable to identify the magic packet from the corresponding TCP flow. In that case, you may run the ``nc`` client multiple times to collect several TCP flows and then perform per-flow analysis to learn how the magic packet is hidden in the flow.
:::

:::warning
If you have difficulties capturing packets, you may consider using our captured pcap file from [here](https://inp111.zoolab.org/lab01.2/lab_tcpdump.pcap). You can check the TTL values for packets stored in the pcap file. The SHA1 value for the pcap file is:<br/> ``ba332a3c528d68c5c6874aae681929199dbb6c98``
:::

## Demonstration

1. [70%] If you can complete the steps manually, you can get 70% of the points. *Note* that you have to decode the payload of the identified packet to ensure that you have found the right one.

1. [15%] If you can identify the magic packet using your implemented script (or program), you can get 85%. *Note* that you have to (manually) decode the payload of the identified packet to ensure that you have found the right one.

1. [15%] If you can decode the content using your implemented script (or program), you can get 100% of the points.

:::info
Note that your script (or program) can use a saved ``pcap`` file created by ``tcpdump`` or ``wireshark`` as its input.
:::
