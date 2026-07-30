#include <algorithm>
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
    TGAImage specularbuffer;
    TGAImage emissivebuffer;
    TGAImage diffusebuffer;
    TGAImage normal_tanbuffer;
    PhongShader(const vec3 light, const Model &m) : model(m) {
        l = normalized((ModelView*vec4{light.x, light.y, light.z, 0.}).xyz()); // transform the light vector to view coordinates
    }

    virtual vec4 vertex(const int face, const int vert) {
        vec3 v = model.vert(face, vert);                          // current vertex in object coordinates
        vec3 nrml = model.nrml(face, vert);
        
        vec4 gl_Position = ModelView * vec4{v.x, v.y, v.z, 1.};
        vec4 gl_Normal = ModelView.invert_transpose() * vec4{ nrml.x, nrml.y, nrml.z, 0 };
        tri[vert] = gl_Position.xyz();                            // in eye coordinates
        norml_[vert] = gl_Normal.xyz(); // extract xyz and convert to vec3
        uv_[vert] = model.UV(face, vert);
        return Perspective * gl_Position;                         // in clip coordinates
    }



    // nearest-neighbour texture fetch: each map has its own resolution, and uv==1 must not run off the edge
    static TGAColor sample(const TGAImage &img, const vec2 uv) {
        int x = std::min(img.width() -1, std::max(0, int(uv.x * img.width() )));
        int y = std::min(img.height()-1, std::max(0, int(uv.y * img.height())));
        return img.get(x, y);
    }

    virtual std::pair<bool,TGAColor> fragment(const vec3 bar) const {
        vec2 U = bar[0] * uv_[0] + bar[1] * uv_[1] + bar[2] *  uv_[2];

        vec3 n_obj = normalized(norml_[0] * bar[0] + norml_[1] * bar[1] + norml_[2] * bar[2]);

        vec3 e0 = tri[1] - tri[0];
        vec3 e1 = tri[2] - tri[0];
        vec2 u0 = uv_[1] - uv_[0];
        vec2 u1 = uv_[2] - uv_[0];

        mat<2, 3> emat;
        emat.rows[0] = e0;
        emat.rows[1] = e1;

        mat<2, 2> umat;
        umat.rows[0] = u0;
        umat.rows[1] = u1;

        mat<2, 3> tb = umat.invert() * emat;
        mat<3, 3> tbn;
        tbn[0] = normalized(tb[0]);
        tbn[1] = normalized(tb[1]);
        tbn[2] = n_obj;

        TGAColor normaltanColor = sample(normal_tanbuffer, U);
        vec3 ntan_obj = { 0 };
        for (int i = 0; i < 3; i++)
            ntan_obj[i] = normaltanColor[2 - i] / 255. * 2. - 1.;       // bgra -> xyz, [0,255] -> [-1,1]

        vec3 n = normalized(ntan_obj * tbn);
        
        vec3 r = normalized(n * (n * l)*2 - l);                   // reflected light direction



        TGAColor dc = sample(diffusebuffer, U);
        vec3 albedo = { dc[2]/255., dc[1]/255., dc[0]/255. };     // bgra -> rgb, normalized to [0,1]
        double gloss = sample(specularbuffer, U)[0];              // the spec map is GRAYSCALE and stores the Phong exponent

        double ambient = .3;                                      // ambient light intensity
        double diff = std::max(0., n * l);                        // diffuse light intensity
        double spec = std::pow(std::max(r.z, 0.), std::max(1., gloss)); // specular intensity, note that the camera lies on the z-axis (in eye coordinates), therefore simple r.z, since (0,0,1)*(r.x, r.y, r.z) = r.z

        TGAColor gl_FragColor = {0, 0, 0, 255};                   // output color of the fragment
        for (int channel : {0, 1, 2}) {                           // everything above lives in [0,1], so clamp before quantizing
            double v = albedo[channel] * (ambient + diff) + .6 * spec;
            gl_FragColor[2 - channel] = std::uint8_t(std::min(1., v) * 255.); // rgb -> bgra
        }
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
        Model model("D:/programming/myOwnTinyRenderer/obj/african_head.obj");             // load the data
        PhongShader shader(light, model);
        shader.normalbuffer.read_tga_file("D:/programming/myOwnTinyRenderer/obj/african_head_nm.tga");
        shader.normal_tanbuffer.read_tga_file("D:/programming/myOwnTinyRenderer/obj/african_head_nm_tangent.tga");
        shader.specularbuffer.read_tga_file("D:/programming/myOwnTinyRenderer/obj/african_head_spec.tga");
        shader.diffusebuffer.read_tga_file("D:/programming/myOwnTinyRenderer/obj/african_head_diffuse.tga");
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

