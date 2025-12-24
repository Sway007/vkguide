#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include <iostream>
#include <sstream>
#include <vulkan/vulkan_raii.hpp>

#include "Engine.hpp"

constexpr int WINDOW_WIDTH = 640;
constexpr int WINDOW_HEIGHT = 480;

void throwSDLError(const char* api) {
    std::stringstream ss;
    ss << api << " Error: " << SDL_GetError();
    throw std::runtime_error(ss.str());
}

void createSDLSurface(const vk::Instance& vkInstance, SDL_Window*& window, VkSurfaceKHR& surface) {
    window = SDL_CreateWindow("Vk SDL Project", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_VULKAN);
    if(window == nullptr) {
        throwSDLError("SDL_CreateWindow");
    }

    if(!SDL_Vulkan_CreateSurface(window, vkInstance, nullptr, &surface)) {
        throwSDLError("SDL_Vulkan_CreateSurface");
    }
}

void mainLoop(Engine& engine) {
    SDL_Event e;
    bool      bQuit{false};

    while(!bQuit) {
        while(SDL_PollEvent(&e)) {
            if(e.type == SDL_EVENT_QUIT || e.type == SDL_EVENT_TERMINATING) bQuit = true;
        }
        engine.draw();
    }
}

int main() {
    SDL_Init(SDL_INIT_VIDEO);

    uint32_t                 count;
    auto                     pExtensions = SDL_Vulkan_GetInstanceExtensions(&count);
    std::vector<const char*> extensions(count);

    for(uint i = 0; i < count; i++) {
        extensions[i] = pExtensions[i];
    }
    std::vector<const char*> layers = {"VK_LAYER_KHRONOS_validation"};
    {
        Engine engine(extensions, layers);

        VkSurfaceKHR SDLSurface = nullptr;
        SDL_Window*  window = nullptr;

        createSDLSurface(engine.m_instance, window, SDLSurface);
        engine.initWithSurface(SDLSurface);

        std::cout << "vk render app.\n";
        mainLoop(engine);

        SDL_DestroyWindow(window);
        SDL_Quit();
    }
    std::cout << "exit gracefully~ bye!\n";
}