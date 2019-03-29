Collections in TÖVE are table-like objects that hold paths, subpaths, and curves.

## Count

`collection.count` yields the number of elements in `collection`. For example, `graphics.paths.count` gets the number of paths in `graphics`.

## By Index

`collection[i]` yields the `i`-th element in `collection`. `i` is 1-based. For example, `graphics.paths[1]` gets the first path in `graphics`.

## By Name

`collection.name` yields the element named `name`. For example, `graphics.paths[1].subpaths[1].curves[2].cp1x` yields the x coordinate of path 1, subpath 1, curve 2 in `graphics`.


