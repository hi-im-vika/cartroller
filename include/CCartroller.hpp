/**
 * CCartroller.hpp - new file
 * 2025-03-20
 * vika <https://github.com/hi-im-vika>
 */

#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#define FOURCC_DXT1 0x31545844 // Equivalent to "DXT1" in ASCII
#define FOURCC_DXT3 0x33545844 // Equivalent to "DXT3" in ASCII
#define FOURCC_DXT5 0x35545844 // Equivalent to "DXT5" in ASCII

#include <iostream>
#include <sstream>
#include <fstream>
#include <queue>
#include <spdlog/spdlog.h>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>
#include <SDL3/SDL.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL3/SDL_opengles2.h>
#else
#include <SDL3/SDL_opengl.h>
#endif

#include "CBase.hpp"
#include "Mesh.hpp"

class CCartroller : public CBase {
private:
    // SDL
    SDL_Window* _window;
    SDL_GLContext _gl_context;

    // OpenGL
    GLuint _VAO, _VBO, _uv_buf;
    GLuint _program_id, _matrix_id, _view_matrix_id, _model_matrix_id;
    GLuint _texture;
    GLuint _texture_id;

    glm::vec3 _orientation;
    glm::vec3 _position;
    Mesh _cube;
    Uint32 _last_update;

    // control
    SDL_Gamepad* _gp;

    std::vector<double> _gyro_vals;
    std::vector<double> _accl_vals;
    std::queue<SDL_GamepadSensorEvent> _gyro_evts;
    std::queue<SDL_GamepadSensorEvent> _accl_evts;
    std::vector<std::vector<double>> _log_gyro_values;
    std::vector<std::vector<double>> _log_accl_values;
    std::vector<u_int64_t> _log_gyro_timestamps;
    std::vector<u_int64_t> _log_accl_timestamps;
    bool _do_log;

    // imgui
    bool _show_demo_window;
    bool _show_another_window;

    std::string _text;
    unsigned int _count;

    GLuint load_shaders(const char *vertex_file_path, const char *fragment_file_path);
    GLuint load_dds(const char *imagepath);
    bool load_obj(
            const char * path,
            std::vector<glm::vec3> & out_vertices,
            std::vector<glm::vec2> & out_uvs,
            std::vector<glm::vec3> & out_normals
    );
public:
    CCartroller();
    ~CCartroller();

    void update() override;
    void draw() override;
};