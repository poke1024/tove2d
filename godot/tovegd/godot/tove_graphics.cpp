/*************************************************************************/
/*  tove_graphics.cpp                                                    */
/*************************************************************************/

#include "tove_graphics.h"
#include "os/file_access.h"
#include "scene/resources/primitive_meshes.h"

ToveGraphics::ToveGraphics() {
	graphics.ptr = nullptr;
    display = TOVE_DISPLAY_MESH;
	quality = 0.5f;
}

ToveGraphics::~ToveGraphics() {
	if (graphics.ptr) {
		ReleaseGraphics(graphics);
		graphics.ptr = nullptr;
	}
}

Error ToveGraphics::load(const String &p_path, const String &p_units, float p_dpi) {

	Vector<uint8_t> buf = FileAccess::get_file_as_array(p_path);

	String str;
	str.parse_utf8((const char *)buf.ptr(), buf.size());

	if (graphics.ptr) {
		ReleaseGraphics(graphics);
		graphics.ptr = nullptr;
	}
	graphics = NewGraphics(str.utf8().ptr(), p_units.utf8().ptr(), p_dpi);

	ToveBounds bounds = GraphicsGetBounds(graphics, true);
	float s = 1024.0f / MAX(bounds.x1 - bounds.x0, bounds.y1 - bounds.y0);
	if (s > 1.0f) {
		GraphicsSet(graphics, graphics, true, s, 0, 0, 0, s, 0);
	}

	return OK;
}

void ToveGraphics::update_mesh(MeshInstance2D *p_instance) {

    ERR_FAIL_COND(!graphics.ptr);

    ToveBounds bounds = GraphicsGetBounds(graphics, true);
    float resolution = quality;

    float w = bounds.x1 - bounds.x0;
    float h = bounds.y1 - bounds.y0;

    if (display == TOVE_DISPLAY_MESH) {

    	ToveTesselationQuality tquality;
    	tquality.stopCriterion = TOVE_MAX_ERROR;
    	tquality.recursionLimit = 8;
    	tquality.maximumError = 1.0 / (quality * 2);
    	tquality.arcTolerance = 1.0 / (quality * 2);

    	ToveMeshRef toveMesh = NewColorMesh();
    	GraphicsTesselate(graphics, toveMesh, 1024, &tquality, HOLES_CW, 0);

    	const int n = MeshGetVertexCount(toveMesh);
    	int stride = sizeof(float) * 2 + 4;
    	int size = n * stride;
    	uint8_t *vertices = new uint8_t[size];
    	MeshCopyVertexData(toveMesh, vertices, size);

    	ToveTriangles triangles = MeshGetTriangles(toveMesh);

    	PoolVector<int> iarr;
    	iarr.resize(triangles.size);
    	{
    		PoolVector<int>::Write w = iarr.write();
    		for (int i = 0; i < triangles.size; i++) {
    			w[i] = triangles.array[i] - 1; // 1-based
    		}
    	}

    	ReleaseMesh(toveMesh);

        const float cx = w / 2;
        const float cy = h / 2;

    	PoolVector<Vector3> varr;
    	varr.resize(n);

    	{
    		PoolVector<Vector3>::Write w = varr.write();
    		for (int i = 0; i < n; i++) {
    			float *p = (float*)(vertices + i * stride);
    			w[i] = Vector3(p[0] - cx, p[1] - cy, 0);
    		}
    	}

    	PoolVector<Color> carr;
    	carr.resize(n);

    	{
    		PoolVector<Color>::Write w = carr.write();
    		for (int i = 0; i < n; i++) {
    			uint8_t *p = vertices + i * stride + 2 * sizeof(float);
    			w[i] = Color(p[0] / 255.0, p[1] / 255.0, p[2] / 255.0, p[3] / 255.0);
    		}
    	}

    	delete[] vertices;

    	Ref<ArrayMesh> mesh = Ref<ArrayMesh>(memnew(ArrayMesh));

    	Array arr;
    	arr.resize(Mesh::ARRAY_MAX);
    	arr[Mesh::ARRAY_VERTEX] = varr;
    	arr[Mesh::ARRAY_INDEX] = iarr;
    	arr[Mesh::ARRAY_COLOR] = carr;

    	mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arr);
    	p_instance->set_mesh(mesh);
        p_instance->set_texture(Ref<Texture>());
    } else if (display == TOVE_DISPLAY_TEXTURE) {

        Ref<QuadMesh> quad_mesh;
        quad_mesh.instance();
        quad_mesh->set_size(Size2(w, -h));
        p_instance->set_mesh(quad_mesh);

        Ref<ImageTexture> texture;
        texture.instance();
        texture->create_from_image(rasterize(
            w * resolution, h * resolution,
            -bounds.x0, bounds.y0,
            resolution), ImageTexture::FLAG_FILTER);
        p_instance->set_texture(texture);
    }
}

Ref<Image> ToveGraphics::rasterize(int p_width, int p_height, float p_tx, float p_ty, float p_scale) const {

	ERR_FAIL_COND_V(!graphics.ptr, Ref<Image>());

	ERR_FAIL_COND_V(p_width <= 0, Ref<Image>());
	ERR_FAIL_COND_V(p_height <= 0, Ref<Image>());

	const int w = p_width;
	const int h = p_height;

	PoolVector<uint8_t> dst_image;
	dst_image.resize(w * h * 4);

    {
        PoolVector<uint8_t>::Write dw = dst_image.write();
        ToveImageRecord imageRecord = GraphicsRasterize(graphics, p_width,
        	p_height, p_tx, p_ty, p_scale, nullptr);
        memcpy(&dw[0], imageRecord.pixels, w * h * 4);
        DeleteImage(imageRecord);
    }

	Ref<Image> image;
	image.instance();
	image->create(w, h, false, Image::FORMAT_RGBA8, dst_image);

	return image;
}

void ToveGraphics::_bind_methods() {

	ClassDB::bind_method(D_METHOD("load", "path", "units", "dpi"), &ToveGraphics::load, DEFVAL("px"), DEFVAL(96.0f));
	ClassDB::bind_method(D_METHOD("rasterize", "tx", "ty", "scale"), &ToveGraphics::rasterize, DEFVAL(0.0f), DEFVAL(0.0f), DEFVAL(1.0f));

    ClassDB::bind_method(D_METHOD("set_display", "display"), &ToveGraphics::set_display, DEFVAL(TOVE_DISPLAY_MESH));
	ClassDB::bind_method(D_METHOD("get_display"), &ToveGraphics::get_display);

	ClassDB::bind_method(D_METHOD("set_quality", "quality"), &ToveGraphics::set_quality, DEFVAL(1.0f));
	ClassDB::bind_method(D_METHOD("get_quality"), &ToveGraphics::get_quality);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "display", PROPERTY_HINT_ENUM, "Texture,Mesh"), "set_display", "get_display");
    ADD_PROPERTY(PropertyInfo(Variant::REAL, "quality"), "set_quality", "get_quality");
}

void ToveGraphics::set_display(ToveDisplay p_display) {
	display = p_display;
}

ToveDisplay ToveGraphics::get_display() {
	return display;
}

void ToveGraphics::set_quality(float quality) {
	this->quality = quality;
}

float ToveGraphics::get_quality() {
	return quality;
}
