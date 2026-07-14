/*

    [ Need to Make ]
    assertions and boundary checks

*/

#include <cmath>
#include <tuple>

#include "tgaimage.h"
#include "model.h"

constexpr TGAColor white = { 255, 255, 255, 255 }; // attention, BGRA order
constexpr TGAColor green = { 0, 255,   0, 255 };
constexpr TGAColor red = { 0,   0, 255, 255 };
constexpr TGAColor blue = { 255, 128,  64, 255 };
constexpr TGAColor yellow = { 0, 200, 255, 255 };

constexpr int width = 800;
constexpr int height = 800;

TGAImage gFramebuffer(width, height, TGAImage::RGB);

void line(int ax, int ay, int bx, int by, TGAImage& gFramebuffer, TGAColor color) {
    bool steep = std::abs(ax - bx) < std::abs(ay - by);
    if (steep) { // if the line is steep, we transpose the image
        std::swap(ax, ay);
        std::swap(bx, by);
    }
    if (ax > bx) { // make it left−to−right
        std::swap(ax, bx);
        std::swap(ay, by);
    }
    int y = ay;
    int ierror = 0;
    for (int x = ax; x <= bx; x++) {
        if (steep) // if transposed, de−transpose
            gFramebuffer.set(y, x, color);
        else
            gFramebuffer.set(x, y, color);
        ierror += 2 * std::abs(by - ay);
        y += (by > ay ? 1 : -1) * (ierror > bx - ax);
        ierror -= 2 * (bx - ax) * (ierror > bx - ax);
    }
}

std::tuple<int, int> ProjectModelToScreen(const Vector3d& point) {
    
    /*
    
        [ Question ]
        Where is the z? and why use this calculation?

    */
    return {
        (point.x + 1.f) * width / 2,
        (point.y + 1.f) * height / 2
    };
}

int main(int argc, char** argv) {



    
    Model myModel("diablo3_pose.obj");

    for (Face& face : myModel.mFaceBuffer) {
        const Vector3d& a = myModel.GetVertexByIndex(face.indices[0]);
        const Vector3d& b = myModel.GetVertexByIndex(face.indices[1]);
        const Vector3d& c = myModel.GetVertexByIndex(face.indices[2]);
        auto [ax, ay] = ProjectModelToScreen(a);
        auto [bx, by] = ProjectModelToScreen(b);
        auto [cx, cy] = ProjectModelToScreen(c);
        line(ax, ay, bx, by, gFramebuffer, red);
        line(bx, by, cx, cy, gFramebuffer, red);
        line(cx, cy, ax, ay, gFramebuffer, red);
    }

    for (Vector3d& point : myModel.mVertexBuffer) {
        auto [x, y] = ProjectModelToScreen(point);
        gFramebuffer.set(x, y, white);
    }

    gFramebuffer.write_tga_file("framebuffer.tga");
    return 0;
}