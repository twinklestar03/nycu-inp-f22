# INP111 Lab04 Week #5 (2022-10-13)

Date: 2022-10-13

[TOC]

# #1 nkat

This lab aims to be familiar with a simple TCP server implementation and a few interesting system calls. Please follow the instructions to complete this lab. Once you have completed everything, please demo your implementation to a TA.

:::info
You can do everything either on your own laptop or on the classroom desktop PC.
:::

:::danger
You must implemenet everything in C or C++.
:::

## Description

1. Please implement a TCP server that behaves like ``ncat -p <port-number> -nkle /path/to/an/external/program``. You may play it with a sample command ``ncat -p 11111 -nlke `which date` ``. Once you have invoked it successfully, you can connect to the local server at port 11111 using the command `nc localhost 11111`. It would behave like a daytime TCP server.

1. Your program has to accept multiple arguments. Suppose your program is named ``nkat``, the command argument format is: ``./nkat <port-numner> /path/to/an/external/program [optional arguments ...]``. For example, using your program to invoke exactly the same command as ``ncat`` is ``./nkat 11111 `which date` ``.

1. Your program should show a message like `New connection from <client ip>:<port>` for each new connection.

1. Your program should show an error message when the command execution is failed. Note that you should not send the error message to the client.

1. Once you have complete your implementation, you can test your program by following the steps presented in the *Demonstration* section.

:::warning
You may need the following functions/headers to complete this lab.
* functions
    * [fork(2)](https://man7.org/linux/man-pages/man2/fork.2.html)
    * [dup(2)](https://man7.org/linux/man-pages/man2/dup.2.html)
    * [dup2(2)](https://man7.org/linux/man-pages/man2/dup2.2.html)
    * [execve(2)](https://man7.org/linux/man-pages/man2/execve.2.html) and its variants

Reference: https://man7.org/linux/man-pages/
:::
## Demonstration

Suppose your program is named `nkat`. We will run your program listening on the same port number (`11111`). Once your server runs, connect to port `11111` from a local client using the command ``nc localhost 11111``. You should run the `nc` command multiple times to ensure your server works consistently.

1. [10%] Run the server using the command ``./nkat 11111 `which date` ``. 

1. [20%] Run the server using the command ``./nkat 11111 date``.

1. [20%] Run the server using the command ``./nkat 11111 /path/to/an/invalid/executable``. Your server should handle errors appropriately.

1. [20%] Run the server using the command ``./nkat 11111 ls -la /tmp /home /xxx``.

1. [20%] Run the server using the command ``./nkat 11111 timeout 60 /bin/bash -c 'cd /; exec bash'``. Your server should handle multiple concurrent clients appropriately.

1. [10%] Your program can not produce ***zombie processes*** in any test cases.

<!--1. [XXX%] Download a sample program [XXX]() from here, and then run the server using the command ``./nkat 11111 /path/to/XXX``. Your server should handle multiple concurrent clients appropriately.

1. [XXX%] Download a sample program [XXX]() from here, and then run the server using the command ``./nkat 11111 timeout 5 /path/to/XXX``. Your server should handle command line arguments appropriately.-->
