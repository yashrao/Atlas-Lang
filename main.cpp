#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <cctype>
#include <cstring>

// debug
#include <memory>

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
    TK_TYPE
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
    NODE_CONSTANT,
    NODE_VAR,
    NODE_RETURN,
    NODE_IF,
    NODE_FOR,
    NODE_TYPE,
    NODE_ARRAY_EXPR,
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

struct CallNode : Node {
    Token* name;
    std::vector<struct ExpressionNode*> args;
};

struct BinaryOpNode : Node {
    struct ExpressionNode* lhs;
    struct ExpressionNode* rhs;
    Token* op;
};

struct UnaryOpNode : Node {

};

struct ConstantNode : Node {
    int value;
};

struct ArrayNode : Node {
    std::vector<ExpressionNode*> elements;
};

struct ExpressionNode : Node {
    union {
        BinaryOpNode* binop;
        ConstantNode* constant;
        UnaryOpNode* unary_op;
        VarNode* var_node;
        CallNode* call_node;
        ArrayNode* array;
    };
};

struct ForNode : Node {
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

struct ParamNode : Node {
    Token* type;
};

struct FunctionNode : Node {
    std::vector<ParamNode*> params;
    struct BlockNode* block;
    Token* return_types;
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
    std::vector<StatementNode*>* statements;
};

ExpressionNode* ast_create_expression(std::vector<Token*> tokens, bool is_args, bool is_cond, bool is_arr, int* i);
ExpressionNode* ast_create_expr_prec(std::vector<Token*> tokens, int precedence, bool is_args, bool is_cond, bool is_arr, int* i);
void codegen_block(BlockNode* block, std::ofstream* file, int tab_level);
BlockNode* ast_create_block(std::vector<Token*> tokens, int* i);
void codegen_tabs(std::ofstream* file, int tab_level);
bool codegen_statement(StatementNode* statement, std::ofstream* file, int tab_level);
void codegen_expr(ExpressionNode* expression, std::ofstream* file);

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
    default:
        return "NODE_INVALID";
    }
}


void print_token(Token* token) {
    std::cout << "TOKEN" << "<" << get_tt_str(token->tt) << "> " 
        << "(" << token->line << "," << token->column << ")" 
        << ": " << token->token << "\n";
}

void print_error_msg(const char* error) {
    std::cout << CL_RED << "[COMPILER ERROR]: " << CL_RESET << error << "\n";
}

void print_tokens (std::vector<Token*> tokens) {
    for (Token* token : tokens) {
        print_token(token);
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
    }

    return TK_INVALID;
}

std::string read_file (const char* filename) {
    std::ifstream file;
    file.open(filename);
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
        c == ';' || c == ',') 
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
        } 
        if (isalnum(c)) {
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
                column = 0;
                line++;
            } else if (c == '+' || c == '(' || c == ')' || c == '*' ||
                       c == '{' || c == '}' || c == '=' || c == '[' ||
                       c == ']' || c == ',') {
                if (c == '=' && lookahead == '=') {
                    i += 1;
                    ret.push_back(save_token(line, column, TK_EQUAL, "=="));
                } else {
                    ret.push_back(save_token(line, column, get_tt(c), std::string(1, c)));
                }
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
        }  
    }

    return ret;
}

