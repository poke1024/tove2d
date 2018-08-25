/*************************************************************************/
/*  vg_adaptive_renderer.cpp                                             */
/*************************************************************************/

#include "vg_adaptive_renderer.h"
#include "../src/cpp/mesh/meshifier.h"

VGAdaptiveRenderer::VGAdaptiveRenderer() : quality(100) {
	create_tesselator();
}

void VGAdaptiveRenderer::create_tesselator() {
	tesselator = tove::tove_make_shared<tove::AdaptiveTesselator>(
		new tove::AdaptiveFlattener<tove::DefaultCurveFlattener>(
			tove::DefaultCurveFlattener(quality, 6)));
	//tove::TesselatorRef tess = tove::tove_make_shared<tove::RigidTesselator>(3, TOVE_HOLES_NONE);
}

float VGAdaptiveRenderer::get_quality() {
	return quality;
}

void VGAdaptiveRenderer::set_quality(float p_quality) {
	quality = p_quality;
	create_tesselator();
	emit_changed();
}

void VGAdaptiveRenderer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_quality", "quality"), &VGAdaptiveRenderer::set_quality);
	ClassDB::bind_method(D_METHOD("get_quality"), &VGAdaptiveRenderer::get_quality);

	ADD_PROPERTY(PropertyInfo(Variant::REAL, "quality", PROPERTY_HINT_RANGE, "0,2000,10"), "set_quality", "get_quality");
}
