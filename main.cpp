#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <cctype>
#include <cstring>
#include <unistd.h>
// debug
#include <memory>

#define DEPEND_LIBC

// Colors
const char* CL_BLACK   = "\e[0;30m";
const char* CL_RED     = "\e[0;31m";
const char* CL_GREEN   = "\e[0;32m";
const char* CL_YELLOW  = "\e[0;33m";
const char* CL_BLUE    = "\e[0;34m";
const char* CL_MAGENTA = "\e[0;35m";
const char* CL_CYAN    = "\e[0;36m";
const char* CL_WHITE   = "\e[0;37m";
const char* CL_RESET   = "\e[0m";

struct State {
    bool debug;
    bool run = false;
    bool emit_c = true;
    std::string output_file_path;
    std::string input_file_dir;
    std::string input_filename;
    std::string include_path;
};

struct Scope {
    struct Scope* prev;
    std::vector<struct VarNode*> names;
    std::vector<struct FunctionNode*> func_names;
};

enum TokenType {
    TK_NEWLINE = 100,
    TK_PLUS,
    TK_DASH,
    TK_STAR,
    TK_SLASH,
    TK_GT,
    TK_GTE,
    TK_LT,
    TK_LTE,
    TK_NOT,
    TK_IDENTIFIER,
    TK_CONSTANT,
    TK_COLON,
    TK_DOUBLE_C,
    TK_CURLY_OPEN,
    TK_CURLY_CLOSE,
    TK_PAREN_OPEN,
    TK_PAREN_CLOSE,
    TK_LOGICAL_AND,
    TK_LOGICAL_OR,
    TK_ASSIGN,
    TK_EQUAL,
    TK_NOT_EQUAL,
    TK_COMMA,
    TK_ARROW,
    TK_DOT,
    TK_QUOTE,
    TK_SQUARE_OPEN,
    TK_SQUARE_CLOSE,
    TK_INVALID,
    TK_RETURN,
    TK_IF,
    TK_ELSE,
    TK_FOR,
    TK_TYPE,
    TK_FN,
    TK_PERCENT,
    TK_CHAR,
    TK_INCLUDE,
    TK_NEW, // am I using these?
    TK_FREE,// am I using these?
    TK_PTR_DEREFERENCE,
    TK_DOT_CURLY
};

enum ForType {
    FOR_WHILE = 0,
    FOR_EACH = 1, // not implemented yet
    FOR_LOOP = 2,
};

struct Token {
    int line;
    int column;
    TokenType tt;
    std::string token;
};

typedef enum VarType {
    TYPE_I8 = 300,
    TYPE_I16,
    TYPE_I32,
    TYPE_I64,
    TYPE_U8,
    TYPE_U16,
    TYPE_U32,
    TYPE_U64,
    TYPE_F32,
    TYPE_F64,
    TYPE_UNKNOWN,
    TYPE_INVALID
} VarType;

enum NodeType {
    NODE_FUNC = 200,
    NODE_PARAM,
    NODE_BLOCK,
    NODE_ROOT,

    // STMTs
    NODE_VAR_DECL,
    NODE_CALL,
    NODE_ASSIGN,
    NODE_BINOP,
    NODE_UNARY,
    NODE_CONSTANT,
    NODE_VAR,
    NODE_RETURN,
    NODE_IF,
    NODE_FOR,
    NODE_TYPE,
    NODE_ARRAY_EXPR,
    NODE_CHAR,
    NODE_QUOTE,
    NODE_SUBSCRIPT,
    NODE_MEMBER_ACCESS,
    NODE_TYPE_INST,
};

struct Node {
    NodeType nt;
    Token* token;
};

struct VarNode : Node {
    VarType type;
    Token* type_;
    Token* identifier;

    bool is_array;
    int ptr_level = 0;
    struct ExpressionNode* arr_size;
    // Codegen
    //struct Node* scoped_var;
};

struct VarDeclNode : Node {
    VarType type;
    VarNode* lhs;
    struct ExpressionNode* rhs;
};

struct TypeNode : Node {
    Token* name;
    std::vector<VarDeclNode*> declarations;
};

struct BinaryOpNode : Node {
    struct ExpressionNode* lhs;
    struct ExpressionNode* rhs;
    Token* op;
};

struct CallNode : Node {
    Token* name;
    std::vector<struct ExpressionNode*> args;
};

struct SubscriptNode : Node {
    bool is_declaration = false;
    //ExpressionNode* arr_ref;
    Token* arr_ref;
    std::vector<struct ExpressionNode*> indexes;
};

struct TypeInstNode : Node {
    std::vector<struct ExpressionNode*> values;
};

struct MemberAccessNode : Node {
    Token* member_access;
};

struct UnaryOpNode : Node {
    //TODO: look to move certain things to here
    NodeType operator_type;
    union {
        SubscriptNode* subscript;
        CallNode* call_node;
    };
    struct ExpressionNode* operand;
};

struct ConstantNode : Node {
    int value;
};

struct QuoteNode : Node {
    Token* quote_token;
};

struct CharacterNode : Node {
    std::string value;
};

struct ArrayNode : Node {
    std::vector<ExpressionNode*> elements;
};

struct ExpressionNode : Node {
    union {
        BinaryOpNode* binop;
        ConstantNode* constant;
        CharacterNode* character;
        UnaryOpNode* unary_op;
        VarNode* var_node;
        CallNode* call_node;
        ArrayNode* array;
        QuoteNode* quote;
        SubscriptNode* subscript;
        TypeInstNode* type_inst;
    };
};

struct ForNode : Node {
    ForType for_type;
    struct StatementNode* init;
    ExpressionNode* test;
    struct StatementNode* update;
    struct BlockNode* block;
};

struct IfNode : Node {
    struct BlockNode* block;
    ExpressionNode* condition;
    struct ElseNode* _else; // can be NULL
};

struct ElseNode : Node {
    struct StatementNode* else_if; // can be NULL
    struct BlockNode* block;
};

struct AssignNode : Node {
    VarNode* lhs;
    ExpressionNode* rhs;
};

struct ParamNode : VarNode {
    // TODO: maybe get rid of this?
};

struct FunctionNode : Node {
    std::vector<ParamNode*> params;
    struct BlockNode* block;
    Token* return_types;
    bool is_prototype;
};

struct ReturnNode : Node {
    ExpressionNode* expr;
};

struct StatementNode : Node {
    // LHS
    union {
        VarDeclNode* vardecl_lhs;
        AssignNode* assign_lhs;
        FunctionNode* func_lhs;
        ExpressionNode* expr_lhs;
        ReturnNode* return_lhs;
        IfNode* if_lhs;
        ForNode* for_lhs;
        TypeNode* type_lhs;
    };
    // RHS
    union {
        ExpressionNode* expr_rhs;
    };
};

struct BlockNode : Node {
    Scope* scope;
    std::vector<StatementNode*> statements;
};

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

static State* global_state = NULL;

