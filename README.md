Readme for overlaytool
==================

[![License](https://img.shields.io/badge/license-BSD%203--Clause-blue.svg?style=flat-square)](https://github.com/mikaelsundell/logctool/blob/master/README.md)

Introduction
------------

overlaytool is a utility for creating overlay images.

![Sample image or figure.](images/image.png 'it8tool')

Building
--------

The overlaytool app can be built both from commandline or using optional Xcode `-GXcode`.

```shell
mkdir build
cd build
cmake .. -DCMAKE_MODULE_PATH=<path>/modules -GXcode
cmake --build . --config Release -j 8
```

**Example using 3rdparty on arm64 with Xcode**

```shell
mkdir build
cd build
cmake ..
cmake .. -DCMAKE_MODULE_PATH=<path>/modules -DCMAKE_PREFIX_PATH=<path>/3rdparty/build/macosx/arm64.debug -GXcode
```

Usage
-----

Print overlaytool help message with flag ```--help```.

```shell
overlaytool -- a utility for creating overlay images

Usage: overlaytool [options] ...

General flags:
    --help                     Print help message
    -v                         Verbose status messages
    -d                         Debug status messages
Input flags:
    --centerpoint              Use centerpoint for overlay
    --symmetrygrid             Use symmetry grid for overlay
    --label                    Use label for overlay
    --aspectratio ASPECTRATIO  Set aspectratio (default:1.5)
    --scale SCALE              Set scale (default: 0.5)
    --color COLOR              Set color (default: 1.0, 1.0, 1.0)
    --size SIZE                Set size (default: 1024, 1024)
Output flags:
    --outputfile OUTPUTFILE    Set output file
```

**Input flags**

The input flags are used to set-up the overlay geometry. 

```--centerpoint``` centerpoint cross added to the center of the aspect ratio geometry   
```--symmetrygrid ``` symmetry grid inside aspect ratio geometry    
```--label ``` label for width, heigh, aspect ratio and scale   
```--scale ``` scale of aspect ratio geometry  
```--color ``` color of geometry   
```--size ``` size of image   

**Output flags**

```--outputfile``` overlay output file


Example overlay image
--------

```shell
./overlaytool
--symmetrygrid
--aspectratio 2.35 
--outputfile overlay.png 
--size "2350,1000" 
--scale 0.8 
```

Download
---------

Overlay is included as part of pipeline tools. You can download it from the releases page:

* https://github.com/mikaelsundell/pipeline/releases

Dependencies
-------------

| Project     | Description |
| ----------- | ----------- |
| Imath       | [Imath project @ Github](https://github.com/AcademySoftwareFoundation/Imath)
| OpenImageIO | [OpenImageIO project @ Github](https://github.com/OpenImageIO/oiio)
| 3rdparty    | [3rdparty project containing all dependencies @ Github](https://github.com/mikaelsundell/3rdparty)

Project
-------------

* GitHub page   
https://github.com/mikaelsundell/overlaytool
* Issues   
https://github.com/mikaelsundell/overlaytool/issues


Resources
---------

* Dynamic Symmetry: The Foundation of Masterful Art   
Author: Tavis Leaf Glover

Copyright
---------

* Roboto font   
https://fonts.google.com/specimen/Roboto   
Designed by Christian Robertson
