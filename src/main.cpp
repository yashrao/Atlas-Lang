#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <cstring>
#include <unistd.h>
// debug
#include <memory>

#include "tokenize.hpp"
#include "error.hpp"
#include "ast.hpp"

#define DEPEND_LIBC

ExpressionNode* ast_create_expression(std::vector<Token*> tokens, bool is_args, bool is_cond, bool is_arr, int* i);
ExpressionNode* ast_create_expr_prec(std::vector<Token*> tokens, int precedence, bool is_args, bool is_cond, bool is_arr, int* i);
void codegen_block(BlockNode* block, std::ofstream* file, int tab_level);
BlockNode* ast_create_block(std::vector<Token*> tokens, int* i);
void codegen_tabs(std::ofstream* file, int tab_level);
bool codegen_statement(StatementNode* statement, std::ofstream* file, int tab_level);
void codegen_expr(ExpressionNode* expression, std::ofstream* file);
std::string read_file (std::string filename);
StatementNode* ast_create_declaration(std::vector<Token*> tokens, int* i);
std::string ast_get_file_full_path(std::string filename);
std::vector<StatementNode*> ast_create(std::vector<Token*> tokens);
VarType get_var_type(Token* var_type);
std::string codegen_get_c_type(Token* atlas_type);

#include "global.hpp"
State* global_state = NULL;

std::vector<FunctionNode*> function_table; // "table"

std::string get_nt_str(NodeType nt) {
    switch(nt) {
    case NODE_FUNC:
        return "NODE_FUNC";
    case NODE_PARAM:
        return "NODE_PARAM";
    case NODE_BLOCK:
        return "NODE_BLOCK";
    case NODE_ROOT:
        return "NODE_ROOT";
    case NODE_VAR_DECL:
        return "NODE_VAR_DECL";
    case NODE_CALL:
        return "NODE_CALL";
    case NODE_ASSIGN:
        return "NODE_ASSIGN";
    case NODE_BINOP:
        return "NODE_BINOP";
    case NODE_CONSTANT:
        return "NODE_CONSTANT";
    case NODE_VAR:
        return "NODE_VAR";
    case NODE_RETURN:
        return "NODE_RETURN";
    case NODE_IF:
        return "NODE_IF";
    case NODE_FOR:
        return "NODE_FOR";
    case NODE_TYPE:
        return "NODE_TYPE";
    case NODE_ARRAY_EXPR:
        return "NODE_ARRAY_EXPR";
    case NODE_CHAR:
        return "NODE_CHAR";
    case NODE_QUOTE:
        return "NODE_QUOTE";
    case NODE_SUBSCRIPT:
        return "NODE_SUBSCRIPT";
    case NODE_UNARY:
        return "NODE_UNARY";
    case NODE_MEMBER_ACCESS:
        return "NODE_MEMBER_ACCESS";
    case NODE_TYPE_INST:
        return "NODE_TYPE_INST";
    case NODE_CINCLUDE:
        return "NODE_CINCLUDE";
    default:
        return "NODE_INVALID";
    }
}

