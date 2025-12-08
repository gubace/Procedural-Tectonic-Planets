#pragma once
#include <GL/glew.h>
#include <GL/gl.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include "Vec3.h"

class ShaderProgram {
public:
    GLuint programID;
    
    ShaderProgram(const char* vertexPath, const char* fragmentPath, const char* geometryPath = nullptr) {
        std::string vertexCode = readFile(vertexPath);
        std::string fragmentCode = readFile(fragmentPath);
        
        if (vertexCode.empty()) {
            std::cerr << "ERROR: Vertex shader file is empty or not found: " << vertexPath << std::endl;
            exit(EXIT_FAILURE);
        }
        if (fragmentCode.empty()) {
            std::cerr << "ERROR: Fragment shader file is empty or not found: " << fragmentPath << std::endl;
            exit(EXIT_FAILURE);
        }
        
        GLuint vertex = compileShader(vertexCode.c_str(), GL_VERTEX_SHADER, "VERTEX");
        GLuint fragment = compileShader(fragmentCode.c_str(), GL_FRAGMENT_SHADER, "FRAGMENT");
        
        GLuint geometry = 0;
        if (geometryPath != nullptr) {
            std::string geometryCode = readFile(geometryPath);
            if (geometryCode.empty()) {
                std::cerr << "ERROR: Geometry shader file is empty or not found: " << geometryPath << std::endl;
                exit(EXIT_FAILURE);
            }
            
            // Vérifier que les geometry shaders sont supportés
            if (!GLEW_VERSION_3_2 && !GLEW_ARB_geometry_shader4) {
                std::cerr << "ERROR: Geometry shaders are not supported on this system!" << std::endl;
                exit(EXIT_FAILURE);
            }
            
            geometry = compileShader(geometryCode.c_str(), GL_GEOMETRY_SHADER, "GEOMETRY");
        }
        
        programID = glCreateProgram();
        if (programID == 0) {
            std::cerr << "ERROR: Failed to create shader program!" << std::endl;
            exit(EXIT_FAILURE);
        }
        
        glAttachShader(programID, vertex);
        glAttachShader(programID, fragment);
        if (geometry != 0) {
            glAttachShader(programID, geometry);
        }
        glLinkProgram(programID);
        
        checkCompileErrors(programID, "PROGRAM");
        
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        if (geometry != 0) {
            glDeleteShader(geometry);
        }
    }
    
    void use() { glUseProgram(programID); }
    
    void setMat4(const char* name, const float* mat) {
        glUniformMatrix4fv(glGetUniformLocation(programID, name), 1, GL_FALSE, mat);
    }
    
    void setVec3(const char* name, float x, float y, float z) {
        glUniform3f(glGetUniformLocation(programID, name), x, y, z);
    }
    
    void setFloat(const char* name, float value) {
        glUniform1f(glGetUniformLocation(programID, name), value);
    }

    void setInt(const char* name, int value) {
        glUniform1i(glGetUniformLocation(programID, name), value);
    }

    void setVec3(const char* name, const Vec3& vec) {
        glUniform3f(glGetUniformLocation(programID, name), vec[0], vec[1], vec[2]);
    }

private:
    std::string readFile(const char* path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "ERROR: Could not open file: " << path << std::endl;
            return "";
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();
        std::cout << "Loaded shader (" << path << "): " << content.length() << " bytes" << std::endl;
        return content;
    }
    
    GLuint compileShader(const char* code, GLenum type, const std::string& typeStr) {
        GLuint shader = glCreateShader(type);
        
        if (shader == 0) {
            std::cerr << "ERROR: Failed to create " << typeStr << " shader! glCreateShader returned 0" << std::endl;
            std::cerr << "OpenGL Error: " << glGetError() << std::endl;
            exit(EXIT_FAILURE);
        }
        
        glShaderSource(shader, 1, &code, NULL);
        glCompileShader(shader);
        
        checkCompileErrors(shader, typeStr);
        return shader;
    }
    
    void checkCompileErrors(GLuint shader, std::string type) {
        GLint success;
        GLchar infoLog[1024];
        if (type != "PROGRAM") {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::SHADER_COMPILATION::" << type << "\n" << infoLog << std::endl;
            } else {
                std::cout << "SUCCESS: " << type << " shader compiled successfully" << std::endl;
            }
        } else {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::PROGRAM_LINKING\n" << infoLog << std::endl;
            } else {
                std::cout << "SUCCESS: Program linked successfully" << std::endl;
            }
        }
    }
};