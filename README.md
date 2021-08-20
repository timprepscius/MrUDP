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
  * Send coalescing
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
  
  * reliable stream rather than reliable packets.
  
  While implementing the send coalescing, I realized it should be trivial to do send coalescing in packet mode or stream mode.  Stream mode just fills up the last packet before writing another, packet mode will never break packets.  Stream mode could obviously be used to send packets larger than the MAX_PACKET_LENGTH
  
