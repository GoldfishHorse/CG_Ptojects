#include <Windows.h>
#include <commdlg.h> /* 用于文件选择对话框 */
#include <chrono>
#include "ui.h"

/* 全局变量, 用于在回调函数与 UI class 之间传递信息 */
static std::string objFilePath; /* obj 文件的路径 */
static ZBufferChoice zBufferChoice = ZBufferChoice::NAIVE_ZBUFFER; /* 选择了哪一个 ZBuffer 算法 */
static Camera camera;   /* 相机视角变换 */
static bool loadSignal = false; /* 是否加载了新模型 */
static bool switchSignal = false;   /* 是否切换了 ZBuffer 算法 */

enum
{
    MENU_ID_OPEN_FILE = 1,
    MENU_ID_NO_ZBUFFER = 2,
    MENU_ID_NAIVE_ZBUFFER = 3,
    MENU_ID_SCANLINE_ZBUFFER = 4,
    MENU_ID_HIERARCHICAL_ZBUFFER = 5,
    MENU_ID_KDTREE_HIERARCHICAL_ZBUFFER = 6,
};

#define SelectMenu(hZBufferMenu, Selection)  \
    CheckMenuRadioItem(                      \
        hZBufferMenu,                        \
        MENU_ID_NO_ZBUFFER,                  \
        MENU_ID_KDTREE_HIERARCHICAL_ZBUFFER, \
        Selection,                           \
        MF_BYCOMMAND)

void UI::initD3D(const HWND hWnd)
{
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.hDeviceWindow = hWnd;
    d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;

    d3dpp.BackBufferWidth = this->width;
    d3dpp.BackBufferHeight = this->height;
    d3dpp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;

    /* 打开 Direct3D 中的 Z-buffer */
    // d3dpp.EnableAutoDepthStencil = TRUE;
    // d3dpp.AutoDepthStencilFormat = D3DFMT_D16;

    HRESULT hr = d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &d3ddev);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create D3D device. errorCode = 0x%08lx.", hr);
    }
    return;
}

void UI::cleanD3D(void)
{
    d3ddev->Release();
    d3d->Release();
}

static void openFile(const HWND hWnd)
{
    OPENFILENAME ofn;
    std::string filename(MAX_PATH, '\0');

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFile = filename.data();
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = "OBJ Files\0*.obj\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&ofn) == TRUE)
    {
        ::objFilePath = std::string(ofn.lpstrFile);
    }
}

void UI::modelRender(Model &model, const Shader &shader)
{
    if (::loadSignal)
    {
        /* 加载模型 */
        model = Model{::objFilePath};
        shader.shadeModel(model);
        model.project(this->width, this->height);
    }
    else if (model.empty())
    {
        /* 模型还未加载 */
        return;
    }

    Matrix4x4f matView = ::camera.getViewMatrix();

    /*
        实行 lazy 更新。
        若 matView 不是恒等矩阵, 说明视角变化, 那么重新渲染;
        若重新加载的模型 (loadSignal = true), 那么重新渲染;
        若重新选择了剔除算法 (switchSignal = true), 那么重新渲染。
     */
    if (!D3DXMatrixIsIdentity(&matView) || ::loadSignal || ::switchSignal)
    {
        model.transfrom(matView);
        model.moveToWindowsCenter(this->width, this->height);
        switch (::zBufferChoice)
        {
        case ZBufferChoice::NONE_ZBUFFER:
            this->noneZBuffer.renderModel(model);
            this->data = this->noneZBuffer.getFrameBuffer();
            break;
        case ZBufferChoice::NAIVE_ZBUFFER:
            this->naiveZBuffer.renderModel(model);
            this->data = this->naiveZBuffer.getFrameBuffer();
            break;
        case ZBufferChoice::SCANLINE_ZBUFFER:
            this->scanLineZBuffer.renderModel(model);
            this->data = this->scanLineZBuffer.getFrameBuffer();
            break;
        case ZBufferChoice::HIERARCHICAL_ZBUFFER:
            this->hierarchicalZBuffer.renderModel(model);
            this->data = this->hierarchicalZBuffer.getFrameBuffer();
            break;
        case ZBufferChoice::KDTREE_HIERARCHICAL_ZBUFFER:
            this->kdTreeHierarchialZBuffer.renderModel(model);
            this->data = this->kdTreeHierarchialZBuffer.getFrameBuffer();
            break;
        default:
            LOG_ERROR("Invalid ZBufferChoice!");
            break;
        }
        ::loadSignal = ::switchSignal = false;
    }
    return;
}

