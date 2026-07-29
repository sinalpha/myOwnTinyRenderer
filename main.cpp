#include "our_gl.h"
#include "model.h"

extern mat<4,4> ModelView, Perspective; // "OpenGL" state matrices and
extern std::vector<double> zbuffer;     // the depth buffer

struct PhongShader : IShader {
    const Model &model;
    vec3 l;          // light direction in eye coordinates
    vec3 tri[3];     // triangle in eye coordinates
    vec3 norml_[3];
    vec2 uv_[3];
    TGAImage normalbuffer;


    PhongShader(const vec3 light, const Model &m) : model(m) {
        l = normalized((ModelView*vec4{light.x, light.y, light.z, 0.}).xyz()); // transform the light vector to view coordinates
    }

    virtual vec4 vertex(const int face, const int vert) {
        vec3 v = model.vert(face, vert);                          // current vertex in object coordinates
        vec3 nrml = model.nrml(face, vert);
        
        vec4 gl_Position = ModelView * vec4{v.x, v.y, v.z, 1.};
        vec4 gl_Normal = ModelView.invert_transpose() * vec4{ nrml.x, nrml.y, nrml.z, 1 };
        tri[vert] = gl_Position.xyz();                            // in eye coordinates
        norml_[vert] = gl_Normal.xyz(); // extract xyz and convert to vec3
        uv_[vert] = model.UV(face, vert);
        return Perspective * gl_Position;                         // in clip coordinates
    }

    virtual std::pair<bool,TGAColor> fragment(const vec3 bar) const {
        TGAColor gl_FragColor = {255, 255, 255, 255};             // output color of the fragment
        vec3 p = bar[0] * tri[0] + bar[1] * tri[1] + bar[2] * tri[2];
        vec2 U = bar[0] * uv_[0] + bar[1] * uv_[1] + bar[2] *  uv_[2];
        TGAColor normalColor = normalbuffer.get(U.x * 1024, U.y * 1024);
        vec3 n = { 0 };
        for (int i = 0; i < 3; i++) {
            n[i] = normalColor[2 - i] / 255.f * 2.f - 1.f;
        }
        vec3 r = normalized(n * (n * l)*2 - l);                   // reflected light direction
        double ambient = .3;                                      // ambient light intensity
        double diff = std::max(0., n * l);                        // diffuse light intensity
        double spec = std::pow(std::max(r * p, 0.), 35);            // specular intensity, note that the camera lies on the z-axis (in eye coordinates), therefore simple r.z, since (0,0,1)*(r.x, r.y, r.z) = r.z
        

        
        for (int channel : {0,1,2})
            gl_FragColor[channel] *= std::min(1., ambient + .4*diff + .9*spec);
        return {false, gl_FragColor};                             // do not discard the pixel
    }
};

int main(int argc, char** argv) {

    constexpr int width  = 800;      // output image size
    constexpr int height = 800;
    constexpr vec3  light{ 1, 1, 1}; // light source
    constexpr vec3    eye{-1, 0, 2}; // camera position
    constexpr vec3 center{ 0, 0, 0}; // camera direction
    constexpr vec3     up{ 0, 1, 0}; // camera up vector

    lookat(eye, center, up);                                   // build the ModelView   matrix
    init_perspective(norm(eye-center));                        // build the Perspective matrix
    init_viewport(width/16, height/16, width*7/8, height*7/8); // build the Viewport    matrix
    init_zbuffer(width, height);
    TGAImage framebuffer(width, height, TGAImage::RGB);


    for (int m=1; m<2; m++) {                    // iterate through all input objects
        Model model("D:/programming/myOwnTinyRenderer/obj/diablo3_pose.obj");             // load the data
        PhongShader shader(light, model);
        shader.normalbuffer.read_tga_file("D:/programming/myOwnTinyRenderer/obj/diablo3_pose_nm.tga");
        shader.normalbuffer.flip_vertically();
        for (int f=0; f<model.nfaces(); f++) {      // iterate through all facets
            Triangle clip = { shader.vertex(f, 0),  // assemble the primitive
                              shader.vertex(f, 1),
                              shader.vertex(f, 2) };
            rasterize(clip, shader, framebuffer);   // rasterize the primitive
        }
    }

    framebuffer.write_tga_file("framebuffer.tga");
    return 0;
}