void atlas_lib(std::ofstream* file) {
    *file << "extern int open(const char* filename, int flags, int mode);\n";
    *file << "extern int close(int fileds);\n";
    *file << "extern void* malloc(long unsigned int size);\n";
    *file << "extern void free(void* ptr);\n";
    
    //TODO: might not need this part lol
    *file << "#define SYSCALL_EXIT 60\n"
          << "#define SYSCALL_WRITE 1\n"
          << "typedef unsigned char uchar;\n"
          << "typedef unsigned char byte;\n"
          << "typedef char sbyte;\n"
          << "typedef short int16;\n"
          << "typedef unsigned short uint16;\n"
          << "typedef unsigned short ushort;\n"
          << "typedef int int32;\n"
          << "typedef unsigned int uint32;\n"
          << "typedef unsigned int uint;\n"
          << "typedef long long int64;\n"
          << "typedef unsigned long long uint64;\n"
          << "typedef enum { false, true } bool;\n"
          << "#define true 1\n"
          << "#define false 0\n";

    *file << "\n";

    *file << "void atlas_exit(int exit_code)\n"
          << "{\n"
          << "\tasm volatile\n"
          << "\t(\n"
          << "\t\t\"syscall\"\n"
          << "\t\t:\n" 
          << "\t\t: \"a\"(SYSCALL_EXIT), \"D\"(exit_code)\n"
          << "\t\t: \"rcx\", \"r11\", \"memory\"\n"
          << "\t);\n"
          << "}\n\n";

    *file << "void atlas_putchar(char c) {\n"
          << "\tasm volatile (\n"
          << "\t\t\"movq $1, %%rax\\n\"\n"
          << "\t\t\"movq $1, %%rdi\\n\"\n"
          << "\t\t\"movq %0, %%rsi\\n\"\n"
          << "\t\t\"movq $1, %%rdx\\n\"\n"
          << "\t\t\"syscall\"\n"
          << "\t\t:\n"
          << "\t\t: \"r\"(&c)\n"
          << "\t\t: \"%rax\", \"%rdi\", \"%rsi\", \"%rdx\"\n"
          << "\t);\n"
          << "}\n\n";
}

void codegen_init_c(std::vector<StatementNode*> ast, std::ofstream* file) {
    // TODO: going to be dependent on libc for some time lol
    #ifndef DEPEND_LIBC
	*file << "void _start(void)\n"
          << "{\n"
          << "\tint ret = main();\n";

    *file << "\tsys_exit(ret);\n"
          << "}\n\n";
    #endif
}

void codegen_end(std::ofstream* file, std::string backend) {
    (*file).close();

    std::string output_file_path;
    if(global_state->output_file_path.size() != 0) {
        output_file_path = global_state->output_file_path;
    } else {
        output_file_path = "a.out";
    }
    if (!global_state->emit_c) {
        std::string remove = "rm out.c";
        std::system(remove.c_str());
    }

    std::string command = backend + " -nostdlib out.c -o " + output_file_path;
    log_print("Running \"" + command + "\"\n");
    // Compile C code
    int ret = std::system(command.c_str());
    //TODO: not sure what scenarios this works/not works
    if (WEXITSTATUS(ret) != 0x00) {
        std::string err = backend + " backend Failed to compile C program";
        print_error_msg(err.c_str());
        std::cout << "                  Check " << backend << " error messages\n";
        std::cout << "                  Exit Code: " << ret << "\n";
        exit(1);
    }
    log_print("Generated binary \"" + output_file_path + "\"\n");
}

void codegen_end_libc(std::ofstream* file, std::string backend) {
    (*file).close();

    std::string output_file_path;
    if(global_state->output_file_path.size() != 0) {
        output_file_path = global_state->output_file_path;
    } else {
        output_file_path = "a.out";
    }

    //TODO: get rid of this mimalloc string?
    //      for some reason it doesn't link properly on my machine
    std::string command = backend + " out.c -o " + output_file_path;
    log_print("Running \"" + command + "\"\n");
    // Compile C code
    int ret = std::system(command.c_str());
    //TODO: not sure what scenarios this works/not works
    if (WEXITSTATUS(ret) != 0x00) {
        std::string err = backend + " backend Failed to compile C program";
        print_error_msg(err.c_str());
        std::cout << "                  Check " << backend << " error messages\n";
        std::cout << "                  Exit Code: " << ret << "\n";
        exit(1);
    }
    log_print("Generated binary \"" + output_file_path + "\"\n");
}

void codegen_array_expr(ArrayNode* array, std::ofstream* file) {
    *file << "{";
    for (int i = 0; i < array->elements.size(); i++) {
        //std::cout << array->elements[i] << ": ";
        //std::cout << get_nt_str(array->elements[i]->nt) << "\n";
        codegen_expr(array->elements[i], file);
        if (i != array->elements.size() - 1) {
            *file << ", ";
        }
    }
    *file << "}";
}

