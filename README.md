# oas
Output Address Selection
## Purpose
Select the output address based on cmdline.
## Motivation
oas is a library wrapper around the connect() syscall and can be
used to influence the IPv6 source address for outgoing TCP connections.

In IPv6, nodes might be using multiple IP addresses simultaneously.
In fact, this is the normal case in many networks. Different IP
networks might be configured with different privileges, making it
important to use the correct source IPv6 address for outgoing
connections.

While many programs allow selecting IPv6 source addresses,
others don't. Also, the source address mechanisms provided by
the operating system (see RFC6724, ip addrlabel on Linux) have
some known shortcomings. For example, Linux takes the routing
decision for locally originated datagrams before doing IPv6 source
address selection, which might cause the system to send the
outgoing datagram to the wrong default gateway where it might
be dropped.

oas influences the IPv6 source address of an outgoing connection
early enough for the routing decision to be taken correctly.

## Prerequisites
Any nonhistorical Linux System with /proc and glibc
should do. If it doesn't work on Debian Stable, it's a bug.
## Scope
TCP/IPv6 outgoing Connections. Future versions may support UDP.
There are no plans to implement IPv4 support.
## How does it work?
Basically it is a wrapper around connect(2). Before calling the
real connect(), oas tries to bind an address to the socket, according
to the configuration. If no available address matches the configuration
or the bind(2) fails for any reason, your program should work like
it does without oas.
## When you don't need oas
oas is not needed on a router. When a datagram comes in from
another system, its source address is already set and thus the
in-kernel mechanisms can work as designed.

If all your software allows to set the IPv6 source address, you
probably don't nee oas as well. It might be, however, easier to
have source address selection configured in a central place.
## Installation
1. Build oas.so with ./compile
2. Place it at your convenience
3. Let the environment variable LD_PRELOAD point to it
4. Create a configuration file
5. Let the environment variable OAS_CONF_F point to it
6. Adjust the environment variable OAS_LOGLEVEL to your needs
7. Start your programm

oas is packaged in Debian GNU/Linux (unstable suite) as
liboas0, so just need to apt install liboas0 to take advantage of oas.

## Configuration

Lines starting with # or containing nothing but whitespaces are ignored.
A line starting with
cmdlines:
starts a list of Posix Extended Regular expressions, each on a line of
its own. If your cmdline contains one of them the corresponding address pattern list will be used and all further cmdline-patterns will be ignored. 

A line starting with
addresses:
starts a list of address Pattern, again one per line. If your system has a configured address matching one of these patterns, it is chosen for the output address. 

An address pattern specifies the value of an address to match and 
a mask for relevant bits. The mask may be given as prefix length.
Address and mask are separated by "/".

You may have a look at config_example.

There are currently no sanity checks for the configuration.

## Logging
Logging goes to syslog. You can set the level with the environment variable
OAS_LOGLEVEL. Default is NOTICE. Available are EMERG,ALERT,CRIT,ERR,WARN,NOTICE,INFO and DEBUG.



