# Prerequisites

These are the base requirements to build and use qsfs from a source package (as described below): 
- GNU Compiler Collection (GCC) 5.4.0 or later
- CMake v3.5 or later

**qsfs** is integrated with QingStor via the [QingStor SDK for C++][qs-sdk-cpp link], so you need to get the QingStor SDK for C++ installed at first.

Additional Requirements
To compile on Linux, you must have the header files (-dev packages) for libfuse, libglog, libgflags, libgtest.

Typically, you'll find the packages in your system's package manager.

To install these packages on Ubuntu16.04:
```sh
 $ [sudo] apt-get install build-essential
 $ [sudo] apt-get install fuse g++ git gflags glog gtest
```

To install these packages on Debian:
```sh
 $ 
```

To install these packages on CentOS 7:
```sh
 $ [sudo] yum install cmake fuse fuse-devel gcc-c++ git
```

# Build from Source using CMake

Clone the qsfs source from [yunify/qsfs][qsfs github link] on Github
```sh
 $ git clone https://github.com/jimhuaang/qsfs.git
```

Enter the project directory containing the package's source code
```sh
 $ cd qsfs
```

It is always a good idea to not pollute the source with build files,
so create a directory in which to create your build files.
```sh
 $ mkdir build
 $ cd build
```

Run cmake and make to build
```sh
 $ cmake ..
 $ make
```

Running unit tests
```sh
 $ make test
```
    or
```sh
 $ ctest -R qsfs -V
```
Running integration tests:

Several directories are appended with *integration-tests. After building your project, you can run these executables to ensure everything works properly.

Install the programs and any data files and documentation
```sh
 $ [sudo] make install
```

You can remove the program binaries and object files from the source code directory
```sh
 $ make clean
```

To remove the installed files
```sh
 $ [sudo] make uninstall
```


[qsfs github link]: [https://github.com/jimhuaang/qsfs]
[qs-sdk-cpp link]: [https://]
