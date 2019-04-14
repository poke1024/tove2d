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

void encounteredNilRef(const char *typeName);

template<typename T, typename R>
inline const T &_deref(const R &ref) {
	const T &p = *static_cast<const T*>(ref.ptr);
	if (!p.get()) {
		encounteredNilRef(typeid(T).name());
	}
	return p;
}

inline const GraphicsRef &deref(const ToveGraphicsRef &ref) {
	return _deref<GraphicsRef>(ref);
}

inline const PathRef &deref(const TovePathRef &ref) {
	return _deref<PathRef>(ref);
}

inline const SubpathRef &deref(const ToveSubpathRef &ref) {
	return _deref<SubpathRef>(ref);
}

inline const PaintRef &deref(const TovePaintRef &ref) {
	return *static_cast<const PaintRef*>(ref.ptr);
}

inline const FeedRef &deref(const ToveFeedRef &ref) {
	return _deref<FeedRef>(ref);
}

inline const MeshRef &deref(const ToveMeshRef &ref) {
	return _deref<MeshRef>(ref);
}

inline const TesselatorRef &deref(const ToveTesselatorRef &ref) {
	return _deref<TesselatorRef>(ref);
}

inline const PaletteRef &deref(const TovePaletteRef &ref) {
	return _deref<PaletteRef>(ref);
}

inline const NameRef &deref(const ToveNameRef &ref) {
	return _deref<NameRef>(ref);
}

extern References<Graphics, ToveGraphicsRef> shapes;
extern References<Path, TovePathRef> paths;
extern References<Subpath, ToveSubpathRef> trajectories;
extern References<AbstractPaint, TovePaintRef> paints;
extern References<AbstractFeed, ToveFeedRef> shaderLinks;
extern References<AbstractMesh, ToveMeshRef> meshes;
extern References<AbstractTesselator, ToveTesselatorRef> tesselators;
extern References<Palette, TovePaletteRef> palettes;
extern References<std::string, ToveNameRef> names;

#endif // TOVE_TARGET_LOVE2D

END_TOVE_NAMESPACE
