#include "scene.h"
#include "shader.h"

#include <algorithm>
#include <sstream>
#include <random>
#include <functional>

#include "FreeImage.h"

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

static const float cube_points[] = {
    -10.0f, 10.0f, -10.0f,
    -10.0f, -10.0f, -10.0f,
    10.0f, -10.0f, -10.0f,
    10.0f, -10.0f, -10.0f,
    10.0f, 10.0f, -10.0f,
    -10.0f, 10.0f, -10.0f,

    -10.0f, -10.0f, 10.0f,
    -10.0f, -10.0f, -10.0f,
    -10.0f, 10.0f, -10.0f,
    -10.0f, 10.0f, -10.0f,
    -10.0f, 10.0f, 10.0f,
    -10.0f, -10.0f, 10.0f,

    10.0f, -10.0f, -10.0f,
    10.0f, -10.0f, 10.0f,
    10.0f, 10.0f, 10.0f,
    10.0f, 10.0f, 10.0f,
    10.0f, 10.0f, -10.0f,
    10.0f, -10.0f, -10.0f,

    -10.0f, -10.0f, 10.0f,
    -10.0f, 10.0f, 10.0f,
    10.0f, 10.0f, 10.0f,
    10.0f, 10.0f, 10.0f,
    10.0f, -10.0f, 10.0f,
    -10.0f, -10.0f, 10.0f,

    -10.0f, 10.0f, -10.0f,
    10.0f, 10.0f, -10.0f,
    10.0f, 10.0f, 10.0f,
    10.0f, 10.0f, 10.0f,
    -10.0f, 10.0f, 10.0f,
    -10.0f, 10.0f, -10.0f,

    -10.0f, -10.0f, -10.0f,
    -10.0f, -10.0f, 10.0f,
    10.0f, -10.0f, -10.0f,
    10.0f, -10.0f, -10.0f,
    -10.0f, -10.0f, 10.0f,
    10.0f, -10.0f, 10.0f
};


scene_t::scene_t()
        : geometry_(4.2f, 30, vec3(-1, -1, -1), vec3(1, 1, 1), metaballs_),
          draw_mode_(draw_mode_t::SOLID),
          metaball_speed_(0.1f),
          metaball_amplitude_(0.5f) {
    metaball_freq_ = { 4, 5, 6 };

    lighting_.pos = vec3(5, 0, 0);
    lighting_.ambient = 0.5f;
    lighting_.diffuse = 0.8f;
    lighting_.specular = 0.2f;
    lighting_.shininess = 50;

    init_controls();
    init_metaballs();

    metaball_vs_ = create_shader(GL_VERTEX_SHADER, "shaders//metaball.vp");
    metaball_fs_ = create_shader(GL_FRAGMENT_SHADER, "shaders//metaball.fp");
    metaball_gs_ = create_shader(GL_GEOMETRY_SHADER, "shaders//metaball.geom");
    metaball_program_ = create_program(metaball_vs_, metaball_fs_, metaball_gs_);

    cube_vs_ = create_shader(GL_VERTEX_SHADER, "shaders//cubemap.vp");
    cube_fs_ = create_shader(GL_FRAGMENT_SHADER, "shaders//cubemap.fp");
    cube_program_ = create_program(cube_vs_, cube_fs_);

    geometry_.init_tables(metaball_program_);
    load_cubemap_texture();
    init_buffer();
    init_vertex_array();
}

