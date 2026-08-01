#include "our_gl.h"
#include "model.h"

extern mat<4,4> ModelView, Perspective, Viewport;
extern std::vector<double> zbuffer;     // the depth buffer
extern std::vector<double> shadowbuffer;

struct PhongShader : IShader {
    const Model &model;
    const double bias = 0.01; // shadow bias
    vec4 l;              // light direction in eye coordinates
    mat<4,4> shadow_matrix;
    int shadow_w, shadow_h; // resolution of the shadow map (shadowbuffer)
    vec2  varying_uv[3]; // triangle uv coordinates, written by the vertex shader, read by the fragment shader
    vec4 varying_nrm[3]; // normal per vertex to be interpolated by the fragment shader
    vec4 tri[3];         // triangle in view coordinates

    PhongShader(const vec3 light, const Model &m, const mat<4,4> shadow_mat, const int shadow_width, const int shadow_height) : model(m), shadow_w(shadow_width), shadow_h(shadow_height) {
        l = normalized((ModelView*vec4{light.x, light.y, light.z, 0.})); // transform the light vector to view coordinates
        shadow_matrix = shadow_mat;
    }

    virtual vec4 vertex(const int face, const int vert) {
        varying_uv[vert]  = model.uv(face, vert);
        varying_nrm[vert] = ModelView.invert_transpose() * model.normal(face, vert);
        vec4 gl_Position = ModelView * model.vert(face, vert);
        tri[vert] = gl_Position;
        return Perspective * gl_Position;                         // in clip coordinates
    }

    virtual std::pair<bool,TGAColor> fragment(const vec3 bar) const {
        mat<2,4> E = { tri[1]-tri[0], tri[2]-tri[0] };
        mat<2,2> U = { varying_uv[1]-varying_uv[0], varying_uv[2]-varying_uv[0] };
        mat<2,4> T = U.invert() * E;
        mat<4,4> D = {normalized(T[0]),  // tangent vector
                      normalized(T[1]),  // bitangent vector
                      normalized(varying_nrm[0]*bar[0] + varying_nrm[1]*bar[1] + varying_nrm[2]*bar[2]), // interpolated normal
                      {0,0,0,1}}; // Darboux frame
        vec2 uv = varying_uv[0] * bar[0] + varying_uv[1] * bar[1] + varying_uv[2] * bar[2];
        vec4 n = normalized(D.transpose() * model.normal(uv));
        double n_dot_l = n * l;
        vec4 r = normalized(n * n_dot_l*2 - l);                   // reflected light direction
        double ambient  = .4;                                     // ambient light intensity
        double diffuse  = 1.*std::max(0., n_dot_l);                // diffuse light intensity
        double specular = (3.*sample2D(model.specular(), uv)[0]/255.) * std::pow(std::max(r.z, 0.), 35);  // specular intensity, note that the camera lies on the z-axis (in eye coordinates), therefore simple r.z, since (0,0,1)*(r.x, r.y, r.z) = r.z
        
        vec4 p_view = tri[0] * bar[0] + tri[1] * bar[1] + tri[2] * bar[2];
        vec4 p_light = Viewport * shadow_matrix * ModelView.invert() * p_view;
        p_light = p_light / p_light.w;

        double u = p_light.x;
        double v = p_light.y;
        double z_current = p_light.z;

        double shadow = 1.0; // outside the shadow map, assume no shadow
        if (u >= 0 && u < shadow_w && v >= 0 && v < shadow_h) {
            int idx = int(u) + int(v) * shadow_w;
            // shadowbuffer holds the depth closest to the light (larger = closer);
            // only mark as occluded when that recorded depth is meaningfully closer
            // than this fragment's own depth, otherwise self-comparison always loses
            shadow = (shadowbuffer[idx] > z_current + bias) ? 0.0 : 1.0;
        }
        TGAColor gl_FragColor = sample2D(model.diffuse(), uv);
        for (int channel : {0,1,2})
            gl_FragColor[channel] = std::min<int>(255, gl_FragColor[channel] * (ambient +  shadow * (diffuse + specular)));
        return {false, gl_FragColor};                             // do not discard the pixel
    }
};

struct DepthShader : IShader {
    const Model& model;
    vec4 tri[3];     

    DepthShader(const Model& m) : model(m) {}

    virtual vec4 vertex(const int iface, const int nthvert){
        vec4 gl_Vertex = model.vert(iface, nthvert);
        gl_Vertex = ModelView * gl_Vertex;
        tri[nthvert] = gl_Vertex;
        return  Perspective * gl_Vertex;
    }

    virtual std::pair<bool, TGAColor> fragment(const vec3 bar) const override {
        
        double z = tri[0].z * bar[0] + tri[1].z * bar[1] + tri[2].z * bar[2];

        unsigned char shadow_pixel = static_cast<unsigned char>(z);

        return { false, TGAColor{shadow_pixel, shadow_pixel, shadow_pixel} };
    }

};

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " obj/model.obj" << std::endl;
        return 1;
    }

    constexpr int width  = 800;      // output image size
    constexpr int height = 800;
    constexpr vec3  light{ 1, 1, 1}; // light source
    constexpr vec3    eye{-1, 0, 2}; // camera position
    constexpr vec3 center{ 0, 0, 0}; // camera direction
    constexpr vec3     up{ 0, 1, 0}; // camera up vector


    lookat(light, center, up);
    init_perspective(norm(light - center));
    init_viewport(width/16, height/16, width*7/8, height*7/8); // build the Viewport    matrix
    init_zbuffer(width, height);
    
    const mat<4, 4> shadow_matrix = Perspective * ModelView;
    TGAImage depth_image(width, height, TGAImage::RGB);

    for (int m = 1; m < argc; m++) {                    // iterate through all input objects
        Model model(argv[m]);                       // load the data
        DepthShader shader(model);
        for (int f = 0; f < model.nfaces(); f++) {      // iterate through all facets
            Triangle clip = { shader.vertex(f, 0),  // assemble the primitive
                              shader.vertex(f, 1),
                              shader.vertex(f, 2) };
            rasterize(clip, shader, depth_image);   // rasterize the primitive
        }
    }

    shadowbuffer = zbuffer;
    depth_image.write_tga_file("depthbuffer.tga");

    lookat(eye, center, up);                                   // build the ModelView   matrix
    init_perspective(norm(eye-center));                        // build the Perspective matrix
    init_zbuffer(width, height);
    TGAImage framebuffer(width, height, TGAImage::RGB);

    for (int m=1; m<argc; m++) {                    // iterate through all input objects
        Model model(argv[m]);                       // load the data
        PhongShader shader(light, model, shadow_matrix, width, height);
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

