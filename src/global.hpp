#pragma once

#include <iostream>
#include "ast.hpp"

struct State {
    bool debug;
    bool run = false;
    bool emit_c = true;
    std::string output_file_path;
    std::string input_file_dir;
    std::string input_filename;
    std::string include_path;
};

extern State* global_state;
extern std::vector<FunctionNode*> function_table; // "table"
