--- @module display

--- Render using a texture.
-- @usage
-- graphics:setDisplay("texture")
-- @table texture
-- @see Graphics:draw
-- @see Graphics:setDisplay
-- @see Graphics:setResolution

--- Render using a mesh.
-- If no tesselator is specified, a default tesselator will be chosen based on the configured
-- usage of the @{Graphics}.
-- @usage
-- graphics:setDisplay("mesh") -- default configuration (based on configured usage)
--
-- graphics:setDisplay("mesh", 1024) -- make it look good at a size of 1024 pixels
-- -- will choose tesselation settings based on configured usage
--
-- graphics:setDisplay("mesh", "rigid", 4) -- 4 subdivision levels (=2^4 points per curve)
-- graphics:setDisplay("mesh", tove.newRigidTesselator(4)) -- same as above
--
-- graphics:setDisplay("mesh", "adaptive", 1024) -- target a size of 1024
-- graphics:setDisplay("mesh", tove.newAdaptiveTesselator(1024)) -- same as above
-- @table mesh
-- @tparam string mode "mesh"
-- @tparam[opt] Tesselator|string|number tesselator tesselator to use or name of tesselator type or target size
-- @tparam[optchain] ... args configuration of tesselator
-- @see Graphics:draw
-- @see Graphics:setDisplay
-- @see Graphics:setResolution
-- @see Graphics:setUsage

--- Render on the GPU.
-- "gpux" means "gpu exclusive": this mode will calculate all curve geometry on the fly
-- on the GPU. This has considerable resolution until it breaks down, however it can also be
-- quite expensive in terms of performance. It is quite experimental as well: TÖVE is - to
-- my best knowledge - the first library capable of rendering complex cubic bezier shapes
-- inside GLSL shaders without doing some kind of tesselation.
-- @usage
-- graphics:setDisplay("gpux")
-- @table gpux
-- @tparam string mode "gpux"
-- @tparam[opt] string lineRenderer line shader: "fragment" or "vertex"
-- @tparam[optchain] number lineQuality line quality
-- @see Graphics:draw
-- @see Graphics:setDisplay

--- @type Tesselator

-- Tesselators may be created using `tove.newAdaptiveTesselator` and `tove.newRigidTesselator`.

--- Create an adaptive tesselator.
-- Use this for high quality static graphics content that does not animate its shape in realtime.
-- Adaptive Tesselators create meshes that give more subdivision (i.e. points and triangles) to areas where 
-- higher detail is needed, whereas other areas might get less subdivision. The number of vertices produced by
-- Adaptive Tesselators depends on the specific geometry. They are not suited for shape animation, as each shape
-- change would result in a different number of vertices and triangles.
-- @function tove.newAdaptiveTesselator
-- @tparam number resolution resolution at which the mesh should look good
-- @tparam[opt] int recursionLimit maximum number of recursions during subdivision

--- Create a rigid tesselator.
-- Use this to obtain low or medium quality meshes that can distort and animate their shapes in realtime.
-- Rigid tesselators subdivide geometry using fixed steps and thus produce a fixed number of vertices. As an
-- effect, all areasy of the geometry receive the same level of detail and subdivision. They are suited for
-- shape animation, as the vertex count stays constant and underlying meshes can be updated efficiently.
-- @function tove.newRigidTesselator
-- @tparam int subdivisions number of recursive subdivisions; 3 means 2^3=8 points for each bezier curve,
-- 4 indicates 2^4=16 points, and so on.