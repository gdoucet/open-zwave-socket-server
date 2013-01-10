open zwave socket server
========================

This is a simple open zwave socket server allowing communication with the Zwave protocol. Also allowing for a command to be executed on events, in my specific case this is a PHP script hence the cli.php file as an example.

Building
========

Only tested on ubuntu, but you will need the following packages:

    sudo apt-get install build-essential g++ libconfig-dev subversion

Clone the project

    git clone git://github.com/x1nick/open-zwave-socket-server.git

You will also need open-zwave in the parent folder

    svn checkout http://open-zwave.googlecode.com/svn/trunk/ open-zwave-read-only open-zwave

Simply use the Makefile included

    cd open-zwave-socket-server
    make


Terms
=====

I take no responsibility for any issues caused by this code. 


Acknowledgements
================

Thanks goes out to the following developers for developing the initial part of this application

tagroup (http://thomasloughlin.com)

    https://github.com/tagroup/open-zwave-tcp-socket-server-and-client

http://conradvassallo.com/

    https://code.google.com/p/open-zwave-controller/