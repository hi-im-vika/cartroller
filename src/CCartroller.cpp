/**
 * CCartroller.cpp - new file
 * 2025-03-20
 * vika <https://github.com/hi-im-vika>
 */

#include "../include/CCartroller.hpp"

CCartroller::CCartroller() {
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

    // Initialize GLEW
    glewExperimental = true; // Needed for core profile
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        exit(-1);
    }

    // ogl

    _program_id = load_shaders("../shader/vertex.glsl", "../shader/fragment.glsl");

    _cube.init();
    _last_update = SDL_GetTicks();
    _last_ori = {0.0f, 0.0f, 0.0f};

    // ImGui setup here
    {

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
        _gyro_vals = std::vector<double>(3);
        _accl_vals = std::vector<double>(3);

        _do_log = false;
    }
    _demo_rotate = false;
}

CCartroller::~CCartroller() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    if (_gp) SDL_CloseGamepad(_gp);
    glDeleteProgram(_program_id);
    SDL_GL_DestroyContext(_gl_context);
    SDL_DestroyWindow(_window);
    SDL_Quit();
}

void CCartroller::update() {
    // immediately skip update if no values available
    if (_gyro_evts.empty() || _accl_evts.empty()) {
        std::this_thread::sleep_until(std::chrono::system_clock::now() + std::chrono::microseconds(1000));
        return;
    }

    // if gyro values exist
    if (_gp && SDL_GamepadSensorEnabled(_gp,SDL_SENSOR_GYRO) && !(_gyro_evts.empty())) {
        SDL_GamepadSensorEvent this_evt = _gyro_evts.front();
        _gyro_evts.pop();
        if(_gyro_n.size() > 2) _gyro_n.pop_back();
        _gyro_n.emplace_front(std::vector<double>{(double) this_evt.sensor_timestamp, this_evt.data[0], this_evt.data[1], this_evt.data[2]});
        if (_do_log) {
            _log_gyro_values.emplace_back(std::vector<double>{this_evt.data[0], this_evt.data[1], this_evt.data[2]});
            _log_gyro_timestamps.emplace_back(this_evt.sensor_timestamp);
        }
    }

    // if accl values exist
    if (_gp && SDL_GamepadSensorEnabled(_gp,SDL_SENSOR_ACCEL) && !(_accl_vals.empty())) {
        SDL_GamepadSensorEvent this_evt = _accl_evts.front();
        if(_accl_n.size() > 2) _accl_n.pop_back();
        _accl_n.emplace_front(std::vector<double>{(double) this_evt.sensor_timestamp, this_evt.data[0], this_evt.data[1], this_evt.data[2]});
        _accl_evts.pop();
        if (_do_log) {
            _log_accl_values.emplace_back(std::vector<double>{this_evt.data[0], this_evt.data[1], this_evt.data[2]});
            _log_accl_timestamps.emplace_back(this_evt.sensor_timestamp);
        }
    }

    if (!_do_log) {
        if (!(_log_gyro_values.empty()) || !(_log_accl_values.empty())) {
            std::ofstream csv_out("data.csv");
            std::stringstream ss;
            for (int i = 0; i < _log_gyro_values.size(); i++) {
                ss.str(std::string());
                ss << _log_gyro_timestamps.at(i) << "," <<
                   _log_gyro_values.at(i).at(0) << "," <<
                   _log_gyro_values.at(i).at(1) << "," <<
                   _log_gyro_values.at(i).at(2) << "," <<
                   _log_accl_timestamps.at(i) << "," <<
                   _log_accl_values.at(i).at(0) << "," <<
                   _log_accl_values.at(i).at(1) << "," <<
                   _log_accl_values.at(i).at(2) << std::endl;
                csv_out << ss.str();
            }
            csv_out.close();
            _log_gyro_values.clear();
            _log_gyro_timestamps.clear();
            _log_accl_values.clear();
            _log_accl_timestamps.clear();
        }
    }
}

