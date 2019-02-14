This is highly experimental. And buggy.

# Install and build

* Move "tovegd" into your godot /modules folder (i.e. as /modules/tovegd).
* Replace "tovegd/src" with the real tove2d src folder.
* Build godot using scons.

# Basic usage in Godot

Add a new VGPath node in your scene (under a Node2D).

In the inspector, set its Renderer to a new VGAdaptiveRenderer. Now, in
the toolbar at the top, click on the circle. Drag and drop on the canvas
to draw a vector circle.

Select a VGPath and double click onto it (while having the arrow tool
selected) to see and edit control points and curves.

You can add new control points by clicking on a curve.

Select a control point and hit the delete key to remove it.

Rendering quality can be changed by editing the VGAdaptiveRenderer's
quality setting (lower number means less triangles). You can do this
interactively in the Godot editor.

# Enabling SVG import

In order to be able to drag and drop SVG assets into the Godot editor
as vector graphics, you need to make one patch in Godot; take a look at
the accompanying patches/0001-tovegd-svg-support.patch, which is this:

	In editor/canvas_item_editor_plugin.cpp, add

		Node *createVectorSprite(Ref<Resource> p_resource, Node *p_owner);

	then replace

		child = memnew(Sprite); // default

	with:

		child = createVectorSprite(texture, target_node);

After recompiling, you should now be able to drag and drop an SVG file
into your Godot FileSystem. From there you can drag it into your scene,
and all SVG paths should get converted into VGPaths.