const char* get_tt_str(TokenType tt) {
    if (tt == TK_NEWLINE) {
        return "TK_NEWLINE";
    } else if (tt == TK_PLUS) {
        return "TK_PLUS";
    } else if (tt == TK_DASH) {
        return "TK_DASH";
    } else if (tt == TK_STAR) {
        return "TK_STAR";
    } else if (tt == TK_SLASH) {
        return "TK_SLASH";
    } else if (tt == TK_GT) {
        return "TK_GT";
    } else if (tt == TK_GTE) {
        return "TK_GTE";
    } else if (tt == TK_LT) {
        return "TK_LT";
    } else if (tt == TK_LTE) {
        return "TK_LTE";
    } else if (tt == TK_NOT) {
        return "TK_NOT";
    } else if (tt == TK_IDENTIFIER) {
        return "TK_IDENTIFIER";
    } else if (tt == TK_CONSTANT) {
        return "TK_CONSTANT";
    } else if (tt == TK_NOT) {
        return "TK_NOT";
    } else if (tt == TK_COLON) {
        return "TK_COLON";
    } else if (tt == TK_DOUBLE_C) {
        return "TK_DOUBLE_C";
    } else if (tt == TK_CURLY_OPEN) {
        return "TK_CURLY_OPEN";
    } else if (tt == TK_CURLY_CLOSE) {
        return "TK_CURLY_CLOSE";
    } else if (tt == TK_PAREN_OPEN) {
        return "TK_PAREN_OPEN";
    } else if (tt == TK_PAREN_CLOSE) {
        return "TK_PAREN_CLOSE";
    } else if (tt == TK_LOGICAL_AND) {
        return "TK_LOGICAL_AND";
    } else if (tt == TK_LOGICAL_OR) {
        return "TK_LOGICAL_OR";
    } else if (tt == TK_ASSIGN) {
        return "TK_ASSIGN";
    } else if (tt == TK_NOT_EQUAL) {
        return "TK_NOT_EQUAL";
    } else if (tt == TK_COMMA) {
        return "TK_COMMA";
    } else if (tt == TK_ARROW) {
        return "TK_ARROW";
    } else if (tt == TK_DOT) {
        return "TK_DOT";
    } else if (tt == TK_QUOTE) {
        return "TK_QUOTE";
    } else if (tt == TK_SQUARE_OPEN) {
        return "TK_SQUARE_OPEN";
    } else if (tt == TK_SQUARE_CLOSE) {
        return "TK_SQUARE_CLOSE";
    } else if (tt == TK_ASSIGN) {
        return "TK_ASSIGN";
    } else if (tt == TK_IF) {
        return "TK_IF";
    } else if (tt == TK_EQUAL) {
        return "TK_EQUAL";
    } else if (tt == TK_FOR) {
        return "TK_FOR";
    } else if (tt == TK_TYPE) {
        return "TK_TYPE";
    } else if (tt == TK_FN) {
        return "TK_FN";
    } else if (tt == TK_PERCENT) {
        return "TK_PERCENT";
    } else if (tt == TK_CHAR) {
        return "TK_CHAR";
    } else if (tt == TK_INCLUDE) {
        return "TK_INCLUDE";
    } else if (tt == TK_DOT_CURLY) {
        return "TK_DOT_CURLY";
    } else {
        return "TK_INVALID";
    }
}

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
    default:
        return "NODE_INVALID";
    }
}

void log_print(std::string message) {
    if(global_state->debug) {
        std::cout << "[INFO]: " + message;
    }
}

void print_token(Token* token) {
    if (global_state->debug) {
        std::string message = std::string("") + "TOKEN" + "<" + std::string(get_tt_str(token->tt)) + "> " 
            + "(" + std::to_string(token->line) + "," + std::to_string(token->column) + ")" 
            + ": " + token->token + "\n";
        log_print(message);
    }
}

void print_error_msg(std::string error) {
    std::cout << CL_RED << "[COMPILER ERROR]: " << CL_RESET << error << "\n";
}

void print_tokens (std::vector<Token*> tokens) {
    if (global_state->debug) {
        for (Token* token : tokens) {
            print_token(token);
        }
    }
}

Token* save_token(int line, 
                  int column, 
                  TokenType tt, 
                  std::string token_string) 
{
    return new Token{line, column, tt, token_string};
}

TokenType get_tt(char c) {
    if (c == '\n' || c == ';') {
        return TK_NEWLINE;
    } else if (c == '+') {
        return TK_PLUS;
    } else if (c == '-') {
        return TK_DASH;
    } else if (c == '*') {
        return TK_STAR;
    } else if (c == '/') {
        return TK_SLASH;
    } else if (c == '>') {
        return TK_GT;
    } else if (c == '<') {
        return TK_LT;
    } else if (c == '!') {
        return TK_NOT;
    } else if (c == ':') {
        return TK_COLON;
    } else if (c == '{') {
        return TK_CURLY_OPEN;
    } else if (c == '}') {
        return TK_CURLY_CLOSE;
    } else if (c == '(') {
        return TK_PAREN_OPEN;
    } else if (c == ')') {
        return TK_PAREN_CLOSE;
    } else if (c == '=') {
        return TK_ASSIGN;
    } else if (c == ',') {
        return TK_COMMA;
    } else if (c == '"') {
        return TK_QUOTE;
    } else if (c == '.') {
        return TK_DOT;
    } else if (c == '[') {
        return TK_SQUARE_OPEN;
    } else if (c == ']') {
        return TK_SQUARE_CLOSE;
    } else if (c == '=') {
        return TK_ASSIGN;
    } else if (c == '%') {
        return TK_PERCENT;
    } else if (c == '\'') {
        return TK_CHAR;
    }

    return TK_INVALID;
}

std::string read_file (std::string filename) {
    std::ifstream file;
    file.open(filename);
    if(file.fail()) {
        std::string err = "File \"" + std::string(filename) + "\" could not be opened";
        print_error_msg(err);
        exit(1);
    }
    std::string ret;
    while(file.good()) {
        ret += file.get();
    }
    return ret;
}

bool is_delim(char c) {
    if (c == ' ' || c == '+' || 
        c == '-' || c == '/' || 
        c == '*' || c == '%' || 
        c == '<' || c == '>' ||
        c == '=' || c == ':' ||
        c == '{' || c == '}' ||
        c == '(' || c == ')' ||
        c == '[' || c == ']' || 
        c == '"' || c == '\'' ||
        c == ':' || c == '\n' || 
        c == ';' || c == ',' ||
        c == '.')  
    {
        return true;
    }
    return false;
}

int is_number(std::string str) {
    for (char c : str) {
        if (!isdigit(c)) {
            return false;
        }
    }
    return true;
}

TokenType tokenize_get_reserved_word(std::string word) {
    // assumes not a number/constant
    if (word == "if") {
        return TK_IF;
    } else if (word == "else") {
        return TK_ELSE;  
    } else if (word == "for") {
        return TK_FOR;
    } else if (word == "type") {
        return TK_TYPE;    
    } else if (word == "include") {
        return TK_INCLUDE;
    } else if (word == "fn") {
        return TK_FN;
    } else {
        return TK_IDENTIFIER;
    }
}