void UI::render_frame()
{
    static auto lastTime = std::chrono::steady_clock::now();
    static int frameCount = 0;

    d3ddev->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
    d3ddev->BeginScene();

    /* 锁定后台缓冲区 */
    IDirect3DSurface9 *backBuffer = nullptr;
    D3DLOCKED_RECT lockedRect;
    HRESULT hr = d3ddev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backBuffer);

    if (FAILED(hr) || !backBuffer)
    {
        LOG_ERROR("Failed to get back buffer. ErrorCode = 0x%08lx.", hr);
        d3ddev->EndScene();
        return;
    }

    hr = backBuffer->LockRect(&lockedRect, NULL, D3DLOCK_DISCARD);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to lock back buffer. ErrorCode = 0x%08lx.", hr);
        backBuffer->Release();
        d3ddev->EndScene();
        return;
    }

    /* 将 framebuffer 的 data 复制到后台缓冲区 */
    for (INT y = 0; y < height; ++y)
    {
        DWORD *pDst = (DWORD *)((BYTE *)lockedRect.pBits + y * lockedRect.Pitch);
        for (int x = 0; x < width; ++x)
        {
            /* 若 data == nullptr, 说明模型还未加载, 直接全部绘制白色。 */
            pDst[x] = data != nullptr ? (D3DCOLOR)(this->data[(height - 1 - y) * width + x]) : D3DCOLOR_WHITE;
        }
    }

    /* 解锁后台缓冲区 */
    backBuffer->UnlockRect();
    backBuffer->Release();
    d3ddev->EndScene();
    d3ddev->Present(NULL, NULL, NULL, NULL);

    /* 实时更新帧率显示 */
    ++frameCount;
    auto currentTime = std::chrono::steady_clock::now();
    std::chrono::duration<float> elapsedTime = currentTime - lastTime;

    if (elapsedTime.count() >= 1.0f)
    {
        float fps = frameCount / elapsedTime.count();
        frameCount = 0;
        lastTime = currentTime;
        TCHAR fpsText[50];
        _stprintf_s(fpsText, _T("FPS: %.2f"), fps);
        SetWindowText(GetActiveWindow(), fpsText);
    }
    return;
}

/* 处理窗口信息的回调函数 */
static LRESULT CALLBACK windowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HMENU hMenu = GetMenu(hWnd);
    static HMENU hZBufferMenu = GetSubMenu(hMenu, 1);
    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case MENU_ID_OPEN_FILE:
            /* 呼出文件资源管理器, 选择 obj 文件 */
            openFile(hWnd);
            loadSignal = true;
            break;
        /* 菜单栏选择不同的剔除算法 */
        case MENU_ID_NO_ZBUFFER:
            SelectMenu(hZBufferMenu, MENU_ID_NO_ZBUFFER);
            ::zBufferChoice = ZBufferChoice::NONE_ZBUFFER;
            ::switchSignal = true;
            break;
        case MENU_ID_NAIVE_ZBUFFER:
            SelectMenu(hZBufferMenu, MENU_ID_NAIVE_ZBUFFER);
            ::zBufferChoice = ZBufferChoice::NAIVE_ZBUFFER;
            ::switchSignal = true;
            break;
        case MENU_ID_SCANLINE_ZBUFFER:
            SelectMenu(hZBufferMenu, MENU_ID_SCANLINE_ZBUFFER);
            ::zBufferChoice = ZBufferChoice::SCANLINE_ZBUFFER;
            ::switchSignal = true;
            break;
        case MENU_ID_HIERARCHICAL_ZBUFFER:
            SelectMenu(hZBufferMenu, MENU_ID_HIERARCHICAL_ZBUFFER);
            ::zBufferChoice = ZBufferChoice::HIERARCHICAL_ZBUFFER;
            ::switchSignal = true;
            break;
        case MENU_ID_KDTREE_HIERARCHICAL_ZBUFFER:
            SelectMenu(hZBufferMenu, MENU_ID_KDTREE_HIERARCHICAL_ZBUFFER);
            ::zBufferChoice = ZBufferChoice::KDTREE_HIERARCHICAL_ZBUFFER;
            ::switchSignal = true;
            break;
        }
        break;
    case WM_KEYDOWN:
    {
        float yaw = 0.0f;
        float pitch = 0.0f;
        /* 控制旋转的速度 */
        constexpr float rotationStep = 0.01f;
        switch (wParam)
        {
        /* 上下键控制俯仰角 */
        case VK_UP:
            pitch -= rotationStep;
            break;
        case VK_DOWN:
            pitch += rotationStep;
            break;
        /* 左右键控制偏航角 */
        case VK_LEFT:
            yaw -= rotationStep;
            break;
        case VK_RIGHT:
            yaw += rotationStep;
            break;
        default:
            break;
        }
        ::camera.lookAt(yaw, pitch);
    }
    break;
        // case WM_MOUSEMOVE:
        // {
        // static POINT prevMousePos = {0, 0};
        // static float totalYaw = 0.0f;
        // static float totalPitch = 0.0f;

        // POINT currentMousePos;
        // GetCursorPos(&currentMousePos);
        // ScreenToClient(hWnd, &currentMousePos);

        // float deltaX = (currentMousePos.x - prevMousePos.x) * 0.01f;
        // float deltaY = (currentMousePos.y - prevMousePos.y) * 0.01f;

        // totalYaw += deltaX;
        // totalPitch += deltaY;

        // camera.lookAt(totalYaw, totalPitch);

        // prevMousePos = currentMousePos;
    //     break;
    // }
    // case WM_MOUSEWHEEL:
    // {
    // int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
    // zoomFactor += zDelta * 0.001f;
    // if (zoomFactor < 0.1f)
    //     zoomFactor = 0.1f;
    // if (zoomFactor > 10.0f)
    //     zoomFactor = 10.0f;
    // updateCamera(LOWORD(lParam), HIWORD(lParam), zoomFactor);
    //     break;
    // }
    default:
        ::camera.lookAt(0.0f, 0.0f);
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

