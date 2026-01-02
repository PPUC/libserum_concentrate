# libserum

This is a cross-platform library for decoding Serum files, a colorization format for pinball ROMs.

Originally, libserum has been created by [Zed](https://github.com/zesinger).
Since he quit to maintain his original version, this (friendly) fork of the [original libserum](https://github.com/zesinger/libserum) is the official successor where bugfixing and the development of new features is happening.

Some of these new features are
- support of the cROMc format which leads to faster loading times and a way lower memory footprint
- better support for iOS and Android
- rotation scenes
- monochrome mode for not colorized frames like error messages, system diagnostics, settings menu, tools like motor adjustments, coin door open warnings or older or patched ROM versions
