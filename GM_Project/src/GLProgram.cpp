#include "GLProgram.h"
#include "glm/ext.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "MyTinyNurbs.h"
#include "Utils.h"

/* 项目根目录
   在  CmakeLists.txt 中定义: 
   add_definitions(-DPROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}") */
#ifndef PROJECT_SOURCE_DIR
#define PROJECT_SOURCE_DIR ".."
#endif

/* shader source code paths */
static constexpr const char* vertexShaderPath           = PROJECT_SOURCE_DIR "/shaders/vertexShader.vs";
static constexpr const char* whiteFragmentShaderPath    = PROJECT_SOURCE_DIR "/shaders/whiteFragmentShader.fs";
static constexpr const char* redFragmentShaderPath      = PROJECT_SOURCE_DIR "/shaders/redFragmentShader.fs";
static constexpr const char* greenFragmentShaderPath    = PROJECT_SOURCE_DIR "/shaders/greenFragmentShader.fs";
static constexpr const char* blueFragmentShaderPath     = PROJECT_SOURCE_DIR "/shaders/blueFragmentShader.fs";
static constexpr const char* lightvertexShaderPath      = PROJECT_SOURCE_DIR "/shaders/lightvertexShader.vs";
static constexpr const char* fragmentShaderPath         = PROJECT_SOURCE_DIR "/shaders/fragmentShader.fs";
static constexpr const char* imguiFontPath              = PROJECT_SOURCE_DIR "/external/imgui/misc/fonts/DroidSans.ttf"; /* 字体路径 */



/* declare static members for use in callback functions */
int GLProgram::windowWidth = WINDOW_WIDTH;
int GLProgram::windowHeight = WINDOW_HEIGHT;
Camera GLProgram::camera;
bool GLProgram::mousePressed = false;
double GLProgram::prevMouseX, GLProgram::prevMouseY;
Mat4f GLProgram::modelMatrix = Mat4f(1.0f);


GLProgram::GLProgram() :
    deltaTime(0.0f), prevTime(0.0f) {
    surfaceRender.clear();
    contourCurveDisplayer.clear();
    trajectoryCurveDisplayer.clear();
    meshRender.clear();
    normalRender.clear();
    return;
}

void DisplayerInit(const RationalCurve3f& curve, CurveDisplay& displayer) {
    arrayVec3f crvvertex;
    crvvertex.resize(1000);
    float u = 0;
    constexpr float delta = 1.0 / (3000 / 3 - 1);
    /* Rendering contour curves */
    for (int i = 0; i < 3000 / 3; i++)
    {
        crvvertex[i] = curve.Point(u);
        u += delta;
    }
    displayer.curveRender.Initial(vertexShaderPath, whiteFragmentShaderPath, crvvertex);

    arrayVec3f crvcontrol_points;
    for (int i = 0; i < curve.control_points.size(); i++)
    {
        crvcontrol_points.push_back(curve.control_points[i]);
    }

    displayer.curveControlPointLineRender.Initial(vertexShaderPath, greenFragmentShaderPath, crvcontrol_points);


    constexpr float p = 0.5;
    vector<arrayVec3f> Vertices;
    vector<arrayVec3f> OffsetVertex;

    Point3f tmp = curve.Point(p);

    Point3f tmp1 = normalize(curve.Derivatives(1, p)[1]); /* 曲线一点的导数运算结果 */

    const float len = 1.f;
    
    Point3f tmp2 = tmp - tmp1 * len; /* 保存延伸后的数据点 */

    Vertices.resize(1);
    OffsetVertex.resize(1);

    Vertices[0].push_back(tmp);
    OffsetVertex[0].push_back(tmp2);

    displayer.curveDerivateRender.Initial(vertexShaderPath, redFragmentShaderPath, Vertices, OffsetVertex);
    return;
}

void GLProgram::DisplayingCurve(const RationalCurve3f& curve, CurveDisplay& displayer,
    bool show_curves, bool show_control_points, bool show_derivate, float curves_derivate_u) {
    if (show_curves)
        displayer.curveRender.Draw(camera, modelMatrix, lightPos, windowWidth, windowHeight);
    if (show_control_points)
        displayer.curveControlPointLineRender.Draw(camera, modelMatrix, lightPos, windowWidth, windowHeight);
    if (show_derivate) {
        float p = curves_derivate_u;

        Point3f tmp = curve.Point(p);

        arrayVec3f derivateData = curve.Derivatives(1, p);

        DisplayingVec({ derivateData[1] }, { tmp }, displayer.curveDerivateRender, 1);
    }
    return;
}

