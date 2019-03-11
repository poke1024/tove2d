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

BEGIN_TOVE_NAMESPACE

#if TOVE_TARGET == TOVE_TARGET_LOVE2D

template<typename T, typename ToveType>
struct References {
public:
	typedef SharedPtr<T> Ref;

	template<typename... Params>
	inline ToveType make(Params... params) {
		return publish(tove_make_shared<T>(params...));
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

#define DEREF(RefType)													\
	{ const RefType &r = *static_cast<const RefType*>(ref.ptr); 		\
	if (!r.get()) { tove::report::warn(#RefType " is nil"); } return r; }

inline const GraphicsRef &deref(const ToveGraphicsRef &ref) {
	DEREF(GraphicsRef)
}

inline const PathRef &deref(const TovePathRef &ref) {
	DEREF(PathRef)
}

inline const SubpathRef &deref(const ToveSubpathRef &ref) {
	DEREF(SubpathRef)
}

inline const PaintRef &deref(const TovePaintRef &ref) {
	return *static_cast<const PaintRef*>(ref.ptr);
}

inline const FeedRef &deref(const ToveFeedRef &ref) {
	DEREF(FeedRef)
}

inline const MeshRef &deref(const ToveMeshRef &ref) {
	DEREF(MeshRef)
}

inline const TesselatorRef &deref(const ToveTesselatorRef &ref) {
	DEREF(TesselatorRef)
}

#undef DEREF

References<Graphics, ToveGraphicsRef> shapes;
References<Path, TovePathRef> paths;
References<Subpath, ToveSubpathRef> trajectories;
References<AbstractPaint, TovePaintRef> paints;
References<AbstractFeed, ToveFeedRef> shaderLinks;
References<AbstractMesh, ToveMeshRef> meshes;
References<AbstractTesselator, ToveTesselatorRef> tesselators;

#endif // TOVE_TARGET_LOVE2D

END_TOVE_NAMESPACE