std::vector<Token*> tokenize (std::string src) {
    std::vector<Token*> ret;
    int line = 1;
    int column = 0;
    std::string current_token_string;
    Token* current_token = new Token;
    bool prev_delim = true; // if the previous character was a delim
    for(int i = 0; i < src.length(); i++) {
        char c = src[i];
        char lookahead;
        if (i != src.length()) {
            lookahead = src[i + 1];
        } else {
            //lookahead = NULL;
        }
        column++;
        if (c == ' ' && lookahead == ' ') {
            continue;
        } else if (c == '/') {
                if (lookahead == '/') {
                    while (c != '\n') {
                        i++;
                        c = src[i];
                    }
                    column = 0;
                    line++;
                }
        }
        if (isalnum(c) || c == '_') {
            current_token_string += c;
            prev_delim = false;
        } else if(is_delim(c)) {
            // TODO: new token
            TokenType tt = TK_INVALID;
            if(!prev_delim) {
                if (is_number(current_token_string)) {
                    ret.push_back(save_token(line, column, TK_CONSTANT, current_token_string));
                } else {
                    tt = tokenize_get_reserved_word(current_token_string);
                    ret.push_back(save_token(line, column, tt, current_token_string));
                }
                current_token = new Token;
                current_token_string.clear();
                column++;
            }
            prev_delim = true;
            if (c == '\n') {
                // TODO: new token
                ret.push_back(save_token(line, column, get_tt(c), "NEWLINE"));
                column = 0;
                line++;
            } else if (c == ';') {
                // TODO: new token
                ret.push_back(save_token(line, column, get_tt(c), ";"));
            } else if (c == '+' || c == '(' || c == ')' || c == '*' ||
                       c == '{' || c == '}' || c == '=' || c == '[' ||
                       c == ']' || c == ',' || c == '%' || c == '/')
            {
                if (c == '=' && lookahead == '=') {
                    i += 1;
                    ret.push_back(save_token(line, column, TK_EQUAL, "=="));
                } else {
                    ret.push_back(save_token(line, column, get_tt(c), std::string(1, c)));
                }
            } else if (c == '\'') {
                //TODO: escape sequences
                std::string character = "";
                int start = column;
                i += 1;
                column += 1;
                c = src[i];
                while(c != '\'') {
                    character += c;
                    i += 1;
                    column += 1;
                    c = src[i];
                }
                ret.push_back(save_token(line, start, TK_CHAR, character));
            } else if (c == '"') {
                //TODO: escape sequences
                std::string str = "";
                int start = column;
                i += 1;
                column += 1;
                c = src[i];
                while(c != '"') {
                    str += c;
                    i += 1;
                    column += 1;
                    c = src[i];
                }
                ret.push_back(save_token(line, start, TK_QUOTE, str));

            } else if (c == ':') {
                if (lookahead == ':') {
                    i += 1;
                    ret.push_back(save_token(line, column, TK_DOUBLE_C, "::"));
                }
            } else if (c == '-') {
                if (lookahead == '>') {
                    i += 1;
                    ret.push_back(save_token(line, column, TK_ARROW, "->"));
                } else {
                    ret.push_back(save_token(line, column, TK_DASH, std::string(1,c)));
                }
            } else if (c == '&') {
                if (lookahead == '&') {
                    i += 1;
                    ret.push_back(save_token(line, column, TK_LOGICAL_AND, "&&"));
                } else {
                    ret.push_back(save_token(line, column, TK_INVALID, "&"));
                }
            } else if (c == '|') {
                if (lookahead == '|') {
                    i += 1;
                    ret.push_back(save_token(line, column, TK_LOGICAL_OR, "||"));
                } else {
                    ret.push_back(save_token(line, column, TK_INVALID, "|"));
                }
            } else if (c == '<') {
                if (lookahead == '=') {
                    i += 1;
                    ret.push_back(save_token(line, column, TK_LTE, "<="));
                } else {
                    ret.push_back(save_token(line, column, TK_LT, "<"));
                }
            } else if (c == '>') {
                if (lookahead == '=') {
                    i += 1;
                    ret.push_back(save_token(line, column, TK_GTE, ">="));
                } else {
                    ret.push_back(save_token(line, column, TK_LT, ">"));
                }
            } else if (c == '.') {
                if (lookahead == '{') {
                    i += 1;
                    ret.push_back(save_token(line, column, TK_DOT_CURLY, ".{"));
                } else {
                    ret.push_back(save_token(line, column, TK_DOT, "."));
                }
            }
        }  
    }

    return ret;
}

void expect(Token* token, TokenType expected) {
    if (token->tt != expected) {
        //std::cout << expected;
        std::string err = " Expected: \"" + std::string(get_tt_str(expected))
                          + "\"\n         Got: " + token->token
                          + " (" + std::to_string(token->line) + ", " + std::to_string(token->column) + ")";
        print_error_msg(err);
        exit(1);
    }
}

void print_tabs(int tab_level) {
    for (int i; i < tab_level; i++) {
        std::cout << "\t";
    }
}

void print_node(Node* node, int tab_level) {
    printf("[DEBUG]: ");
    print_tabs(tab_level);
    printf(" %s├─%s", CL_YELLOW, CL_RESET);
    printf(" ");
}

void print_nodes(std::vector<Node*> nodes) {
    for(auto node : nodes) {
        print_node(node, 0);
    }
}

ParamNode* ast_create_param(Token* type,
                            Token* name,
                            bool is_array,
                            int ptr_level,
                            ExpressionNode* arr_size)
{
    ParamNode* ret = new ParamNode;
    ret->nt = NODE_PARAM;
    ret->token = name;
    ret->identifier = name;
    ret->type = get_var_type(type);
    ret->type_ = type;
    ret->is_array = is_array;
    ret->ptr_level = ptr_level;
    ret->arr_size = arr_size;
    return ret;
}

Token* next_token(std::vector<Token*> tokens, int* i) {
    (*i)++;
    return tokens[*i];
}

std::vector<ParamNode*> ast_parse_params(std::vector<Token*> tokens, int* i) {
    std::vector<ParamNode*> params;
    Token* current_token = tokens[*i];
    while (current_token->tt != TK_PAREN_CLOSE) {
        expect(current_token, TK_IDENTIFIER);
        Token* type = current_token;
        current_token = next_token(tokens, i);
        bool is_array = false;
        int ptr_level = 0;
        ExpressionNode* arr_size = NULL; // incase the var is an array
        if (current_token->tt == TK_SQUARE_OPEN) {
            // array type   
            is_array = true;
            ptr_level++;
            current_token = next_token(tokens, i); // expr
            arr_size = ast_create_expression(tokens, false, false, true, i);
            current_token = tokens[*i]; // update current_token
            expect(current_token, TK_SQUARE_CLOSE);
            current_token = next_token(tokens, i); // expr
        } else if (current_token->tt == TK_STAR) {
            // TODO: handle deeper layers of pointers
            while(current_token->tt == TK_STAR) {
                ptr_level++;
                current_token = next_token(tokens, i);
            }
        }
        expect(current_token, TK_IDENTIFIER);
        Token* name = current_token;
        current_token = next_token(tokens, i);
        ParamNode* param = ast_create_param(type, name, is_array, ptr_level,
                                            arr_size);
        params.push_back(param);
        if (current_token->tt == TK_COMMA) {
            current_token = next_token(tokens, i); // skip the comma
        } else if (current_token->tt == TK_PAREN_CLOSE) {
            break;
        }
        if (*i == tokens.size()) {
            break;
        }
    }
    return params;
}

