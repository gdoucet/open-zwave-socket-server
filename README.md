open zwave socket server
========================

This is a simple open zwave socket server allowing communication with the Zwave protocol. It uses the protocol to use with the IOS/Android application Lightswitch.

More about the protocol can be found here:
http://forum.melloware.com/viewtopic.php?f=15&t=7977


Known Issues
============

Only Binary Switch and Multilevel switch is supported. 
It does not update the client with UPDATE command.

Need to modify open-zwave/cpp/src/Defs.h RETRY_TIMEOUT from 40000 to 2000 (https://code.google.com/p/open-zwave/issues/detail?id=164) due to slow performance.



Building
========

Only tested on ubuntu, but you will need the following packages:

    sudo apt-get install build-essential g++ libconfig-dev libudev-dev subversion

Clone the project

    git clone git://github.com/gdoucet/open-zwave-socket-server.git

You will also need open-zwave in the parent folder

    svn checkout http://open-zwave.googlecode.com/svn/trunk/ open-zwave

Simply use the Makefile included

    cd open-zwave-socket-server
    make


Terms
=====

I take no responsibility for any issues caused by this code. 


Acknowledgements
================

Thanks goes out to the following developers for developing the initial part of this application

phillipsnick
	https://github.com/phillipsnick/open-zwave-socket-server

tagroup (http://thomasloughlin.com)

    https://github.com/tagroup/open-zwave-tcp-socket-server-and-client

http://conradvassallo.com/

    https://code.google.com/p/open-zwave-controller/