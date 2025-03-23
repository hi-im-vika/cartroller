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
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigDockingTransparentPayload = true;
    ImGui::StyleColorsDark();

    ImGui_ImplSDL3_InitForOpenGL(_window, _gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Our state
    _show_demo_window = true;
    _show_another_window = false;

    // Init some variables
    _clear_colour = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    _imgui_float = 0.0f;
    _imgui_ctr = 0;
}

CMain::~CMain() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DestroyContext(_gl_context);
    SDL_DestroyWindow(_window);
    SDL_Quit();
}

void CMain::update() {
    _count++;
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
                update_gamepad_list();
                break;
            case SDL_EVENT_GAMEPAD_REMOVED:
                spdlog::info("Gamepad removed");
                SDL_CloseGamepad(SDL_GetGamepadFromID(event.gdevice.which));
                update_gamepad_list();
                break;
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
    ImGui::Text("Choose gamepad:");

    int item_selected_idx = 0;
    std::string combo_preview_value = _num_joysticks ? SDL_GetGamepadName(SDL_GetGamepadFromID(_jss[0])) : "No gamepads connected";

    ImGui::BeginDisabled(!_num_joysticks);
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::BeginCombo("##gpselect", combo_preview_value.c_str())) {
        for (int i = 0; i < _num_joysticks; i++) {
            const bool is_selected = (item_selected_idx == i);
            if (ImGui::Selectable(SDL_GetGamepadName(SDL_GetGamepadFromID(_jss[i])), is_selected)) {
                item_selected_idx = i;
                _gp = SDL_GetGamepadFromID(_jss[i]);
            }
            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
            if (is_selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();
    ImGui::EndDisabled();

    ImGui::Text("This is some useful text.");                   // Display some text (you can use a format strings too)
    ImGui::Checkbox("Demo Window", &_show_demo_window);         // Edit bools storing our window open/close state
    ImGui::Checkbox("Another Window", &_show_another_window);

    ImGui::SliderFloat("float", &_imgui_float, 0.0f, 1.0f);     // Edit 1 float using a slider from 0.0f to 1.0f
    ImGui::ColorEdit3("clear color", (float *) &_clear_colour); // Edit 3 floats representing a color

    if (ImGui::Button("Button")) _imgui_ctr++;                  // Buttons return true when clicked (most widgets return true when edited/activated)
    ImGui::SameLine();
    ImGui::Text("counter = %d", _imgui_ctr);

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
    glClearColor(_clear_colour.x * _clear_colour.w, _clear_colour.y * _clear_colour.w,
                 _clear_colour.z * _clear_colour.w, _clear_colour.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(_window);
    std::this_thread::sleep_until(std::chrono::system_clock::now() + std::chrono::milliseconds(1));
}

void CMain::update_gamepad_list() {
    _jss = SDL_GetGamepads(&_num_joysticks);
    // if joysticks are connected
    if (_num_joysticks) {
        // if no gp assigned already, open first one
        if (!_gp) _gp = SDL_OpenGamepad(_jss[0]);
    } else {
        // if nothing connected, set gp to nullptr
        _gp = nullptr;
    }
}

int main(int argc, char *argv[]) {
    CMain c = CMain();
    c.run();
    return 0;
}