#include "scene.h"
#include "shader.h"

#include <algorithm>
#include <sstream>
#include <random>
#include <functional>

static void TW_CALL tw_toggle_fullscreen(void *) {
    glutFullScreenToggle();
}

static void TW_CALL tw_add_metaball(void* data) {
    static_cast<scene_t*>(data)->add_metaball();
}

static void TW_CALL tw_remove_metaball(void* data) {
    auto input = static_cast<std::pair<scene_t*, int>*>(data);
    scene_t* scene = input->first;
    int metaball_id = input->second;

    scene->remove_metaball(metaball_id);

    delete input;
}


scene_t::scene_t()
        : geometry_(4.2f, 30, vec3(-1, -1, -1), vec3(1, 1, 1), metaballs_),
          draw_mode_(draw_mode_t::SOLID),
          metaball_speed_(0.1f),
          metaball_amplitude_(0.5f) {
    metaball_freq_ = { 4, 5, 6 };

    init_controls();
    init_metaballs();

    vs_ = create_shader(GL_VERTEX_SHADER, "shaders//dummy.vp");
    fs_ = create_shader(GL_FRAGMENT_SHADER, "shaders//dummy.fp");
    gs_ = create_shader(GL_GEOMETRY_SHADER, "shaders//metaball.geom");
    program_ = create_program(vs_, fs_, gs_);

    geometry_.init_tables(program_);
    init_buffer();
    init_vertex_array();
}

scene_t::~scene_t() {
    // Удаление ресурсов OpenGL
    glDeleteVertexArrays(1, &vao_);
    glDeleteBuffers(1, &vx_buf_);

    for (GLuint shader : {vs_, fs_}) {
        glDeleteShader(shader);
    }

    TwDeleteAllBars();
    TwTerminate();
}

void scene_t::init_controls() {
#ifdef USE_CORE_OPENGL
    TwInit(TW_OPENGL_CORE, NULL);
#else
    TwInit(TW_OPENGL, NULL);
#endif
    bar_ = TwNewBar("Parameters");
    TwDefine(" Parameters size='300 300' color='70 100 120' valueswidth=150 iconpos=topleft");

    TwAddButton(bar_, "Fullscreen toggle", tw_toggle_fullscreen, NULL,
        " label='Toggle fullscreen mode' key=f");

    TwAddVarRW(bar_, "isolevel", TW_TYPE_FLOAT, &geometry_.isolevel(), " label='isolevel' min=0 max=10 step=0.01 ");
    TwAddVarRW(bar_, "resolution", TW_TYPE_FLOAT, &geometry_.resolution(), " label='resolution' min=10 max=1000");

    TwAddVarRW(bar_, "speed", TW_TYPE_FLOAT, &metaball_speed_, " label='speed' min=0 max=10 step=0.01 ");
    TwAddVarRW(bar_, "amp", TW_TYPE_FLOAT, &metaball_amplitude_, " label='amplitude' min=0 max=1.5 step=0.01 ");

    TwAddVarRW(bar_, "ObjRotation", TW_TYPE_QUAT4F, &rotation_by_control_,
        " label='Object orientation' opened=true help='Change the object orientation.' ");

    TwAddButton(bar_, "Add metaball", tw_add_metaball, this, " label='Add metaball' ");

    TwEnumVal modeEV[] =
    {
        { draw_mode_t::SOLID, "Solid" },
        { draw_mode_t::WIREFRAME, "Wireframe" },
        { draw_mode_t::WIREFRAME_OVER_SOLID, "Wireframe over solid" }
    };
    TwType modeType = TwDefineEnum("Mode", modeEV, 3);

    TwAddVarRW(bar_, "Draw mode", modeType, &draw_mode_, "");
}

void scene_t::init_metaballs() {
    const int default_cnt = 3;

    for (int i = 0; i < default_cnt; ++i) {
        add_metaball();
    }
}

void scene_t::init_buffer() {
    glGenBuffers(1, &vx_buf_);
}

void scene_t::init_vertex_array() {
    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vx_buf_);
    GLint const pos_location = glGetAttribLocation(program_, "in_pos");
    glVertexAttribPointer(pos_location, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), 0);
    glEnableVertexAttribArray(pos_location);

    glBindVertexArray(0);
};