ExpressionNode* ast_create_binop(ExpressionNode* lhs, ExpressionNode* rhs, Token* op) {
    ExpressionNode* ret = new ExpressionNode;
    BinaryOpNode* bin_op = new BinaryOpNode;
    bin_op->lhs = lhs;
    bin_op->rhs = rhs;
    bin_op->op = op;
    ret->binop = bin_op;
    ret->nt = NODE_BINOP;
    bin_op->nt = NODE_BINOP;
    return ret;
}

ExpressionNode* ast_create_call(Token* name, std::vector<Token*> tokens, int* i) {
    log_print("Creating CallNode\n");
    ExpressionNode* ret = new ExpressionNode;
    CallNode* call = new CallNode;
    std::vector<ExpressionNode*> args;
    (*i)++; // the index should be at the open_paren
    (*i)++; // the index should be at the first part of the expr
    for (; *i < tokens.size(); (*i)++) {
        Token* current_token = tokens[*i];
        if (current_token->tt == TK_PAREN_CLOSE || current_token->tt == TK_NEWLINE) {
            // if (current_token->tt == TK_PAREN_CLOSE) {
            //     (*i)++; // get rid of bracket
            // }
            break;
        }
        args.push_back(ast_create_expression(tokens, true, false, false, i));
    }
    call->args = args;
    ret->call_node = call;
    call->name = name;
    ret->nt = NODE_CALL;
    return ret;
}

StatementNode* ast_create_return(std::vector<Token*> tokens, int* i) {
    StatementNode* ret = new StatementNode;
    ReturnNode* ret_node = new ReturnNode;
    (*i)++;
    ret_node->expr = ast_create_expression(tokens, false, false, false, i);
    ret->nt = NODE_RETURN;

    ret->return_lhs = ret_node;
    ret_node->nt = ret_node->expr->nt;

    return ret;
}

VarNode* ast_create_var(Token* identifier) {
    log_print("Creating VariableNode \"" + identifier->token + "\"\n");
    VarNode* ret = new VarNode;
    ret->type_ = NULL;
    ret->nt = NODE_VAR;
    ret->identifier = identifier;
    ret->is_array = false; // needs to be changed externally
    return ret;
}

VarDeclNode* ast_create_var_decl(VarType type, Token* type_id, Token* identifier, ExpressionNode* rhs) {
    log_print("Creating Variable Declaration\n");
    VarDeclNode* ret = new VarDeclNode;
    VarNode* _node = new VarNode;
    ret->nt = NODE_VAR_DECL;
    _node->type_ = type_id;
    _node->nt = NODE_VAR_DECL;
    _node->type = type;
    _node->identifier = identifier;
    ret->lhs = _node;
    ret->rhs = rhs;
    return ret;
}

ExpressionNode* ast_create_variable_expr(VarType type, Token* identifier, int* i) {
    log_print("Creating VariableNode (e)\"" + identifier->token + "\"\n");
    ExpressionNode* ret = new ExpressionNode;
    ret->nt = NODE_VAR;
    VarNode* _node = ast_create_var(identifier);
    _node->type = type;
    ret->var_node = _node;
    return ret;
}

ExpressionNode* ast_create_constant(Token* constant_value) {
    log_print("Creating ConstantNode\n");
    ExpressionNode* ret = new ExpressionNode;
    ConstantNode* constant = new ConstantNode;
    constant->value = stoi(constant_value->token);
    ret->nt = NODE_CONSTANT;

    ret->constant = constant;
    return ret;
}

ExpressionNode* ast_create_quote(Token* quote_token) {
    log_print("Creating QuoteNode\n");
    ExpressionNode* ret = new ExpressionNode;
    QuoteNode* quote = new QuoteNode;
    quote->quote_token = quote_token;
    ret->nt = NODE_QUOTE;
    ret->quote = quote;

    return ret;
}

ExpressionNode* ast_create_char(Token* character) {
    log_print("Creating CharacterNode\n");
    ExpressionNode* ret = new ExpressionNode;
    CharacterNode* character_node = new CharacterNode;
    if (character->token.size() > 1) {
        print_token(character);
        print_error_msg("Single quotes used for more than one character");
        //exit(1);
    }
    character_node->value = character->token;

    ret->character = character_node;
    ret->nt = NODE_CHAR;
    return ret;
}

int get_prec(TokenType tt) {
    switch(tt) {
    case TK_PAREN_OPEN:
        return 7;
    case TK_SQUARE_OPEN:
    case TK_DOT:
        return 6;
    case TK_STAR:
    case TK_SLASH:
    case TK_PERCENT:
        return 5;
    case TK_PLUS:
    case TK_DASH:
        return 4;
    case TK_GT:
    case TK_LT:
    case TK_GTE:
    case TK_LTE:
        return 3;
    case TK_EQUAL:
        return 2;
    case TK_ASSIGN:
        return 1;
    case TK_NEWLINE:
    case TK_CURLY_OPEN:
    case TK_PAREN_CLOSE:
    case TK_SQUARE_CLOSE:
    case TK_COMMA:
        return -1;
    default:
        print_error_msg("Invalid Operator");
        std::string err = "Operator: " + std::string(get_tt_str(tt)) + "\n";
        print_error_msg(err);
        exit(1);
    }
}

Token* ast_get_lookahead(std::vector<Token*> tokens, int* i) {
    if (*i + 1 < tokens.size()) {
        return tokens[*i + 1];
    } else {
        return NULL;
    }
}

ExpressionNode* ast_create_array_decl(std::vector<Token*> tokens, int* i) {
    log_print("Creating ArrayNode\n");
    ExpressionNode* ret = new ExpressionNode;
    ArrayNode* arr = new ArrayNode;
    Token* current_token = next_token(tokens, i); // get the next token

    if (current_token->tt != TK_SQUARE_CLOSE) {
        for (; *i < tokens.size(); (*i)++) {
            arr->elements.push_back(ast_create_expression(tokens,
                                                          false,
                                                          false,
                                                          true,
                                                          i));
            current_token = tokens[*i];
            Token* lookahead = ast_get_lookahead(tokens, i);
            if (current_token->tt == TK_SQUARE_CLOSE
                || lookahead->tt == TK_SQUARE_CLOSE)
            {
                break;
            }
        }
    }
    current_token = tokens[*i];
    //expect(current_token, TK_SQUARE_CLOSE);
    ret->array = arr;
    ret->nt = NODE_ARRAY_EXPR;
    Token* lookahead = ast_get_lookahead(tokens, i);
    expect(lookahead, TK_NEWLINE);

    return ret;
}

