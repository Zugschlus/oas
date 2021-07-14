# oas
Output Address Selection
## Purpose
Select the output address based on cmdline.
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

## Installation
1. Build oas.so with ./compile
2. Place it at your convenience
3. Let the environment variable LD_PRELOAD point to it
4. Create a configuration file
5. Let the environment variable OAS_CONF_F point to it
6. Start your programm

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


