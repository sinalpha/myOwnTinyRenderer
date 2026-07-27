#include <cstdlib>
#include <random>

#include "our_gl.h"
#include "model.h"

extern mat<4, 4> ModelView, Perspective; // "OpenGL" state matrices and
extern std::vector<double> zbuffer;     // the depth buffer

struct RandomShader : IShader {
    const Model& model;
    TGAColor color = {};
    vec3 tri[3];  // triangle in eye coordinates
    vec3 normal;
    vec3 light;

    RandomShader(const Model& m) : model(m) {
    }

    virtual vec4 vertex(const int face, const int vert) {
        vec3 v = model.vert(face, vert);                          // current vertex in object coordinates
        vec4 gl_Position = ModelView * vec4{ v.x, v.y, v.z, 1. };
        tri[vert] = gl_Position.xyz();                            // in eye coordinates
        return Perspective * gl_Position;                         // in clip coordinates
    }

    virtual void calculateNormal() {
        vec3 ab = tri[1] - tri[0];
        vec3 ac = tri[2] - tri[0];

        normal = cross(ab, ac);
        normal = normal / norm(normal);
    }

    virtual void calculateLight(const vec3 light_) {
        light = normalized(light_);
    }

    virtual std::pair<bool, TGAColor> fragment(const vec3 bar) const {

        const vec3 r = 2 * normal * (normal * light) - light;
        const vec3 v = normalized(-bar.x * tri[0] + -bar.y * tri[1] + -bar.z * tri[2]);
        const vec3 ambient{ 0.1 };
        const vec3 diffuse{ std::max(0., normal * light) };
        const vec3 specular{ std::max(0., r * v)};

        vec3 sum = (ambient + diffuse + specular) * 255.f;

        TGAColor res = TGAColor{ 
            static_cast<unsigned char>(sum.x), 
            static_cast<unsigned char>(sum.y),
            static_cast<unsigned char>(sum.z),
            255 };
        return { false,  res };// do not discard the pixel
    }
};

int main(int argc, char** argv) {

    constexpr int width = 800;      // output image size
    constexpr int height = 800;
    constexpr vec3    eye{ -1, 0, 2 }; // camera position
    constexpr vec3 center{ 0, 0, 0 }; // camera direction
    constexpr vec3     up{ 0, 1, 0 }; // camera up vector

    lookat(eye, center, up);                                   // build the ModelView   matrix
    init_perspective(norm(eye - center));                        // build the Perspective matrix
    init_viewport(width / 16, height / 16, width * 7 / 8, height * 7 / 8); // build the Viewport    matrix
    init_zbuffer(width, height);
    TGAImage framebuffer(width, height, TGAImage::RGB, { 177, 195, 209, 255 });

    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, 254);


    for (int m = 1; m < 2; m++) {                    // iterate through all input objects
        Model model("D:/programming/myOwnTinyRenderer/obj/diablo3_pose.obj");
        RandomShader shader(model);
        for (int f = 0; f < model.nfaces(); f++) {      // iterate through all facets
            shader.color = { static_cast<unsigned char>(std::rand() % 255),
                             static_cast<unsigned char>(std::rand() % 255),
                             static_cast<unsigned char>(std::rand() % 255),
                             255 };
            Triangle clip = { shader.vertex(f, 0),  // assemble the primitive
                              shader.vertex(f, 1),
                              shader.vertex(f, 2) };
            shader.calculateNormal();
            //the sun light always locate in origin of xy and z = 10;
            shader.calculateLight(vec3(0.f, 0.f, 10.f));
            rasterize(clip, shader, framebuffer);   // rasterize the primitive
        }
    }

    framebuffer.write_tga_file("framebuffer.tga");
    return 0;
}