scene_t::~scene_t() {
    glDeleteVertexArrays(1, &metaball_vao_);
    glDeleteVertexArrays(1, &cube_vao_);
    glDeleteBuffers(1, &metaball_vx_buf_);
    glDeleteBuffers(1, &cube_vx_buf_);

    for (GLuint shader : {metaball_vs_, metaball_fs_, metaball_gs_,
                          cube_vs_, cube_fs_}) {
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
    TwDefine(" Parameters size='300 300' color='70 100 120' alpha=200 valueswidth=150 iconpos=topleft");

    TwAddButton(bar_, "Fullscreen toggle", tw_toggle_fullscreen, NULL,
        " label='Toggle fullscreen mode' key=f");

    TwAddVarRW(bar_, "isolevel", TW_TYPE_FLOAT, &geometry_.isolevel(), " label='isolevel' min=0 max=10 step=0.01 ");
    TwAddVarRW(bar_, "resolution", TW_TYPE_FLOAT, &geometry_.resolution(), " label='resolution' min=10 max=1000");

    TwAddVarRW(bar_, "speed", TW_TYPE_FLOAT, &metaball_speed_, " label='speed' min=0 max=10 step=0.01 ");
    TwAddVarRW(bar_, "amp", TW_TYPE_FLOAT, &metaball_amplitude_, " label='amplitude' min=0 max=1.5 step=0.01 ");

    TwAddVarRW(bar_, "light_dir", TW_TYPE_DIR3F, &lighting_.pos, " group='light' label='direction' ");
    TwAddVarRW(bar_, "light_ambient", TW_TYPE_FLOAT, &lighting_.ambient, " group='light' label='ambient' min=0 max=1 step=0.01");
    TwAddVarRW(bar_, "light_diffuse", TW_TYPE_FLOAT, &lighting_.diffuse, " group='light' label='diffuse' min=0 max=1 step=0.01");
    TwAddVarRW(bar_, "light_specular", TW_TYPE_FLOAT, &lighting_.specular, " group='light' label='specular' min=0 max=1 step=0.01");
    TwAddVarRW(bar_, "light_shininess", TW_TYPE_FLOAT, &lighting_.shininess, " group='light' label='shininess' min = 0 max=1000");

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
    glGenBuffers(1, &metaball_vx_buf_);

    glGenBuffers(1, &cube_vx_buf_);
    glBindBuffer(GL_ARRAY_BUFFER, cube_vx_buf_);
        glBufferData(GL_ARRAY_BUFFER, 3 * 36 * sizeof(float), &cube_points, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void scene_t::init_vertex_array() {
    glGenVertexArrays(1, &metaball_vao_);
    glBindVertexArray(metaball_vao_);
    glBindBuffer(GL_ARRAY_BUFFER, metaball_vx_buf_);
    GLint const pos_location = glGetAttribLocation(metaball_program_, "in_pos");
    glVertexAttribPointer(pos_location, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), 0);
    glEnableVertexAttribArray(pos_location);

    glBindVertexArray(0);

    glGenVertexArrays(1, &cube_vao_);
    glBindVertexArray(cube_vao_);
    glBindBuffer(GL_ARRAY_BUFFER, cube_vx_buf_);
    GLuint const cube_pos_loc = glGetAttribLocation(cube_program_, "in_pos");
    glVertexAttribPointer(cube_pos_loc, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), 0);
    glEnableVertexAttribArray(cube_pos_loc);
    glBindVertexArray(0);
};

void scene_t::add_metaball() {
    if (metaballs_.size() == geometry_.MAX_METABALL_CNT) {
        return;
    }

    std::uniform_real_distribution<float> distrib(0, 6);
    std::uniform_int_distribution<int> uniform_int(1, 10);
    unique_ptr<metaball_t> metaball(new metaball_t());

    int id = metaballs_.size() == 0 ? 0 : metaballs_.back()->id + 1;
    metaball->shift = vec3(distrib(generator_), distrib(generator_), distrib(generator_));
    metaball->freq = vec3(uniform_int(generator_), uniform_int(generator_), uniform_int(generator_));

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
        metaball->position = metaball_amplitude_ *
            glm::sin(metaball->freq * metaball_speed_ * time_from_start + metaball->shift);
    }
}

