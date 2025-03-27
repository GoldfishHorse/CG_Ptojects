#pragma once

#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Shader.h"
#include "Camera.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "SurfaceRender.h"
#include "CurveRender.h"
#include "MeshRender.h"
#include "NormalRender.h"

#include "MyTinyNurbs.h"

struct CurveDisplay {
    CurveRender curveRender;
    CurveRender curveControlPointLineRender; /* Displaying polyline with control points as turning points */
    NormalRender curveDerivateRender;
};

void DisplayerInit(const RationalCurve3f& curve, CurveDisplay& displayer);

class GLProgram {
    private:
        GLFWwindow* window;

        float deltaTime, prevTime;

        struct color {
            float r = 0.0f;
            float g = 0.0f;
            float b = 0.0f;
            float alpha = 1.0f;
        } clearColor;

        static Vec3f getArcballVector(float x, float y); /* helper to cursor callback, (x,y) are raw mouse coordinates */



        vector<CurveDisplay> contourCurveDisplayer;
        vector<CurveDisplay> trajectoryCurveDisplayer;
        vector<vector<CurveDisplay>> profileCurveDisplayer;
        vector<vector<vector<NormalRender>>> frameDisplayer;

        
        vector<SurfaceRender> surfaceRender;
        vector<MeshRender> meshRender;
        vector<MeshRender> surfaceControlRender;
        vector<NormalRender> normalRender;
        vector<NormalRender> UsurfaceDerivateRender;
        vector<NormalRender> VsurfaceDerivateRender;

        void DisplayingCurve(const RationalCurve3f& curve, CurveDisplay& displayer,
            bool show_curves, bool show_control_points, bool show_derivate, float curves_derivate_u);
        void DisplayingVec(const arrayVec3f& Vector, const arrayVec3f& Position,
            NormalRender& Render, double len = 1);



    public:

        /* input */
        bool enableControl = true;

        static int windowWidth, windowHeight;
        static Camera camera;
        static bool mousePressed;
        static double prevMouseX, prevMouseY;
        static Mat4f modelMatrix;

        /* lighting */
        Vec3f lightPos;

        GLProgram();

        void init(const vector<RationalCurve3f>& contour_curves,
            const vector<RationalCurve3f>& trajectory_curves,
            const vector<vector<RationalCurve3f> >& profile_curves,
            const vector<vector<arrayVec3f> >& frames,
            const vector<RationalSurface3f>& surfaces);
        void run(const vector<RationalCurve3f>& contour_curves,
            const vector<RationalCurve3f>& trajectory_curves,
            const vector<vector<RationalCurve3f> >& profile_curves,
            const vector<vector<arrayVec3f> >& frames,
            const vector<arrayf> v_bar,
            const vector<RationalSurface3f>& surfaces);
        void cleanup(void);

        void setClearColor(float r, float g, float b, float alpha);

        /* event callback functions */
        void framebufferSizeCallback(GLFWwindow* window, int width, int height);
        void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
        void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
        void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);

        /* input */
        void processInput(void);


private:
    /* Wrapper for glfwGetWindowUserPointer() returning this class instance. */
    static GLProgram* _this(GLFWwindow* window)
    {
        return static_cast<GLProgram*>(glfwGetWindowUserPointer(window));
    }
    static void _framebufferSizeCallback(GLFWwindow* window, int width, int height)
    {
        _this(window)->framebufferSizeCallback(window, width, height);
    }
    static void _scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
    {
        _this(window)->scrollCallback(window, xoffset, yoffset);
    }
    static void _mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
    {
        _this(window)->mouseButtonCallback(window, button, action, mods);
    }
    static void _cursorPosCallback(GLFWwindow* window, double xpos, double ypos)
    {
        _this(window)->cursorPosCallback(window, xpos, ypos);
    }

};
