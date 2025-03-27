#include "defs.h"
#include "utils.h"
#include "ui.h"
#include "Model.h"
#include "Shader.h"

static constexpr INT width = 800;
static constexpr INT height = 600;

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
    // LOG_INFO( "Screen resolution: %d x %d.", width, height);
    UI ui{width, height};
    Model model;
    Shader shader;

    return ui.mainWindow(model, shader, hInstance, hPrevInstance, lpCmdLine, nCmdShow);
}