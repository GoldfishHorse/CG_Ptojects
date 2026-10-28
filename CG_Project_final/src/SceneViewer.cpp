#include "SceneViewer.h"

#include "glad.h"
#include "GLFW/glfw3.h"

#include <iostream>
#include <chrono>
#include <thread>
#include <omp.h>

using namespace std;


void SceneViewer::renderFuncCallBack() {
    const GLvoid * data = static_cast<GLvoid * >
        (glm::value_ptr(this->frame.front()));
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(
        0, 
        static_cast<GLfloat>(this->w), 
        0, 
        static_cast<GLfloat>(this->h), 
        -1, 1);
    glRasterPos2i(0, 0);
    const std::lock_guard<std::mutex> lock(this->writeLock);
    glDrawPixels(
        static_cast<GLsizei>(this->w), 
        static_cast<GLsizei>(this->h), 
        GL_RGB, GL_FLOAT, data);
}



void SceneViewer::viewScene() {
    if (this->pathTracer == nullptr)
    {
        LOG_ERROR("Scene Viewer not inited yet!");
        return;
    }
    if (!glfwInit()) {
        LOG_ERROR("Failed to initialize GLFW.");
        return;
    }
    LOG_INFO("Open Window...");
    GLFWwindow* window = glfwCreateWindow(
        this->w,
        this->h,
        this->pathTracer->scene->name.c_str(), 
        NULL, NULL);
    if (!window) {
        LOG_ERROR("Failed to create GLFW window.");
        glfwTerminate();
        return;
    } else {
        this->windowOpened = true;
    }

    std::thread renderThread([this]() {
        while(this->windowOpened) {
            this->pathTracer->renderScene();
            this->updateFrame();
        }
    });
    

    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        LOG_ERROR("Failed to initialize GLAD.");
        this->windowOpened = false;
        renderThread.join();
        glfwDestroyWindow(window);
        glfwTerminate();
        return;
    }


    while (!glfwWindowShouldClose(window)) {
        this->renderFuncCallBack();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    this->windowOpened = false;

    glfwDestroyWindow(window);
    glfwTerminate();

    renderThread.join();
}

void SceneViewer::gammaCorrection(const float gamma)
{
    #pragma omp parallel for
    for (ssize_t i = 0; i < static_cast<ssize_t>(this->frame.size()); ++i)
    {
        this->frame[i] = glm::clamp(
            ::gammaCorrection(this->frame[i], gamma), 
            Vec3f{0.f}, 
            Vec3f{1.f});
    }
    return;
}

void SceneViewer::updateFrame()
{
    const std::lock_guard<std::mutex> lock(this->writeLock);
    this->frame = this->pathTracer->getFrameBufferData();
    this->gammaCorrection();
    return;
}