std::string codegen_get_intrinsic_name(std::string name) {
    if (name == "putchar") {
        return "atlas_putchar";
    } else if (name == "alloc") {
        return "malloc";
    } else if (name == "free") {
        return "free";
    } else if (name == "open") {
        return "open";
    } else if (name == "close") {
        return "close";
    } else if (name == "sizeof") {
        return "sizeof";
    } else if (name == "exit") {
        return "atlas_exit";
    } else if (name == "new") {
        return "new"; // TODO: implement
    } else {
        std::string err = "\"" + name + "\"" + " intrinsic has not been defined\n";
        print_error_msg(err);
        exit(1);
    }
}

bool codegen_is_intrinsic_type(Token* atlas_type) {
    if (atlas_type->token == "i64") {
        return true;
    } else if (atlas_type == NULL) {
        return true;
    } else if (atlas_type->token == "i32") {
        return true;
    } else if (atlas_type->token == "i16") {
        return true;
    } else if (atlas_type->token == "i8") {
        return true;
    } else if (atlas_type->token == "u64") {
        return true;
    } else if (atlas_type->token == "u32") {
        return true;
    } else if (atlas_type->token == "u16") {
        return true;
    } else if (atlas_type->token == "u8") {
        //std::cout << "[ERROR]: \"u8\" IS NOT SUPPORTED\n";
        return true;
        //exit(1);
    }
    return false;
}

std::string codegen_get_c_intrinsic_type(Token* atlas_type) {
    if (atlas_type->token == "i64") {
        return "int64";
    } else if (atlas_type == NULL) {
        return "void"; //NOTE: what is this?
    } else if (atlas_type->token == "i32") {
        return "int32";
    } else if (atlas_type->token == "i16") {
        return "int16";
    } else if (atlas_type->token == "i8") {
        return "char";
    } else if (atlas_type->token == "u64") {
        return "uint64";
    } else if (atlas_type->token == "u32") {
        return "uint32";
    } else if (atlas_type->token == "u16") {
        return "uint16";
    } else if (atlas_type->token == "u8") {
        return "uchar";
    }
    print_error_msg("Something wrong has occurred in codegen_get_c_intrinsic_type");
}

bool codegen_is_intrinsic_function(std::string call_name) {
    if (call_name == "putchar") {
        return true;
    } else if (call_name == "open") {
        return true;
    } else if (call_name == "close") {
        return true;
    } else if (call_name == "alloc") {
        return true;
    } else if (call_name == "free") {
        return true;
    } else if (call_name == "sizeof") {
        return true;
    } else if (call_name == "exit") {
        return true;
    } else {
        return false;
    }
}

void codegen_char(CharacterNode* character, std::ofstream* file) {
    *file << "'";
    if (character->value.size() != 0) {
        *file << character->value;
    }
    *file << "'";
}

void codegen_quote(QuoteNode* quote, std::ofstream* file) {
    //*file << "atlas_create_string("
    *file << "Z_19atlas_create_string6string("
          << "\"" << quote->quote_token->token << "\""
          << ","  << quote->quote_token->token.size()
          << ")";
    //*file << "\"" << quote->quote_token->token << "\"";
}

void codegen_subscript(SubscriptNode* subscript, std::ofstream* file) {
    if (subscript->is_declaration) {
        *file << "{";
    } else {
        *file << "[";
    }
    for (int i = 0; i < subscript->indexes.size(); i++) {
        ExpressionNode* index = subscript->indexes[i];
        codegen_expr(index, file);
        if (i == subscript->indexes.size() - 1) {
            break;
        }
        *file << ", ";
    }
    if (subscript->is_declaration) {
        *file << "}";
    } else {
        *file << "]";
    }
}

void codegen_type_inst(TypeInstNode* type_inst, std::ofstream* file) {
    *file << "{";
    for (int i = 0; i < type_inst->values.size(); i++) {
        ExpressionNode* value = type_inst->values[i];
        codegen_expr(value, file);
        if (i == type_inst->values.size() - 1) {
            break;
        }
        *file << ", ";
    }
    *file << "}";
}

