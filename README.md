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

  * Optimize for maximal throughput and minimal latency.
  
Currently overlapped IO on windows and linux is not implemented.  Target date is August 15th.  OSX does not support this feature (AFIK)
Overlapped IO's performance benefits are large.
  
  * UDP send and receive coalescing support.

It is not clear exactly how I want to implement this.  It could be purely send side, or it could be on the receive side with acks becoming multiple acks.
Acks and sends will need more metadata, as to "time within coalescing queue" so that ping times are calculated correctly.

  * Encryption
  
I plan to implement an RSA handshake transferring AES keys.  All packets except for handshake should be encrypted past a session identifier.

  * Handles
  
Handles are a bit strange right now.  I'm not sure if I want to keep it the way it is, or change it.

Basically, a handle is a pointer to a strong_ptr to whatever object the handle references.
This way, when referencing a connection or a socket or the service, there is no mutex that needs to be locked in order to traverse the handle. 
 If I were to change the pointers into pure number/file handles, then in order to send a packet on a connection, a global mutex which would 
 surround the file handle look up table would need to be momentarily locked.
 
Perhaps there is a way around this.  For the time being, I will continue to use the pointers with no global mutex.
