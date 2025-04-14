#include <iostream>
#include "Application.h"

int main() {
    Bcg::Application app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    } catch (...) {
        std::cerr << "Fatal Error: Unknown exception occurred!" << std::endl;
        return EXIT_FAILURE;
    }


    return EXIT_SUCCESS;
}