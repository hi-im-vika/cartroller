/**
 * Mesh.hpp - new file
 * 2025-03-27
 * vika <https://github.com/hi-im-vika>
 */


#pragma once
#include <vector>
#include <SDL3/SDL.h>
#include <iostream>
#include <GL/glew.h>

class Mesh {
public:
    Mesh();
    void draw();
    void init();
private:
    GLuint VertexArrayID, vertexbuffer, elementbuffer, vertex_size;
};
