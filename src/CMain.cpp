/**
 * CMain.cpp - new file
 * 2025-03-20
 * vika <https://github.com/hi-im-vika>
 */

#include "../include/CMain.hpp"

CMain::CMain() {
    // Setup SDL
    // [If using SDL_MAIN_USE_CALLBACKS: all code below until the main loop starts would likely be your SDL_AppInit() function]
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        spdlog::error("Error: SDL_Init(): {}", SDL_GetError());
        exit(-1);
    }

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100 (WebGL 1.0)
    const char* glsl_version = "#version 100";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(IMGUI_IMPL_OPENGL_ES3)
    // GL ES 3.0 + GLSL 300 es (WebGL 2.0)
    const char* glsl_version = "#version 300 es";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
    // GL 3.2 Core + GLSL 150
    const char *glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    Uint32 window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN;
    _window = SDL_CreateWindow("Dear ImGui SDL3+OpenGL3 example", 1280, 720, window_flags);
    if (_window == nullptr) {
        spdlog::error("Error: SDL_CreateWindow(): {}", SDL_GetError());
        exit(-1);
    }
    SDL_SetWindowPosition(_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    _gl_context = SDL_GL_CreateContext(_window);
    if (_gl_context == nullptr) {
        spdlog::error("Error: SDL_GL_CreateContext(): {}", SDL_GetError());
        exit(-1);
    }

    SDL_GL_MakeCurrent(_window, _gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync
    SDL_ShowWindow(_window);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
//    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigDockingTransparentPayload = true;
    ImGui::StyleColorsDark();

    ImGui_ImplSDL3_InitForOpenGL(_window, _gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Our state
    _show_demo_window = false;
    _show_another_window = false;

    // Init some variables
    _gp_vals = std::vector<double>(4);
    _gp_sens = std::vector<double>(3);
    _gp_dir = std::vector<double>(3);
}

CMain::~CMain() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    if (_gp) {
        SDL_CloseGamepad(_gp);
    }
    SDL_GL_DestroyContext(_gl_context);
    SDL_DestroyWindow(_window);
    SDL_Quit();
}

void CMain::update() {
//    if (_gp && SDL_GamepadSensorEnabled(_gp,SDL_SENSOR_GYRO)) {
//        spdlog::info(SDL_GetGamepadSensorDataRate(_gp,SDL_SENSOR_GYRO));
//        float data[3] = { 0.0f };
//        if (SDL_GetGamepadSensorData(_gp,SDL_SENSOR_GYRO,data,3)) {
//            _gp_sens = {data[0], data[1], data[2]};
//        }
//    }
    if (_gp && SDL_GamepadSensorEnabled(_gp,SDL_SENSOR_GYRO) && !(_sens_evts.empty())) {
        static SDL_GamepadSensorEvent last_evt;
        static SDL_GamepadSensorEvent this_evt;
        this_evt = _sens_evts.front();
        _sens_evts.pop();
        auto delta_t = (double) (this_evt.sensor_timestamp - last_evt.sensor_timestamp) * 1E-9;
        if (delta_t != 0.0f) {
            double delta_x = (this_evt.data[0] * delta_t);
            double delta_y = (this_evt.data[2] * delta_t);
            double delta_z = (this_evt.data[1] * delta_t);
            if (abs(delta_x) >= 0.0001) _gp_dir.at(0) += delta_x;
            if (abs(delta_y) >= 0.0001) _gp_dir.at(2) += delta_y;
            if (abs(delta_z) >= 0.0001) _gp_dir.at(1) += delta_z;
        }
        last_evt = this_evt;
    } else {
        std::this_thread::sleep_until(std::chrono::system_clock::now() + std::chrono::microseconds(1000));
    }
}

void CMain::draw() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);
        switch (event.type) {
            case SDL_EVENT_QUIT:
                spdlog::info("Quit");
                _do_exit = true;
                break;
            case SDL_EVENT_GAMEPAD_ADDED:
                spdlog::info("Gamepad added");
                _gp = SDL_OpenGamepad(event.gdevice.which);
                break;
            case SDL_EVENT_GAMEPAD_REMOVED:
                spdlog::info("Gamepad removed");
                if (SDL_GetGamepadFromID(event.gdevice.which)) {
                    SDL_CloseGamepad(SDL_GetGamepadFromID(event.gdevice.which));
                }
                break;
            case SDL_EVENT_GAMEPAD_SENSOR_UPDATE:
                if (event.gsensor.sensor == SDL_SENSOR_GYRO) {
                    _gp_sens = {event.gsensor.data[0],event.gsensor.data[1],event.gsensor.data[2]};
                    _sens_evts.emplace(event.gsensor);
                    spdlog::info(event.gsensor.sensor_timestamp);
                }
            case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
            case SDL_EVENT_GAMEPAD_BUTTON_UP:
            case SDL_EVENT_GAMEPAD_AXIS_MOTION:
            default:
                break;
        }
    }

    // [If using SDL_MAIN_USE_CALLBACKS: all code below would likely be your SDL_AppIterate() function]
    if (SDL_GetWindowFlags(_window) & SDL_WINDOW_MINIMIZED) {
        std::this_thread::sleep_until(std::chrono::system_clock::now() + std::chrono::milliseconds(10));
        return;
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    ImGui::DockSpaceOverViewport();
    ImGuiIO &io = ImGui::GetIO();

    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    if (_show_demo_window) ImGui::ShowDemoWindow(&_show_demo_window);

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
    ImGui::Begin("Hello, world!");                              // Create a window called "Hello, world!" and append into it.

    // control settings
    ImGui::SeparatorText("Controls");

    if (_gp) {
        if (SDL_GamepadHasSensor(_gp,SDL_SENSOR_GYRO) && !SDL_GamepadSensorEnabled(_gp,SDL_SENSOR_GYRO))
            SDL_SetGamepadSensorEnabled(_gp,SDL_SENSOR_GYRO,true);
    }

    ImGui::Text("Accel X: %+.5f", _gp_sens.at(0));
    ImGui::Text("Accel Y: %+.5f", _gp_sens.at(2));
    ImGui::Text("Accel Z: %+.5f", _gp_sens.at(1));

    ImGui::Text("Dir X: %+.5f", _gp_dir.at(0));
    ImGui::Text("Dir Y: %+.5f", _gp_dir.at(2));
    ImGui::Text("Dir Z: %+.5f", _gp_dir.at(1));

    ImGui::Text("This is some useful text.");                   // Display some text (you can use a format strings too)
    ImGui::Checkbox("Demo Window", &_show_demo_window);         // Edit bools storing our window open/close state

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
    ImGui::End();

    // 3. Show another simple window.
    if (_show_another_window) {
        ImGui::Begin("Another Window", &_show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        ImGui::Text("Hello from another window!");
        if (ImGui::Button("Close Me"))
            _show_another_window = false;
        ImGui::End();
    }

    // Rendering
    ImGui::Render();
    glViewport(0, 0, (int) io.DisplaySize.x, (int) io.DisplaySize.y);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(_window);
    std::this_thread::sleep_until(std::chrono::system_clock::now() + std::chrono::microseconds(1000));
}

int main(int argc, char *argv[]) {
    CMain c = CMain();
    c.run();
    return 0;
}