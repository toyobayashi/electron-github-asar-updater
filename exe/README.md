# updater

## Environment

* Linux target: Windows 10 + WSL (make, gcc, cmake) + VSCode
* Windows target: Windows 10 + Visual Studio 2019 + CMake

## Quick Start

Windows:

``` bat
> .\script\configure.bat
```

Then open `.sln` with Visual Studio.

Linux

``` bash
$ ./script/configure.sh
$ cd ./out/linux/debug
$ make
```
