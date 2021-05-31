GS1 Barcode Encoders Library
============================

The GS1 Barcode Encoder Library is a native C library that supports the generation of GS1 barcode symbols.

The supported symbologies are:

* GS1 DataBar family
* GS1-128
* UPC and EAN
* 2D Composite Components are supported for each of the above.
* Data Matrix (including GS1 DataMatrix)
* QR Code (including GS1 QR Code)

The library is a robust barcode generation implementation that can be integrated into the widest variety of settings.

It is intended to facilitate the development of robust barcode generation applications and GS1 data processing workflows without being a turnkey solution to any particular problem.

This project includes a simple console application that demonstrates how to use the library to manually create one-off images of GS1 barcodes.


License
-------

Copyright (c) 2000-2021 GS1 AISBL

Licensed under the Apache License, Version 2.0 (the "License"); you may not use
this library except in compliance with the License.

You may obtain a copy of the License at:

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed
under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
CONDITIONS OF ANY KIND, either express or implied. See the License for the
specific language governing permissions and limitations under the License.


Documentation
-------------

The library API is fully documented in the src/c-lib/docs/ directory.

Instructions for getting started with the console application are provided in the [Console Application User Guide](https://github.com/gs1/gs1-encoders/wiki/User-Guide).


Using the library
------------------

The library is provided with full source and also in the form of a pre-built library (portable DLL) along with associated development headers (.h) and linker (.lib) files.

The license is permissive allowing for the source code to be vendored into an application codebase (Open Source or proprietary) or for the pre-built shared library to be redistributed with an application.

| Directory  | Purpose                                                                                                                                        |
| ---------- | ---------------------------------------------------------------------------------------------------------------------------------------------- |
| src/c-lib  | Source for the native C library ("The library"), unit tests and console application                                                            |
| src/dotnet | Source for an early prototype C# .NET WPF application that uses a p/invoke wrapper class to provide a managed code interface to native library |

Note: The .NET wrapper class and desktop application is provided as a technical demonstration of how the native library can be wrapped for use in managed code settings.


### Building on Windows

The library and demonstration application can be rebuilt on Windows using MSVC.

The project contains a solution file (.sln) compatible with recent versions of Microsoft Visual Studio. In the Visual Studio Installer you will need to ensure that MSVC is installed by selecting the "C++ workload".

Alternatively, it can be built from the command line by opening a Developer Command Prompt, cloning this repository, changing to the `src/c-lib` directory and building the solution using:

    msbuild /p:Configuration=release gs1encoders.sln


### Building on Linux

The library and demonstration application can be rebuilt on any Linux system that has a C compiler (such as gcc or clang).

To build using the default compiler change into the `src/c-lib` directory and run:

    make

A specific compiler can be chosen by setting the CC argument for example:

    make CC=gcc

    make CC=clang

There are a number of other targets that are useful for library development purposes:

    make test [SANITIZE=yes]  # Run the unit test suite, optionally building using LLVM sanitizers
    make fuzzer               # Build fuzzers for exercising the individual encoders. Requires LLVM libfuzzer.


Installing the Pre-built Demonstration Application
--------------------------------------------------

A demonstration console application is provided in the form of an .EXE file compatible with modern 64-bit Windows operating systems and as a .bin file compatible with 64-bit Linux operating systems. There are no installation dependencies and the file can be run from any location on the file system.

The most recent version of the console application can be [downloaded from here](https://github.com/gs1/gs1-encoders/releases/latest).

For Windows systems download the asset named `gs1encoders.exe`. For Linux systems download the asset named `gs1encoders-linux.bin`. In the event of issues with antivirus software consult the note in the [User Guide](https://github.com/gs1/gs1-encoders/wiki/User-Guide).
