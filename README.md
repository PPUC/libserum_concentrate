# libserum
This is a cross-platform library for decoding Serum files, a colorization format for pinball ROMs.

## License 
The code in this directory and all sub-directories is licenced under GPLv2 (or later), except if a different license is
mentioned in a file's header or in a sub-directory. Be aware of the fact that your own enhancements of libserum need to
be licenced under a compatible licence.

Due to complicated dependency management on different platforms, these libraries are included as source code copy:
* [miniz-cpp](https://github.com/tfussell/miniz-cpp) by Thomas Fussel

## Compiling

#### Linux or macOS
```shell
cmake -DCMAKE_BUILD_TYPE=Release -B build/Release
cmake --build build/Release
```

#### Windows
Use Visual Studio.
