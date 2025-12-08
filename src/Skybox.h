#pragma once
#include <GL/glew.h>
#include "ShaderProgram.h"

class Skybox {
public:
    GLuint VAO, VBO;
    ShaderProgram* shader;
    GLuint textureID;
    
    Skybox(const char* vertPath, const char* fragPath);
    ~Skybox();
    void draw(const float* view, const float* projection);
    void loadTexture(const char* imagePath);
    
private:
    void setupMesh();
};