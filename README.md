GS1 Barcode Engine
==================

The GS1 Barcode Engine is a native C library that supports the generation of
GS1 barcode symbols and processing of GS1 Application Identifier data. It
includes bindings for C# .NET and demonstration console and desktop
applications.

The supported symbologies are:

* GS1 DataBar family
* GS1-128
* UPC and EAN
* 2D Composite Components are supported for each of the above.
* Data Matrix (including GS1 DataMatrix)
* QR Code (including GS1 QR Code)

The library is a robust barcode generation and GS1 data processing
implementation that is intended to be integrated into the widest variety of
settings.

This project includes:

  * A native C library that can be vendored into third-party code or compiled for use as a shared library (Linux / BSD) or dynamic-link library (Windows).
  * An example console application whose code demonstrates how to use the native library to generate barcode symbols.
  * A C# .NET wrapper class that provides an object interface to the native library from managed code, using Platform Invoke (P/Invoke).
  * An example .NET desktop application using Windows Presentation Foundation (WPF) that demonstrates how to use the C# .NET wrapper to access the native library.


License
-------

Copyright (c) 2000-2021 GS1 AISBL

Licensed under the Apache License, Version 2.0 (the "License"); you may not use
this library except in compliance with the License.

You may obtain a copy of the License at:

<http://www.apache.org/licenses/LICENSE-2.0>

Unless required by applicable law or agreed to in writing, software distributed
under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
CONDITIONS OF ANY KIND, either express or implied. See the License for the
specific language governing permissions and limitations under the License.


Documentation
-------------

The C library API is fully documented in the docs/ directory and is
available online here: <https://gs1.github.io/gs1-barcode-engine/>

Instructions for getting started with the console application are provided in
the [Console Application User Guide](https://github.com/gs1/gs1-barcode-engine/wiki/Console-Application-User-Guide).

Instructions for getting started with the GUI application are provided in
the [Desktop Application User Guide](https://github.com/gs1/gs1-barcode-engine/wiki/Desktop-Application-User-Guide).


Using the library
------------------

The library is provided with full source and also in the form of a pre-built
library (portable DLL) along with associated development headers (.h) and
linker (.lib) files.

Pre-built assets are available here:

<https://github.com/gs1/gs1-barcode-engine/releases/latest>

The license is permissive allowing for the source code to be vendored into an
application codebase (Open Source or proprietary) or for the pre-built shared
library to be redistributed with an application.

This repository contains:

| Directory      | Purpose                                                                                           |
| -------------- | ------------------------------------------------------------------------------------------------- |
| src/c-lib      | Source for the native C library ("The library"), unit tests, fuzzers and demo console application |
| docs           | Documentation for the public API of the native C library                                          |
| src/dotnet-lib | C# .NET wrappers that provide a managed code interface to the native library using P/Invoke       |
| src/dotnet-app | A demo C# .NET application (WPF) that uses the wrappers and native library                        |


### Building on Windows

The library, wrappers and demonstration applications can be rebuilt on Windows
using MSVC.

The project contains a solution file (.sln) compatible with recent versions of
Microsoft Visual Studio. In the Visual Studio Installer you will need to ensure
that MSVC is installed by selecting the "C++ workload" and that a recent .NET
Core SDK is available.

Alternatively, all components can be built from the command line by opening a
Developer Command Prompt, cloning this repository, changing to the `src`
directory and building the solution using:

    msbuild /p:Configuration=release gs1encoders.sln


### Building on Linux

The library and demonstration application can be rebuilt on any Linux system
that has a C compiler (such as gcc or clang).

To build using the default compiler change into the `src/c-lib` directory and run:

    make

A specific compiler can be chosen by setting the CC argument for example:

    make CC=gcc

    make CC=clang

There are a number of other targets that are useful for library development
purposes:

    make test [SANITIZE=yes]  # Run the unit test suite, optionally building using LLVM sanitizers
    make fuzzer               # Build fuzzers for exercising the individual encoders. Requires LLVM libfuzzer.


Installing the Pre-built Demo Console Application
-------------------------------------------------

A demonstration console application is provided in the form of an .EXE file
compatible with modern 64-bit Windows operating systems and as a .bin file
compatible with 64-bit Linux operating systems. There are no installation
dependencies and the file can be run from any location on the file system.

The most recent version of the console application can be
[downloaded from here](https://github.com/gs1/gs1-barcode-engine/releases/latest).

For Windows systems download the asset named
`gs1encoders-windows-console-app.zip`. For Linux systems download the asset
named `gs1encoders-linux.bin`. In the event of issues with antivirus software
consult the note in the
[User Guide](https://github.com/gs1/gs1-barcode-engine/wiki/Console-Application-User-Guide).

The pre-built application requires that the Visual C++ Redistributable 2019 (32
bit) is installed: <https://visualstudio.microsoft.com/downloads/#microsoft-visual-c-redistributable-for-visual-studio-2019>


Installing the Pre-built Demo GUI Application
---------------------------------------------

A demonstration GUI application is provided in the form of an .EXE file
compatible with modern 64-bit Windows operating systems and a recent .NET
Framework.

The most recent version of the GUI application can be
[downloaded from here](https://github.com/gs1/gs1-barcode-engine/releases/latest).

For Windows systems download the asset named `gs1encoders-windows-gui-app.zip`. In
the event of issues with antivirus software consult the note in the
[User Guide](https://github.com/gs1/gs1-barcode-engine/wiki/Desktop-Application-User-Guide).

The pre-built application requires that the .NET Core 3.1 Desktop Runtime -
Windows x86 is installed: <https://dotnet.microsoft.com/download/dotnet/3.1/runtime>
