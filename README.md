# QingStor File System

[![Build Status](https://travis-ci.org/jimhuaang/qsfs.svg?branch=master)][build link]
[![License](http://img.shields.io/badge/license-apache%20v2-blue.svg)][license link]

**qsfs** is a FUSE based filesystem written in modern C++(version C++ 11 or later), that allows you to mount a qingstor bucket in Linux.


## Features

- Large subset of POSIX including reading/writing files, directories, symlinks 
  (mode, uid/gid is coming later).
- File Permissions:
  - Default permission for a file is 0644 and for a directory is 0755.
  - Default uid/gid of a file is the uid/gid of the user.
  - Support for sticky bit in file permissions (if set, only owner can delete/rename).
- File renames via server-side move.
- File Transfer:
  - Large files uploads via multipart parallel uploads.
  - Large files downloads via parallel byte-range downloads.
  - Large files transfer in chunks (10MB chunks by default). If you are uploading large
  files (e.g. larger than 1GB), you can increase the transfer buffer size and the max
  parallel transfers (5 by default) by specifying *-u* and *-n* option, respectively.
- Cache:
  - In-memory metadata caching. You can specify the expire time (in minutes) by *-e*
  option to invalidate the file metadata in cache. You can specify the max count
  of metadata entrys in cache by *-t* option.
  - In-memory file data caching. You can specify max cache size for file data cache by
  *-Z* option. For a big file, partial file data may been stored in a local disk file
  when the file cache is not available, by default the disk file will be put under
  */tmp/qsfs_cache/*, you can change it by *-D* option.
- Logging:
  - You can specify any of the logging levels like INFO, WARN, ERROR and FATAL by *-L*
  option, e.g. *-L=INFO*.
  - You can log messages to console by specifying forground option *-f*.
  - You can also log messages to log file by specifying log dir option *-l*, e.g.
  *-l=/path/to/logdir/*. The default location where the logs are stored is */opt/qsfs/qsfs_log/*.
- Debugging:
  - You can turn on debug message to log by specifying debug option *-d*, this option
  will also enable FUSE debug mode.
  - You can turn on debug message from libcurl by specifying option *-U*.
- Thread Pooled Executor:
  - Submit tasks to run synchronously or asynchronously.
  - Every HTTP request to QingStor through SDK is handled through executor.
  - Operations which is time-consuming and do not need instant response could be handle
  through exectuor asynchronously, such as list objects when entering a directory, rename
  directory, delete file, upload file, etc. This is default behaviour, you can turn off
  such behaviour by enable qsfs single thread option *-S*.
- Retry strategy:
  - You can specify the retry times to retry a faild transaction by *-r* option.
  - The retry strategy defaults to exponential backoff.
- Request Timeout:
  - This value determines the length of time, in milliseconds, to wait before timing out
  a request.
  - For these requests which is not time-consuming, such as head a file, make an empty
  file, make a folder, delete a file, etc., a default time-out (in milliseconds) value is
  set (500 ms by default). You can increase this value by option *-R*.
  - For these requests which is time-consuming, the request time-out will be evaluated
  dependently, e.g. if you need to transfer large files, the time-out will be depend on the file size and the time-out value set by option *-R*.
- Security:
  - By default, access to the mount directory is restricted to the user who mounted qsfs.
- User-specified regions
  - You can sepcify the zone or region by option *-z*, e.g. *-z=sh1a*, default is pek3a.
  You must ensure the qinstor service you want is availabe in the region you configure.
- Protocol:
  - You can specify the protocol by option *-p*, e.g. *-p=HTTP*, default is HTTPS. You can
  set this value to HTTP if the information you are passing is not sensitive and the service
  to which you want to connect supports an HTTP endpoint.
- Compatibility:
  - Easy to extend to support other object stores such as Amazon S3.


## Installation

See the [INSTALL][install link] for installation instructions.


## Usage

Enter your qingstor access key id and secret key in a file `/path/to/cred`:
```sh
 $ echo MyAccessKeyId:MySecretKey > /path/to/cred
```

Make sure the file `/path/to/cred` has proper permissions (if you get 'permissions' error when mounting):
```sh
 $ chmod 600 /path/to/cred
```

Run qsfs with an existing bucket `mybucket` and directory `/path/to/mountpoint`:
```sh
 $ [sudo] qsfs -b=mybucket -m=/path/to/mountpoint -c=/path/to/cred
```

If you encounter any errors, enable debug output:
```sh
 $ [sudo] qsfs -b=mybucket -m=/path/to/mountpoint -c=/path/to/cred -d
```

You can log messages to console:
```sh
 $ [sudo] qsfs -b=mybucket -m=/path/to/mountpoint -c=/path/to/cred -f
```

Or you can log messages to log file by specifying a directory `/path/to/logdir/`:
```sh
 $ [sudo] qsfs -b=mybucket -m=/path/to/mountpoint -c=/path/to/cred -l=/path/to/logdir/
```

Specify log level (INFO,WARN,ERROR and FATAL):
```sh
 $ [sudo] qsfs -b=mybucket -m=/path/to/mountpoint -c=/path/to/cred -L=INFO -d
```

To umount:
```sh
 $ [sudo] fusermount -uqz /path/to/mountpoint
```
  or
```sh
 $ [sudo] umount -l /path/to/mountpoint
```

For help:
```sh
 $ qsfs -h
```

## Limitations

Generally qingstor cannot offer the same performance or semantics as a local file system.  More specifically:
- Random writes or appends to files require rewriting the entire file
- Metadata operations such as listing directories have poor performance due to network latency
- [Eventual consistency][eventual consistency wiki] can temporarily yield stale data
- No atomic renames of files or directories
- No coordination between multiple clients mounting the same bucket
- No hard links


## Frequently Asked Questions

- [FAQ wiki page][faq wiki link]


## Support

If you notice any issue, please open an [issue][issue link] on GitHub. Please search the existing issues and see if others are also experiencing the issue before opening a new issue. Please include the version of qsfs, Compiler version, CMake version, and OS you’re using.


## License

See the [LICENSE][license link] for details. In summary, qsfs is licensed under the Apache License (Version 2.0, January 2004).


[build link]: https://travis-ci.org/jimhuaang/qsfs
[eventual consistency wiki]: https://en.wikipedia.org/wiki/Eventual_consistency
[faq wiki link]: https://github.com/jimhuaang/qsfs/wiki/FAQ
[install link]: https://github.com/jimhuaang/qsfs/blob/master/INSTALL.md
[issue link]: https://github.com/jimhuaang/qsfs/issues
[license link]: https://github.com/jimhuaang/qsfs/blob/master/COPYING