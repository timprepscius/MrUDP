MrUDP
======

MrUDP is an implementation of a reliable transport mechanism over UDP.
It is cross-platform, written in C++ and designed to be a general purpose communication library.

## Protocol Features

MrUDP has many benefits when compared to TCP:

  * Parallel streams of reliable and unreliable application data.
  * Connection RTT (round trip time) measurements in realtime.
  * Single ports can have infinite connections to and from other ports.
  * One socket can both accept and connect.
  * Encryption using OpenSSL: RSA Handshake -> AES Session Keys
  * Send coalescing via contiguous data frames or a continuous stream
  * Survives a change in the clients' IP address or port.
  * Provided backend uses boost asio for cross platform sockets.
  * Extremely small code base
  
## Building
```
git clone https://github.com/timprepscius/mrudp
mkdir mrudp/build
cd mrudp/build
cmake .. -DUSE_BOOST=/where/ever/you/have/boost/boost
make
```

## In Progress
