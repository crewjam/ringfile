ringfile
========

A tool (and library) for writing and reading fixed size circular log files using
familiar unix semantics. You pipe lines into the ringfile command and read them
out again:

    echo "Hello, World!" | ringfile --size=500m --append /var/log/my_service.log

    ringfile /var/log/my_service.log

When your consume all the space in your ring file the oldest records are 
replaced by new ones.

Background
----------

Managing log files is a pain in the butt. The standard unix-y way of handling 
log files is to have a directory full of log files (/var/log) and a daemon that is
responsible for routing and filtering and eventually appending a log record to 
a file. This works nicely except that log files grow and grow and if you don't
do something about it they consume all the disk space. 

So people built tools like logrotate to manage deleting old logs and replacing 
them with new ones. The problem with this is that we usually run logrotate by
cron with the assumption that our cron interval is short enough that the log 
files never grow too large. 

This approach works until everything breaks. We've had occasions where a failure
causes services to vomit into logs and the log vomit fills up disks which causes
cascading failures. 

Ringfile is motivated by the apparent lack of simple solutions to this problem.
Although its use is not limited to logs, capturing logs is our first use case.

Configuration examples
----------------------

You can just pipe the output of any old program to ringfile as you would 
logger. For example:

    LOG_SIZE=256m
    . /etc/default/mydaemon
    mydaemon --daemonize 2>&1 | ringfile --size=$LOG_SIZE --append /var/log/mydaemon
  
Some services log only to a file. To capture those logs, you could try using a 
FIFO to capture the output and a ringfile process capturing that output:

    mkfifo /opt/mything/log/system.log
    ringfile --size=256m --append /var/log/mything.ring < /opt/mything/log/system.log &
    /opt/mything/bin/mything --log-file=/opt/mything/log/system.log

You might want to route system logs to a circular log file which would eliminate
the need to manage log rotation. With syslog-ng this might look like:

    destination d_ring {
      program("/usr/local/bin/ringfile --size=256m --append /var/log/ring" 
        flags(no_multi_line)
        template("<$PRI>$DATE $HOST $MSG\n"));
    };

Library
-------

There are C and Python bindings for ringfile. To create a new ring and write a 
record to it:

    char * message = "Hello, World!"
    
    RINGFILE * f = ringfile_create("hello.bin", 4096)
    ringfile_write(message, strlen(message), f)
    ringfile_close(f);


The same thing in python:

    import ringfile
    f = ringfile.Ringfile.Create("hello.bin", 4096)
    f.write("Hello, World!")
    f.close()

Read all records from a file:

    RINGFILE * f = ringfile_open("hello.bin", "r");
    while (!ringfile_eof(f)) {
      size_t size = ringfile_next_record_size(f);
      char * buffer = malloc(size);
      ringfile_read(buffer, size, f);
    }

    
Read all records from a file (Python):

    import ringfile
    f = ringfile.Ringfile("hello.bin")
    for record in f:
      print record

File format
-----------

Each file starts with a header containing the following fields:

 - a 4-byte magic number `RING`
 - a 4-byte flags field which is currently unused and must be set to 0.
 - an 8-byte little endian offset to the first record in the file
 - an 8-byte little endian offset to the end of the last record in the file
 
Note: the file offsets are relative to the end of the header, not the 
beginning of the file.

Each record consists of:
 
  - a 4-byte little endian length of the data
  - the data

TODO
----

- ~~support incremental reading~~
- ~~support incremental writing~~
- support terminator delimited not just length delimited
- make the file format work on different endiannesses
- clear up confusion about integer types uint64/uint32/size_t/ssize_t/int
- ~~implement c wrapper~~
- export c++ class
- ~~example use with syslog-ng, etc~~
- ~~write some text about motivations etc~~
- support sparse files (this may be a bad idea)
- ~~dont corrupt the file on partial writes~~
- ~~python interface/implementation~~
- compresssion?
- ~~support iteration in python interface~~
- variable length encoding for record lengths