void GLProgram::DisplayingVec(const arrayVec3f& Vector, const arrayVec3f& Position,
    NormalRender& Render, double len) {
    vector<arrayVec3f> Vertices;
    vector<arrayVec3f> OffsetVertex;
    Vertices.clear();
    OffsetVertex.clear();
    Vertices.resize(1);
    OffsetVertex.resize(1);
    for (size_t i = 0; i < Vector.size(); i++) {
        Point3f tmp1 = normalize(Vector[i]);
        /* Position with offset */
        Point3f tmp2 = Position[i] - tmp1 * static_cast<float>(len);
        OffsetVertex[0].push_back(tmp2);
    }
    Vertices[0].assign(Position.begin(), Position.end());
    Render.Initial(vertexShaderPath, redFragmentShaderPath, Vertices, OffsetVertex);
    Render.Draw(camera, modelMatrix, lightPos, windowWidth, windowHeight);
    return;
}


void GLProgram::init(const vector<RationalCurve3f>& contour_curves,
    const vector<RationalCurve3f>& trajectory_curves,
    const vector<vector<RationalCurve3f>>& profile_curves,
    const vector<vector<arrayVec3f>>& frames,
    const vector<RationalSurface3f>& surfaces) {
    
    /* lighting */
    lightPos = Point3f(10.f, 4.0f, 10.0f);

    /* initialize window system */
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    this->window = glfwCreateWindow(this->windowWidth,this-> windowHeight, "3D Surface Plotter", NULL, NULL);
    if (this->window == NULL) {
        LOG_ERROR("FAILED TO CREATE GLFW WINDOW.");
        glfwTerminate();
        exit(-1);
    }
    
    glfwSetWindowUserPointer(window, this);

    glfwMakeContextCurrent(this->window);
    glfwSetFramebufferSizeCallback(this->window, _framebufferSizeCallback);
    glfwSetScrollCallback(window, _scrollCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR);
    glfwSetMouseButtonCallback(window, _mouseButtonCallback);
    glfwSetCursorPosCallback(window, _cursorPosCallback);
    
    /* initialize GLAD before making OpenGL calls */
    int ret = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    ASSERT(ret, "FAILED TO INITIALIZE GLAD");
    
    /* GL calls */
    glViewport(0, 0, this->windowWidth, this->windowHeight);
    glEnable(GL_DEPTH_TEST);


    this->surfaceRender.resize(surfaces.size());

    this->meshRender.resize(surfaceRender.size());

    this->normalRender.resize(surfaceRender.size());

    this->surfaceControlRender.resize(surfaceRender.size());

    this->UsurfaceDerivateRender.resize(surfaceRender.size());
    this->VsurfaceDerivateRender.resize(surfaceRender.size());

    for (size_t k = 0; k < this->surfaceRender.size(); k++)
    {
        float u = 0, v = 0;
        constexpr size_t numX = 40;
        constexpr size_t numY = 40;
        vector<arrayVec3f> Vertices;
        vector<arrayVec3f> Normal;
        vector<arrayVec3f> OffsetVertex;

        Normal.resize(numX);
        Vertices.resize(numX);
        OffsetVertex.resize(numX);
        const float delta = 1.0f / (numX - 1);
        for (size_t x = 0; x < numX; x++) {
            v = 0;
            Vertices[x].resize(numY);
            OffsetVertex[x].resize(numY);
            Normal[x].resize(numY);
            for (size_t y = 0; y < numY; y++) {
                /* add vertex */
                Point3f tmp = surfaces[k].Point(u, v);
                Point3f tmp1 = surfaces[k].Normal(u, v);
                constexpr float  len = 0.2;
                /*  */
                Point3f tmp2 = tmp - tmp1 * len;
                Vertices[x][y] = tmp;
                OffsetVertex[x][y] = tmp2;
                Normal[x][y] = tmp1;
                v += delta;
            }
            u += delta;
        }
        this->surfaceRender[k].Initial(lightvertexShaderPath, fragmentShaderPath, Vertices, Normal);

        this->meshRender[k].Initial(vertexShaderPath, whiteFragmentShaderPath, Vertices);
 
        vector<arrayVec3f> MeshVertex;

        MeshVertex.resize(surfaces[k].control_points.rows());

        for (size_t i = 0; i < surfaces[k].control_points.rows(); ++i)
        {
            MeshVertex[i].resize(surfaces[k].control_points.cols());
            for (size_t j = 0; j < surfaces[k].control_points.cols(); ++j) {
                MeshVertex[i][j] = surfaces[k].control_points.at(i, j);

            }

        }
        this->surfaceControlRender[k].Initial(vertexShaderPath, whiteFragmentShaderPath, MeshVertex);
    }


    this->contourCurveDisplayer.resize(contour_curves.size());
    this->trajectoryCurveDisplayer.resize(trajectory_curves.size());
    this->profileCurveDisplayer.resize(contour_curves.size());
    this->frameDisplayer.resize(contour_curves.size());

    for (size_t k = 0; k < this->contourCurveDisplayer.size(); k++)
    {
        DisplayerInit(contour_curves[k], contourCurveDisplayer[k]);
        DisplayerInit(trajectory_curves[k], trajectoryCurveDisplayer[k]);
        profileCurveDisplayer[k].resize(profile_curves[k].size());
        frameDisplayer[k].resize(frames[k].size());
        for (int j = 0; j < profile_curves[k].size(); j++) {
            DisplayerInit(profile_curves[k][j], profileCurveDisplayer[k][j]);
            frameDisplayer[k][j].resize(3);
        }
    }

    /* initialize IMGUI */
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsLight();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 450");
    io.Fonts->AddFontFromFileTTF(imguiFontPath, 32.0f);
    return;
}