ExpressionNode* ast_create_subscript_node(std::vector<Token*> tokens, int* i) {
    log_print("Creating Subscript Node\n");
    ExpressionNode* ret = new ExpressionNode;
    SubscriptNode* subscript = new SubscriptNode;
    Token* current_token = next_token(tokens, i);
    if (current_token->tt != TK_SQUARE_CLOSE) {
        for (; *i < tokens.size(); (*i)++) {
            subscript->indexes.push_back(ast_create_expression(tokens,
                                                          false,
                                                          false,
                                                          true,
                                                          i));
            current_token = tokens[*i];
            Token* lookahead = ast_get_lookahead(tokens, i);
            if (current_token->tt == TK_SQUARE_CLOSE)
                //|| lookahead->tt == TK_SQUARE_CLOSE)
            {
                break;
            }
        }
    }
    ret->nt = NODE_SUBSCRIPT;
    subscript->nt = NODE_SUBSCRIPT;
    ret->subscript = subscript;
    return ret;
}

ExpressionNode* ast_create_type_instantiation(std::vector<Token*> tokens, int* i) {
    log_print("Creating Type instantiation Node\n");
    ExpressionNode* ret = new ExpressionNode;
    TypeInstNode* type_inst = new TypeInstNode;
    Token* current_token = next_token(tokens, i);
    if (current_token->tt != TK_CURLY_CLOSE) {
        for (; *i < tokens.size(); (*i)++) {
            type_inst->values.push_back(ast_create_expression(tokens,
                                                              false,
                                                              false,
                                                              true, // not sure if it should be true or false
                                                            i));
            current_token = tokens[*i];
            Token* lookahead = ast_get_lookahead(tokens, i);
            if (lookahead->tt == TK_CURLY_CLOSE) {
                (*i)++;
                break;
            }
        }
    }
    log_print("type_instantiation end\n");
    ret->nt = NODE_TYPE_INST;
    type_inst->nt = NODE_TYPE_INST;
    ret->type_inst = type_inst;
    return ret;
}
bool is_op_binary(Token* op) {
    switch(op->tt) {
    case TK_EQUAL:
    case TK_PLUS:
    case TK_DASH:
    case TK_STAR:
    case TK_SLASH:
    case TK_GT:
    case TK_GTE:
    case TK_LT:
    case TK_LTE:
    case TK_DOT:
    case TK_PERCENT:
    case TK_ASSIGN:
    case TK_SQUARE_OPEN:
        return true;
    case TK_NOT:
        return false;
    default:
        std::string err = "\"" + op->token + "\" is not a valid operator";
        print_error_msg(err);
        exit(1);
    }
}

ExpressionNode* 
ast_create_expr_prec(
        std::vector<Token*> tokens,
        int precedence,
        bool is_args,
        bool is_cond,
        bool is_arr,
        int* i) 
{
    ExpressionNode* lhs;
    Token* current_token = tokens[*i];
    Token* lookahead = ast_get_lookahead(tokens, i);
    log_print("lhs:");
    print_token(current_token);
    if (current_token->tt == TK_IDENTIFIER && lookahead->tt == TK_PAREN_OPEN) {
        lhs = ast_create_call(current_token, tokens, i);
    } else if (current_token->tt == TK_IDENTIFIER) {
        lhs = ast_create_variable_expr(TYPE_UNKNOWN, tokens[*i], i);
    } else if (current_token->tt == TK_CONSTANT) {
        lhs = ast_create_constant(tokens[*i]);
    } else if (current_token->tt == TK_QUOTE) {
        lhs = ast_create_quote(tokens[*i]);
    } else if (current_token->tt == TK_SQUARE_OPEN) {
        //lhs = ast_create_array_decl(tokens, i); // only for array_decl
        lhs = ast_create_subscript_node(tokens, i); // only for array_decl
    } else if (current_token->tt == TK_DOT_CURLY) {
        lhs = ast_create_type_instantiation(tokens, i);
    } else if (current_token->tt == TK_CHAR) {
        lhs = ast_create_char(tokens[*i]);
    } else {
        print_error_msg("Invalid token for lhs in ast_get_expr_prec\n");
        print_token(current_token);
        exit(1);
    }
    //operator is the lookahead
    ExpressionNode* rhs = NULL;
    ExpressionNode* unary_op_expr = NULL;

    // Make sure it is updated
    current_token = tokens[*i];  
    lookahead = ast_get_lookahead(tokens, i);
    if (is_args && current_token->tt == TK_PAREN_CLOSE) {
        (*i)++; // Throw away closed bracket
        return lhs;
    } else if (is_arr && (current_token->tt == TK_SQUARE_CLOSE)) {
        // only for var_decl
        //TODO: remove this? -- seems like this never happens
        print_error_msg("SUPPOSED TO BE UNREACHABLE?");
        exit(1);
        //return lhs;
    } else if (is_arr && (lookahead->tt == TK_SQUARE_CLOSE)) {
        // only for var_decl
        (*i)++; // Throw away closed square bracket
        return lhs;
    } else if (is_arr && (current_token->tt == TK_COMMA) || (lookahead->tt == TK_COMMA)) {
        (*i)++; // Now token should be on ']' or ','
        return lhs;
    }

    current_token = tokens[*i];  
    lookahead = ast_get_lookahead(tokens, i);

    if (current_token->tt != TK_NEWLINE && lookahead->tt != TK_CURLY_OPEN) {
        // FIX to make the second half of OR check if lookahead operator has different associativity
        if (lookahead->tt == TK_CURLY_CLOSE) {
            return lhs;
        }
        while(get_prec(lookahead->tt) >= precedence || lookahead->tt == TK_ASSIGN) {
            current_token = next_token(tokens, i);
            Token* op = current_token; // operator is a lookahead
            if (!is_op_binary(op)) {
                exit(50);
                UnaryOpNode* unary_op_node = new UnaryOpNode;
                unary_op_node->nt = NODE_UNARY;
                unary_op_node->operand = lhs;
                lhs = new ExpressionNode;
                lhs->nt = NODE_UNARY;
                // unary op
                switch(op->tt) {
                case TK_PTR_DEREFERENCE:
                    break;
                case TK_NOT:
                    // NOTE: NOT AND * ARE LEFT ASSOCIATIVE
                    std::string err = "\"" + op->token + "\" is not implemented yet";
                    print_error_msg(err);
                    exit(1);
                    //TODO: fix this switch
                //default:
                    //std::string err = "\"" + op->token + "\" is not a valid operator";
                    print_error_msg(err);
                    exit(1);
                }
            } else {
                current_token = next_token(tokens, i); // rhs
                log_print("rhs:");
                print_token(current_token);
                if (op->tt == TK_ASSIGN) {
                    // right to left
                    rhs = ast_create_expression(tokens, false, false, false, i);
                } else {
                    // left to right
                    lookahead = ast_get_lookahead(tokens, i);
                    if (current_token->tt == TK_IDENTIFIER
                        && lookahead->tt == TK_PAREN_OPEN) {
                        // Function Call
                        rhs = ast_create_call(current_token, tokens, i);
                    } else if (current_token->tt == TK_IDENTIFIER) {
                        // Variable
                        rhs = ast_create_variable_expr(TYPE_UNKNOWN, tokens[*i], i);
                    } else if (current_token->tt == TK_CONSTANT) {
                        // Constant TODO: handle quotes 
                        rhs = ast_create_constant(tokens[*i]);
                    } else if (current_token->tt == TK_QUOTE) {
                        rhs = ast_create_quote(tokens[*i]);
                    } else {
                        return lhs;
                    }
                }
                current_token = tokens[*i];
                lookahead = ast_get_lookahead(tokens, i);
                //if (op->tt == TK_SQUARE_OPEN) {
                if (lookahead->tt == TK_SQUARE_CLOSE && !is_arr) {
                    (*i)++;
                    current_token = tokens[*i];
                    expect(current_token, TK_SQUARE_CLOSE);
                    lookahead = ast_get_lookahead(tokens, i);
                    if (lookahead->tt == TK_NEWLINE) {
                        //exit(69);
                        lhs = ast_create_binop(lhs, rhs, op);
                        break;
                    }
                } 
                // TODO: handle left associativity
                // TODO: MIGHT BE FAILING HERE FOR ARRAY EXPR 
                while (get_prec(lookahead->tt) >= get_prec(op->tt)) {
                    if (get_prec(lookahead->tt) > get_prec(op->tt))
                        rhs = ast_create_expr_prec(tokens, get_prec(op->tt) + 1, is_args, is_cond, is_arr, i);
                    else
                        rhs = ast_create_expr_prec(tokens, get_prec(op->tt), is_args, is_cond, is_arr, i);
                    lookahead = ast_get_lookahead(tokens, i);
                    if (lookahead->tt != TK_NEWLINE)
                        current_token = next_token(tokens, i);
                    if (current_token->tt == TK_NEWLINE || current_token->tt == TK_COMMA) {
                        break;
                    }
                }
                lhs = ast_create_binop(lhs, rhs, op);
                lookahead = ast_get_lookahead(tokens, i);
                if (lookahead->tt == TK_NEWLINE) {
                    break;
                }
            }
            lookahead = ast_get_lookahead(tokens, i);
            if (current_token->tt == TK_NEWLINE ||
                lookahead->tt == TK_NEWLINE ||
                current_token->tt == TK_COMMA) {
                break;
            }
        }
    }
    //print_node(lhs, 0);
    return lhs;
}

