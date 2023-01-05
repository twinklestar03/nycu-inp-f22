# INP111 Lab07 Week #11 (2022-11-24)

Date: 2022-11-24

[TOC]

# Reentrant Challenges

The lab aims to realize how reentrant issues cause problems in a program. We have prepared three different challenges. Have fun with the challenges.

## Challenge #1 - Play with the Oracles

### Description

Let the challenge server say `Good Job!` to you!

Run ``nc inp111.zoolab.org 10006`` to connect to the challenge server and have fun!

:::success
Click [here](https://inp111.zoolab.org/code.html?file=lab07/oracle.c) to view the source code of this challenge online, or download it from [here](https://inp111.zoolab.org/lab07/oracle.c).
:::

## Challenge #2 - Play with the Oracles (patch1)

### Description

Let the challenge server say `Good Job!` to you!

Run ``nc inp111.zoolab.org 10007`` to connect to the challenge server and have fun!

:::success
Click [here](https://inp111.zoolab.org/code.html?file=lab07/oraclep1.c) to view the source code of this challenge online, or download it from [here](https://inp111.zoolab.org/lab07/oraclep1.c).
:::

## Challenge #3 - Web Crawler

### Description

There is an internal web server running at `http://localhost:10000`. Can you read the flag from that server?

Run ``nc inp111.zoolab.org 10008`` to connect to the challenge server and have fun!

:::success
Click [here](https://inp111.zoolab.org/code.html?file=lab07/webcrawler.cpp) to view the source code of this challenge online, or download it from [here](https://inp111.zoolab.org/lab07/webcrawler.cpp). You need two additional arguments `-std=c++20` and `-lcrypto` to compile the server.
:::

:::warning
Note that this challenge requires proof of work (PoW) before accessing the server. Please read the [PoW document](https://md.zoolab.org/s/EHSmQ0szV) to see how it works.
:::

## Demo

We have a [CTF system](https://inpctf.zoolab.org/) for hosting the live scoreboard. For each successfully solved challenge, the challenge server prints out a "flag" for you, and then you can submit the flag to the scoreboard.

:::warning
You must ***submit*** your obtained flag to the scoreboard system and ***demonstrate*** your solution to a TA so that we can record your score in our official class scoresheet!
:::

0. [5 pts] "Hello, World" challange on the scoreboard -- Just for fun!

1. [20 pts] Get the flag from the challenge server #1 running at port 10006.

1. [30 pts] Get the flag from the challenge server #2 running at port 10007.

1. [50 pts] Get the flag from the challenge server #3 running at port 10008.

Your final score is weighed based on your rank on the scoreboard. The weight is:

1. 1.00 if your rank is between 1 and 15

2. 0.95 if your rank is between 16 and 30

3. 0.90 if your rank is between 31 and 45

4. 0.85 if your rank is between 46 and 60

5. 0.80 if your rank is 61 or later

