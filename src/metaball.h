#pragma once
#include "common.h"

struct metaball_t {
    metaball_t()
        : id(0), position(0, 0, 0), potential(1)
    {}

    metaball_t(int id, vec3 position, float potential)
        : id(id), position(position), potential(potential)
    {}

    int id;
    vec3 position;
    float potential;

    vec3 shift;
    vec3 freq;
};

class metaball_geometry {
public:
    metaball_geometry(float isolevel, float resolution, 
        vec3 lower_bound, vec3 upper_bound, 
        std::vector<unique_ptr<metaball_t>> const &  metaballs);

    void init_tables(GLuint program);
    void update_uniforms(GLuint program);

    bool update_grid();

    std::vector<vec3> & grid() {
        return grid_;
    }

    float& isolevel() {
        return isolevel_;
    }

    float& resolution() {
        return resolution_;
    }

    vec3 const & upper_bound() {
        return upper_bound_;
    }

    vec3 const & lower_bound() {
        return lower_bound_;
    }

    static const int MAX_METABALL_CNT;
private:
    void make_cube(vec3 bottom_left, vec3 upper_right, vec3 *cube) const;

    float isolevel_;
    float resolution_;
    std::vector<unique_ptr<metaball_t>> const  & metaballs_;
    std::vector<vec3> grid_;

    const double eps = 0.00001;
    vec3 lower_bound_, upper_bound_;
};