void expect(Token* token, TokenType expected) {
    if (token->tt != expected) {
        //std::cout << expected;
        std::cout << CL_RED << "[Error]:" << CL_RESET
        << " Expected: \"" << get_tt_str(expected) 
        << "\"\n         Got: ";
        print_token(token);
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
    //printf(" %s->%s", CL_YELLOW, CL_RESET);
    printf(" ");
    /*
    switch(node->nt) {
    case NODE_BINOP:
        printf("%sBinOpNode%s <%c>",
               CL_CYAN,
               CL_RESET,
               *(node->binop->op->beg));
        printf("\n");
        print_node(node->binop->lhs, tab_level + 1);
        print_node(node->binop->rhs, tab_level + 1);
        break;
    case NODE_CONSTANT:
        printf("%sConstantNode%s <%d>",
               CL_CYAN,
               CL_RESET,
               node->constant->iVal);
        printf("\n");
        break;
    case NODE_BLOCK: {
        printf("%sBlockNode%s\n", CL_CYAN, CL_RESET);
        Node* current_node = node->block->statements;
        current_node = current_node->next; // skip root node
        while (current_node != NULL) {
            print_node(current_node, tab_level + 1);
            current_node = current_node->next;
        }
        break;
    }
    case NODE_VAR_DECL:
        printf("%sVarDeclNode%s\n", CL_CYAN, CL_RESET);
        print_node(node->var_decl->lhs, tab_level + 1);
        print_node(node->var_decl->rhs, tab_level + 1);
        break;
    case NODE_FUNC: {
        printf("%sFuncNode%s <\"", CL_CYAN, CL_RESET);
        print_token_str(node->func->name);
        printf("\">\n");
        print_node(node->func->block, tab_level + 1);
        break;
    }
    case NODE_VAR: {
        printf("%sVariableNode%s <\"", CL_CYAN, CL_RESET);
        print_token_str(node->var->identifier);
        printf("\">\n");
        break;
    }
    case NODE_RETURN:
        printf("%sReturnNode%s\n", CL_CYAN, CL_RESET);
        print_node(node->ret->ret_expr, tab_level + 1);
        break;
    case NODE_CALL:
        printf("%sCallNode%s <\"", CL_CYAN, CL_RESET);
        print_token_str(node->call->name);
        printf("\">\n");
        break;
    default:
        print_error_msg("Something has horribly gone wrong");
        printf("%d\n", node->nt);
        break;
    }
    */
    //printf("\n");
}

void print_nodes(std::vector<Node*> nodes) {
    for(auto node : nodes) {
        print_node(node, 0);
    }
}

ParamNode* ast_create_param(Token* type, Token* name) {
    ParamNode* ret = new ParamNode;
    ret->nt = NODE_PARAM;
    ret->token = name;
    ret->type = type;
    return ret;
}

Token* next_token(std::vector<Token*> tokens, int* i) {
    (*i)++;
    return tokens[*i];
}

std::vector<ParamNode*> ast_parse_params(std::vector<Token*> tokens, int* index) {
    std::vector<ParamNode*> params;
    Token* current_token = tokens[*index];
    while (current_token->tt != TK_PAREN_CLOSE) {
        expect(current_token, TK_IDENTIFIER);
        Token* type = current_token;
        current_token = next_token(tokens, index);

        expect(current_token, TK_IDENTIFIER);
        Token* name = current_token;
        current_token = next_token(tokens, index);
        params.push_back(ast_create_param(type, name));
        
        if (current_token->tt == TK_COMMA) {
            current_token = next_token(tokens, index); // skip the comma
        } else if (current_token->tt == TK_PAREN_CLOSE) {
            break;
        }
        if (*index == tokens.size()) {
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
    ExpressionNode* ret = new ExpressionNode;
    CallNode* call = new CallNode;
    std::vector<ExpressionNode*> args;
    (*i)++; // the index should be at the open_paren
    (*i)++; // the index should be at the first part of the expr
    for (; *i < tokens.size(); (*i)++) {
        Token* current_token = tokens[*i];
        if (current_token->tt == TK_PAREN_CLOSE) {
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
    VarNode* ret = new VarNode;
    ret->type_ = NULL;
    ret->nt = NODE_VAR;
    ret->identifier = identifier;
    return ret;
}

VarDeclNode* ast_create_var_decl(VarType type, Token* type_id, Token* identifier, ExpressionNode* rhs) {
    VarDeclNode* ret = new VarDeclNode;
    VarNode* _node = new VarNode;
    //VariableNode* _node = calloc(1, sizeof(VariableNode));
    ret->nt = NODE_VAR_DECL;
    _node->type_ = type_id;
    _node->nt = NODE_VAR_DECL;
    _node->type = type;
    _node->identifier = identifier;
    ret->lhs = _node;
    ret->rhs = rhs;
    //ret->var = _node;
    return ret;
}

ExpressionNode* ast_create_variable_expr(VarType type, Token* identifier) {
    ExpressionNode* ret = new ExpressionNode;
    VarNode* _node = new VarNode;
    //VariableNode* _node = calloc(1, sizeof(VariableNode));
    ret->nt = NODE_VAR;
    _node->nt = NODE_VAR;
    _node->type = type;
    _node->identifier = identifier;
    ret->var_node = _node;
    //ret->var = _node;
    return ret;
}

ExpressionNode* ast_create_constant(Token* constant_value) {
    ExpressionNode* ret = new ExpressionNode;
    ConstantNode* constant = new ConstantNode;
    constant->value = stoi(constant_value->token);
    ret->nt = NODE_CONSTANT;

    ret->constant = constant;
    return ret;
}

int get_prec(TokenType tt) {
    switch(tt) {
    case TK_PAREN_OPEN:
        return 5;
    case TK_STAR:
    case TK_SLASH:
        return 4;
    case TK_PLUS:
    case TK_DASH:
        return 3;
    case TK_GT:
    case TK_LT:
    case TK_GTE:
    case TK_LTE:
        return 2;
    case TK_EQUAL:
        return 1;
    case TK_NEWLINE:
    case TK_CURLY_OPEN:
    case TK_PAREN_CLOSE:
    case TK_COMMA:
        return -1;
    default:
        print_error_msg("Invalid Operator");
        std::cout << "Operator: " << get_tt_str(tt) << ':' << tt << "\n";
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

ExpressionNode* ast_create_array_expr(std::vector<Token*> tokens, int* i) {
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
    expect(current_token, TK_SQUARE_CLOSE);
    ret->array = arr;
    ret->nt = NODE_ARRAY_EXPR;

    return ret;
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
    if (current_token->tt == TK_IDENTIFIER && lookahead->tt == TK_PAREN_OPEN) {
        lhs = ast_create_call(current_token, tokens, i);
    } else if (current_token->tt == TK_IDENTIFIER) {
        lhs = ast_create_variable_expr(TYPE_UNKNOWN, tokens[*i]);
    } else if (current_token->tt == TK_CONSTANT) {
        lhs = ast_create_constant(tokens[*i]);
    } else if (current_token->tt == TK_SQUARE_OPEN) {
        lhs = ast_create_array_expr(tokens, i);
    }
    //operator is the lookahead
    ExpressionNode* rhs = NULL;

    // Make sure it is updated
    current_token = tokens[*i];  
    lookahead = ast_get_lookahead(tokens, i);
    if (is_args && current_token->tt == TK_PAREN_CLOSE) {
        (*i)++; // Throw away closed bracket
        return lhs;
    } else if (is_arr && (current_token->tt == TK_SQUARE_CLOSE) || (lookahead->tt == TK_SQUARE_CLOSE)) {
        (*i)++; // Throw away closed square bracket
        return lhs;
    } else if (is_arr && (current_token->tt == TK_COMMA) || (lookahead->tt == TK_COMMA)) {
        (*i)++; // Now token should be on ']' or ','
        return lhs;
    }

    if (current_token->tt != TK_NEWLINE && lookahead->tt != TK_CURLY_OPEN) {
        while(get_prec(lookahead->tt) >= precedence) {
            current_token = next_token(tokens, i);
            Token* op = current_token; // operator is a lookahead
            current_token = next_token(tokens, i); // rhs
            lookahead = ast_get_lookahead(tokens, i);
            if (current_token->tt == TK_IDENTIFIER
                && lookahead->tt == TK_PAREN_OPEN) {
                // Function Call
                rhs = ast_create_call(current_token, tokens, i);
            } else if (current_token->tt == TK_IDENTIFIER) {
                // Variable
                rhs = ast_create_variable_expr(TYPE_UNKNOWN, tokens[*i]);
            } else if (current_token->tt == TK_CONSTANT) {
                // Constant TODO: handle quotes 
                rhs = ast_create_constant(tokens[*i]);
            } else {
                return lhs;
            }
            lookahead = ast_get_lookahead(tokens, i);
            // TODO: handle left associativity
            // TODO: MIGHT BE FAILING HERE FOR ARRAY EXPR 
            // std::cout << "LOOKAHEAD\n";
            // print_token(lookahead);
            while (get_prec(lookahead->tt) >= get_prec(op->tt)) {
                if (get_prec(lookahead->tt) > get_prec(op->tt))
                    //rhs = ast_create_expr_prec(tokens, get_prec(op->tt) + 1, false, false, false i);
                    rhs = ast_create_expr_prec(tokens, get_prec(op->tt) + 1, is_args, is_cond, is_arr, i);
                else
                    //rhs = ast_create_expr_prec(tokens, get_prec(op->tt), false, false, false, i);
                    rhs = ast_create_expr_prec(tokens, get_prec(op->tt), is_args, is_cond, is_arr, i);
                current_token = next_token(tokens, i);
                if (current_token->tt == TK_NEWLINE || current_token->tt == TK_COMMA) {
                    break;
                }
                lookahead = ast_get_lookahead(tokens, i);
            }
            lhs = ast_create_binop(lhs, rhs, op);
            lookahead = ast_get_lookahead(tokens, i);
            if (current_token->tt == TK_NEWLINE ||
                lookahead->tt == TK_NEWLINE ||
                current_token->tt == TK_COMMA) {
                break;
            }
        }
    }
    if (is_arr && current_token->tt != TK_SQUARE_CLOSE) {
        (*i)++;
    }
    //print_node(lhs, 0);
    return lhs;
}

ExpressionNode* ast_create_expression(std::vector<Token*> tokens, bool is_args, bool is_cond, bool is_arr, int* i) {
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

StatementNode* ast_create_for(std::vector<Token*> tokens, int* i) {
    StatementNode* ret = new StatementNode;
    ForNode* for_node = new ForNode;

    Token* current_token = next_token(tokens, i); // skip for
    //TODO: make it more generic for other types of statements
    // init statement
    // for now assuming its a var_decl but need to fix this later
    StatementNode* init_statement = new StatementNode;
    Token* type = current_token;
    current_token = next_token(tokens, i);
    expect(current_token, TK_DOUBLE_C);
    current_token = next_token(tokens, i);
    expect(current_token, TK_IDENTIFIER);
    Token* id = current_token;
    current_token = next_token(tokens, i);
    expect(current_token, TK_ASSIGN);
    current_token = next_token(tokens, i);
    ExpressionNode* rhs = ast_create_expression(tokens, false, false, false, i);
    VarDeclNode* lhs = ast_create_var_decl(get_var_type(type), type, id, rhs);
    init_statement->nt = NODE_VAR_DECL;
    init_statement->vardecl_lhs = lhs;
    init_statement->expr_rhs = rhs;
    for_node->init = init_statement;

    // test expression
    current_token = next_token(tokens, i);
    expect(current_token, TK_NEWLINE);
    current_token = next_token(tokens, i); // skip NEWLINE
    ExpressionNode* test_expr = ast_create_expression(tokens, false, false, false, i);
    current_token = next_token(tokens, i);
    expect(current_token, TK_NEWLINE);
    for_node->test = test_expr;

    // update statement
    current_token = next_token(tokens, i); // skip NEWLINE
    StatementNode* update_statement = new StatementNode;
    Token* var = current_token;
    VarNode* var_node = ast_create_var(var);
    current_token = next_token(tokens, i); 
    current_token = next_token(tokens, i); // skip =
    ExpressionNode* expr_rhs = ast_create_expression(tokens, false, false, false, i);
    AssignNode* assign = new AssignNode;
    assign->lhs = var_node;
    assign->rhs = expr_rhs;
    update_statement->nt = NODE_ASSIGN;
    update_statement->assign_lhs = assign;
    for_node->update = update_statement;

    // block
    current_token = next_token(tokens, i);
    expect(current_token, TK_CURLY_OPEN);
    BlockNode* for_block = ast_create_block(tokens, i);
    for_node->block = for_block;

    ret->for_lhs = for_node;
    return ret;
}

StatementNode* ast_create_if(std::vector<Token*> tokens, int* i) {
    StatementNode* ret = new StatementNode;
    IfNode* if_node = new IfNode;
    if_node->_else = NULL;
    Token* current_token = next_token(tokens, i); // skip if token
    ExpressionNode* cond = ast_create_expression(tokens, false, true, false, i);
    (*i)++;
    BlockNode* block = ast_create_block(tokens, i);
    Token* lookahead = ast_get_lookahead(tokens, i);
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
    std::vector<StatementNode*>* statements = new std::vector<StatementNode*>;
    BlockNode* block = new BlockNode;
    block->nt = NODE_BLOCK;
    for (; *i < tokens.size(); (*i)++) {
        Token* current_token = tokens[*i];
        if (current_token->tt == TK_CURLY_CLOSE) {
            break;
        }
        if (current_token->tt == TK_IDENTIFIER) {
            //expect(current_token, TK_IDENTIFIER);
            Token* tmp = current_token;
            current_token = next_token(tokens, i);
            bool is_array = false;
            ExpressionNode* arr_size = NULL; // incase the var is an array
            if (current_token->tt == TK_DOUBLE_C || current_token->tt == TK_SQUARE_OPEN) {
                Token* type = tmp;
                if (current_token->tt == TK_SQUARE_OPEN) {
                    // array type   
                    is_array = true;
                    current_token = next_token(tokens, i); // expr
                    arr_size = ast_create_expression(tokens, false, false, true, i);
                    current_token = tokens[*i]; // update current_token
                    expect(current_token, TK_SQUARE_CLOSE);
                    current_token = next_token(tokens, i);
                }
                expect(current_token, TK_DOUBLE_C);
                current_token = next_token(tokens, i);
                expect(current_token, TK_IDENTIFIER);
                Token* id = current_token;
                current_token = next_token(tokens, i);
                expect(current_token, TK_ASSIGN);
                current_token = next_token(tokens, i);
                ExpressionNode* rhs = ast_create_expression(tokens, false, false, false, i);
                VarDeclNode* lhs = ast_create_var_decl(get_var_type(type), type, id, rhs);
                lhs->lhs->is_array = is_array;
                lhs->lhs->arr_size = arr_size;
                //print_node(expr, 0);
                StatementNode* statement = new StatementNode;
                statement->nt = NODE_VAR_DECL;
                statement->vardecl_lhs = lhs;
                statement->expr_rhs = rhs;
                statements->push_back(statement);
            } else if (current_token->tt == TK_ASSIGN) {
                Token* var = tmp;
                VarNode* var_node = ast_create_var(var);
                current_token = next_token(tokens, i); // skip =
                ExpressionNode* expr_rhs = ast_create_expression(tokens, false, false, false, i);
                AssignNode* assign = new AssignNode;
                assign->lhs = var_node;
                assign->rhs = expr_rhs;
                StatementNode* statement = new StatementNode;
                statement->nt = NODE_ASSIGN;
                statement->assign_lhs = assign;
                statements->push_back(statement);
            }
        } else if (current_token->tt == TK_ARROW) {
            StatementNode* statement = ast_create_return(tokens, i);
            statement->nt = NODE_RETURN;
            statements->push_back(statement);
        } else if (current_token->tt == TK_IF) {
            StatementNode* statement = ast_create_if(tokens, i);
            statement->nt = NODE_IF;
            statements->push_back(statement);
        } else if (current_token->tt == TK_FOR) {
            StatementNode* statement = ast_create_for(tokens, i);
            statement->nt = NODE_FOR;
            statements->push_back(statement);
        }
    }
    block->statements = statements;
    return block;
}

std::vector<StatementNode*> ast_create(std::vector<Token*> tokens) {
    std::vector<StatementNode*> ret;
    bool is_block = false;
    for (int i = 0; i < tokens.size(); i++) {
        Token* current_token = tokens[i];
        TokenType tt = current_token->tt;
        if (!is_block) {
            if (tt == TK_IDENTIFIER) {
                //TODO: check if its reserved keyword like type/enum/etc...
                FunctionNode* fn = new FunctionNode;
                fn->nt = NODE_FUNC;
                // Function Name
                Token* name = current_token;
                current_token = next_token(tokens, &i);
                BlockNode* block = NULL;
                if (current_token->tt == TK_PAREN_OPEN) {
                    current_token = next_token(tokens, &i);
                    auto params = ast_parse_params(tokens, &i);
                    current_token = tokens[i]; // update current_token

                    fn->token = name;
                    fn->params = params;
                    expect(current_token, TK_PAREN_CLOSE);
                    current_token = next_token(tokens, &i);
                    if (current_token->tt == TK_ARROW) {
                        // parse return types
                        // TODO: support multiple types
                        current_token = next_token(tokens, &i);
                        auto type = current_token;
                        fn->return_types = type;
                        current_token = next_token(tokens, &i);
                    } else {
                        fn->return_types = NULL;
                    }
                    expect(current_token, TK_CURLY_OPEN);
                    block = ast_create_block(tokens, &i);
                }
                fn->block = block;
                StatementNode* stmt = new StatementNode;
                stmt->nt = NODE_FUNC;
                stmt->func_lhs = fn;
                ret.push_back(stmt);
            } else if (tt == TK_TYPE) {
                Token* name = next_token(tokens, &i);
                current_token = next_token(tokens, &i);
                TypeNode* type_node = new TypeNode;
                expect(current_token, TK_CURLY_OPEN);
                current_token = next_token(tokens, &i);
                expect(current_token, TK_NEWLINE);
                for (; i < tokens.size();) {
                    current_token = next_token(tokens, &i);
                    print_token(current_token);
                    if (current_token->tt == TK_CURLY_CLOSE) {
                        current_token = next_token(tokens, &i); // skip curly close
                        break;
                    }
                    expect(current_token, TK_IDENTIFIER); // type
                    Token* type = current_token;
                    current_token = next_token(tokens, &i);
                    expect(current_token, TK_DOUBLE_C);
                    current_token = next_token(tokens, &i); // name
                    Token* name = current_token;
                    VarDeclNode* var = ast_create_var_decl(get_var_type(type), type, name, NULL);
                    type_node->name = name;
                    type_node->declarations.push_back(var);
                    current_token = next_token(tokens, &i); // newline
                    expect(current_token, TK_NEWLINE);
                }
                StatementNode* statement = new StatementNode;
                statement->type_lhs = type_node;
                statement->nt = NODE_TYPE;
                ret.push_back(statement);
            }
        }
    }
    return ret;
}

void atlas_lib(std::ofstream* file) {
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
          << "#define bool ushort\n"
          << "#define true 1\n"
          << "#define false 0\n"
          << "#define NULL 0\n";

    *file << "#define MEMORY_POOL_SIZE 690000\n"
          << "static uint allocated_size = 0;\n"
          << "typedef struct Block {\n"
          << "    uint size;\n"
          << "    struct Block* next;\n"
          << "} Block;\n"
          << "static char memory_pool[MEMORY_POOL_SIZE];\n"
          << "static Block* head = NULL;\n"
          << "void* atlas_malloc(uint size) {\n"
          << "    if (allocated_size + size > MEMORY_POOL_SIZE) {\n"
          << "        return NULL;\n"
          << "    }\n"
          << "    void* ptr = &memory_pool[allocated_size];\n"
          << "    allocated_size += size;\n"
          << "    return ptr;\n"
          << "}\n"
          << "void atlas_free(void* ptr) {\n"
          << "    if (ptr == NULL) {\n"
          << "        return;\n"
          << "    }\n"
          << "    Block* block = (Block*)((char*)ptr - sizeof(Block));\n"
          << "    block->next = head;\n"
          << "    head = block;\n"
          << "}\n";

    *file << "\n";

    *file << "void sys_exit(int error_code)\n"
          << "{\n"
          << "\tasm volatile\n"
          << "\t(\n"
          << "\t\t\"syscall\"\n"
          << "\t\t:\n" 
          << "\t\t: \"a\"(SYSCALL_EXIT), \"D\"(error_code)\n"
          << "\t\t: \"rcx\", \"r11\", \"memory\"\n"
          << "\t);\n"
          << "}\n\n";

	*file << "int sys_write(unsigned fd, const char* buf, unsigned count)\n"
          << "{\n"
          << "\tunsigned ret;\n"
          << "\tasm volatile\n"
          << "\t(\n"
          << "\t\t\"syscall\"\n"
          << "\t\t: \"=a\"(ret)\n"
          << "\t\t: \"a\"(SYSCALL_WRITE), \"D\"(fd), \"S\"(buf), \"d\"(count)\n"
          << "\t\t: \"rcx\", \"r11\", \"memory\"\n"
          << "\t);\n"
          << "\treturn ret;\n"
          << "}\n\n";

}

void codegen_init_c(std::vector<StatementNode*> ast, std::ofstream* file) {
	*file << "void _start(void)\n"
          << "{\n"
          << "\tint ret = main();\n";
    // for (StatementNode* statement : ast) {
    //     if (statement->nt == NODE_FUNC) {
    //         *file << "\t" << statement->func_lhs->token->token << "();\n";
    //     }
    // }

    *file << "\tsys_exit(ret);\n"
          << "}\n\n";
}

void codegen_end(std::ofstream* file, std::string backend) {
    (*file).close();

    std::string command = backend + " -nostdlib out.c -o out";
    std::cout << "[COMPILER]: Running \"" << command << "\"\n";
    // Compile C code
    int ret = std::system(command.c_str());
    //TODO: not sure what scenarios this works/not works
    if (WEXITSTATUS(ret) != 0x10) {
        std::string err = backend + " backend Failed to compile C program";
        print_error_msg(err.c_str());
        std::cout << "                  Check " << backend << " error messages";
        exit(1);
    }
    std::cout << "Generated binary \"out\"\n";
}

void codegen_array_expr(ArrayNode* array, std::ofstream* file) {
    *file << "{";
    for (int i = 0; i < array->elements.size(); i++) {
        std::cout << array->elements[i] << ": ";
        std::cout << get_nt_str(array->elements[i]->nt) << "\n";
        codegen_expr(array->elements[i], file);
        if (i != array->elements.size() - 1) {
            *file << ", ";
        }
    }
    *file << "}";
}

void codegen_expr(ExpressionNode* expression, std::ofstream* file) {
    switch(expression->nt) {
        case NODE_BINOP:
            codegen_expr(expression->binop->lhs, file);
            *file << " " << expression->binop->op->token << " ";
            codegen_expr(expression->binop->rhs, file);
            break;
        case NODE_CONSTANT:
            *file << expression->constant->value;
            break;
        case NODE_CALL:
            *file << expression->call_node->name->token << "("; 
            for (auto arg : expression->call_node->args) {
                codegen_expr(arg, file);
            }
            *file << ")";
            break;
        case NODE_VAR:
            *file << expression->var_node->identifier->token;
            break;
        case NODE_ARRAY_EXPR:
            codegen_array_expr(expression->array, file);
            break;
        default:
            std::string err = "CODEGEN EXPR " + get_nt_str(expression->nt) + "\n";
            print_error_msg(err.c_str());
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
    }
    print_error_msg("This type is not supported");
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
        *file << codegen_get_c_type(var_decl->lhs->type_) << " " << var_decl->lhs->identifier->token;
    } else {
        *file << var_decl->lhs->type_->token << var_decl->lhs->identifier->token;
    }

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
    *file << "for(";
    codegen_statement(for_node->init, file, tab_level);
    *file << "; ";
    codegen_expr(for_node->test, file);
    *file << "; ";
    codegen_statement(for_node->update, file, tab_level);
    *file << ")\n";
    codegen_block(for_node->block, file, tab_level + 1);
}

void codegen_assign(AssignNode* assign, std::ofstream* file) {
    // lhs
    *file << assign->lhs->identifier->token;
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
            codegen_assign(statement->assign_lhs, file);
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
    for (ParamNode* param : func->params) {
        *file << codegen_get_c_type(param->type) << " ";
        *file << param->token->token;
        if (func->params.size() > 1 && add_comma) {
            *file << ", ";
            add_comma = false;
        }
    }
    if (func->params.size() == 0) {
        *file << "void";
    }
    *file << ")\n";
    codegen_block(func->block, file, 1);
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
        for (StatementNode* statement : *(block->statements)) {
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
    std::cout << "Ast size: " << ast.size() << "\n";
    for (StatementNode* node : ast) {
        std::cout << "Generating Node: " <<  get_nt_str(node->nt) << "\n";
        if (node->nt == NODE_FUNC) {
            codegen_func(node->func_lhs, &file);
        } else if (node->nt == NODE_TYPE) {
            codegen_type(node->type_lhs, &file);
        }
    }
    codegen_init_c(ast, &file);
    codegen_end(&file, backend);
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

int main(int argc, char** argv) {
    if (argc != 2) {
        //TODO: print a usage
        std::cout << "atlas: " << CL_RED << "error:" << CL_RESET <<" no input files";
        return 1;
    }
    std::string BACKEND = "gcc";

    std::string src = read_file(argv[1]);
    std::cout << src << "\n";
    std::cout << "-----TOKENIZING START------\n";
    auto tokens = tokenize(src);
    print_tokens(tokens);
    std::cout << "------TOKENIZING END-------\n\n";
    std::cout << "--------AST START----------\n";
    auto ast = ast_create(tokens);
    std::cout << "---------AST END-----------\n\n";
    std::cout << "------CODEGEN START--------\n";
    codegen_start(ast, "out.c", BACKEND);
    std::cout << "-------CODEGEN END---------\n";
    return 0;
}
