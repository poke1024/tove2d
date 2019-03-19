# TÖVE <img align="right" src="https://github.com/poke1024/tove2d/blob/master/docs/images/tovelogo.png" height="200">
Animated vector graphics for [LÖVE](https://love2d.org/).

## Description
TÖVE is a vector drawing canvas for LÖVE, think HTML5's `<canvas>` or  OpenFL's `Graphics`. Which you can animate. Efficiently.

## Features
* brings crisp vector graphics and SVGs to LÖVE
* offers a substantial subset of SVG
* built for realtime animation
* uses OpenGL ES 
* < 1 MB in binary size

## Capabilities
Do not expect TÖVE to produce print quality vector renderings. This is neither Cairo nor Skia, but a simplified vector graphics library for games. Here's what you can expect from TÖVE's different renderers:

|                     | texture            | mesh (1)           | mesh (2)               | gpux                    |
|---------------------|--------------------|--------------------|------------------------|-------------------------|
| Holes Support       | :heavy_check_mark: | :heavy_check_mark: | :heavy_check_mark: (4) | :heavy_check_mark:      |
| Non-Zero Fill Rule  | :heavy_check_mark: | :heavy_check_mark: | :x:				     | :heavy_check_mark:      |
| Even-Odd Fill Rule  | :heavy_check_mark: | :heavy_check_mark: | :x:    				 | :heavy_check_mark:      |
| Clip Paths          | :heavy_check_mark: | :heavy_check_mark: | :x:                    | :x:                     |
| Solid Colors        | :heavy_check_mark: | :heavy_check_mark: | :heavy_check_mark:     | :heavy_check_mark:      |
| Linear Gradients    | :heavy_check_mark: | :heavy_check_mark: | :heavy_check_mark:     | :heavy_check_mark:      |
| Radial Gradients    | :heavy_check_mark: | :heavy_check_mark: | :heavy_check_mark:     | :heavy_check_mark:      |
| Thick Lines         | :heavy_check_mark: | :heavy_check_mark: | :heavy_check_mark:     | :heavy_check_mark:      |
| Miter Joins         | :heavy_check_mark: | :heavy_check_mark: | :heavy_check_mark:     | :heavy_check_mark:      |
| Bevel Joins         | :heavy_check_mark: | :heavy_check_mark: | :heavy_check_mark:     | :heavy_check_mark:      |
| Round Joins         | :heavy_check_mark: | :heavy_check_mark: | :x:                    | (3)  				   |
| Line Dashes         | :heavy_check_mark: | :heavy_check_mark: | :x:                    | :x:                     |
| Line Caps           | :heavy_check_mark: | :heavy_check_mark: | :x:                    | :x:                     |
| GL Draw Calls       | 1 per SVG          | 1 per SVG          | 1 per SVG              | >= 1 per path           |

* (1) if used as static mesh, i.e. building it once and not animating its shape.
* (2) if used as animated mesh, i.e. when calling `setUsage` on "points" with "dynamic".
* (3) for some shapes, by using the fragment shader lines option.
* (4) for some shapes, hole polygons must not intersect non-hole polygons.

Releases are available for macOS, Linux and Windows: https://github.com/poke1024/tove2d/releases.

## Getting Started
I've written up the essential stuff in these four sections:

* [Getting Started](docs/Getting_Started.md)
* [The Demos](docs/Demos.md)
* [Choosing Renderers](docs/Renderers.md)
* [Animating Things](docs/Animation.md)

Note: on Windows, in order to run the demos, you need to call `setup.bat` once (this
will fix the links to the lib and asset folders, so you can run e.g. `love demos/blob`).

## Building TÖVE Yourself

On Linux and macOS:

```
git clone --recurse-submodules https://github.com/poke1024/tove2d
cd tove2d
scons
love demos/hearts
```

On Windows:

```
git clone --recurse-submodules https://github.com/poke1024/tove2d
cd tove2d
setup.bat /C
```

Take a look inside `setup.bat` to learn more about installing a compiler environment.

## License
TÖVE is licensed under the MIT license.

Details and the licenses of the included third party code are found in the [LICENSE](https://github.com/poke1024/tove2d/blob/master/LICENSE) file.

## Credits
TÖVE owes a lot to Mikko Mononen's excellent NanoSVG. Not only does TÖVE call NanoSVG in various places, it has also inherited some of its code.

TÖVE's API was inspired by [EaselJS](https://www.createjs.com/easeljs) and [PlotDevice](https://plotdevice.io/PlotDevice).

The rabbit you see in the tutorials and demos is from [Kenney](https://kenney.nl/). The font in the demos is "amatic" by Vernon Adams.

## About the name
TÖVE is a portmanteau of **LÖVE**, **To**wards **Ve**ctor Graphics and **Tove** Jansson, the Swedish author and illustrator of the Moomins.

## Similar Projects
* For a pure-lua implementation of SVG rendering, look at Lovable Vector Graphics: https://github.com/Bananicorn/lvg
* A super-elegant NanoSVG wrapper can be found at https://love2d.org/forums/viewtopic.php?f=5&t=86419
