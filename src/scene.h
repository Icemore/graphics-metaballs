#pragma once
#include "common.h"
#include "metaball.h"
#include <random>

enum draw_mode_t {
    SOLID, WIREFRAME, WIREFRAME_OVER_SOLID
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
    void draw_metaballs(float time_from_start, bool wired, bool clear);

private:
    GLuint vx_buf_;
    GLuint vao_;
    GLuint vs_, gs_, fs_, program_;
    quat   rotation_by_control_;
    
    metaball_geometry geometry_;
    std::vector<unique_ptr<metaball_t>> metaballs_;

    draw_mode_t draw_mode_;

    float metaball_speed_;
    float metaball_amplitude_;
    std::vector<float> metaball_freq_;

    std::default_random_engine generator_;
    TwBar* bar_;
    const string potential_prefix = "potential #";
    const string remove_prefix = "remove #";
};