void scene_t::add_metaball() {
    if (metaballs_.size() == geometry_.MAX_METABALL_CNT) {
        return;
    }

    std::uniform_real_distribution<float> distrib(0, 6);
    unique_ptr<metaball_t> metaball(new metaball_t());

    int id = metaballs_.size() == 0 ? 0 : metaballs_.back()->id + 1;
    metaball->time_shift = distrib(generator_);
    metaball->potential = 0.5;
    metaball->id = id;

    metaballs_.push_back(std::move(metaball));
    
    // setup controls for new metaball
    string str_id = std::to_string(id);
    string metaball_label = potential_prefix + str_id;
    string remove_label = remove_prefix + str_id;
    TwAddVarRW(bar_, metaball_label.c_str(), TW_TYPE_FLOAT, &metaballs_.back()->potential,
        (" min=-5 max=5 step=0.01 group='Metaball #" + str_id + "' label='potential'").c_str());
    TwAddButton(bar_, remove_label.c_str(), tw_remove_metaball, new std::pair<scene_t*, int>(this, id),
        (" group='Metaball #" + str_id + "' label='remove' ").c_str());
}

void scene_t::remove_metaball(int metaball_id) {
    string str_id = std::to_string(metaball_id);
    string metaball_label = potential_prefix + str_id;
    string remove_label = remove_prefix + str_id;

    TwRemoveVar(bar_, metaball_label.c_str());
    TwRemoveVar(bar_, remove_label.c_str());

    for (auto it = metaballs_.begin(); it != metaballs_.end(); ++it) {
        if ((*it)->id == metaball_id) {
            metaballs_.erase(it);
            break;
        }
    }
}

void scene_t::animate(float time_from_start) {
    for (unique_ptr<metaball_t> & metaball : metaballs_) {
        for (int i = 0; i < 3; ++i) {
            metaball->position[i] = metaball_amplitude_ *
                sin(metaball_freq_[i] * metaball_speed_ * time_from_start + metaball->time_shift);
        }
    }
}

void scene_t::draw_frame(float time_from_start) {
    animate(time_from_start);

    if (geometry_.update_grid()) {
        auto& grid = geometry_.grid();
        glBindBuffer(GL_ARRAY_BUFFER, vx_buf_);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * grid.size(), &grid[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    geometry_.update_uniforms(program_);

    draw_metaballs(time_from_start, draw_mode_ == WIREFRAME, true);

    if (draw_mode_ == WIREFRAME_OVER_SOLID) {
        draw_metaballs(time_from_start, true, false);
    }
}

void scene_t::draw_metaballs(float time_from_start, bool wired, bool clear) {
    float const w = (float)glutGet(GLUT_WINDOW_WIDTH);
    float const h = (float)glutGet(GLUT_WINDOW_HEIGHT);

    mat4  const proj = perspective(45.0f, w / h, 0.1f, 100.0f);
    mat4  const view = lookAt(vec3(0, 0, 4), vec3(0, 0, 0), vec3(0, 1, 0));
    mat4 const rotation = mat4_cast(rotation_by_control_);
    mat4  const model = rotation;
    mat4  const mvp = proj * view * model;

    if (wired) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glEnable(GL_POLYGON_OFFSET_LINE);
        glPolygonOffset(-2, -2);
    }
    else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    if (clear) {
        glClearColor(0.2f, 0.2f, 0.2f, 1);
        glClearDepth(1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }


    glUseProgram(program_);

    GLuint const mvp_location = glGetUniformLocation(program_, "mvp");
    glUniformMatrix4fv(mvp_location, 1, GL_FALSE, &mvp[0][0]);

    GLuint const model_loc = glGetUniformLocation(program_, "model");
    glUniformMatrix4fv(model_loc, 1, GL_FALSE, &model[0][0]);

    GLuint const is_wireframe_location = glGetUniformLocation(program_, "is_wireframe");
    glUniform1ui(is_wireframe_location, wired);

    glBindVertexArray(vao_);


    glDrawArrays(GL_POINTS, 0, geometry_.grid().size());

    if (wired) {
        glDisable(GL_POLYGON_OFFSET_LINE);
    }
}
