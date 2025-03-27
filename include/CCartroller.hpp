/**
 * CCartroller.hpp - new file
 * 2025-03-20
 * vika <https://github.com/hi-im-vika>
 */

#pragma once

#include <iostream>
#include <fstream>
#include <spdlog/spdlog.h>
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

class CCartroller : public CBase {
private:
    // SDL
    SDL_Window* _window;
    SDL_GLContext _gl_context;

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
public:
    CCartroller();
    ~CCartroller();

    void update() override;
    void draw() override;
};