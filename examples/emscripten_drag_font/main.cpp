// Dear ImGui: standalone example application for SDL2 + OpenGL
// (SDL is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

// ============================================================================
// EMSCRIPTEN DRAG & DROP DEMO
// ============================================================================
// This example demonstrates how to implement file drag-and-drop in a browser
// using Emscripten. It shows two methods of loading files:
//
// 1. FILE PICKER BUTTON:
//    - ImGui button triggers JavaScript via emscripten_run_script()
//    - JavaScript opens a hidden HTML <input type="file"> element
//    - User selects a file, triggering the 'change' event handler
//
// 2. DRAG AND DROP:
//    - HTML5 drag-and-drop event listeners attached to the canvas
//    - 'dragover' event shows visual feedback (green border)
//    - 'drop' event captures the dropped file
//
// JAVASCRIPT TO C++ BRIDGE:
//    - JavaScript reads file as ArrayBuffer using FileReader API
//    - Allocates memory in Emscripten heap with Module._malloc()
//    - Copies file data to the allocated memory
//    - Calls HandleFontFile() C++ function via Module.ccall()
//    - HandleFontFile() processes the data and stores it in g_fontFile
//    - JavaScript frees the temporary memory with Module._free()
//
// The C++ side simply stores the file data and displays it in an ImGui window.
// ============================================================================

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <string.h>
#include <SDL.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL_opengles2.h>
#else
#include <SDL_opengl.h>
#endif
#ifdef _WIN32
#include <windows.h>        // SetProcessDPIAware()
#endif

// This example can also compile and run with Emscripten! See 'Makefile.emscripten' for details.
#ifdef __EMSCRIPTEN__
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#include <emscripten.h>
#endif

// Global state for loaded font file
struct FontFileData
{
    char filename[256];
    unsigned char* data;
    int size;
    bool loaded;
};

static FontFileData g_fontFile = { "", nullptr, 0, false };

// ============================================================================
// HandleFontFile: JavaScript-callable C++ function
// ============================================================================
// This function is called from JavaScript (see shell_drag_font.html) via:
//   Module.ccall('HandleFontFile', null, ['string', 'number', 'number'], 
//                [filename, size, bufferPtr]);
//
// The EMSCRIPTEN_KEEPALIVE macro ensures this function is not optimized away
// by the linker and remains accessible from JavaScript. The function is also
// listed in EXPORTED_FUNCTIONS in Makefile.emscripten.
//
// Parameters:
//   - filename: Original file name from the browser
//   - size: Size of the file in bytes
//   - buffer: Pointer to file data in Emscripten heap (allocated by JavaScript)
//
// The function:
//   1. Frees any previously loaded file data
//   2. Copies the filename to g_fontFile
//   3. Allocates new memory and copies the file data
//   4. Sets the loaded flag so the UI can display the file info
//
// Note: JavaScript is responsible for freeing the temporary buffer it allocates.
// ============================================================================
#ifdef __EMSCRIPTEN__
extern "C" {
    EMSCRIPTEN_KEEPALIVE
    void HandleFontFile(const char* filename, int size, const unsigned char* buffer)
    {
        // Free previous data if any
        if (g_fontFile.data)
        {
            free(g_fontFile.data);
            g_fontFile.data = nullptr;
        }
        
        // Store filename
        strncpy(g_fontFile.filename, filename, sizeof(g_fontFile.filename) - 1);
        g_fontFile.filename[sizeof(g_fontFile.filename) - 1] = '\0';
        
        // Copy buffer data
        g_fontFile.size = size;
        g_fontFile.data = (unsigned char*)malloc(size);
        if (g_fontFile.data)
        {
            memcpy(g_fontFile.data, buffer, size);
            g_fontFile.loaded = true;
        }
    }
}
#endif


// GUI function
static void ShowFontDragDropDemo()
{
    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
    ImGui::Begin("Font File Drag & Drop Demo");
    
    ImGui::Text("This demo shows how to load font files in the browser.");
    ImGui::Separator();
    
    // Browse button
    if (ImGui::Button("Browse for Font File..."))
    {
#ifdef __EMSCRIPTEN__
        emscripten_run_script("openFilePicker();");
#endif
    }
    
    ImGui::SameLine();
    ImGui::TextDisabled("(Or drag & drop a font file onto the window)");
    
    ImGui::Separator();
    
    // Display loaded file info
    if (g_fontFile.loaded)
    {
        ImGui::Text("File loaded:");
        ImGui::BulletText("Filename: %s", g_fontFile.filename);
        ImGui::BulletText("Size: %d bytes", g_fontFile.size);
        
        ImGui::Separator();
        ImGui::Text("First 32 bytes (hex):");
        
        // Display hex dump - 2 rows of 16 bytes each
        int bytesToShow = (g_fontFile.size < 32) ? g_fontFile.size : 32;
        char hexLine[128];
        
        // First row (bytes 0-15)
        if (bytesToShow > 0)
        {
            hexLine[0] = '\0';
            int firstRowBytes = (bytesToShow < 16) ? bytesToShow : 16;
            for (int i = 0; i < firstRowBytes; i++)
            {
                char hex[4];
                snprintf(hex, sizeof(hex), "%02X ", g_fontFile.data[i]);
                strcat(hexLine, hex);
            }
            ImGui::Text("0x00: %s", hexLine);
        }
        
        // Second row (bytes 16-31)
        if (bytesToShow > 16)
        {
            hexLine[0] = '\0';
            int secondRowBytes = bytesToShow - 16;
            for (int i = 16; i < 16 + secondRowBytes; i++)
            {
                char hex[4];
                snprintf(hex, sizeof(hex), "%02X ", g_fontFile.data[i]);
                strcat(hexLine, hex);
            }
            ImGui::Text("0x10: %s", hexLine);
        }
    }
    else
    {
        ImGui::TextDisabled("No font file loaded yet.");
    }
    
    ImGui::End();
}



// Main code
int main(int, char**)
{
    // Setup SDL
#ifdef _WIN32
    ::SetProcessDPIAware();
#endif
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return 1;
    }

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100 (WebGL 1.0)
    const char* glsl_version = "#version 100";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(IMGUI_IMPL_OPENGL_ES3)
    // GL ES 3.0 + GLSL 300 es (WebGL 2.0)
    const char* glsl_version = "#version 300 es";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
    // GL 3.2 Core + GLSL 150
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    // From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    float main_scale = ImGui_ImplSDL2_GetContentScaleForDisplay(0);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow("Dear ImGui SDL2+OpenGL3 example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, (int)(1280 * main_scale), (int)(800 * main_scale), window_flags);
    if (window == nullptr)
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return 1;
    }

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (gl_context == nullptr)
    {
        printf("Error: SDL_GL_CreateContext(): %s\n", SDL_GetError());
        return 1;
    }

    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;        // Set initial font scale. (in docking branch: using io.ConfigDpiScaleFonts=true automatically overrides this for every window depending on the current monitor)

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    bool done = false;

#ifdef __EMSCRIPTEN__
    // For an Emscripten build we are disabling file-system access, so let's not attempt to do a fopen() of the imgui.ini file.
    // You may manually call LoadIniSettingsFromMemory() to load settings from your own storage.
    io.IniFilename = nullptr;
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (!done)
#endif
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }
        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED)
        {
            SDL_Delay(10);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ShowFontDragDropDemo();

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