ExpressionNode* ast_create_expression(std::vector<Token*> tokens, bool is_args, bool is_cond, bool is_arr, int* i) {
    log_print("Creating ExpressionNode\n");
    return ast_create_expr_prec(tokens, 0, is_args, is_cond, is_arr, i);
}

VarType get_var_type(Token* var_type) {
    int n = var_type->token.length();
    if (strncmp(var_type->token.c_str(), "i8", n) == 0 && n == 2) {
        return TYPE_I8;
    } else if (strncmp(var_type->token.c_str(), "i16", n) == 0 && n == 3) {
        return TYPE_I16;
    } else if (strncmp(var_type->token.c_str(), "i32", n) == 0 && n == 3) {
        return TYPE_I32;
    } else if (strncmp(var_type->token.c_str(), "i64", n) == 0 && n == 3) {
        return TYPE_I64;
    } else if (strncmp(var_type->token.c_str(), "u8", n) == 0 && n == 2) {
        return TYPE_U8;
    } else if (strncmp(var_type->token.c_str(), "u16", n) == 0 && n == 3) {
        return TYPE_U16;
    } else if (strncmp(var_type->token.c_str(), "u32", n) == 0 && n == 3) {
        return TYPE_U32;
    } else if (strncmp(var_type->token.c_str(), "u64", n) == 0 && n == 3) {
        return TYPE_U64;
    } else if (strncmp(var_type->token.c_str(), "f32", n) == 0 && n == 3) {
        return TYPE_F32;
    } else if (strncmp(var_type->token.c_str(), "f64", n) == 0 && n == 3) {
        return TYPE_F64;
    } else if (strncmp(var_type->token.c_str(), "int", n) == 0 && n == 3) {
        return TYPE_I64;
    } else if (strncmp(var_type->token.c_str(), "float", n) == 0 && n == 5) {
        return TYPE_F64;
    }

    return TYPE_INVALID;
}

int ast_create_for_determine_for(std::vector<Token*> tokens, int* i) {
    int newline_count = 0;
    for (int j = *i; j < tokens.size(); j++) {
        Token* current_token = tokens[j];
        if (current_token->tt == TK_CURLY_OPEN) {
            break;
        } else if (current_token->tt == TK_NEWLINE) {
            newline_count++;
        }
    }
    return newline_count;
}

StatementNode* ast_create_for(std::vector<Token*> tokens, int* i) {
    log_print("Creating ForNode\n");
    StatementNode* ret = new StatementNode;
    ForNode* for_node = new ForNode;

    Token* current_token = next_token(tokens, i); // skip for
    int for_type = ast_create_for_determine_for(tokens, i);
    if (for_type == FOR_LOOP) {
        // init statement
        // for now assuming its a var_decl but need to fix this later
        StatementNode* init_statement = ast_create_declaration(tokens, i);
        init_statement->vardecl_lhs->lhs->is_array = false;
        //init_statement->vardecl_lhs->lhs->is_array = false;
        for_node->init = init_statement;

        //TODO: skip test expression case
        // test expression
        current_token = next_token(tokens, i);
        expect(current_token, TK_NEWLINE);
        current_token = next_token(tokens, i); // skip NEWLINE
        ExpressionNode* test_expr = ast_create_expression(tokens, false, false, false, i);
        if (tokens[*i]->tt == TK_NEWLINE) {
            current_token = tokens[*i];
        } else {
            current_token = next_token(tokens, i);
        }
        expect(current_token, TK_NEWLINE);
        for_node->test = test_expr;

        // update statement
        current_token = next_token(tokens, i); // skip NEWLINE
        StatementNode* update_statement = new StatementNode;
        ExpressionNode* expr = ast_create_expression(tokens, false, false, false, i);
        update_statement->nt = expr->nt;
        update_statement->expr_lhs = expr;
        for_node->update = update_statement;
        for_node->for_type = FOR_LOOP;
    } else if (for_type == FOR_WHILE) {
        //current_token = next_token(tokens, i); // skip NEWLINE
        ExpressionNode* test_expr = ast_create_expression(tokens, false, false, false, i);
        for_node->test = test_expr;
        for_node->for_type = FOR_WHILE;
    } else {
        print_error_msg("For Loop not recognized... Perhaps it is not supported yet");
        exit(1);
    }

    // block
    current_token = next_token(tokens, i);
    expect(current_token, TK_CURLY_OPEN);
    BlockNode* for_block = ast_create_block(tokens, i);
    for_node->block = for_block;

    ret->for_lhs = for_node;
    return ret;
}

