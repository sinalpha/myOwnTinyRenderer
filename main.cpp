#include <cmath>
#include <tuple>
#include <algorithm>
#include <array>

#include "geometry.h"
#include "model.h"
#include "tgaimage.h"


constexpr int width  = 128;
constexpr int height = 128;

constexpr TGAColor white   = {255, 255, 255, 255}; // attention, BGRA order
constexpr TGAColor green   = {  0, 255,   0, 255};
constexpr TGAColor red     = {  0,   0, 255, 255};
constexpr TGAColor blue    = {255, 128,  64, 255};
constexpr TGAColor yellow  = {  0, 200, 255, 255};

void line(int ax, int ay, int bx, int by, TGAImage &framebuffer, TGAColor color) {
    bool steep = std::abs(ax-bx) < std::abs(ay-by);
    if (steep) { // if the line is steep, we transpose the image
        std::swap(ax, ay);
        std::swap(bx, by);
    }
    if (ax>bx) { // make it left−to−right
        std::swap(ax, bx);
        std::swap(ay, by);
    }
    int y = ay;
    int ierror = 0;
    for (int x=ax; x<=bx; x++) {
        if (steep) // if transposed, de−transpose
            framebuffer.set(y, x, color);
        else
            framebuffer.set(x, y, color);
        ierror += 2 * std::abs(by-ay);
        if (ierror > bx - ax) {
            y += by > ay ? 1 : -1;
            ierror -= 2 * (bx-ax);
        }
    }
}

void scanline(int ax, int ay, int bx, int by, int cx, int cy, TGAImage& framebuffer, TGAColor color) {

    //min x point, max x point.
    //min y point, max y point.
    struct vec2 {
        int x;
        int y;
    };

    std::array<vec2, 3> pts;

    pts.at(0) = { ax, ay };
    pts.at(1) = { bx, by };
    pts.at(2) = { cx, cy };

    std::sort(pts.begin(), pts.end(), [](const vec2& a, const vec2& b) {
        return a.y < b.y;
        });

    vec2& minYpt = pts.at(0);
    vec2& midYpt = pts.at(1);
    vec2& maxYpt = pts.at(2);

    for (int y = minYpt.y; y <= maxYpt.y; ++y) {

        int startX = minYpt.x;
        int endX = minYpt.x;

        if (y < midYpt.y) {
            startX = (midYpt.y == minYpt.y) ? midYpt.x :
                minYpt.x + (int)((midYpt.x - minYpt.x) * ((float)(y - minYpt.y) / (midYpt.y - minYpt.y)));
        }
        else {
            startX = (maxYpt.y == midYpt.y) ? midYpt.x :
                midYpt.x + (int)((maxYpt.x - midYpt.x) * ((float)(y - midYpt.y) / (maxYpt.y - midYpt.y)));
        }

        if (maxYpt.y != minYpt.y) {
            float t2 = (float)(y - minYpt.y) / (maxYpt.y - minYpt.y);
            endX = minYpt.x + (int)((maxYpt.x - minYpt.x) * t2);
        }
        if (startX > endX) std::swap(startX, endX);

        line(startX, y, endX, y, framebuffer, color);

    }

}

void triangle(int ax, int ay, int bx, int by, int cx, int cy, TGAImage &framebuffer, TGAColor color) {
    line(ax, ay, bx, by, framebuffer, color);
    line(bx, by, cx, cy, framebuffer, color);
    line(cx, cy, ax, ay, framebuffer, color);
    scanline(ax, ay, bx, by, cx, cy, framebuffer, color);
}



int main(int argc, char** argv) {
    TGAImage framebuffer(width, height, TGAImage::RGB);
    triangle(  7, 45, 35, 100, 45,  60, framebuffer, red);
    triangle(120, 35, 90,   5, 45, 110, framebuffer, white);
    triangle(115, 83, 80,  90, 85, 120, framebuffer, green);
    framebuffer.write_tga_file("framebuffer.tga");
    return 0;
}

