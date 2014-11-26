#pragma once
#include "common.h"

struct metaball_t {
    metaball_t()
        : id(0), position(0, 0, 0), direction(1, 1, 1), potential(1)
    {}

    metaball_t(int id, vec3 position, vec3 direction, float potential)
        : id(id), position(position), direction(direction), potential(potential)
    {}

    int id;
    vec3 position;
    vec3 direction;
    float potential;
};

class metaball_geometry {
public:
    metaball_geometry(float isolevel, float resolution, 
        vec3 lower_bound, vec3 upper_bound, 
        std::vector<unique_ptr<metaball_t>> const &  metaballs);

    std::vector<vec3> & generate_geometry();

    std::vector<vec3>& get_geometry() {
        return geometry_;
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

    float calculate_level(unique_ptr<metaball_t> const & metaball, vec3 const & pos) const;

private:
    void make_cube(vec3 bottom_left, vec3 upper_right, vec3 *cube) const;
    float calculate_level(vec3 const & pos) const;
    vec3 interpolate(vec3 first_pos, vec3 second_pos, 
        float first_level, float second_level) const;

    void processCube(vec3 bottom_left, vec3 upper_right);        

    float isolevel_;
    float resolution_;
    std::vector<unique_ptr<metaball_t>> const  & metaballs_;
    std::vector<vec3> geometry_;

    const double eps = 0.00001;
    vec3 lower_bound_, upper_bound_;
};
