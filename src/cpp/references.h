/*
 * TÖVE - Animated vector graphics for LÖVE.
 * https://github.com/poke1024/tove2d
 *
 * Copyright (c) 2018, Bernhard Liebl
 *
 * Distributed under the MIT license. See LICENSE file for details.
 *
 * All rights reserved.
 */

template<typename T, typename ToveType>
struct References {
public:
	typedef std::shared_ptr<T> Ref;

	template<typename... Params>
	inline ToveType make(Params... params) {
		return publish(std::make_shared<T>(params...));
	}

	inline ToveType publish(const Ref &ref) {
		assert(ref.get() != nullptr);
		Ref *newRef = new Ref(ref);
		return ToveType{newRef};
	}

	inline ToveType publishEmpty() {
		Ref *newRef = new Ref();
		return ToveType{newRef};
	}

	inline ToveType publishOrNil(const Ref &ref) {
		if (ref.get() == nullptr) {
			return ToveType{nullptr};
		} else {
			Ref *newRef = new Ref(ref);
			return ToveType{newRef};
		}
	}

	inline void release(const ToveType &ref) {
		delete static_cast<const Ref*>(ref.ptr);
	}
};

inline const GraphicsRef &deref(const ToveGraphicsRef &ref) {
	return *static_cast<const GraphicsRef*>(ref.ptr);
}

inline const PathRef &deref(const TovePathRef &ref) {
	return *static_cast<const PathRef*>(ref.ptr);
}

inline const TrajectoryRef &deref(const ToveTrajectoryRef &ref) {
	return *static_cast<const TrajectoryRef*>(ref.ptr);
}

inline const PaintRef &deref(const TovePaintRef &ref) {
	return *static_cast<const PaintRef*>(ref.ptr);
}

inline const ShaderLinkRef &deref(const ToveShaderLinkRef &ref) {
	return *static_cast<const ShaderLinkRef*>(ref.ptr);
}

inline const MeshRef &deref(const ToveMeshRef &ref) {
	return *static_cast<const MeshRef*>(ref.ptr);
}

References<Graphics, ToveGraphicsRef> shapes;
References<Path, TovePathRef> paths;
References<Trajectory, ToveTrajectoryRef> trajectories;
References<AbstractPaint, TovePaintRef> paints;
References<AbstractShaderLink, ToveShaderLinkRef> shaderLinks;
References<AbstractMesh, ToveMeshRef> meshes;
