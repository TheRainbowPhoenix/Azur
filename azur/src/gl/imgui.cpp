#include <azur/gl/gl.h>
#include <azur/log.h>
#include <azur/azur.h>

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>

static bool initialized = false;

bool azur_init_imgui(void)
{
#if AZUR_TOOLKIT_GINT
    return false;
#endif

    azlog(INFO, "Initializing ImGui...\n");

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    if(!ImGui_ImplSDL2_InitForOpenGL(
            azur_sdl_window(), SDL_GL_GetCurrentContext())) {
        azlog(FATAL, "ImGui_ImplSDL2_InitForOpenGL failed\n");
        ImGui::DestroyContext();
        return false;
    }

#if AZUR_GRAPHICS_OPENGL_3_3
    char const *glsl_version = "#version 330";
#elif AZUR_GRAPHICS_OPENGL_ES_2_0
    char const *glsl_version = "#version 100";
#elif AZUR_GRAPHICS_OPENGL_ES_3_0
    char const *glsl_version = "#version 300 es";
#endif

    if(!ImGui_ImplOpenGL3_Init(glsl_version)) {
        azlog(FATAL, "ImGui_ImplOpenGL3_Init(\"%s\") failed\n", glsl_version);
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
        return false;
    }

    initialized = true;
    return true;
}

namespace azur_internal {

void azur_quit_imgui(void)
{
    if(!initialized)
        return;

    azlog(INFO, "Shutting down ImGui...\n");
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    initialized = false;
}

} /* namespace azur_internal */
