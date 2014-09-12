ringfile
========

A tool and library for writing and reading fixed size circular log files.

On the command line:

    echo "Hello, World!" | ringfile -s 500m --append /var/log/my_service.log

Reading:

    ringfile /var/log/my_service.log

Stat:

    ringfile --stat /var/log/my_service.log

Writing a file:

    char * message = "Hello, World!"
    RINGFILE * f = ringfile_open("hello.bin", "a")
    ringfile_resize(f, 100 * 1024 * 1024);
    ringfile_write(message, strlen(message), f)
    ringfile_close(f);

Reading example:

    RINGFILE * f = ringfile_open("hello.bin", "r");
    while (!ringfile_eof(f)) {
      size_t size = ringfile_next_record_size(f);
      char * buffer = malloc(size);
      ringfile_read(buffer, size, f);
    }

TODO
----

- support incremental reading/writing
- support terminator delimited not just length delimited
- make the file format work on different endiannesses
- clear up confusion about integer types uint64/uint32/size_t/ssize_t/int
- ~~implement c wrapper~~
- export c++ class
- example use with syslog-ng, etc
- write some text about motivations etc
- support sparse files (this may be a bad idea)
- dont corrupt the file on partial writes
- ~~python interface/implementation~~