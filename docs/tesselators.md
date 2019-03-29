Tesselators may be created using `tove.newAdaptiveTesselator` and `tove.newRigidTesselator`.

# Rigid Tesselators

Rigid tesselators subdivide geometry using fixed steps and thus produce a fixed number of vertices. As an effect, all area of the geometry receive the same level of detail and subdivision. They are suited for animating meshes. Use `tove.newRigidTesselator` to create them.

# Adaptive Tesselators

Adaptive Tesselators create a tesselation that gives more subdivision to areas where it's needed, whereas other areas might get less subdivision. The number of vertices produced by Adaptive Tesselators depends on the specific geometry. They are not suited for animation. Use `tove.newAdaptiveTesselator` to create them.