INT UI::mainWindow(Model &model, const Shader &shader,
                   HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	(void)hPrevInstance;
	(void)lpCmdLine;
	WNDCLASS wc{};
    wc.lpfnWndProc = windowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = _T("WindowClass");
    RegisterClass(&wc);

    HMENU hMenu = CreateMenu();
    HMENU hFileMenu = CreateMenu();
    HMENU hZBufferMenu = CreateMenu();

    /* 文件菜单项 */
    AppendMenu(hFileMenu, MF_STRING, MENU_ID_OPEN_FILE, _T("&Open"));
    AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)hFileMenu, _T("&File"));

    /* 添加 Zbuffer 菜单项 */
    AppendMenu(hZBufferMenu, MF_STRING, MENU_ID_NO_ZBUFFER, _T("No Zbuffer"));
    AppendMenu(hZBufferMenu, MF_STRING, MENU_ID_NAIVE_ZBUFFER, _T("Naive ZBuffer"));
    AppendMenu(hZBufferMenu, MF_STRING, MENU_ID_SCANLINE_ZBUFFER, _T("ScanLine ZBuffer"));
    AppendMenu(hZBufferMenu, MF_STRING, MENU_ID_HIERARCHICAL_ZBUFFER, _T("Hierarchical ZBuffer"));
    AppendMenu(hZBufferMenu, MF_STRING, MENU_ID_KDTREE_HIERARCHICAL_ZBUFFER, _T("KDTree Hierarchial ZBuffer"));

    AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)hZBufferMenu, _T("&Zbuffer"));

    /* 默认为 naive zbuffer */
    SelectMenu(hZBufferMenu, MENU_ID_NAIVE_ZBUFFER);

    LOG_INFO("Screen resolution: %d x %d.", width, height);
    HWND hWnd = CreateWindowEx(0, _T("WindowClass"), _T("3D Viewer"), WS_OVERLAPPEDWINDOW, 100, 100, width, height, NULL, hMenu, hInstance, NULL);
    if (!hWnd)
    {
        LOG_ERROR("CreateWindowEx failed with error: %lu", GetLastError());
    }
    ShowWindow(hWnd, nCmdShow);

    this->initD3D(hWnd);

    /* 主窗口循环 */
    while (msg.message != WM_QUIT)
    {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        this->modelRender(model, shader);
        this->render_frame();
    }

    this->cleanD3D();
    return msg.wParam;
}
