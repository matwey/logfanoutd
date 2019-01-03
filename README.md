[![Build Status](https://semaphoreci.com/api/v1/projects/4991dcb7-bb37-4b56-a9e8-277a454b2ff1/483984/badge.svg)](https://semaphoreci.com/matwey/logfanoutd)
[![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/matwey/logfanoutd.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/matwey/logfanoutd/context:cpp)

# logfanoutd
logfanoutd is a simple HTTP server able to handle range requests as per [RFC7233] powered by [libmicrohttpd] library.
Conventional text log file can be thought as of persistent message queue and then HTTP range requests can be used as mechanism of message consumption relying on client performance.
Range requests allow the multiple remote clients to fetch existing log file as well as to continuously request newly appended data with the pace which the client wishes.
Using [libmicrohttpd] allows to achieve low resource requirements for the server application.

## Building
Following components are required to build logfanoutd:
* [cmake] - cross-platform open-source build system;
* [libmicrohttpd] - small C library to run an HTTP server;
* [check] - unit testing framework for C;
* [libcurl] - multiprotocol file transfer library.

Then, the application can be compiled as the following:
```
mkdir build && cd build
cmake ..
make all test
```

## Running
To run the server, the following options are available
* ```--port``` - specify port number to listen on
* ```--root_dir``` - specify root directory to serve
* ```--listen``` - specify addr to listen on (optional)

```
logfanout --port=8014 --root_dir=/var/log/remote
```

Or listen on loopback interface only:
```
logfanout --listen=127.0.0.1 --port=8014 --root_dir=/var/log/remote
```

## Development
Any pull-requests to the project are always welcome.

[libmicrohttpd]:http://www.gnu.org/software/libmicrohttpd/
[RFC7233]:https://tools.ietf.org/html/rfc7233
[cmake]:http://www.cmake.org/
[check]:http://check.sourceforge.net/
[libcurl]:http://curl.haxx.se/libcurl/

