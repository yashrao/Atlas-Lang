#include "error.hpp"
#include "global.hpp"

void print_error_msg(std::string error) {
    std::cout << CL_RED << "[COMPILER ERROR]: " << CL_RESET << error << "\n";
}

void log_print(std::string message) {
    if(global_state->debug) {
        std::cout << "[INFO]: " + message;
    }
}

