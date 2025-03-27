#include "defs.h"
#include "ZBuffer.h"

#include "NaiveZBuffer.h"
#include "ScanLineZBuffer.h"
#include "HierarchicalZBuffer.h"
#include "KDTreeHierarchialZBuffer.h"

#include "Model.h"
#include "Shader.h"

#include <string>

class Camera
{
public:
    Camera() = default;
    ~Camera() = default;

    /** @brief 获取世界坐标到相机坐标的旋转矩阵 */
    Matrix4x4f getViewMatrix() const
    {
        /*
            yaw 偏航角
            pitch 俯仰角
        */
        /* 绕 Y 轴的旋转矩阵（偏航角）*/
        Matrix4x4f matYaw;
        D3DXMatrixRotationY(&matYaw, yaw);
        /* 绕X轴的旋转矩阵（俯仰角）*/
        Matrix4x4f matPitch;
        D3DXMatrixRotationX(&matPitch, pitch);
        /* 组合旋转矩阵 */
        Matrix4x4f matRotation = matPitch * matYaw;
        return matRotation;
    }

    /** @brief 设置偏航角和俯仰角 */
    void lookAt(const FLOAT yaw, const FLOAT pitch /*, const FLOAT zoom = 1.0f*/)
    {
        this->yaw = yaw;
        this->pitch = pitch; 
        // this->zoom = zoom;
    }

private:
    FLOAT yaw = 0.0f, pitch = 0.0f;
    // FLOAT zoom = 1.0f;
};

/* 不同 Z-Buffer 算法选择 */
enum class ZBufferChoice
{
    NONE_ZBUFFER,
    NAIVE_ZBUFFER,
    SCANLINE_ZBUFFER,
    HIERARCHICAL_ZBUFFER,
    KDTREE_HIERARCHICAL_ZBUFFER,
};

/** 使用 MFC 实现 GUI; 
 *  使用 DirectX9 将 ZBuffer 类中的帧缓冲区数据写到窗口。 */
class UI
{
public:
    UI() = delete;
    UI(const INT windowWidth, const INT windowHeight)
        : width(windowWidth),
          height(windowHeight),
          noneZBuffer{windowWidth, windowHeight},
          naiveZBuffer{windowWidth, windowHeight},
          scanLineZBuffer{windowWidth, windowHeight},
          hierarchicalZBuffer{windowWidth, windowHeight},
          kdTreeHierarchialZBuffer{windowWidth, windowHeight} {};
    ~UI() = default;

    LPDIRECT3D9 d3d;
    LPDIRECT3DDEVICE9 d3ddev = NULL;

    INT mainWindow(Model &model, const Shader &shader,
                   HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, INT nCmdShow);

private:
    HWND hWnd;
    INT width, height;
    MSG msg;

    const Color *data = nullptr;

    ZBuffer noneZBuffer;
    NaiveZBuffer naiveZBuffer;
    ScanLineZBuffer scanLineZBuffer;
    HierarchicalZBuffer hierarchicalZBuffer;
    KDTreeHierarchicalZBuffer kdTreeHierarchialZBuffer;

    void initD3D(const HWND hWnd);
    void cleanD3D(void);
    void render_frame();

    void modelRender(Model &model, const Shader &shader);
};