void GLProgram::run(const vector<RationalCurve3f>& contour_curves,
    const vector<RationalCurve3f>& trajectory_curves,
    const vector<vector<RationalCurve3f>>& profile_curves,
    const vector<vector<arrayVec3f>>& frames,
    const vector<arrayf> v_bar,
    const vector<RationalSurface3f>& surfaces) {


    ImGuiIO& io = ImGui::GetIO();
    struct {
        bool show_grid = false;
        bool show_tangent = false;
        bool show_normal = false;
        float tangent_u = 0.5f, tangent_v = 0.5f;
        bool show_control_mesh = false;
        bool show_shade_suf = false;
        bool show_contour = false;
        bool show_contour_derivate = false;
        bool show_contour_control_points = false;
        float contour_derivate_u = 0.5f;
        bool show_trajectory = false;
        bool show_trajectory_derivate = false;
        bool show_trajectory_control_points = false;
        float trajectory_derivate_u = 0.5f;
        bool show_frame = false;
        bool show_profile = false;
        bool show_profile_derivate = false;
        bool show_profile_control_points = false;
        float profile_derivate_u = 0.5f;
    } state;
 
    /* main loop */
    while (!glfwWindowShouldClose(this->window)) {

        /* per-frame time logic */
        float currTime = glfwGetTime();
        this->deltaTime = currTime - this->prevTime;
        this->prevTime = currTime;


        /* input */
        enableControl = (!io.WantCaptureMouse && !io.WantCaptureKeyboard);

        /* input */
        processInput();

        glClearColor(this->clearColor.r, this->clearColor.g, this->clearColor.b, this->clearColor.alpha);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        /* --- imgui */
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Settings");
        ImGui::Checkbox("show grid", &state.show_grid);
        ImGui::Checkbox("show tangent", &state.show_tangent);
        if (state.show_tangent || state.show_normal)
        {
            ImGui::SliderFloat("u##1", &state.tangent_u, 0.0f, 1.0f);
            ImGui::SliderFloat("v##1", &state.tangent_v, 0.0f, 1.0f);
        }
        ImGui::Checkbox("show normal", &state.show_normal);

        ImGui::Checkbox("show shade surface", &state.show_shade_suf);

        ImGui::Checkbox("show control mesh", &state.show_control_mesh);

        ImGui::Checkbox("show contour curves", &state.show_contour);

        ImGui::Checkbox("show contour curves derivate", &state.show_contour_derivate);

        ImGui::Checkbox("show contour curve control points line", &state.show_contour_control_points);

        ImGui::Checkbox("show trajectory curves", &state.show_trajectory);

        ImGui::Checkbox("show trajectory curves derivate", &state.show_trajectory_derivate);

        ImGui::Checkbox("show trajectory curve control points line", &state.show_trajectory_control_points);

        ImGui::Checkbox("show frames", &state.show_frame);

        ImGui::Checkbox("show profile curves", &state.show_profile);

        ImGui::Checkbox("show profile curves derivate", &state.show_profile_derivate);

        ImGui::Checkbox("show profile curve control points line", &state.show_profile_control_points);

        if (state.show_contour_derivate)
        {
            ImGui::SliderFloat("u##2", &state.contour_derivate_u, 0.0f, 1.0f);
        }

        if (state.show_trajectory_derivate)
        {
            ImGui::SliderFloat("u##2", &state.trajectory_derivate_u, 0.0f, 1.0f);
        }

        if (state.show_profile_derivate)
        {
            ImGui::SliderFloat("u##2", &state.profile_derivate_u, 0.0f, 1.0f);
        }


        ImGui::End();
 
        /* Draw Surfaces */
        for (size_t k = 0; k < this->surfaceRender.size(); k++)
        {

            Point3f tmp = surfaces[k].Point(state.tangent_u, state.tangent_v);

            if (state.show_tangent) {
                array2Vec3f res = surfaces[k].Derivatives(1, state.tangent_u, state.tangent_v);
                DisplayingVec({ res.at(1, 0) }, { tmp }, UsurfaceDerivateRender[k], 0.2);
                DisplayingVec({ res.at(0, 1) }, { tmp }, VsurfaceDerivateRender[k], 0.2);
            }
            if (state.show_normal) {
                DisplayingVec({ surfaces[k].Normal(state.tangent_u, state.tangent_v) }, {tmp}, normalRender[k], 0.2);
            }

            if (state.show_shade_suf)
                this->surfaceRender[k].Draw(camera, modelMatrix, lightPos, windowWidth, windowHeight);
        }
        /* Draw Curves */
        for (size_t k = 0; k < this->contourCurveDisplayer.size(); k++)
        {
            DisplayingCurve(contour_curves[k], contourCurveDisplayer[k], state.show_contour, state.show_contour_control_points,
                state.show_contour_derivate, state.contour_derivate_u);
            DisplayingCurve(trajectory_curves[k], trajectoryCurveDisplayer[k], state.show_trajectory, 
                state.show_trajectory_control_points, state.show_trajectory_derivate, state.trajectory_derivate_u);
            for (int j = 0; j < this->profileCurveDisplayer[k].size(); j++)
            {
                DisplayingCurve(profile_curves[k][j], profileCurveDisplayer[k][j], state.show_profile,
                    state.show_profile_control_points, state.show_profile_derivate, state.profile_derivate_u);
                if (state.show_frame) {
                    Point3f tmp = trajectory_curves[k].Point(v_bar[k][j]);
                    for (int i = 0; i < 3; i++) {
                        DisplayingVec({ frames[k][j][i] }, { tmp }, frameDisplayer[k][j][i], 1);
                    }
                }
            }
        }
        /* Draw Mesh */
        for (size_t k = 0; k < this->surfaceRender.size(); k++)
        {
            if (state.show_grid)
                this->meshRender[k].Draw(camera, modelMatrix, lightPos, windowWidth, windowHeight);
            if(state.show_control_mesh)
                this->surfaceControlRender[k].Draw(camera, modelMatrix, lightPos, windowWidth, windowHeight);
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        /* check and call events and swap buffers */
        glfwSwapBuffers((this->window));
        glfwPollEvents();
    }
    return;
}


void GLProgram::cleanup(void) {
    /* clean up gl resources */
    for (size_t k = 0; k < this->surfaceRender.size(); k++)
    {
        glDeleteVertexArrays(1, &(this->surfaceRender[k].VAO));
        glDeleteBuffers(1, &(this->surfaceRender[k].VBO));
        glDeleteBuffers(1, &this->surfaceRender[k].EBO);

        glDeleteVertexArrays(1, &(this->VsurfaceDerivateRender[k].VAO));
        glDeleteBuffers(1, &(this->VsurfaceDerivateRender[k].VBO));
        glDeleteBuffers(1, &this->VsurfaceDerivateRender[k].EBO);

        glDeleteVertexArrays(1, &(this->UsurfaceDerivateRender[k].VAO));
        glDeleteBuffers(1, &(this->UsurfaceDerivateRender[k].VBO));
        glDeleteBuffers(1, &this->UsurfaceDerivateRender[k].EBO);
    }

    for (size_t k = 0; k < this->contourCurveDisplayer.size(); k++)
    {
        glDeleteVertexArrays(1, &(this->contourCurveDisplayer[k].curveRender.VAO));
        glDeleteBuffers(1, &(this->contourCurveDisplayer[k].curveRender.VBO));
        glDeleteBuffers(1, &this->contourCurveDisplayer[k].curveRender.EBO);

        glDeleteVertexArrays(1, &(this->trajectoryCurveDisplayer[k].curveRender.VAO));
        glDeleteBuffers(1, &(this->trajectoryCurveDisplayer[k].curveRender.VBO));
        glDeleteBuffers(1, &this->trajectoryCurveDisplayer[k].curveRender.EBO);
        for (int j = 0; j < profileCurveDisplayer[k].size(); j++)
        {
            glDeleteVertexArrays(1, &(this->profileCurveDisplayer[k][j].curveRender.VAO));
            glDeleteBuffers(1, &(this->profileCurveDisplayer[k][j].curveRender.VBO));
            glDeleteBuffers(1, &this->profileCurveDisplayer[k][j].curveRender.EBO);
        }
    }

    for (size_t k = 0; k < this->surfaceRender.size(); k++)
    {
        glDeleteVertexArrays(1, &(this->meshRender[k].VAO));
        glDeleteBuffers(1, &(this->meshRender[k].VBO));
        glDeleteBuffers(1, &this->meshRender[k].EBO);
    }

    for (size_t k = 0; k < this->normalRender.size(); k++)
    {
        glDeleteVertexArrays(1, &(this->normalRender[k].VAO));
        glDeleteBuffers(1, &(this->normalRender[k].VBO));
        glDeleteBuffers(1, &this->normalRender[k].EBO);
    }
    /* clean up glfw */
    glfwTerminate();
    return;
}

void GLProgram::setClearColor(float r, float g, float b, float alpha) {
    this->clearColor = {r, g, b, alpha};
    return;
}

void GLProgram::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    windowWidth = width;
    windowHeight = height;
    return;
}