void scene_t::draw_frame(float time_from_start) {
    animate(time_from_start);

    if (geometry_.update_grid()) {
        auto& grid = geometry_.grid();
        glBindBuffer(GL_ARRAY_BUFFER, metaball_vx_buf_);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * grid.size(), &grid[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    geometry_.update_uniforms(metaball_program_);
    set_lights();

    float const w = (float)glutGet(GLUT_WINDOW_WIDTH);
    float const h = (float)glutGet(GLUT_WINDOW_HEIGHT);
    mat4 proj = perspective(45.0f, w / h, 0.1f, 100.0f);
    mat4 world_rotation = mat4_cast(quat(-0.27, 0, -0.96, 0));
    mat4 view = lookAt(vec3(0, 0, 4), vec3(0, 0, 0), vec3(0, -1, 0));
    view = view * world_rotation;

    draw_cubemap(view, proj);

    draw_metaballs(view, proj, draw_mode_ == WIREFRAME);

    if (draw_mode_ == WIREFRAME_OVER_SOLID) {
        draw_metaballs(view, proj, true);
    }
}

void scene_t::set_lights() {
    glUseProgram(metaball_program_);

    GLuint light_pos_loc = glGetUniformLocation(metaball_program_, "light.pos");
    GLuint light_ambient_loc = glGetUniformLocation(metaball_program_, "light.ambient");
    GLuint light_diffuse_loc = glGetUniformLocation(metaball_program_, "light.diffuse");
    GLuint light_specular_loc = glGetUniformLocation(metaball_program_, "light.specular");
    GLuint light_shininess_loc = glGetUniformLocation(metaball_program_, "light.shininess");

    vec3 light_pos = -lighting_.pos;
    glUniform3fv(light_pos_loc, 1, &light_pos[0]);
    glUniform1f(light_ambient_loc, lighting_.ambient);
    glUniform1f(light_diffuse_loc, lighting_.diffuse);
    glUniform1f(light_specular_loc, lighting_.specular);
    glUniform1f(light_shininess_loc, lighting_.shininess);
}

void scene_t::draw_metaballs(mat4 const & view, mat4 const & proj, bool wired) {
    mat4 const rotation = mat4_cast(rotation_by_control_);
    mat4 const model = rotation;
    mat4 const mv = view * model;
    mat4 const mvp = proj * view * model;

    if (wired) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glEnable(GL_POLYGON_OFFSET_LINE);
        glPolygonOffset(-2, -2);
    }
    else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    glUseProgram(metaball_program_);

    GLuint const mvp_location = glGetUniformLocation(metaball_program_, "mvp");
    glUniformMatrix4fv(mvp_location, 1, GL_FALSE, &mvp[0][0]);

    GLuint const mv_loc = glGetUniformLocation(metaball_program_, "mv");
    glUniformMatrix4fv(mv_loc, 1, GL_FALSE, &mv[0][0]);

    mat4 inv_view = glm::inverse(view);
    GLuint const view_loc = glGetUniformLocation(metaball_program_, "inv_view");
    glUniformMatrix4fv(view_loc, 1, GL_FALSE, &inv_view[0][0]);

    GLuint const is_wireframe_location = glGetUniformLocation(metaball_program_, "is_wireframe");
    glUniform1ui(is_wireframe_location, wired);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, tex_cube_);
    GLuint const tex_loc = glGetUniformLocation(metaball_program_, "cube_texture");
    glUniform1i(tex_loc, 1);

    glBindVertexArray(metaball_vao_);

    glDrawArrays(GL_POINTS, 0, geometry_.grid().size());

    if (wired) {
        glDisable(GL_POLYGON_OFFSET_LINE);
    }
}

void scene_t::draw_cubemap(mat4 const & view, mat4 const & proj) {
    mat4 mvp = proj * view;

    glClearColor(0.2f, 0.2f, 0.2f, 1);
    glClearDepth(1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDepthMask(GL_FALSE);

    glUseProgram(cube_program_);

    GLuint const mvp_location = glGetUniformLocation(cube_program_, "mvp");
    glUniformMatrix4fv(mvp_location, 1, GL_FALSE, &mvp[0][0]);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, tex_cube_);
    GLuint const tex_loc = glGetUniformLocation(cube_program_, "cube_texture");
    glUniform1i(tex_loc, 1);

    glBindVertexArray(cube_vao_);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    glDepthMask(GL_TRUE);
}

void scene_t::load_cubemap_texture() {
    glActiveTexture(GL_TEXTURE1);
    glGenTextures(1, &tex_cube_);

    bool ok = true;
    ok = ok && load_cube_side(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, cube_tex_path + "negx.jpg");
    ok = ok && load_cube_side(GL_TEXTURE_CUBE_MAP_POSITIVE_X, cube_tex_path + "posx.jpg");
    ok = ok && load_cube_side(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, cube_tex_path + "negy.jpg");
    ok = ok && load_cube_side(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, cube_tex_path + "posy.jpg");
    ok = ok && load_cube_side(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, cube_tex_path + "negz.jpg");
    ok = ok && load_cube_side(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, cube_tex_path + "posz.jpg");

    if (!ok) {
        std::cerr << "Failed to load cubemap texture" << std::endl;
        exit(EXIT_FAILURE);
    }

    glBindTexture(GL_TEXTURE_CUBE_MAP, tex_cube_);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

bool scene_t::load_cube_side(GLenum side_target, std::string const & file_name) {
    FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;
    FIBITMAP *dib = 0;

    fif = FreeImage_GetFileType(file_name.c_str(), 0);
    if (fif == FIF_UNKNOWN) {
        fif = FreeImage_GetFIFFromFilename(file_name.c_str());
    }

    if (FreeImage_FIFSupportsReading(fif)) {
        dib = FreeImage_Load(fif, file_name.c_str());
    }

    if (!dib) {
        return false;
    }

    BYTE* bits = 0;
    unsigned int width = 0, height = 0;

    bits = FreeImage_GetBits(dib);
    width = FreeImage_GetWidth(dib);
    height = FreeImage_GetHeight(dib);

    if ((bits == 0) || (width == 0) || (height == 0)) {
        return false;
    }


    glBindTexture(GL_TEXTURE_CUBE_MAP, tex_cube_);
    glTexImage2D(side_target, 0, GL_RGB8, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, bits);

    FreeImage_Unload(dib);
    return true;
}