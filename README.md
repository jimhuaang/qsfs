# QingStor File System

[![Build Status](https://travis-ci.org/jimhuaang/qsfs.svg?branch=master)][build link]
[![License](http://img.shields.io/badge/license-apache%20v2-blue.svg)][license link]

**qsfs** is a FUSE based filesystem written in modern C++(version C++ 11 or later), that allows you to mount an qingstor bucket in Linux.


## Features

- Large subset of POSIX including reading/writing regular files, directories, symlinks 
  (mode, uid/gid is coming later).
- File permissions:
  - Default permission for a file is 0644 and for a directory is 0755.
  - Default uid/gid of a file is the uid/gid of the owner of the file.
  - Support for sticky bit in file permissions (if set, only owner can delete/rename).
- File Cache
  - A file cache is maintained to store file metadata as well as the actual file data.
  - In-memory metadata caching, you can enable.
  - In-memory & local disk file data caching.
  - File metadata in the file cache is invalidated only after 1 minute.
  - Files are cached as a list of pages, where each page can be of arbitrary size.
  - Every READ and WRITE request to a file is passed through the file cache.
- File Transfer
  - Transfer Manager achieves enhanced throughput, performance, and reliability by making extensive use of multi-threaded multipart uploads.
  - Large files transfer in chunks (10MB chunks) to restrict memory usage for large file uploads or downloads.
  - Large files uploads via multi-part upload and handled by thread pool.
  - Large files downloads via range download and handled by thread pool.
- File renames via server-side copy
- Logging:
  - User can use any of the logging levels like DEBUG, INFO, WARN, ERROR and FATAL, by specifying the *-L* parameter in command line, such as *-L=INFO*.
  - User can log messages to console by specifying forground options *-f*.
  - User can also log messages to log file by specifying log dir parameter *-l*, such as
  *-l=/path/to/logdir/*.  The default location where the logs are stored is */opt/qsfs/qsfs_log/*.
- Thread Pool
  - Every HTTP request to QingStor is handled through thread pool.
  - Some operations such as list objects, rename directory, delete file, upload file is
  handle through thread pool asynchronizely.
- Retry strategy
  - The retry strategy defaults to exponential backoff.
- Security
  - By default, access to the mount directory is restricted to the user who mounted GDFS.
- User-specified regions
  - The region specifies where you want the client to communicate. Examples include pek3a or sh1a. You must ensure the qinstor service you want is availabe in the region you configure.
- Request Timeout
  - This value determines the length of time, in milliseconds, to wait before timing out a request. You can increase this value if you need to transfer large files.
- User Agent
  - The user agent is built in the constructor and pulls information from your operating system. Do not alter the user agent.
- Scheme
  - The default value for scheme is HTTPS. You can set this value to HTTP if the information you are passing is not sensitive and the service to which you want to connect supports an HTTP endpoint.
- Works with the Standard Template Library (STL).
- C++ 11 features used and supported.
- Builds with CMake.


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

Or you can log messages to file by specifying a directory `/path/to/logdir/`:
```sh
 $ [sudo] qsfs -b=mybucket -m=/path/to/mountpoint -c=/path/to/cred -l="/path/to/logdir/"
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

If you notice any issue, please open an [issue][issue link] on Github. Please search the existing issues and see if others are also experiencing the issue before opening a new issue. Please include the version of qsfs, Compiler version, CMake version, and OS you’re using. Please also include repro case when appropriate.

If you have any questions, suggestions or feedback, feel free to send an email to <jimhuang@yunify.com>.


## License

See the [LICENSE][license link] for details. In summary, qsfs is licensed under the Apache License (Version 2.0, January 2004).


[build link]: https://travis-ci.org/jimhuaang/qsfs
[eventual consistency wiki]: https://en.wikipedia.org/wiki/Eventual_consistency
[faq wiki link]: https://github.com/jimhuaang/qsfs/wiki/FAQ
[install link]: https://github.com/jimhuaang/qsfs/blob/master/INSTALL.md
[issue link]: https://github.com/jimhuaang/qsfs/issues
[license link]: https://github.com/jimhuaang/qsfs/blob/master/COPYING