void codegen_unary_op(UnaryOpNode* unary_op, std::ofstream* file) {
    //TODO: old code?
    std::string str;
    switch(unary_op->operator_type) {
    case NODE_SUBSCRIPT:
        codegen_expr(unary_op->operand, file);
        codegen_subscript(unary_op->subscript, file);
        break;
    default:
        print_error_msg("unary op not implemented yet\n");
        exit(1);
    }
}

std::string codegen_get_call_mangled(std::string name) {
    for (FunctionNode* func : function_table) {
        if (func->token->token == name) {
            return func->mangled_name;
        } else if (codegen_is_intrinsic_function(name)) {
            return codegen_get_intrinsic_name(name);
        }
    }
    // just return the original name if not found for whatever reason
    return name;
}

void codegen_expr(ExpressionNode* expression, std::ofstream* file) {
    file->flush();
    if(expression->needs_paren) {
        *file << "(";
    }
    switch(expression->nt) {
    case NODE_BINOP:
        codegen_expr(expression->binop->lhs, file);
        if (expression->binop->op->tt == TK_DOT || expression->binop->op->tt == TK_SQUARE_OPEN) {
            *file << expression->binop->op->token;
        } else {
            *file << " " << expression->binop->op->token << " ";
        }
        // handle rhs
        codegen_expr(expression->binop->rhs, file);
        if (expression->binop->op->tt == TK_SQUARE_OPEN) {
            *file << "]";
        }
        break;
    case NODE_CONSTANT:
        *file << expression->constant->value;
        break;
    case NODE_CALL:
    {
        //std::string call_name = expression->call_node->name->token;
        std::string call_name = codegen_get_call_mangled(expression->call_node->name->token);

        *file << call_name << "("; 
        auto args = expression->call_node->args;
        for (int i = 0; i < args.size(); i++) {
            codegen_expr(args[i], file);
            if (i < args.size() - 1) {
                *file << ", ";
            }
        }
        *file << ")";
        break;
    }
    case NODE_VAR:
    {
        //TODO: make this better lol - for reserved types 
        if (codegen_is_intrinsic_type(expression->var_node->identifier)) {
            *file << codegen_get_c_intrinsic_type(expression->var_node->identifier);
        } else {
            *file << expression->var_node->identifier->token;
        }
        break;
    }
    case NODE_ARRAY_EXPR:
        codegen_array_expr(expression->array, file);
        break;
    case NODE_CHAR:
        codegen_char(expression->character, file);
        break;
    case NODE_QUOTE:
        codegen_quote(expression->quote, file);
        break;
    case NODE_SUBSCRIPT:
        codegen_subscript(expression->subscript, file);
        break;
    case NODE_UNARY:
        //TODO:
        codegen_unary_op(expression->unary_op, file);
        break;
    case NODE_TYPE_INST:
        codegen_type_inst(expression->type_inst, file);
        break;
    default:
        std::string err = "CODEGEN EXPR " + get_nt_str(expression->nt) + "\n";
        print_error_msg(err);
        std::cout << expression->nt;
        exit(1);
    }
    if(expression->needs_paren) {
        *file << ")";
    }
}

std::string codegen_get_c_type(Token* atlas_type) {
    if (atlas_type->token == "i64") {
        return "int64";
    } else if (atlas_type == NULL) {
        return "void";
    } else if (atlas_type->token == "i32") {
        return "int32";
    } else if (atlas_type->token == "i16") {
        return "int16";
    } else if (atlas_type->token == "i8") {
        return "int8";
    } else if (atlas_type->token == "u64") {
        return "uint64";
    } else if (atlas_type->token == "u32") {
        return "uint32";
    } else if (atlas_type->token == "u16") {
        return "uint16";
    } else if (atlas_type->token == "u8") {
        return "char";
        //print_error_msg("\"u8\" IS NOT SUPPORTED");
        //exit(1);
    } else if (atlas_type->token == "string") {
        //return "AtlasTypeString";
        return "string";
    } else if (atlas_type->token == "bool") {
        return "bool";
    }
    std::string err = "The \"" + atlas_type->token + "\" type is not supported";
    print_error_msg(err);
    exit(1);
}