void GLProgram::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {

    if (this->enableControl) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (action == GLFW_PRESS) {
                glfwGetCursorPos(window, &prevMouseX, &prevMouseY);
                mousePressed = true;
            }
            else if (action == GLFW_RELEASE) {
                mousePressed = false;
            }
        }
    }
    return;
}

void GLProgram::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    if(this->enableControl)
        camera.processMouseScroll(static_cast<float>(yoffset));
    return;
}

void GLProgram::cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {

    if (mousePressed && enableControl) {

        /* get current cursor coordinates */
        double currMouseX, currMouseY;
        glfwGetCursorPos(window, &currMouseX, &currMouseY);

        /* get points on arcball */
        Point3f va = getArcballVector(prevMouseX, prevMouseY);
        Point3f vb = getArcballVector(currMouseX, currMouseY);

        constexpr float speedFactor = 0.1f;
        const float angleOfRotation = speedFactor * acos(std::min(1.0f, glm::dot(va, vb)));

        /* to get the axis of rotation, need to convert from camera coordinates to world coordinates */
        Point3f axisCamera = glm::cross(va, vb);
        Mat3f cameraToModel = glm::inverse(Mat3f(camera.getViewMatrix() * modelMatrix));
        Point3f axisModel = cameraToModel * axisCamera;

        /* update model rotation matrix */
        constexpr float tolerance = 1e-4;
        if (angleOfRotation > tolerance)
            modelMatrix = glm::rotate(modelMatrix, glm::degrees(angleOfRotation), axisModel);

        /* update cursor position */
        prevMouseX = currMouseX;
        prevMouseY = currMouseY;
    }
}