void CCartroller::draw() {
    // poll and handle events
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
                    _gyro_vals = {event.gsensor.data[0], event.gsensor.data[1], event.gsensor.data[2]};
                    _gyro_evts.emplace(event.gsensor);
                }
                if (event.gsensor.sensor == SDL_SENSOR_ACCEL) {
                    _accl_vals = {event.gsensor.data[0], event.gsensor.data[1], event.gsensor.data[2]};
                    _accl_evts.emplace(event.gsensor);
                }
            case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
            case SDL_EVENT_GAMEPAD_BUTTON_UP:
            case SDL_EVENT_GAMEPAD_AXIS_MOTION:
            default:
                break;
        }
    }

    // don't do anything if window minimized
    if (SDL_GetWindowFlags(_window) & SDL_WINDOW_MINIMIZED) {
        std::this_thread::sleep_until(std::chrono::system_clock::now() + std::chrono::milliseconds(10));
        return;
    }

    // ogl
    Uint32 current = SDL_GetTicks();
    int w, h;
    SDL_GetWindowSize(_window, &w, &h);
    glViewport(0, 0, w, h);

    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

//    float dT = (current - _last_update / 1000.0f);
    auto model = glm::mat4(1.0f);

    // if sensor new sensor data
    if (_gyro_n.size() >= 2 || _accl_n.size() >= 2) {
//        if (_demo_rotate) {
//            _demo_rotate = false;
//            _last_ori = {0.0f, 0.0f, 0.0f};
//        }
        // get values
        std::vector<double> gyro_now = {_gyro_n.at(0).at(1), _gyro_n.at(0).at(2), _gyro_n.at(0).at(3)};
        std::vector<double> accl_now = {_accl_n.at(0).at(1), _accl_n.at(0).at(2), _accl_n.at(0).at(3)};
        auto delta_t = (float) ((_gyro_n.at(0).at(0) - _gyro_n.at(1).at(0)) * 1E-9);

        // create correction matrix for rotation
        double roll_rad = atan2(accl_now.at(1), accl_now.at(2));
        double pitch_rad = atan2(-accl_now.at(0), sqrt(pow(accl_now.at(1), 2) + pow(accl_now.at(2), 2)));
        double mat_roll[9] = {1, 0, 0,
                              0, cos(roll_rad), -sin(roll_rad),
                              0, sin(roll_rad), cos(roll_rad)
        };
        double mat_pitch[9] = {cos(pitch_rad), 0, sin(pitch_rad),
                               0, 1, 0,
                               -sin(pitch_rad), 0, cos(pitch_rad)
        };
        glm::mat3 R_roll = glm::transpose(glm::make_mat3(mat_roll));
        glm::mat3 R_pitch = glm::transpose(glm::make_mat3(mat_pitch));
        glm::mat3 R = R_pitch * R_roll;

        // get column vector of uncorrected gyro values and multiply with correction matrix
        glm::vec3 inst_avel = {gyro_now.at(0), gyro_now.at(1), gyro_now.at(2)};
        glm::vec3 fix_avel = R * inst_avel;

        // integrate angular velocity and add to previous position
        _last_ori += fix_avel * delta_t;

//        auto qf = glm::quat(glm::vec3(_last_ori[0], _last_ori[1], _last_ori[2])); // quat

//        spdlog::info("gyro avel (rad/s): {:+.6f} {:+.6f} {:+.6f} delta_t {:.6f} s", fix_avel[0], fix_avel[1],
//                     fix_avel[2], delta_t);
//        spdlog::info("rot (rad): {:+.6f} {:+.6f} {:+.6f}", _last_ori[0], _last_ori[1], _last_ori[2]);
        model = glm::rotate(model, -_last_ori[2], glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::rotate(model, -_last_ori[1], glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, _last_ori[0], glm::vec3(1.0f, 0.0f, 0.0f));
    }
//    } else {
//        auto qf = glm::quat(glm::vec3(0.0f, 0.0f, 1.0f));
//        model = glm::rotate(model, glm::radians(0.1f)*dT, glm::vec3(qf.x, qf.y, qf.z));
//    }


    auto view = glm::mat4(1.0f);
//    view = glm::translate(view, glm::vec3(0.0f, 0.0f, -3.0f));
    view = glm::translate(view, glm::vec3(0.0f, 0.0f, -3.0f));
    view = glm::rotate(view, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat4 projection;
    projection = glm::perspective(glm::radians(60.0f), float(w)/(float)h, 0.1f, 100.0f);

    int modelLoc = glGetUniformLocation(_program_id, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    int viewLoc = glGetUniformLocation(_program_id, "view");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    int projectionLoc = glGetUniformLocation(_program_id, "projection");
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glUseProgram(_program_id);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    _cube.draw();

    // draw imgui stuff on top
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    ImGui::DockSpaceOverViewport(0,nullptr,ImGuiDockNodeFlags_PassthruCentralNode);
    ImGuiIO &io = ImGui::GetIO();

    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    if (_show_demo_window) ImGui::ShowDemoWindow(&_show_demo_window);

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
    ImGui::Begin("Hello, world!");                              // Create a window called "Hello, world!" and append into it.

    // control settings
    ImGui::SeparatorText("Controls");

    // enable both gyro and accelerometer in controller
    if (_gp) {
        if (SDL_GamepadHasSensor(_gp,SDL_SENSOR_GYRO) && !SDL_GamepadSensorEnabled(_gp,SDL_SENSOR_GYRO))
            SDL_SetGamepadSensorEnabled(_gp,SDL_SENSOR_GYRO,true);
        if (SDL_GamepadHasSensor(_gp,SDL_SENSOR_ACCEL) && !SDL_GamepadSensorEnabled(_gp,SDL_SENSOR_ACCEL))
            SDL_SetGamepadSensorEnabled(_gp,SDL_SENSOR_ACCEL,true);
    }

    ImGui::Text("Gyro X: %+.5f rad/s", _gyro_vals.at(0));
    ImGui::Text("Gyro Y: %+.5f rad/s", _gyro_vals.at(1));
    ImGui::Text("Gyro Z: %+.5f rad/s", _gyro_vals.at(2));

    ImGui::Text("Accl X: %+.5f m/s^2", _accl_vals.at(0));
    ImGui::Text("Accl Y: %+.5f m/s^2", _accl_vals.at(1));
    ImGui::Text("Accl Z: %+.5f m/s^2", _accl_vals.at(2));

    ImGui::Text("Angl X: %+.5f rad", _last_ori[0]);
    ImGui::Text("Angl Y: %+.5f rad", _last_ori[1]);
    ImGui::Text("Angl Z: %+.5f rad", _last_ori[2]);

    ImGui::Text("This is some useful text.");                   // Display some text (you can use a format strings too)
    ImGui::Checkbox("Demo Window", &_show_demo_window);         // Edit bools storing our window open/close state

    ImGui::BeginDisabled(_do_log);
    if(ImGui::Button("Start log")) _do_log = true;
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(!_do_log);
    if(ImGui::Button("Stop")) _do_log = false;
    ImGui::EndDisabled();

    if(ImGui::Button("Reset Orientation")) _last_ori = {0.0f, 0.0f, 0.0f};

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
    ImGui::End();

    // Rendering
    ImGui::Render();
    glViewport(0, 0, (int) io.DisplaySize.x, (int) io.DisplaySize.y);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(_window);
//    std::this_thread::sleep_until(std::chrono::system_clock::now() + std::chrono::microseconds(1000));
}

GLuint CCartroller::load_shaders(const char * vertex_file_path,const char * fragment_file_path){

    // Create the shaders
    GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

    // Read the Vertex Shader code from the file
    std::string VertexShaderCode;
    std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
    if(VertexShaderStream.is_open()){
        std::stringstream sstr;
        sstr << VertexShaderStream.rdbuf();
        VertexShaderCode = sstr.str();
        VertexShaderStream.close();
    }else{
        printf("Impossible to open %s. Are you in the right directory ? Don't forget to read the FAQ !\n", vertex_file_path);
        getchar();
        return 0;
    }

    // Read the Fragment Shader code from the file
    std::string FragmentShaderCode;
    std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
    if(FragmentShaderStream.is_open()){
        std::stringstream sstr;
        sstr << FragmentShaderStream.rdbuf();
        FragmentShaderCode = sstr.str();
        FragmentShaderStream.close();
    }

    GLint Result = GL_FALSE;
    int InfoLogLength;


    // Compile Vertex Shader
    printf("Compiling shader : %s\n", vertex_file_path);
    char const * VertexSourcePointer = VertexShaderCode.c_str();
    glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
    glCompileShader(VertexShaderID);

    // Check Vertex Shader
    glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if ( InfoLogLength > 0 ){
        std::vector<char> VertexShaderErrorMessage(InfoLogLength+1);
        glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
        printf("%s\n", &VertexShaderErrorMessage[0]);
    }

    // Compile Fragment Shader
    printf("Compiling shader : %s\n", fragment_file_path);
    char const * FragmentSourcePointer = FragmentShaderCode.c_str();
    glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
    glCompileShader(FragmentShaderID);

    // Check Fragment Shader
    glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if ( InfoLogLength > 0 ){
        std::vector<char> FragmentShaderErrorMessage(InfoLogLength+1);
        glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
        printf("%s\n", &FragmentShaderErrorMessage[0]);
    }



    // Link the program
    printf("Linking program\n");
    GLuint ProgramID = glCreateProgram();
    glAttachShader(ProgramID, VertexShaderID);
    glAttachShader(ProgramID, FragmentShaderID);
    glLinkProgram(ProgramID);

    // Check the program
    glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
    glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if ( InfoLogLength > 0 ){
        std::vector<char> ProgramErrorMessage(InfoLogLength+1);
        glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
        printf("%s\n", &ProgramErrorMessage[0]);
    }


    glDetachShader(ProgramID, VertexShaderID);
    glDetachShader(ProgramID, FragmentShaderID);

    glDeleteShader(VertexShaderID);
    glDeleteShader(FragmentShaderID);

    return ProgramID;
}

GLuint CCartroller::load_dds(const char * imagepath){

    unsigned char header[124];

    FILE *fp;

    /* try to open the file */
    fp = fopen(imagepath, "rb");
    if (fp == NULL){
        printf("%s could not be opened. Are you in the right directory ? Don't forget to read the FAQ !\n", imagepath); getchar();
        return 0;
    }

    /* verify the type of file */
    char filecode[4];
    fread(filecode, 1, 4, fp);
    if (strncmp(filecode, "DDS ", 4) != 0) {
        fclose(fp);
        return 0;
    }

    /* get the surface desc */
    fread(&header, 124, 1, fp);

    unsigned int height      = *(unsigned int*)&(header[8 ]);
    unsigned int width	     = *(unsigned int*)&(header[12]);
    unsigned int linearSize	 = *(unsigned int*)&(header[16]);
    unsigned int mipMapCount = *(unsigned int*)&(header[24]);
    unsigned int fourCC      = *(unsigned int*)&(header[80]);


    unsigned char * buffer;
    unsigned int bufsize;
    /* how big is it going to be including all mipmaps? */
    bufsize = mipMapCount > 1 ? linearSize * 2 : linearSize;
    buffer = (unsigned char*)malloc(bufsize * sizeof(unsigned char));
    fread(buffer, 1, bufsize, fp);
    /* close the file pointer */
    fclose(fp);

    unsigned int components  = (fourCC == FOURCC_DXT1) ? 3 : 4;
    unsigned int format;
    switch(fourCC)
    {
        case FOURCC_DXT1:
            format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
            break;
        case FOURCC_DXT3:
            format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
            break;
        case FOURCC_DXT5:
            format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
            break;
        default:
            free(buffer);
            return 0;
    }

    // Create one OpenGL texture
    GLuint textureID;
    glGenTextures(1, &textureID);

    // "Bind" the newly created texture : all future texture functions will modify this texture
    glBindTexture(GL_TEXTURE_2D, textureID);
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);

    unsigned int blockSize = (format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) ? 8 : 16;
    unsigned int offset = 0;

    /* load the mipmaps */
    for (unsigned int level = 0; level < mipMapCount && (width || height); ++level)
    {
        unsigned int size = ((width+3)/4)*((height+3)/4)*blockSize;
        glCompressedTexImage2D(GL_TEXTURE_2D, level, format, width, height,
                               0, size, buffer + offset);

        offset += size;
        width  /= 2;
        height /= 2;

        // Deal with Non-Power-Of-Two textures. This code is not included in the webpage to reduce clutter.
        if(width < 1) width = 1;
        if(height < 1) height = 1;

    }

    free(buffer);

    return textureID;


}

bool CCartroller::load_obj(
        const char * path,
        std::vector<glm::vec3> & out_vertices,
        std::vector<glm::vec2> & out_uvs,
        std::vector<glm::vec3> & out_normals
){
    printf("Loading OBJ file %s...\n", path);

    std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
    std::vector<glm::vec3> temp_vertices;
    std::vector<glm::vec2> temp_uvs;
    std::vector<glm::vec3> temp_normals;


    FILE * file = fopen(path, "r");
    if( file == NULL ){
        printf("Impossible to open the file ! Are you in the right path ? See Tutorial 1 for details\n");
        getchar();
        return false;
    }

    while( 1 ){

        char lineHeader[128];
        // read the first word of the line
        int res = fscanf(file, "%s", lineHeader);
        if (res == EOF)
            break; // EOF = End Of File. Quit the loop.

        // else : parse lineHeader

        if ( strcmp( lineHeader, "v" ) == 0 ){
            glm::vec3 vertex;
            fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z );
            temp_vertices.push_back(vertex);
        }else if ( strcmp( lineHeader, "vt" ) == 0 ){
            glm::vec2 uv;
            fscanf(file, "%f %f\n", &uv.x, &uv.y );
            uv.y = -uv.y; // Invert V coordinate since we will only use DDS texture, which are inverted. Remove if you want to use TGA or BMP loaders.
            temp_uvs.push_back(uv);
        }else if ( strcmp( lineHeader, "vn" ) == 0 ){
            glm::vec3 normal;
            fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z );
            temp_normals.push_back(normal);
        }else if ( strcmp( lineHeader, "f" ) == 0 ){
            std::string vertex1, vertex2, vertex3;
            unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
            int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2] );
            if (matches != 9){
                printf("File can't be read by our simple parser :-( Try exporting with other options\n");
                fclose(file);
                return false;
            }
            vertexIndices.push_back(vertexIndex[0]);
            vertexIndices.push_back(vertexIndex[1]);
            vertexIndices.push_back(vertexIndex[2]);
            uvIndices    .push_back(uvIndex[0]);
            uvIndices    .push_back(uvIndex[1]);
            uvIndices    .push_back(uvIndex[2]);
            normalIndices.push_back(normalIndex[0]);
            normalIndices.push_back(normalIndex[1]);
            normalIndices.push_back(normalIndex[2]);
        }else{
            // Probably a comment, eat up the rest of the line
            char stupidBuffer[1000];
            fgets(stupidBuffer, 1000, file);
        }

    }

    // For each vertex of each triangle
    for( unsigned int i=0; i<vertexIndices.size(); i++ ){

        // Get the indices of its attributes
        unsigned int vertexIndex = vertexIndices[i];
        unsigned int uvIndex = uvIndices[i];
        unsigned int normalIndex = normalIndices[i];

        // Get the attributes thanks to the index
        glm::vec3 vertex = temp_vertices[ vertexIndex-1 ];
        glm::vec2 uv = temp_uvs[ uvIndex-1 ];
        glm::vec3 normal = temp_normals[ normalIndex-1 ];

        // Put the attributes in buffers
        out_vertices.push_back(vertex);
        out_uvs     .push_back(uv);
        out_normals .push_back(normal);

    }
    fclose(file);
    return true;
}

int main(int argc, char *argv[]) {
    CCartroller c = CCartroller();
    c.run();
    return 0;
}