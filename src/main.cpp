#include "core/Engine.hpp"
#include <cstdlib>
#include <exception>
#include <iostream>

int main() {
    try {
        Centralia::Engine engine;
        if (!engine.Start()) {
            std::cerr << "Engine initialization failed.\n";
            return EXIT_FAILURE;
        }

        while (engine.IsRunning()) {
            engine.Update();
            if (engine.IsRunning()) {
                engine.Render();
            }
        }

        engine.Stop();
        return EXIT_SUCCESS;
    } catch (const std::exception& exception) {
        std::cerr << "Fatal error: " << exception.what() << '\n';
        return EXIT_FAILURE;
    }
}