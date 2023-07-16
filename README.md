Readme for overlaytool
==================

[![License](https://img.shields.io/badge/license-BSD%203--Clause-blue.svg?style=flat-square)](https://github.com/mikaelsundell/logctool/blob/master/README.md)

Introduction
------------

overlaytool is a utility for creating overlay images.

Documentation
-------------

Building
--------

The overlaytool app can be built both from commandline or using optional Xcode `-GXcode`.

```shell
mkdir build
cd build
cmake .. -DCMAKE_MODULE_PATH=<path>/logctool/modules -DCMAKE_PREFIX_PATH=<path>/3rdparty/build/macosx/arm64.debug -GXcode
cmake --build . --config Release -j 8
```

Packaging
---------

The `macdeploy.sh` script will deploy mac bundle to dmg including dependencies.

```shell
./macdeploy.sh -e <path>/logctool -d <path>/dependencies -p <path>/path to deploy
```

Web Resources
-------------

* GitHub page:        https://github.com/mikaelsundell/overlaytool
* Issues              https://github.com/mikaelsundell/overlaytool/issues

