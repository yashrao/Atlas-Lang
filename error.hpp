#pragma once

#include <string>
#include <iostream>

void print_error_msg(std::string error);
void log_print(std::string message);

// Colors
//const char* CL_BLACK   = "\e[0;30m";
//const char* CL_RED     = "\e[0;31m";
//const char* CL_GREEN   = "\e[0;32m";
//const char* CL_YELLOW  = "\e[0;33m";
//const char* CL_BLUE    = "\e[0;34m";
//const char* CL_MAGENTA = "\e[0;35m";
//const char* CL_CYAN    = "\e[0;36m";
//const char* CL_WHITE   = "\e[0;37m";
//const char* CL_RESET   = "\e[0m";

#define CL_BLACK  "\e[0;30m"
#define CL_RED    "\e[0;31m"
#define CL_GREEN  "\e[0;32m"
#define CL_YELLOW "\e[0;33m"
#define CL_BLUE   "\e[0;34m"
#define CL_MAGENTA "\e[0;35m"
#define CL_CYAN   "\e[0;36m"
#define CL_WHITE  "\e[0;37m"
#define CL_RESET  "\e[0m"

