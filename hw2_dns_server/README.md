# INP111 Homework 02 Week #15 (2022-12-15)

Date: 2022-12-15
<span style="color:red">Due Date: 2023-01-05</span>

[TOC]

# Your Own Domain Name Server

This homework aims to implement a Domain Name Server (DNS). Your goal is to implement a DNS server that can correctly respond when users ask for a DNS record of a particular domain name. For more details about the domain system and protocol, please refer to [RFC 1035](https://www.rfc-editor.org/rfc/rfc1035).

## Brief Descriptions of the Specification
You have to read [RFC 1035](https://www.rfc-editor.org/rfc/rfc1035) carefully for the details of the query and response messages format of the DNS server. Here we simply provide brief descriptions of the behaviors you must implement in this homework.

:::success
Additional remarks on the behavior of the DNS server are summarized as follows. These remarks might make your implementation simpler.

- First, your DNS server has to read a configuration file containing the configuration of the DNS server, such as domain information and IP address of the foreign name server. An example of this configuration file is provided in the Configuration Files section. You can use the example file to test your implementation.

- Your DNS server should respond to a query if a queried domain is handled by your server (based on the configuration).

- For domains not handled by your server, your DNS server has to forward the request to the configured foreign server.
:::

:::danger
**Note**: You only need to handle protocol messages delivered in UDP.
:::

## Protocol Messages
The protocol message format used by DNS servers is defined in [RFC 1035](https://www.rfc-editor.org/rfc/rfc1035) and section 2 of [RFC 3596](https://www.rfc-editor.org/rfc/rfc3596).

:::success
In this homework, you only need to implement standard queries and responses. You do not need to consider the situation when OPCODE is not 0.
:::


## Type Values
The answer, authority, and additional sections share the same resource record format in RFC 1035. The type fields are used in resource records to specify the meaning of the data in each resource record. You only need to implement the types required in this homework. The required types and their meaning are listed below:

```
A     : a host address in IPv4
AAAA  : a host address in IPv6
NS    : an authoritative name server 
CNAME : the canonical name for an alias
SOA   : marks the start of a zone of authority
MX    : mail exchange
TXT   : text strings 
```


## Additional Requirement: A nip.io like service
nip.io is a service that resolves any IP address you want from the domain. For example, if you query `127.0.0.1.nip.io`, you get the answer like `127.0.0.1.nip.io.       21600   IN      A       127.0.0.1`. You can change the `127.0.0.1` to any IP address you want.

You need to implement a similar service in this homework.

If the query is match `/^([0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3})\.([0-9a-zA-Z]{1,61}\.)*{YOUR DOMAIN}$/`, your service should answer the A record that address is the `([0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3})` part of the query and TTL is `1`. 

For example, suppose that your domain is `inplab.io`, and your server receives a query `87.87.87.87.abc.inplab.io`. Your server should respond `87.87.87.87.abc.inplab.io 1 IN A 87.87.87.87`.


## Configuration Files
The format of the configuration file of this homework is defined as follows.

```
<forwardIP>
<domain 1>,<path of zone file 1>
<domain 2>,<path of zone file 2>
...
```

## Zone Files
The format of a zone file containing records of a domain is defined as follows.
<!--Each zone file defines the domain information of a specific domain declared in the config file. -->

```
<domain>
<NAME>,<TTL>,<CLASS>,<TYPE>,<RDATA>
<NAME>,<TTL>,<CLASS>,<TYPE>,<RDATA>
...
```
    
The format of <RDATA> for each RR type is summarized below.
    
```
A     : <ADDRESS>
AAAA  : <ADDRESS>
NS    : <NSDNAME>
CNAME : <CNAME>
SOA   : <MNAME> <RNAME> <SERIAL> <REFRESH> <RETRY> <EXPIRE> <MINIMUM>
MX    : <PREFERENCE> <EXCHANGE>
TXT   : <TXT-DATA>
```

:::danger
**Note:** We will use a different configuration file of the same format to test your implementation.
**Note:** All the used configuration files are always in the correct format. You do not need to handle the error case. 
**Note:** The sample configuration file ([config.txt](https://drive.google.com/file/d/1a9gbvuZXD4FMjKcnwDd_d7FKXxjkfkrZ/view?usp=sharing)) and zone files ([zone-example1.org.txt](https://drive.google.com/file/d/1aV7dhbNsmFp8EAQf3-w7IdLuLP_mSNwF/view?usp=sharing) | [zone-example2.org.txt](https://drive.google.com/file/d/14hVb7dMi-asMzZfAR_HjwsEUTksH3d33/view?usp=sharing)) are avaible here. You can use them to test your implementation.
:::

## The DNS Client
**dig** (Domain Information Groper) is a tool we introduced in the lecture before. It performs DNS lookups and displays the answers from the queried name server(s). We use it to test your implementation. You can find the details in the demonstration below or check its [main page](https://www.ibm.com/docs/en/aix/7.1?topic=d-dig-commandhttps:).


## Demonstration
Your DNS server must accept two arguments. Assume your program is named `dns`, the command argument format is `./dns <port-number> <path/to/the/config/file>`.     

Suppose the server receives quires from UDP port 1053 on localhost, and the IP address of the foreign name server is 8.8.8.8. You may run the following commands to test each of the test cases listed in the grading policy: 

```
$ dig @<server_ip> -p <port> <domainname in its domain>
```

![](/uploads/upload_dc139c893121d68eb96dabce8528cc98.PNG)

![](/uploads/upload_ff7f720ffc62d8d0734e0831a1631ea4.PNG)
    
![](/uploads/upload_d51da9f6e8613f35cf69d85be3aea518.PNG)

![](/uploads/upload_2fe3bd665aa0016628ea270389439791.PNG)


```
$ dig @<server_ip> -p <port> <domainname not in its domain>
```

![](/uploads/upload_273f994f0345d779a2a026bd4a77ce7c.PNG)


```
$ dig @<server_ip> -p <port> <domainname not in its domain> A
```

![](/uploads/upload_e795aaccb29f7f3835c6a974d6154c44.PNG)


```
$ dig @<server_ip> -p <port> <domainname not in its domain> AAAA
```

![](/uploads/upload_d50aa9c2b561483b3156285f00415fb7.PNG)

```
$ dig @<server_ip> -p <port> <domainname not in its domain> NS
```

![](/uploads/upload_6cc11aa429a7a9ea6b9346e629d3e0ef.PNG)

```
$ dig @<server_ip> -p <port> <domainname not in its domain> CNAME
```

![](/uploads/upload_ccaace14576362510fe86c5a1451874c.PNG)

```
$ dig @<server_ip> -p <port> <domainname not in its domain> SOA
```

![](/uploads/upload_de6037e91f158374187f29d9b2ed9ca2.PNG)


```
$ dig @<server_ip> -p <port> <domainname not in its domain> MX
```

![](/uploads/upload_71811168747c43aab50eb0fc9a617739.PNG)

```
$ dig @<server_ip> -p <port> <domainname not in its domain> TXT
```

![](/uploads/upload_c4ea5eeb13414973bba04fb9e41efb23.PNG)

:::success
You can play with our server with the dig commands:
```
dig @inp111.zoolab.org -p 10013 <domain name> <RR type>
```
:::


# Grading Policy
    
:::success
    
Please download test cases at first and put them in the right place before you start demonstration: [config.txt](https://inp111.zoolab.org/hw02/config.txt)、[zone-demo1.org.txt](https://inp111.zoolab.org/hw02/zone-demo1.org.txt)、[zone-demo2.org.txt](https://inp111.zoolab.org/hw02/zone-demo2.org.txt)
    
:::
    
[25%] Your server can give a correct host address when the requested domain name is in its domain.

[25%] Your server can forward the domain name to the foreign server when your server does not handle the requested domain name. In this way, your server can still receive the correct host address.

[35%] Your server can handle the 7 type values listed above. Your server can filter resource records based on different type values (each type worth 5 points).

[15%] Your server needs to return A record response when the query meets the format `/^([0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3})\.([0-9a-zA-Z]{1,61}\.)*{YOUR DOMAIN}$/` . The detailed description of this part is introduced in [the section](#Additional-Requirement-A-nipio-like-service).
    
    