#include <stdio.h>
#include <stdlib.h>
#include <cstdio>

#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include <chrono>
#include <thread>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "shader.h"

#include <iostream>

void processInput(GLFWwindow* window);
unsigned int loadTexture(const char* path);
void loadAllTextures(std::vector<unsigned int>& renderTextures, std::vector<unsigned int>& albedoTextures, std::vector<unsigned int>& depthTextures, std::vector<unsigned int>& normalTextures, std::vector<unsigned int>& motionTextures, int max_frames);

const unsigned int SCREEN_WIDTH = 1280;
const unsigned int SCREEN_HEIGHT = 720;

int main()
{
    glfwTerminate();

    // glfw
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "CS292F Project 4", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window\n" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // glad
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD\n" << std::endl;
        return -1;
    }

    Shader shaderProgram = Shader("vertexShader.vert", "fragmentShader.frag");

    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    float vertices[] = {
        // positions        // texture coords
         1.0f,  1.0f, 0.0f,  1.0f, 0.0f, // top right
         1.0f, -1.0f, 0.0f,  1.0f, 1.0f, // bottom right
        -1.0f, -1.0f, 0.0f,  0.0f, 1.0f, // bottom left
        -1.0f,  1.0f, 0.0f,  0.0f, 0.0f  // top left 
    };

    unsigned int indices[] = {
        0, 1, 3,  // first triangle
        1, 2, 3   // second triangle
    };

    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    int frame_number = 0;
    int max_frames = 300;
    const double frame_duration = 1.0 / 30.0; // 30 FPS

    // Load all textures before entering the render loop
    std::vector<unsigned int> renderTextures(max_frames);
    std::vector<unsigned int> albedoTextures(max_frames);
    std::vector<unsigned int> depthTextures(max_frames);
    std::vector<unsigned int> normalTextures(max_frames);
    std::vector<unsigned int> motionTextures(max_frames);

    loadAllTextures(renderTextures, albedoTextures, depthTextures, normalTextures, motionTextures, max_frames);

    // Define a framebuffer object and a texture for storing the denoised image
    unsigned int denoisedFBO;
    unsigned int denoisedTexture;

    // Create the denoised texture
    glGenTextures(1, &denoisedTexture);
    glBindTexture(GL_TEXTURE_2D, denoisedTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Create and bind the framebuffer object
    glGenFramebuffers(1, &denoisedFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, denoisedFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, denoisedTexture, 0);

    // Ensure framebuffer is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "Framebuffer is not complete!" << std::endl;
    }

    // Reset framebuffer binding
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // render loop
    while (!glfwWindowShouldClose(window)) {
        auto frame_start_time = std::chrono::high_resolution_clock::now();

        // input
        processInput(window);

        // Bind the denoised FBO
        glBindFramebuffer(GL_FRAMEBUFFER, denoisedFBO);
        // Set the viewport to match the denoised texture size
        glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

        // render

        // draw the quad
        glUseProgram(shaderProgram.ID);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, renderTextures[frame_number]);
        glUniform1i(glGetUniformLocation(shaderProgram.ID, "noisyImage"), 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, albedoTextures[frame_number]);
        glUniform1i(glGetUniformLocation(shaderProgram.ID, "albedoBuffer"), 1);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, depthTextures[frame_number]);
        glUniform1i(glGetUniformLocation(shaderProgram.ID, "depthBuffer"), 2);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, normalTextures[frame_number]);
        glUniform1i(glGetUniformLocation(shaderProgram.ID, "normalBuffer"), 3);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, motionTextures[frame_number]);
        glUniform1i(glGetUniformLocation(shaderProgram.ID, "motionVector"), 4);
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, denoisedTexture);
        glUniform1i(glGetUniformLocation(shaderProgram.ID, "prevDenoisedImage"), 5);

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // swap buffers and poll IO events
        glfwSwapBuffers(window);
        glfwPollEvents();

        frame_number = (frame_number + 1) % max_frames;

        // Frame rate control
        auto frame_end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = frame_end_time - frame_start_time;
        if (elapsed.count() < frame_duration) {
            std::this_thread::sleep_for(std::chrono::duration<double>(frame_duration - elapsed.count()));
        }
    }
    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

unsigned int loadTexture(const char* path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrChannels;
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
    if (data)
    {
        GLenum format;
        if (nrChannels == 1)
            format = GL_RED;
        else if (nrChannels == 3)
            format = GL_RGB;
        else if (nrChannels == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Failed to load texture " << path << "\n" << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

void loadAllTextures(std::vector<unsigned int>& renderTextures, std::vector<unsigned int>& albedoTextures, std::vector<unsigned int>& depthTextures, std::vector<unsigned int>& normalTextures, std::vector<unsigned int>& motionTextures, int max_frames)
{
    for (int frame_number = 0; frame_number < max_frames; ++frame_number)
    {
        std::stringstream ss0;
        ss0 << "frames/frame_" << std::setw(3) << std::setfill('0') << frame_number << ".png";
        renderTextures[frame_number] = loadTexture(ss0.str().c_str());

        std::stringstream ss1;
        ss1 << "frames/albedo_" << std::setw(3) << std::setfill('0') << frame_number << ".png";
        albedoTextures[frame_number] = loadTexture(ss1.str().c_str());

        std::stringstream ss2;
        ss2 << "frames/depth_" << std::setw(3) << std::setfill('0') << frame_number << ".png";
        depthTextures[frame_number] = loadTexture(ss2.str().c_str());

        std::stringstream ss3;
        ss3 << "frames/normal_" << std::setw(3) << std::setfill('0') << frame_number << ".png";
        normalTextures[frame_number] = loadTexture(ss3.str().c_str());

        std::stringstream ss4;
        ss4 << "frames/position_" << std::setw(3) << std::setfill('0') << frame_number << ".png";
        motionTextures[frame_number] = loadTexture(ss4.str().c_str());
    }
}

