GS1 barcode encoder application
===============================

This is a simple console application for manually creating one-off images of certain types of barcode.

The supported symbologies are:

* GS1 DataBar family
* GS1-128
* UPC and EAN

2D Composite Components are supported for each of the above.

The barcode data can either be manually keyed or read from a file and the images are generated in either BMP or TIFF format.


Installation
------------

The application is provided in the form of an .EXE file compatible with modern 64-bit Windows operating systems and as a .bin file compatible with 64-bit Linux operating systems. There are no installation dependencies and the file can be run from any location on the file system.

The most recent version of the application can be [downloaded from here](https://github.com/gs1/gs1-encoders/releases/latest).

For Windows systems download the asset named `gs1encoders.exe`. For Linux systems download the asset named `gs1encoders-linux.bin`. In the event of issues with antivirus software consult the note in the [User Guide](https://github.com/gs1/gs1-encoders/wiki/User-Guide).


User Guide
----------

Instructions for getting started with the application are provided in the [User Guide](https://github.com/gs1/gs1-encoders/wiki/User-Guide).


License
-------

Copyright (c) 2020 GS1 AISBL

Licensed under the Apache License, Version 2.0 (the "License"); you may not use
this library except in compliance with the License.

You may obtain a copy of the License at:

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed
under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
CONDITIONS OF ANY KIND, either express or implied. See the License for the
specific language governing permissions and limitations under the License.


Building from source
--------------------

The application is also provided in the form of source code that can be rebuilt for alternate operating systems or modified according to your needs.


### Building on Windows

The application can be rebuilt on Windows using MSVC.

The project contains a solution file (.sln) compatible with recent versions of Microsoft Visual Studio. In the Visual Studio Installer you will need to ensure that MSVC is installed by selecting the "C++ workload".

Alternatively, it can be built from the command line by opening a Developer Command Prompt, cloning this repository, changing to the `src` directory and building the project file (.proj) using:

    msbuild /p:Configuration=release

Or you can avoid using a toolchain by running the Microsoft C++ compiler tool directly as follows:

    cl *.c /link /out:gs1encoders.exe


### Building on Linux

The application can be rebuilt on any Linux system that has a C compiler (such as gcc or clang).

To build using the default compiler change into the `src` directory and run:

    make

A specific compiler can be chosen by setting the CC argument for example:

    make CC=gcc

    make CC=clang