bool codegen_is_c_type(Token* atlas_type) {
    if (atlas_type->token == "i64") {
        return true;
    } else if (atlas_type == NULL) {
        return true;
    } else if (atlas_type->token == "i32") {
        return true;
    } else if (atlas_type->token == "i16") {
        return true;
    } else if (atlas_type->token == "i8") {
        return true;
    } else if (atlas_type->token == "u64") {
        return true;
    } else if (atlas_type->token == "u32") {
        return true;
    } else if (atlas_type->token == "u16") {
        return true;
    } else if (atlas_type->token == "u8") {
        //std::cout << "[ERROR]: \"u8\" IS NOT SUPPORTED\n";
        return true;
        //exit(1);
    }
    return false;
}

void codegen_var_decl(VarDeclNode* var_decl, std::ofstream* file) {
    if (var_decl->is_static) {
        *file << "static ";
    }
    if (var_decl->is_const) {
        *file << "const ";
    }
    // lhs
    if (codegen_is_c_type(var_decl->lhs->type_)) {
        *file << codegen_get_c_type(var_decl->lhs->type_);
    } else if (var_decl->lhs->type_->token == "string") {
        *file << "string"; // TODO:
    } else {
        *file << var_decl->lhs->type_->token;
    }

    for (int i = 0; i < var_decl->lhs->ptr_level; i++) {
        *file << "*";
    }

    *file << " " << var_decl->lhs->identifier->token;

    print_token(var_decl->lhs->identifier);
    if (var_decl->lhs->is_array == true) {
        *file << "[";
        codegen_expr(var_decl->lhs->arr_size, file);
        *file << "]";
    }

    // rhs
    if (var_decl->rhs != NULL) {
        *file << " = ";
        codegen_expr(var_decl->rhs, file);
    }
}

void codegen_param(ParamNode* param, std::ofstream* file) {
    // lhs
    if (codegen_is_c_type(param->type_)) {
        *file << codegen_get_c_type(param->type_);
    } else if (param->type_->token == "string") {
        *file << "string"; // TODO:
    } else {
        *file << param->type_->token;
    }

    for (int i = 0; i < param->ptr_level; i++) {
        *file << "*";
    }

    *file << " " << param->identifier->token;

    print_token(param->identifier);
    if (param->is_array == true) {
        *file << "[";
        codegen_expr(param->arr_size, file);
        *file << "]";
    }
}

void codegen_type(TypeNode* type, std::ofstream* file) {
    *file << "typedef struct " << type->name->token << "\n";
    *file << "{\n";
    for (VarDeclNode* var : type->declarations) {
        codegen_tabs(file, 1);
        codegen_var_decl(var, file);
        *file << ";\n";
    }
    *file << "}" << type->name->token << ";\n\n";
}

void codegen_return(StatementNode* statement, std::ofstream* file) {
    *file << "return ";
    codegen_expr(statement->return_lhs->expr, file);
}

void codegen_if(IfNode* if_node, std::ofstream* file, int tab_level) {
    *file << "if(";
    codegen_expr(if_node->condition, file);
    *file << ")\n";
    codegen_block(if_node->block, file, tab_level + 1);
    if (if_node->_else != NULL) {
        if (if_node->_else->block != NULL) {
            codegen_tabs(file, tab_level);
            *file << "else\n";
            codegen_block(if_node->_else->block, file, tab_level + 1);
        } else if (if_node->_else->else_if != NULL) {
            codegen_tabs(file, tab_level);
            *file << " else ";
            codegen_if(if_node->_else->else_if->if_lhs, file, tab_level);
        }
    }
}

void codegen_for(ForNode* for_node, std::ofstream* file, int tab_level) {
    if (for_node->for_type == FOR_LOOP) {
        *file << "for(";
        codegen_statement(for_node->init, file, tab_level);
        *file << "; ";
        codegen_expr(for_node->test, file);
        *file << "; ";
        // NOTE: Can't generate statements here now
        codegen_expr(for_node->update->expr_lhs, file);
        *file << ")\n";
        codegen_block(for_node->block, file, tab_level + 1);
    } else if (for_node->for_type == FOR_WHILE) {
        *file << "for(;";
        codegen_expr(for_node->test, file);
        *file << ";)\n";
        codegen_block(for_node->block, file, tab_level + 1);
    } else {
        print_error_msg("Codegen for this for loop type is not implemented yet...");
        exit(1);
    }
}

