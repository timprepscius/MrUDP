MrUDP
======

MrUDP is an implementation of a reliable transport mechanism over UDP.
It is cross-platform, written in C++ and designed to be a general purpose communication library.

## Protocol Features

MrUDP has many benefits when compared to TCP:

  * Parallel streams of reliable and unreliable application data.
  * Survives a change in the clients' IP address or port.
  * Connection RTT (round trip time) measurements in realtime.
  * Single ports can have infinite connections to and from other ports.
  * One socket can both accept and connect.
  * Encryption using OpenSSL: RSA Handshake -> AES Session Keys
  * Extremely small code base
  * Provided backend uses boost asio for cross platform sockets.
  
## Building
```
git clone https://github.com/timprepscius/mrudp
mkdir mrudp/build
cd mrudp/build
cmake .. -DUSE_BOOST=/where/ever/you/have/boost/boost
make
```

## In Progress
  
  * UDP send and receive coalescing support.

  It is not clear exactly how I want to implement this.  It could be purely send side, or it could be on the receive side with acks becoming multiple acks.
  Acks and sends will need more metadata, as to "time within coalescing queue" so that ping times are calculated correctly.

