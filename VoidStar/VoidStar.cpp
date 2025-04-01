// Quantum.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <tracy/Tracy.hpp>
#define USE_PIX
#include "Windows.h"
#include "pix3.h"

#undef near
#undef far

#include "IVoidRenderContext.h"
#include "SDL.h"
#include "VoidStarEngine.h"
#undef main

int main()
{
    tracy::SetThreadName("Main thread");

    //PIXLoadLatestWinPixGpuCapturerLibrary();

    std::cout << "start init\n";

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << '\n';
        return 1;
    }

    // Create an SDL window
    SDL_Window* window = SDL_CreateWindow("SDL2 Window",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1920, // Width
        1080, // Height
        SDL_WINDOW_SHOWN);
    if (window == nullptr) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << '\n';
        SDL_Quit();
        return 1;
    }

    auto extent = VoidExtent2D{ 1920, 1080 };

    auto engine = new star::VoidStarEngine();

    try 
    {
        engine->Init(extent, window);

        engine->Run();

        engine->Shutdown();
    }
    catch(std::runtime_error& e)
    {
        printf("Caught runtime error: ");
        printf(e.what());
        printf("\n");
        abort();
    }

    delete engine;

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