StatementNode* ast_create_if(std::vector<Token*> tokens, int* i) {
    log_print("Creating IfNode\n");
    StatementNode* ret = new StatementNode;
    IfNode* if_node = new IfNode;
    if_node->_else = NULL;
    Token* current_token = next_token(tokens, i); // skip if token
    ExpressionNode* cond = ast_create_expression(tokens, false, true, false, i);
    Token* lookahead = ast_get_lookahead(tokens, i);
    current_token = tokens[*i];
    if (lookahead->tt == TK_CURLY_OPEN) {
        (*i)++;
    }
    BlockNode* block = ast_create_block(tokens, i);
    lookahead = ast_get_lookahead(tokens, i);
    if (lookahead != NULL && lookahead->tt == TK_ELSE) {
        ElseNode* _else = new ElseNode;
        current_token = next_token(tokens, i); // else - skip
        current_token = next_token(tokens, i); // if OR {
        StatementNode* else_if = NULL;
        BlockNode* block_else = NULL;
        if (current_token->tt == TK_IF) {
            else_if = ast_create_if(tokens, i);
        } else if (current_token->tt == TK_CURLY_OPEN) {
            block_else = ast_create_block(tokens, i);
        }
        _else->else_if = else_if;
        _else->block = block_else;
        if_node->_else = _else;
    }
    if_node->nt = NODE_IF;
    if_node->condition = cond;
    if_node->block = block;
    ret->if_lhs = if_node;
    return ret;
}

BlockNode* ast_create_block(std::vector<Token*> tokens, int* i) {
    log_print("Creating BlockNode\n");
    std::vector<StatementNode*> statements;
    BlockNode* block = new BlockNode;
    block->nt = NODE_BLOCK;
    Token* current_token = tokens[*i];
    expect(current_token, TK_CURLY_OPEN);
    current_token = next_token(tokens, i);
    for (; *i < tokens.size(); (*i)++) {
        current_token = tokens[*i];
        TokenType tt = current_token->tt;
        if (tt == TK_CURLY_CLOSE) {
            break;
        }
        if (tt == TK_NEWLINE) {
            continue;
        } else if (tt == TK_INCLUDE) {
            current_token = next_token(tokens, i); // skip include
            std::string filename = current_token->token;
            if (global_state->include_path.size() != 0) {
                filename = global_state->include_path + filename;
            } else {
                std::string dir = ast_get_file_full_path(global_state->input_filename);
                filename = global_state->input_file_dir + dir + filename;
            }
            std::string src = read_file(filename.c_str());
            auto tokens = tokenize(src);
            auto ast = ast_create(tokens);
            statements.insert(statements.end(), ast.begin(), ast.end());
        } else {
            statements.push_back(ast_create_declaration(tokens, i));
        }
        if (tt == TK_CURLY_CLOSE) {
            break;
        }
    }
    block->statements = statements;
    return block;
}

std::string ast_get_file_full_path(std::string filename) {
    int last_slash_idx = filename.rfind('/');
    std::string directory;
    if (std::string::npos != last_slash_idx)
    {
        directory = filename.substr(0, last_slash_idx);
    }
    return directory + '/';
}

TypeNode* ast_create_type_struct(std::vector<Token*> tokens, int* i) {
    TypeNode* type_node = new TypeNode;
    Token* current_token = tokens[*i];
    Token* type_name = current_token;
    current_token = next_token(tokens, i);
    expect(current_token, TK_CURLY_OPEN);
    current_token = next_token(tokens, i);
    expect(current_token, TK_NEWLINE);
    for (; *i < tokens.size();) {
        current_token = next_token(tokens, i);
        print_token(current_token);
        bool is_array = false;
        int ptr_level = 0;
        ExpressionNode* arr_size = NULL; // incase the var is an array
        if (current_token->tt == TK_CURLY_CLOSE) {
            current_token = next_token(tokens, i); // skip curly close
            break;
        }
        expect(current_token, TK_IDENTIFIER); // type
        Token* type = current_token;
        current_token = next_token(tokens, i);
        if (current_token->tt == TK_SQUARE_OPEN) {
            // array type   
            is_array = true;
            ptr_level++;
            current_token = next_token(tokens, i); // expr
            arr_size = ast_create_expression(tokens, false, false, true, i);
            current_token = tokens[*i]; // update current_token
            expect(current_token, TK_SQUARE_CLOSE);
            current_token = next_token(tokens, i); // expr
        } else if (current_token->tt == TK_STAR) {
            // TODO: handle deeper layers of pointers
            while(current_token->tt == TK_STAR) {
                ptr_level++;
                current_token = next_token(tokens, i);
            }
        }
        expect(current_token, TK_DOUBLE_C);
        current_token = next_token(tokens, i); // name
        Token* name = current_token;
        VarDeclNode* var = ast_create_var_decl(get_var_type(type), type, name, NULL);
        var->lhs->ptr_level = ptr_level;
        var->lhs->is_array = is_array;
        var->lhs->arr_size = arr_size;
        type_node->name = type_name;
        type_node->declarations.push_back(var);
        current_token = next_token(tokens, i); // newline
        expect(current_token, TK_NEWLINE);
    }
    return type_node;
}

FunctionNode* ast_create_function(std::vector<Token*> tokens, int* i) {
    FunctionNode* ret = new FunctionNode;
    ret->nt = NODE_FUNC;
    // Function Name
    Token* current_token = tokens[*i];
    Token* name = current_token;
    current_token = next_token(tokens, i);
    BlockNode* block = NULL;
    if (current_token->tt == TK_PAREN_OPEN) {
        current_token = next_token(tokens, i);
        auto params = ast_parse_params(tokens, i);
        current_token = tokens[*i]; // update current_token

        ret->token = name;
        ret->params = params;
        expect(current_token, TK_PAREN_CLOSE);
        current_token = next_token(tokens, i);
        if (current_token->tt == TK_ARROW) {
            // parse return types
            // TODO: support multiple types
            current_token = next_token(tokens, i);
            auto return_type = current_token;
            ret->return_types = return_type;
            current_token = next_token(tokens, i);
        } else {
            ret->return_types = NULL;
        }
        if (current_token->tt == TK_NEWLINE) {
            ret->is_prototype = true;
            // prototype
            block = NULL;
        } else if (current_token->tt == TK_CURLY_OPEN) {
            ret->is_prototype = false;
            block = ast_create_block(tokens, i);
        }
    }
    ret->block = block;
    return ret;
}

bool is_var_decl(std::vector<Token*> tokens, int* i) {

    for(int j = *i; j < tokens.size(); j++) {
        if (tokens[j]->tt == TK_DOUBLE_C) {
            return true;
        }
        if (tokens[j]->tt == TK_NEWLINE) {
            return false;
        }
    }
}

