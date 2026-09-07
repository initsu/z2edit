#include <gflags/gflags.h>
#include "imapp.h"
#include "imgui.h"
#include "util/os.h"
#include "util/gamecontrollerdb.h"
#include "util/logging.h"
#include "util/imgui_hidpi.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"
#include "absl/strings/str_cat.h"

DEFINE_double(hidpi, 1.0, "HiDPI scaling factor");
DEFINE_string(controller_db, "", "Path to the SDL gamecontrollerdb.txt file");


ImApp* ImApp::singleton_;

ImApp::ImApp(const std::string& name, int width, int height)
  : name_(name),
    width_(width),
    height_(height),
    running_(true)
{
    singleton_ = this;
    SDL_Init(SDL_INIT_VIDEO |
             SDL_INIT_AUDIO |
             SDL_INIT_JOYSTICK |
             SDL_INIT_GAMEPAD);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

    window_ = SDL_CreateWindow(name.c_str(), width_, height_,
                               SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    glcontext_ = SDL_GL_CreateContext(window_);
    // Setup ImGui binding
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui_ImplSdl_SetHiDPIScale(FLAGS_hidpi);
    ImGui_ImplSDL3_InitForOpenGL(window_, glcontext_);
    ImGui_ImplOpenGL3_Init(nullptr);
    clear_color_ = ImColor(0, 16, 64);
    fpsmgr_.SetRate(60);

    RegisterCommand("quit", "Quit the application.", this, &ImApp::Quit);
}

ImApp::~ImApp() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DestroyContext(glcontext_);
    SDL_DestroyWindow(window_);
    SDL_Quit();
}

void ImApp::InitControllers() {
    if (FLAGS_controller_db.empty()) {
        SDL_IOStream* f = SDL_IOFromConstMem(kGameControllerDB,
                                             kGameControllerDB_len);
        SDL_AddGamepadMappingsFromIO(f, 1);
    } else {
        SDL_AddGamepadMappingsFromFile(FLAGS_controller_db.c_str());
    }

    int num_joysticks = 0;
    SDL_JoystickID* joysticks = SDL_GetJoysticks(&num_joysticks);
    char guid[33];
    for(int i=0; i<num_joysticks; ++i) {
        const char *name, *desc;
        SDL_GUIDToString(SDL_GetJoystickGUIDForID(joysticks[i]),
                         guid, sizeof(guid));
        if (SDL_IsGamepad(joysticks[i])) {
            name = SDL_GetGamepadNameForID(joysticks[i]);
            desc = "Controller";
            printf("%s %d: %s (guid: %s)\n", desc, i,
                   name ? name : "Unknown", guid);
            printf("Opening controller %d\n", i);
            SDL_OpenGamepad(joysticks[i]);
        } else {
            name = SDL_GetJoystickNameForID(joysticks[i]);
            desc = "Joystick";
            printf("%s %d: %s (guid: %s)\n", desc, i,
                   name ? name : "Unknown", guid);
        }
    }
    SDL_free(joysticks);
}

void ImApp::Quit(DebugConsole* console, int argc, char **argv) {
    running_ = false;
}

void ImApp::SetTitle(const std::string& title, bool with_appname) {
    std::string val;
    if (with_appname) {
        val = title.empty() ? name_ : absl::StrCat(name_, ": ", title);
    } else {
        val = name_;
    }
    SDL_SetWindowTitle(window_, val.c_str());
}

void ImApp::Run() {
    while(running_) {
        if (!ProcessEvents()) {
            running_ = false;
            break;
        }
        BaseDraw();
        Uint32 flags = SDL_GetWindowFlags(window_);
        fpsmgr_.SetRate((flags & SDL_WINDOW_INPUT_FOCUS) ? 60 : 30);
        fpsmgr_.Delay();
    }
}

bool ImApp::ProcessEvents() {
    SDL_Event event;
    bool done = false;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);
        if (event.type == SDL_EVENT_QUIT)
            done = true;
        ProcessEvent(&event);
    }
    return !done;
}

void ImApp::BaseDraw() {
    if (!PreDraw()) {
        glViewport(0, 0,
                   (int)ImGui::GetIO().DisplaySize.x,
                   (int)ImGui::GetIO().DisplaySize.y);
        glClearColor(clear_color_.x, clear_color_.y, clear_color_.z, clear_color_.w);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    console_.Draw();
    for(auto it=draw_callback_.begin(); it != draw_callback_.end();) {
        if ((*it)->visible()) {
            (*it)->Draw();
        } else if ((*it)->want_dispose()) {
            it = draw_callback_.erase(it);
            continue;
        }
        ++it;
    }

    Draw();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(window_);
    for(auto& widget : draw_added_) {
        draw_callback_.emplace_back(std::move(widget));
    }
    draw_added_.clear();
}


void ImApp::InitAudio(int freq, int chan, int bufsz, SDL_AudioFormat fmt) {
    SDL_AudioSpec want;
    SDL_zero(want);
    want.format = fmt;
    want.freq = freq;
    want.channels = chan;

    // Opens a device with an unbound audio stream; SDL3 fills any unconsumed
    // space with silence, so no callback is needed.
    SDL_OpenAudioDeviceStream(0, &want, nullptr, nullptr);
}

void ImApp::HelpButton(const std::string& topickey, bool right_justify) {
    if (right_justify) {
        ImGui::SameLine(ImGui::GetWindowWidth() - 50);
    }
    if (ImGui::Button("Help")) {
        Help(topickey);
    }
}

void ImApp::AudioCallback(SDL_AudioStream* stream,
                          int additional_amount, int total_amount) {
    // Fill any requested audio with silence.  SDL3 already produces silence on
    // its own for unconsumed space, so nothing needs to be done here.
}

void ImApp::AudioCallback_(void* userdata, SDL_AudioStream* stream,
                           int additional_amount, int total_amount) {
    ImApp* instance = (ImApp*)userdata;
    instance->AudioCallback(stream, additional_amount, total_amount);
}

void ImApp::AddDrawCallback(ImWindowBase* window) {
    draw_added_.emplace_back(window);
}
