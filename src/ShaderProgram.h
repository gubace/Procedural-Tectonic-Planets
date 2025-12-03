#pragma once
#include <GL/glew.h>
#include <GL/gl.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class ShaderProgram {
public:
    GLuint programID;
    
    ShaderProgram(const char* vertexPath, const char* fragmentPath) {
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
        
        GLuint vertex = compileShader(vertexCode.c_str(), GL_VERTEX_SHADER);
        GLuint fragment = compileShader(fragmentCode.c_str(), GL_FRAGMENT_SHADER);
        
        programID = glCreateProgram();
        glAttachShader(programID, vertex);
        glAttachShader(programID, fragment);
        glLinkProgram(programID);
        
        checkCompileErrors(programID, "PROGRAM");
        
        glDeleteShader(vertex);
        glDeleteShader(fragment);
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
    
    GLuint compileShader(const char* code, GLenum type) {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &code, NULL);
        glCompileShader(shader);
        checkCompileErrors(shader, type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT");
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
            }
        } else {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::PROGRAM_LINKING\n" << infoLog << std::endl;
            }
        }
    }
};