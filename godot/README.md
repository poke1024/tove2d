This is highly experimental.

# Install and build

* Move "tovegd" into your godot /modules folder (i.e. as /modules/tovegd).
* Replace "tovegd/src" with the real tove2d src folder.
* Build godot using scons.

# Example usage in Godot

Load an SVG into your project. Create a `ToveInstance` in your scene tree. Assign your SVG to the `Graphics` attribute of your `ToveInstance`. There you go:

![Demo](https://github.com/poke1024/tove2d/blob/master/docs/images/demos/godot-sample.png)