void codegen_assign(AssignNode* assign, std::ofstream* file) {
    // lhs
    *file << assign->lhs->identifier->token;
    if (assign->lhs->is_array) {
        *file << "[";
        codegen_expr(assign->lhs->arr_size, file);
        *file << "]";
    }
    // rhs
    *file << " = ";
    codegen_expr(assign->rhs, file);
}

bool codegen_statement(StatementNode* statement, std::ofstream* file, int tab_level) {
    switch(statement->nt) {
    case NODE_VAR_DECL:
        codegen_var_decl(statement->vardecl_lhs, file);
        return true;
    case NODE_ASSIGN:
        //TODO: does this even exist anymore?
        codegen_expr(statement->expr_lhs, file);
        return true;
    case NODE_BINOP:
        codegen_expr(statement->expr_lhs, file);
        return true;
    case NODE_IF:
        codegen_if(statement->if_lhs, file, tab_level);
        return false;
    case NODE_RETURN:
        codegen_return(statement, file);
        return true;
    case NODE_FOR:
        codegen_for(statement->for_lhs, file, tab_level);
        return false;
    case NODE_CALL:
        codegen_expr(statement->expr_lhs, file);
        return true;
    default:
        std::string err = "CODEGEN STATEMENT " + get_nt_str(statement->nt) + "\n";
        print_error_msg(err);
        exit(1);
    }
    return false;
}

void codegen_func(FunctionNode* func, std::ofstream* file) {
    if (func->return_types == NULL) {
        *file << "void ";
    } else {
        *file << codegen_get_c_type(func->return_types) << " ";
    }
    //*file << func->token->token << "(";
    *file << func->mangled_name << "(";
    bool add_comma = true;
    // Args
    for (int i = 0; i < func->params.size(); i++) {
        codegen_param(func->params[i], file);
        if (i + 1 != func->params.size()) {
            *file << ", ";
        }
    }
    if (func->params.size() == 0) {
        *file << "void";
    }
    *file << ")\n";
    if(func->block != NULL) {
        codegen_block(func->block, file, 1);
    } else {
        *file << ";";
    }
    *file << "\n";
}

void codegen_tabs(std::ofstream* file, int tab_level) {
    for (int i = 0; i < tab_level; i++) {
        *file << "\t";
    }
}

void codegen_block(BlockNode* block, std::ofstream* file, int tab_level) {
    codegen_tabs(file, tab_level - 1);
    *file << "{\n";
    if (block != NULL) {
        for (StatementNode* statement : block->statements) {
            codegen_tabs(file, tab_level);
            if(codegen_statement(statement, file, tab_level)) {
                *file << ";\n";
            }
        }
    }
    codegen_tabs(file, tab_level - 1);
    *file << "}\n";
}

void codegen_start(std::vector<StatementNode*> ast, std::string filename, std::string backend) {
    std::ofstream file(filename);
    atlas_lib(&file);
    for (StatementNode* node : ast) {
        log_print("Generating Node: " +  get_nt_str(node->nt) + "\n");
        if (node->nt == NODE_FUNC) {
            codegen_func(node->func_lhs, &file);
        } else if (node->nt == NODE_TYPE) {
            codegen_type(node->type_lhs, &file);
        } else if (node->nt == NODE_CALL) {
            codegen_expr(node->expr_lhs, &file);
        } else if (node->nt == NODE_VAR_DECL) {
            codegen_var_decl(node->vardecl_lhs, &file);
            file << ";\n";
        } else if (node->nt == NODE_CINCLUDE) {
            file << "#include <";
            file << node->cinclude_lhs->name->token;
            file << ">\n";
        }
    }
    codegen_init_c(ast, &file);
    codegen_end_libc(&file, backend);
}

/* Codegen end */