Point3f GLProgram::getArcballVector(float x, float y) {

    /*  get normalized vector from center of the virtual arcball to a point P on the arcball's surface
        if (x,y) is too far away from the arcball, return the nearest point on the arcball's surface */
    Point3f P(x/windowWidth * 2 - 1.0, y/windowHeight * 2 - 1.0, 0.0f);
    P.y = -P.y;

    constexpr float radius = 1.0f;
    const float OP_squared = P.x * P.x + P.y * P.y;

    if (OP_squared <= radius)
        P.z = sqrt(radius - OP_squared); /* apply pythagorean theorem to find z */
    else
        P = glm::normalize(P); /* nearest point */

    return P;
}


void GLProgram::processInput(void) {

#define glPressed(key) glfwGetKey(window, key) == GLFW_PRESS

    /* close window with 'ESC' key */
    if (glfwGetKey(this->window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(this->window, true);

    /* camera movement */
    if ((glPressed(GLFW_KEY_W) || glPressed(GLFW_KEY_UP))  && enableControl)
        camera.processKeyboard(UP, deltaTime);
    if ((glPressed(GLFW_KEY_S) || glPressed(GLFW_KEY_DOWN)) && enableControl)
        camera.processKeyboard(DOWN, deltaTime);
    if ((glPressed(GLFW_KEY_A) || glPressed(GLFW_KEY_LEFT)) && enableControl)
        camera.processKeyboard(LEFT, deltaTime);
    if ((glPressed(GLFW_KEY_D) || glPressed(GLFW_KEY_RIGHT)) && enableControl)
        camera.processKeyboard(RIGHT, deltaTime);

    return;
}
