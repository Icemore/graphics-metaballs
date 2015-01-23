#pragma once
#include "common.h"
#include "metaball.h"
#include <random>

enum draw_mode_t {
    SOLID, WIREFRAME, WIREFRAME_OVER_SOLID
};

struct lighting_t {
    vec3 pos;
    float ambient;
    float diffuse;
    float specular;
    float shininess;
};

class scene_t
{
public:
    scene_t();
    ~scene_t();

    void init_controls();
    void init_metaballs();
    void init_buffer();
    void init_vertex_array();

    void add_metaball();
    void remove_metaball(int metaball_id);

    void animate(float time_from_start);
    void draw_frame(float time_from_start);
    void draw_metaballs(mat4 const & view, mat4 const & proj, bool wired);

    void set_lights();

    void load_cubemap_texture();
    bool load_cube_side(GLenum side_target, std::string const & file_name);
    void draw_cubemap(mat4 const & view, mat4 const & proj);
private:
    GLuint metaball_vx_buf_, metaball_vao_;
    GLuint metaball_vs_, metaball_gs_, metaball_fs_, metaball_program_;
    GLuint cube_vs_, cube_fs_, cube_program_;
    GLuint cube_vx_buf_, cube_vao_, tex_cube_;
    quat   rotation_by_control_;
    
    metaball_geometry geometry_;
    std::vector<unique_ptr<metaball_t>> metaballs_;

    draw_mode_t draw_mode_;

    float metaball_speed_;
    float metaball_amplitude_;
    std::vector<float> metaball_freq_;

    lighting_t lighting_;

    std::default_random_engine generator_;
    TwBar* bar_;
    const string potential_prefix = "potential #";
    const string remove_prefix = "remove #";
    const string cube_tex_path = "content/Park2/";
};

