# INP111 Lab09 Week #13 (2022-12-08)

Date: 2022-12-08

[TOC]

# UNIX Domain Sudoku

This lab aims to practice implementing a UNIX domain socket client that interacts with a server. You must implement a Sudoku solver and upload the solver executable to the challenge server. To solve the puzzle, the solver must interact with the Sudoku server using a UNIX domain socket connection stored at `/sudoku.sock`.

## The Challenge Server

The challenge server can be accessed using the `nc` command:

```
nc inp111.zoolab.org 10010
```

You have to solve the Proof-of-Work challenge first, and then you can activate the challenge server. The challenge server has two channels to receive commands. One receives commands from the terminal, and the other receives commands from a ***stream-based UNIX domain socket*** created at the path `/sudoku.sock` (on the server). The commands and the corresponding responses are the same for both channels, but no welcome messages are sent via the UNIX domain socket. Therefore, you can play with the Sudoku server to get familiar with the commands and responses. With `pwntools`, you can play with our challenge server using the scripts `play.py`([view](https://inp111.zoolab.org/code.html?file=lab09/play.py) | [download](https://inp111.zoolab.org/lab09/play.py)). You can also use the script to upload your solver (pass its path as the first argument to the script).

<span style="color: red">***You are requested to solve the Sudoku challenge in this lab via the UNIX domain socket.***</span> The challenge server allows you to optionally upload a Sudoku solver binary file encoded in base64. The uploaded solver can interact with the challenge server via the UNIX domain socket. The UNIX domain socket reads commands sent from a client to the server and sends the corresponding response to the client.

Command responses for cell number placement and puzzle checks requested from the UNIX domain socket are also displayed on the user terminal. Your solver program may also output messages to `stdout` or `stderr` for you to diagnose your solver implementation.

:::warning
Your solver must be compiled with the `-static` option.
:::

The commands used for the challenge server are listed below for your reference.
```
- H: Show help message.
- P: Print the current board.
- V: Set value for a cell. Usage: V [row] [col] [val]
- C: Check the answer.
- S: Serialize the current board.
- Q: Quit.
```
Most commands do not have arguments except the `V` command. The `V` command sets the value for an initially empty cell. Note that the server does not check the correctness of a filled value. You have to invoke the `C` command to perform the check. A success message is displayed if all the empty cells have been filled and the filled values pass the check.

## Demonstration

- [30 pts] Your solver can connect to the UNIX domain socket. The challenge server shows a message to indicate that a client has been successfully accepted.

- [30 pts] Your solver can place numbers on empty cells. Each cell is worth 1 pt.

- [40 pts] Your solver can solve the sudoku puzzle and pass the check. You have to repeat running your solver four times, and each successful run is worth 10 pts.

