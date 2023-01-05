title: Web Crawler
value: 50
description: There is an internal web server running at `http://localhost:10000`. Can you read the flag from that server?

`nc inp111.zoolab.org 10008`

Click <a href="https://inp111.zoolab.org/code.html?file=lab07/webcrawler.cpp" target="_blank">here</a> to view the source code of this challenge online, or download it from [here](https://inp111.zoolab.org/lab07/webcrawler.cpp). You need two additional arguments `-std=c++20` and `-lcrypto` to compile the server.