VarDeclNode* ast_handle_var_decl_lhs(std::vector<Token*> tokens, int* i) {
    //int save = *i; // save index for beginning of the line
    Token* current_token = tokens[*i];
    Token* type = current_token;
    current_token = next_token(tokens, i);
    bool is_array = false;
    int ptr_level = 0;
    ExpressionNode* arr_size = NULL; // incase the var is an array
    if (current_token->tt == TK_SQUARE_OPEN) {
        // array type   
        is_array = true;
        ptr_level++;
        current_token = next_token(tokens, i); // expr
        arr_size = ast_create_expression(tokens, false, false, true, i);
        current_token = tokens[*i]; // update current_token
        expect(current_token, TK_SQUARE_CLOSE);
        current_token = next_token(tokens, i); // expr
    } else if (current_token->tt == TK_STAR) {
        // TODO: handle deeper layers of pointers
        while(current_token->tt == TK_STAR) {
            ptr_level++;
            current_token = next_token(tokens, i);
        }
    }
    //Token* type = tmp;
    expect(current_token, TK_DOUBLE_C);
    current_token = next_token(tokens, i);
    expect(current_token, TK_IDENTIFIER);
    Token* id = current_token;
    current_token = next_token(tokens, i);
    VarDeclNode* lhs = ast_create_var_decl(get_var_type(type), type, id, NULL);
    lhs->lhs->ptr_level = ptr_level;
    lhs->lhs->is_array = is_array;
    lhs->lhs->arr_size = arr_size;
    return lhs;
}

StatementNode* ast_create_declaration(std::vector<Token*> tokens, int* i) {
    StatementNode* stmt = new StatementNode;
    // assume starts at the beginning of the line
    for (; *i < tokens.size(); (*i)++) {
        Token* current_token = tokens[*i];
        TokenType tt = tokens[*i]->tt;
        int save_beg = *i; // incase its an expression
        if(tt == TK_IDENTIFIER) {
            //current_token = next_token(tokens, i);
            if(is_var_decl(tokens, i)) {
                // var_decl
                //expect(current_token, TK_IDENTIFIER);
                VarDeclNode* lhs = ast_handle_var_decl_lhs(tokens, i);
                current_token = tokens[*i]; // update token
                expect(current_token, TK_ASSIGN);
                current_token = next_token(tokens, i);
                ExpressionNode* rhs = ast_create_expression(tokens, false, false, false, i);
                lhs->rhs = rhs;
                if (lhs->lhs->is_array && rhs->nt == NODE_SUBSCRIPT) {
                    rhs->subscript->is_declaration = true;
                }
                //VarDeclNode* lhs = ast_create_var_decl(get_var_type(type), type, id, rhs);
                StatementNode* statement = new StatementNode;
                statement->nt = NODE_VAR_DECL;
                statement->vardecl_lhs = lhs;
                statement->expr_rhs = rhs;
                return statement;
            } else {
                // Expression
                StatementNode* statement = new StatementNode;
                ExpressionNode* stmt_expr = ast_create_expression(tokens, false, false, false, i);
                statement->nt = stmt_expr->nt;
                statement->expr_lhs = stmt_expr;
                return statement;
            }
        } else if (current_token->tt == TK_FN) {
            // Function
            current_token = next_token(tokens, i); // skip fn
            FunctionNode* fn = ast_create_function(tokens, i);
            StatementNode* stmt = new StatementNode;
            stmt->nt = NODE_FUNC;
            stmt->func_lhs = fn;
            return stmt;
        } else if (current_token->tt == TK_TYPE) {
            // Type struct
            current_token = next_token(tokens, i); // skip type
            TypeNode* type_struct = ast_create_type_struct(tokens, i);
            StatementNode* stmt = new StatementNode;
            stmt->nt = NODE_TYPE;
            stmt->type_lhs = type_struct;
            return stmt;
        } else if (current_token->tt == TK_ARROW) {
            StatementNode* stmt = ast_create_return(tokens, i);
            stmt->nt = NODE_RETURN;
            return stmt;
        } else if (current_token->tt == TK_IF) {
            StatementNode* stmt = ast_create_if(tokens, i);
            stmt->nt = NODE_IF;
            // expect(tokens[*i], TK_NEWLINE);
            // current_token = next_token(tokens, i);
            // expect(tokens[*i], TK_CURLY_CLOSE);
            // current_token = next_token(tokens, i);
            return stmt;
        } else if (current_token->tt == TK_FOR) {
            StatementNode* stmt = ast_create_for(tokens, i);
            stmt->nt = NODE_FOR;
            return stmt;
        } else {
            // Expression
            StatementNode* statement = new StatementNode;
            ExpressionNode* stmt_expr = ast_create_expression(tokens, false, false, false, i);
            statement->nt = stmt_expr->nt;
            statement->expr_lhs = stmt_expr;
            return statement;
        }
    }
}

std::vector<StatementNode*> ast_create(std::vector<Token*> tokens) {
    log_print("Running ast_create\n");
    std::vector<StatementNode*> ret;
    bool is_block = false;
    for (int i = 0; i < tokens.size(); i++) {
        Token* current_token = tokens[i];
        TokenType tt = current_token->tt;
        if (tt == TK_NEWLINE) {
            continue;
        } else if (tt == TK_INCLUDE) {
            current_token = next_token(tokens, &i); // skip include
            std::string filename = current_token->token;
            if (global_state->include_path.size() != 0) {
                filename = global_state->include_path + filename;
            } else {
                std::string dir = ast_get_file_full_path(global_state->input_filename);
                filename = global_state->input_file_dir + dir + filename;
            }
            std::string src = read_file(filename.c_str());
            auto tokens = tokenize(src);
            auto ast = ast_create(tokens);
            ret.insert(ret.end(), ast.begin(), ast.end());
        } else {
            ret.push_back(ast_create_declaration(tokens, &i));
        }
    }
    return ret;
}

void atlas_lib(std::ofstream* file) {
    *file << "#include <mimalloc.h>\n";

    *file << "extern int open(const char* filename, int flags, int mode);\n";
    *file << "extern int close(int fileds);\n";
    
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
    std::string mimalloc = " mimalloc/out/release/mimalloc.o -I mimalloc/include/ ";
    std::string command = backend + mimalloc + "out.c -o " + output_file_path;
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
        return "mi_malloc";
    } else if (name == "free") {
        return "mi_free";
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
    *file << "atlas_create_string("
          << "\"" << quote->quote_token->token << "\""
          << ","  << quote->quote_token->token.size()
          << ")";
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

void codegen_expr(ExpressionNode* expression, std::ofstream* file) {
    file->flush();
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
        std::string call_name = expression->call_node->name->token;
        if (codegen_is_intrinsic_function(call_name)) {
            *file << codegen_get_intrinsic_name(call_name) << "(";
        } else {
            *file << expression->call_node->name->token << "("; 
        }
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
    *file << func->token->token << "(";
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
        std::cout << "atlas: " << CL_RED << "error:" << CL_RESET <<" no input files";
        return 1;
    }

    State* state = set_options(argc, argv);
    global_state = state;
    std::string BACKEND;
    if (state->debug) {
        std::cout << "[INFO]: Debug Mode is enabled";
        BACKEND = "gcc -fcompare-debug-second -w ";
    } else {
        BACKEND = "gcc";
    }

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
