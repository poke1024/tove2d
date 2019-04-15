# TÖVE <img align="right" src="https://github.com/poke1024/tove2d/blob/master/docs/tutorials/images/tovelogo.png" height="200">
Animated vector graphics for [LÖVE](https://love2d.org/).

## Description
TÖVE is a vector drawing canvas for LÖVE, think HTML5's `<canvas>` or  OpenFL's `Graphics`. Which you can animate. Efficiently.

## Features
* brings crisp vector graphics and SVGs to LÖVE
* offers a substantial subset of SVG
* built for realtime animation
* details: https://poke1024.github.io/tove2d/features

## Getting Started
Releases are available for macOS, Linux and Windows: https://github.com/poke1024/tove2d/releases.

Look at the demos. The most essential stuff is described here:

* [Getting Started](docs/tutorials/Getting_Started.md)
* [The Demos](docs/tutorials/Demos.md)
* [Choosing Renderers](docs/tutorials/Renderers.md)
* [Animating Things](docs/tutorials/Animation.md)

There are also [the beginnings of a documentation](https://poke1024.github.io/tove2d/).

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

## Roadmap

I keep my ideas of what might eventually end up in TÖVE in [TÖVE's Public Trello Board](https://trello.com/b/p5nWCZVC/t%C3%B6ve).

## License
TÖVE is licensed under the MIT license.

Details and the licenses of the included third party code are found in the [LICENSE](https://github.com/poke1024/tove2d/blob/master/LICENSE) file.

## Credits
TÖVE owes a lot to Mikko Mononen's excellent NanoSVG. Not only does TÖVE call NanoSVG in various places, it has also inherited some of its code.

TÖVE's API was inspired by [EaselJS](https://www.createjs.com/easeljs) and [PlotDevice](https://plotdevice.io/PlotDevice).

The graphics you see in the tutorials and demos are by [Mike Mac](https://mikemac2d.itch.io), [Kenney](https://kenney.nl/) and [Chris Hildenbrand](https://2dgameartguru.com/). The font in the demos is "amatic" by Vernon Adams. 

## About the Name
TÖVE is a portmanteau of **LÖVE**, **To**wards **Ve**ctor Graphics and **Tove** Jansson, the Swedish author and illustrator of the Moomins.

## Similar Projects
* For a pure-lua implementation of SVG rendering, look at Lovable Vector Graphics: https://github.com/Bananicorn/lvg
* A super-elegant NanoSVG wrapper can be found at https://love2d.org/forums/viewtopic.php?f=5&t=86419
