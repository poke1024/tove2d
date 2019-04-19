# Scope

TÖVE is a simplified vector graphics library for games aimed at realtime scaling and animation.

Even though TÖVE knows some tricks and even supports dithering, do not expect it to have the feature set or the rendering quality comparable of a full fledged print quality library such as <a href="https://www.cairographics.org/">Cairo</a> or <a href="https://skia.org/">Skia</a>.

# Renderers

Here's what you can expect from TÖVE's different renderers:

|                     | texture            | mesh[^1]           | mesh[^2]               | gpux          |
|---------------------|--------------------|--------------------|------------------------|---------------|
| Holes Support       | &#x25CF; 		   | &#x25CF; 			| &#x25CF;[^4] 			 | &#x25CF;      |
| Non-Zero Fill Rule  | &#x25CF; 		   | &#x25CF; 			| &#x25CB;				 | &#x25CF;      |
| Even-Odd Fill Rule  | &#x25CF; 		   | &#x25CF; 			| &#x25CB;    		     | &#x25CF;      |
| Clip Paths          | &#x25CF;		   | &#x25CF; 			| &#x25CB;               | &#x25CB;      |
| Solid Colors        | &#x25CF;	       | &#x25CF; 			| &#x25CF;    			 | &#x25CF;      |
| Linear Gradients    | &#x25CF;           | &#x25CF; 			| &#x25CF;    			 | &#x25CF;      |
| Radial Gradients    | &#x25CF;           | &#x25CF; 			| &#x25CF;    			 | &#x25CF;      |
| Thick Lines         | &#x25CF;           | &#x25CF; 			| &#x25CF;    			 | &#x25CF;      |
| Non-Opaque Lines    | &#x25CF;           | &#x25CF; 			| &#x25CB;               | &#x25CF;      |
| Miter Joins         | &#x25CF;           | &#x25CF; 			| &#x25CF;    			 | &#x25CF;      |
| Bevel Joins         | &#x25CF;           | &#x25CF; 			| &#x25CF;    			 | &#x25CF;      | 
| Round Joins         | &#x25CF;           | &#x25CF; 			| &#x25CB;               | &#x25CF;[^3]  |
| Line Dashes         | &#x25CF;           | &#x25CF; 			| &#x25CB;               | &#x25CB;      |
| Line Caps           | &#x25CF;           | &#x25CF; 			| &#x25CB;               | &#x25CB;      |

<br/>
Also note the number of OpenGL drawing calls/context switches:

| texture            | mesh              | gpux            |
|--------------------|-------------------|-----------------|
| 1 per `Graphics`   | 1 per `Graphics`  | >= 1 per `Path` |

[^1]: if used as static mesh, i.e. building it once and not animating its shape.
[^2]: if used as animated mesh, i.e. when calling `setUsage` on "points" with "dynamic" or "stream".
[^3]: for some shapes, by using the `fragment` option for line rendering.
[^4]: for some shapes, hole polygons must not intersect non-hole polygons.
