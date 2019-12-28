#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <string>
#include <fstream>
#include <sstream>

#include "graphics.h"

std::string fileToString(const char *filePath) {
    std::ifstream input(filePath);

    std::stringstream buffer;
    buffer << input.rdbuf();

    return buffer.str();
}

tove::GraphicsRef graphicsFromFile(const char *filePath) {
    const auto svg = fileToString(filePath);

    return tove::Graphics::createFromSVG(svg.c_str(), "px", 100);
}

int rasterizeToFile(tove::Graphics &graphics, const char *outputFilePath) {
    const float *bounds = graphics.getExactBounds();

    const int x1 = bounds[0];
    const int y1 = bounds[1];
    const int x2 = bounds[2];
    const int y2 = bounds[3];

    const int width = x2 - x1;
    const int height = y2 - y1;

    std::vector<uint8_t> pixels(width * height * 4);

    graphics.rasterize(pixels.data(), width, height, width * 4, - x1, - y1, 1);

    return stbi_write_png(outputFilePath, width, height, 4, pixels.data(), width * 4);
}

int main() {
    auto svg1 = graphicsFromFile("gradient-circle/gradient-circle_00001.svg");
    auto svg2 = graphicsFromFile("gradient-circle/gradient-circle_00240.svg");

    tove::Graphics target;

    static const int frameCount = 8;

    for (int frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
        const float t = static_cast<float>(frameIndex) / (frameCount - 1);

        target.animate(svg1, svg2, t);

        const auto frameFileName = "frames/" + std::to_string(frameIndex) + ".png";

        rasterizeToFile(target, frameFileName.c_str());
    }

    return 0;
}