void check_valid_backend(std::string backend) {
    //TODO: actually check if the backend exists
    if (backend == "gcc" || backend == "clang" || backend == "tcc") {
        return;
    }
    print_error_msg("backend is not supported");
    exit(1);
}

void print_usage() {
    std::cout << "Usage: atlas [options] file...\n";
    std::cout << "Options:\n";
    std::cout << "    --include <dir>\n";
    std::cout << "    -I <dir>          Add directory to the Path to the Include search paths\n";
    std::cout << "    --debug           Used to show logs for Compiler development\n";
    std::cout << "    --run\n";
    std::cout << "    -r                Runs the program after compilation.\n";
    std::cout << "                      NOTE: Removes output file if not specified with -o\n";
    std::cout << "    --output\n";
    std::cout << "    -o <filename>     Place the output file in the specified name\n";
    exit(0);
}

State* set_options(int argc, char** argv) {
    State* state = new State;
    bool filepath_set = false;
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--debug") {
            state->debug = true;
        } else if (arg == "-o" || arg == "--output") {
            if (i == argc) {
                print_error_msg("No output file provided after -o flag");
                exit(1);
            }
            i++;
            state->output_file_path = std::string(argv[i]);
        } else if (arg == "-I" || arg == "--include") {
            if (i == argc) {
                print_error_msg("No path provided after -I flag");
                exit(1);
            }
            i++;
            state->include_path = std::string(argv[i]);
            if (state->include_path[state->include_path.size() - 1] != '/') {
                state->include_path += '/';
            }
        } else if (arg == "-r" || arg == "--run") {
            state->run = true;
        } else if (arg == "-E" || arg == "--emit-c") {
            state->emit_c = true;
        } else if (arg == "--help") {
            print_usage();
        } else if (argv[i][0] == '-') {
            std::string err = "Unknown option: " + arg;
            print_error_msg(err);
            exit(1);
        } else {
            state->input_filename = arg;
            filepath_set = true;
            char BUFF[255]; // Max number of chars for filename in Linux
            //std::string full_path = ;
            state->input_file_dir = getcwd(BUFF, sizeof(BUFF));
            state->input_file_dir += "/";
        }
    }
    if (!filepath_set) {
        std::cout << "atlas: " << CL_RED << "error:" << CL_RESET <<" no input files\n";
        exit(1);
    }
    return state;    
}

void run_program(std::string output_file_path) {
    //TODO: handle case where this fails because codegen didn't succeed
    std::string command;
    bool is_output_specified = true;
    if (output_file_path.size() == 0) {
        command = "./a.out";
        output_file_path = "a.out";
        is_output_specified = false;
    }
    command = "./" + output_file_path;
    int ret = std::system(command.c_str());
    if (!is_output_specified) {
        std::string remove = "rm " + output_file_path;
        std::system(remove.c_str());
    }
}

int main(int argc, char** argv) {
    if (argc == 1) {
        //TODO: print a usage
        std::cout << "atlas: " << CL_RED << "error:" << CL_RESET <<" no input files\n";
        return 1;
    }

    State* state = set_options(argc, argv);
    global_state = state;
    std::string BACKEND;
    if (state->debug) {
        std::cout << "[INFO]: Debug Mode is enabled\n";
        BACKEND = "gcc -fcompare-debug-second -w ";
        //BACKEND = "g++ -fcompare-debug-second -w ";
    } else {
        //BACKEND = "g++";
        BACKEND = "gcc";
    }
    //BACKEND = "clang";

    std::string src = read_file(state->input_filename);
    log_print(src + "\n");
    log_print("-----TOKENIZING START------\n");
    auto tokens = tokenize(src);
    print_tokens(tokens);
    log_print("------TOKENIZING END-------\n\n");
    log_print("--------AST START----------\n");
    auto ast = ast_create(tokens);
    log_print("---------AST END-----------\n\n");
    log_print("------CODEGEN START--------\n");
    codegen_start(ast, "out.c", BACKEND);
    log_print("-------CODEGEN END---------\n\n");
    if (state->run) {
        run_program(state->output_file_path);
    }
    return 0;
}
