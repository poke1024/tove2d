# TÖVE <img align="right" src="https://github.com/poke1024/tove2d/blob/master/docs/tutorials/images/tovelogo.png" height="200">
Animated vector graphics for [LÖVE](https://love2d.org/).

## Description
TÖVE is a vector drawing canvas for LÖVE, think HTML5's <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API">`<canvas>`</a> or  OpenFL's <a href="https://api.openfl.org/openfl/display/Graphics.html">`Graphics`</a>. Which you can animate. Efficiently.

## Background
There have been various libraries for drawing vector graphics out there, but few of them are built for drawing or even animating complex scenes and most of them stopped at the most basic features.

As an example, take the HTML5 `<canvas>` tag: it's great in terms of quality, yet it's slow for animation or games, as it doesn't know how to keep its graphics data retained on the GPU: indeed, it will do a full redraw of the whole scene on each frame.

You can use WebGL, but then you lose vector graphics. There have been attempts to fix this by implementing vector graphics on top of WebGL, such as <a href="https://github.com/bigtimebuddy/pixi-svg">PIXI SVG</a> and the <a href="https://threejs.org/docs/#examples/loaders/SVGLoader">ThreeJS SVG Loader</a>. TÖVE is exactly like these, only more comprehensive. And for LÖVE.

## Features
* brings crisp vector graphics and SVGs to LÖVE
* offers a substantial subset of SVG
* built for realtime scaling and animation
* details: https://poke1024.github.io/tove2d/features

## Getting Started
Releases are available for macOS, Linux and Windows: https://github.com/poke1024/tove2d/releases.

Some tutorials and general information on TÖVE are found in
<a href="https://poke1024.github.io/tove2d/">TÖVE's documentation</a>.

Be sure to check out <a href="https://poke1024.github.io/tove2d-api/">TÖVE's API documentation</a>,
which gives you detailed information on classes and functions.

Note: on Windows, in order to run the demos, you need to call `setup.bat` once (this
will fix the links to the lib and asset folders, so you can run e.g. `love demos/blob`).

## Building TÖVE Yourself

### On Linux and macOS

```
git clone --recurse-submodules https://github.com/poke1024/tove2d
cd tove2d
scons
love demos/hearts
```

### On Windows

```
# in git bash:
git clone --recurse-submodules https://github.com/poke1024/tove2d
cd tove2d

# in regular cmd shell:
cd path/to/tove2d
setup.bat /C
```

Take a look inside `setup.bat` to learn more about installing a compiler environment.

### Experimental

Use `scons --arch=sandybridge` or `scons --arch=haswell` to compile for a custom CPU architecture. On Windows, this enables AVX extensions,
which mainly speeds up the rasterizer dithering code.

On POSIX, in addition, you can enable hardware intrinsics for 16-bit floating point operations by using `scons --f16c`. These intrinsics
might give small performance benefits in the `gpux` mode.

Since enabling custom options causes crashes on some CPUs (https://github.com/poke1024/tove2d/issues/24), I strongly recommend sticking to
the defaults, as they ensure that your binary will run on many CPUs.

## Roadmap

I keep my ideas of what might eventually end up in TÖVE in [TÖVE's Public Trello Board](https://trello.com/b/p5nWCZVC/t%C3%B6ve).

## License
TÖVE is licensed under the MIT license.

Details and the licenses of the included third party code are found in the [LICENSE](https://github.com/poke1024/tove2d/blob/master/LICENSE) file.

## Credits
TÖVE owes a lot to Mikko Mononen's excellent NanoSVG. Not only does TÖVE call NanoSVG in various places, it has also inherited some of its code.

TÖVE's API was inspired by [EaselJS](https://www.createjs.com/easeljs) and [PlotDevice](https://plotdevice.io).

The graphics you see in the tutorials and demos are by [Mike Mac](https://mikemac2d.itch.io), [Kenney](https://kenney.nl/) and [Chris Hildenbrand](https://2dgameartguru.com/). The font in the demos is "amatic" by Vernon Adams. 

## About the Name
TÖVE is a portmanteau of **LÖVE**, **To**wards **Ve**ctor Graphics and **Tove** Jansson, the Swedish author and illustrator of the Moomins.

## Similar Projects
* For a pure-lua implementation of SVG rendering, look at Lovable Vector Graphics: https://github.com/Bananicorn/lvg
* A super-elegant NanoSVG wrapper can be found at https://love2d.org/forums/viewtopic.php?f=5&t=86419
