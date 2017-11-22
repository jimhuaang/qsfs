# Prerequisites

These are the base requirements to build and use qsfs from a source package (as described below): 
- GNU Compiler Collection (GCC) 5.4.0 or later
- CMake v3.0 or later

**qsfs** is integrated with QingStor via the [QingStor SDK for C++][qs-sdk-cpp link], so you need to get the QingStor SDK for C++ installed at first.

### Additional Requirements
qsfs use glog for logging and gtest for unit testing, and qsfs
has used CMake to download and install these dependencies as part of the build's configure step,
so you can just leave them alone.

qsfs is a fuse based filesystem, so you must have libfuse installed.

You may also need to install git in order to clone the source code from
GitHub.

Typically, you'll find these packages in your system's package manager.

To install these packages on Ubuntu14.04:
```sh
 $ [sudo] apt-get install g++ cmake make automake fuse libfuse-dev git
```

To install these packages on CentOS 7:
```sh
 $ [sudo] yum install gcc-c++ cmake make automake fuse fuse-devel git
```

# Build from Source using CMake

Clone the qsfs source from [yunify/qsfs][qsfs github link] on GitHub:
```sh
 $ git clone https://github.com/jimhuaang/qsfs.git
```

Enter the project directory containing the package's source code:
```sh
 $ cd qsfs
```

It is always a good idea to not pollute the source with build files,
so create a directory in which to create your build files:
```sh
 $ mkdir build
 $ cd build
```

Run cmake and make to build, as we install dependencies in cmake configure step, you need root privileges:
```sh
 $ [sudo] cmake ..
 $ [sudo] make
```

Run unit tests:
```sh
 $ [sudo] make test
```
  or
```sh
 $ [sudo] ctest -R qsfs -V
```

Install the programs and any data files and documentation:
```sh
 $ [sudo] make install
```

To clean the generated build files, just remove folder build:
```sh
 $ rm -rf build
```

To clean program binaries and object files, juse remove folder bin and lib:
```sh
 $ rm -rf bin
 $ rm -rf lib
```

To remove all installed files:
```sh
 $ [sudo] make uninstall
```

To only remove installed qsfs files:
```sh
 $ [sudo] make uninstall_qsfs
```

To remove installed depended project's files, such as googletest, googleflags and goolgelog:
```sh
 $ [sudo] make uninstall_googletest
 $ [sudo] make uninstall_googlelog
 $ [sudo] make uninstall_googleflags
```


[qsfs github link]: https://github.com/jimhuaang/qsfs
[qs-sdk-cpp link]: https://git.internal.yunify.com/MorvenHuang/qingstor-sdk-c-and-cpp
