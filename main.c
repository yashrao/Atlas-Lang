#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>

#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>

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

typedef enum TokenType {
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
    TK_SQUARE_CLOSE
} TokenType;

typedef enum ErrorType {
    COMPILER = 200,
    AST_CREATION,
} ErrorType;

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

typedef enum NodeType {
    NODE_FUNC = 200,
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
} NodeType;


typedef struct Token {
    struct Token* head;
    struct Token* next;

    int line, column;
    char* beg;
    char* end;
    TokenType tt;
} Token;

typedef struct Error {
    enum ErrorType type;
    Token* tok;
    char* error_string;
} Error;

typedef struct ConstantNode {
    Token* token;
    union {
        int iVal;
        float fVal;
    };
} ConstantNode;

typedef struct VariableNode {
    VarType type;
    Token* identifier;

    // Codegen
    struct Node* scoped_var;
    LLVMValueRef llvm_value_ref;
    LLVMTypeRef llvm_var_type;
} VariableNode;

typedef struct BinOpNode {
    struct Node* lhs;
    struct Node* rhs;
    Token* op;
} BinOpNode;

typedef struct AssignNode {
    struct Node* lhs; // Variable Node
    struct Node* rhs; // Expression Node
} AssignNode;

typedef struct VarDeclNode {
    VarType type;
    struct Node* lhs;
    struct Node* rhs;
} VarDeclNode;

typedef struct ReturnNode {
    // TODO: this does not handle multiple return values
    struct Node* ret_expr; // expression that is being returned
} ReturnNode;

typedef struct Scope {
    struct Scope* parent_scope;
    struct Node* names;
} Scope;

typedef struct BlockNode {
    struct Scope* scope;
    struct Node* statements;
} BlockNode;

typedef struct FunctionNode {
    Token* name;
    struct Node* args; // VariableNodes
    struct Node* block;
    Token* return_types;
    // for Codegen
    //TODO: params
    struct Node* scoped_node;
    LLVMValueRef llvm_value_ref;
    LLVMTypeRef llvm_fn_type;
} FunctionNode;

typedef struct CallNode {
    Token* name;
    struct Node* arguments;
} CallNode;

typedef struct Node {
    struct Node* next;
    NodeType nt;
    union {
        FunctionNode* func;
        BlockNode* block;
        AssignNode* assign;
        BinOpNode* binop;
        VarDeclNode* var_decl;
        VariableNode* var;
        ConstantNode* constant;
        ReturnNode* ret;
        CallNode* call;
    };
} Node;

Node* ast_create_expression(Token** token_list, bool is_args);

static Error OK = (Error) {COMPILER, NULL};

void print_usage() {
    printf("Atlas: %s[ERROR]:%s no input files\n", CL_RED, CL_RESET);
}

const char* get_tt(TokenType tt) {
    switch(tt) {
    case TK_NEWLINE:
        return "NEWLINE";
        break;
    case TK_PLUS:
        return "PLUS";
        break;
    case TK_DASH:
        return "DASH";
        break;
    case TK_STAR:
        return "STAR";
        break;
    case TK_DOUBLE_C:
        return "DOUBLE_C";
        break;
    case TK_IDENTIFIER:
        return "IDENTIFIER";
        break;
    case TK_PAREN_OPEN:
        return "PAREN_OPEN";
        break;
    case TK_PAREN_CLOSE:
        return "PAREN_CLOSE";
        break;
    case TK_CURLY_OPEN:
        return "CURLY_OPEN";
        break;
    case TK_CURLY_CLOSE:
        return "CURLY_CLOSE";
        break;
    case TK_ASSIGN:
        return "ASSIGN";
        break;
    case TK_EQUAL:
        return "EQUAL";
        break;
    case TK_NOT_EQUAL:
        return "NOT_EQUAL";
        break;
    case TK_ARROW:
        return "ARROW";
        break;
    case TK_DOT:
        return "DOT";
        break;
    case TK_CONSTANT:
        return "CONSTANT";
        break;
    case TK_COMMA:
        return "COMMA";
        break;
    case TK_SQUARE_OPEN:
        return "SQUARE_OPEN";
        break;
    case TK_SQUARE_CLOSE:
        return "SQUARE_CLOSE";
        break;
    case TK_QUOTE:
        return "QUOTE";
        break;
    default:
        return "UNKNOWN TOKEN TYPE";
    }
}

void print_token(Token* token) {
    printf("[DEBUG]: \"");
    char* beg = token->beg;
    char* end = token->end;
    while (beg != end) {
        printf("%c", *beg);
        beg++;
    }
    if (*end == '\n') {
        printf("NEWLINE");
    } else {
        printf("%c", *end);
    }
    printf("\" <%s> ", get_tt(token->tt));
    printf("(%d, %d)\n", token->line, token->column);
}

void print_tokens(Token* token_list) {
    Token* current = token_list;
    printf("[DEBUG]: Printing Tokens\n");
    while (current != NULL) {
        print_token(current);
        current = current->next;
    }
}

void print_tabs(int level) {
    for (int i = 0; i < level * 4; i++) {
        printf(" ");
        if (i % 4 == 0) {
            printf("%s│%s", CL_YELLOW, CL_RESET);
        }
    }
}

void print_error_msg(const char* error) {
    printf("%s[ERROR]:%s %s\n",
            CL_RED, CL_RESET,
            error);
}

void print_token_str(Token* token) {
    char* beg = token->beg;
    while (beg != token->end) {
        printf("%c", *beg);
        beg++;
    }
    printf("%c", *beg);
}

void print_node(Node* node, int tab_level) {
    printf("[DEBUG]: ");
    print_tabs(tab_level);
    printf(" %s├─%s", CL_YELLOW, CL_RESET);
    printf(" ");
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
    //printf("\n");
}

void print_nodes(Node* node) {
    while(node != NULL) {
        print_node(node, 0);
        node = node->next;
    }
}

void fatal(const char* string) {
    printf("%s[FATAL]:%s %s", CL_RED, CL_RESET, string);
    exit(1);
}

bool is_delim(char c) {
    switch(c) {
    case ':':
    case '+':
    case '-':
    case '*':
    case '/':
    case '<':
    case '>':
    case ' ':
    case '\n':
    case '!':
    case '=':
    case '{':
    case '}':
    case '(':
    case ')':
    case ';':
    case '"':
    case '\'':
    case ',':
    case '.':
        return true;
    default:
        return false;
    }
}

Token* create_token(char* beg, char* end, int line, int column, TokenType tt) {
    Token* ret = calloc(1, sizeof(Token));
    ret->beg = beg;
    ret->end = end;
    ret->line = line;
    ret->column = column;
    ret->tt = tt;
    return ret;
}

bool is_num(char* beg, char* end) {
    while (beg != end) {
        if (isalpha(*beg)) {
            return false;
        }
        beg++;
    }
    return true;
}

void add_token(Token* new_token, Token** token_head, Token** token_list) {
    if (*token_list == NULL) {
        *token_head = new_token;
        *token_list = *token_head;
    } else {
        (*token_head)->next = new_token;
        *token_head = (*token_head)->next;
    }
}

Error lexer(char* src, int length, Token** token_head, Token** token_list) {
    char* beg = src;
    char* end = src;
    int line = 1;
    int column = 1;
    for (int i = 0; i < length; i++) {
        if (is_delim(*end)) {
            if (beg != end) {
                // that means there is identifier there, save it first
                Token* new_token;
                if (is_num(beg, end)) {
                    new_token =
                        create_token(beg, end - 1, line, column, TK_CONSTANT);
                } else {
                    new_token =
                        create_token(beg, end - 1, line, column, TK_IDENTIFIER);
                }
                add_token(new_token, token_head, token_list);
                beg = end;
            }
            if (*end == ':' && *(end + 1) == ':') {
                end++;
                Token* new_token =
                    create_token(beg, end, line, column, TK_DOUBLE_C);
                add_token(new_token, token_head, token_list);
                i++;
            } else if (*end == '/' && *(end + 1) == '/') {
                // comment
                while (*end != '\n') {
                    end++;
                    i++;
                }
                Token* new_token =
                    create_token(end, end, line, column, TK_NEWLINE);
                add_token(new_token, token_head, token_list);
                line++;
                column = 0;
            } else if (*end == '=' && *(end + 1) == '=') {
                end ++;
                i++;
                Token* new_token =
                    create_token(beg, end, line, column, TK_EQUAL);
                add_token(new_token, token_head, token_list);
            } else if (*end == '!' && *(end + 1) == '=') {
                end++;
                i++;
                Token* new_token =
                    create_token(beg, end, line, column, TK_NOT_EQUAL);
                add_token(new_token, token_head, token_list);
            } else if (*end == '-' && *(end + 1) == '>') {
                end++;
                i++;
                Token* new_token =
                    create_token(beg, end, line, column, TK_ARROW);
                add_token(new_token, token_head, token_list);
            } else if (*end == '<' && *(end + 1) == '=') {
                end++;
                i++;
                Token* new_token =
                    create_token(beg, end, line, column, TK_LTE);
                add_token(new_token, token_head, token_list);
            } else if (*end == '>' && *(end + 1) == '=') {
                end++;
                i++;
                Token* new_token =
                    create_token(beg, end, line, column, TK_GTE);
                add_token(new_token, token_head, token_list);
            } else if (*end == '+') {
                Token* new_token =
                    create_token(beg, end, line, column, TK_PLUS);
                add_token(new_token, token_head, token_list);
            } else if (*end == '-') {
                Token* new_token =
                    create_token(beg, end, line, column, TK_DASH);
                add_token(new_token, token_head, token_list);
            } else if (*end == '*') {
                Token* new_token =
                    create_token(beg, end, line, column, TK_STAR);
                add_token(new_token, token_head, token_list);
            } else if (*end == '/') {
                Token* new_token =
                    create_token(beg, end, line, column, TK_SLASH);
                add_token(new_token, token_head, token_list);
            } else if (*end == ';') {
                Token* new_token =
                    create_token(beg, end, line, column, TK_NEWLINE);
                add_token(new_token, token_head, token_list);
            } else if (*end == '\n') {
                Token* new_token =
                    create_token(beg, end, line, column, TK_NEWLINE);
                add_token(new_token, token_head, token_list);
                line++;
                column = 0;
            } else if (*end == '{') {
                Token *new_token =
                    create_token(beg, end, line, column, TK_CURLY_OPEN);
                add_token(new_token, token_head, token_list);
            } else if (*end == '}') {
                Token *new_token =
                    create_token(beg, end, line, column, TK_CURLY_CLOSE);
                add_token(new_token, token_head, token_list);
            } else if (*end == '(') {
                Token *new_token =
                    create_token(beg, end, line, column, TK_PAREN_OPEN);
                add_token(new_token, token_head, token_list);
            } else if (*end == ')') {
                Token *new_token =
                    create_token(beg, end, line, column, TK_PAREN_CLOSE);
                add_token(new_token, token_head, token_list);
            } else if (*end == '=') {
                Token *new_token =
                    create_token(beg, end, line, column, TK_ASSIGN);
                add_token(new_token, token_head, token_list);
            } else if (*end == ',') {
                Token *new_token =
                    create_token(beg, end, line, column, TK_COMMA);
                add_token(new_token, token_head, token_list);
            } else if (*end == '<') {
                Token *new_token =
                    create_token(beg, end, line, column, TK_LT);
                add_token(new_token, token_head, token_list);
            } else if (*end == '>') {
                Token *new_token =
                    create_token(beg, end, line, column, TK_GT);
                add_token(new_token, token_head, token_list);
            } else if (*end == '.') {
                Token *new_token =
                    create_token(beg, end, line, column, TK_DOT);
                add_token(new_token, token_head, token_list);
            } else if (*end == '[') {
                Token *new_token =
                    create_token(beg, end, line, column, TK_SQUARE_OPEN);
                add_token(new_token, token_head, token_list);
            } else if (*end == ']') {
                Token *new_token =
                    create_token(beg, end, line, column, TK_SQUARE_CLOSE);
                add_token(new_token, token_head, token_list);
            } else if (*end == '"') {
                beg = end;
                end++;
                i++;
                while(*end != '"') {
                    end++;
                    i++;
                }
                Token *new_token =
                    create_token(beg, end, line, column, TK_QUOTE);
                add_token(new_token, token_head, token_list);
            } else if (*end == ' ') {
                //  Skip space
            }
            beg = end + 1;
            end = beg;
        } else {
            // Identifiers
            end++;
        }
        column++;
    }
    return OK;
}

void print_src(char* src) {
    while (*src != '\0') {
        printf("%c", *src);
        src++;
    }
}

char* read_src(const char* filename, int* count) {
    FILE* fp = fopen(filename, "r");

    if (fp == NULL) {
        fatal("Could not open the file\n");
    }

    *count = 0;
    while (true) {
        if (feof(fp)) {
            break;
        }
        fgetc(fp);
        *count = *count + 1;
    }
    fseek(fp, 0, SEEK_SET);
    char* src = calloc(*count, sizeof(char));

    char c;
    int i = 0;
    while ((c = fgetc(fp))) {
        if (feof(fp)) {
            break;
        }
        src[i] = c;
        i++;
    }
    print_src(src);
    return src;
}

void expect(Token* tok, TokenType expected) {
    if (tok->tt != expected) {
        printf("%s[ERROR]:%s Expected %s but got %s\n",
                CL_RED, CL_RESET,
                get_tt(expected), get_tt(tok->tt));
        exit(1);
    } 
}

VarType get_var_type(Token* var_type) {
    char* beg = var_type->beg;
    char* end = var_type->end;
    int n = 1;
    while (beg != end) {
        beg++;
        n++;
    }
    if (strncmp(var_type->beg, "i8", n) == 0 && n == 2) {
        return TYPE_I8;
    } else if (strncmp(var_type->beg, "i16", n) == 0 && n == 3) {
        return TYPE_I16;
    } else if (strncmp(var_type->beg, "i32", n) == 0 && n == 3) {
        return TYPE_I32;
    } else if (strncmp(var_type->beg, "i64", n) == 0 && n == 3) {
        return TYPE_I64;
    } else if (strncmp(var_type->beg, "u8", n) == 0 && n == 2) {
        return TYPE_U8;
    } else if (strncmp(var_type->beg, "u16", n) == 0 && n == 3) {
        return TYPE_U16;
    } else if (strncmp(var_type->beg, "u32", n) == 0 && n == 3) {
        return TYPE_U32;
    } else if (strncmp(var_type->beg, "u64", n) == 0 && n == 3) {
        return TYPE_U64;
    } else if (strncmp(var_type->beg, "f32", n) == 0 && n == 3) {
        return TYPE_F32;
    } else if (strncmp(var_type->beg, "f64", n) == 0 && n == 3) {
        return TYPE_F64;
    } else if (strncmp(var_type->beg, "int", n) == 0 && n == 3) {
        return TYPE_I64;
    } else if (strncmp(var_type->beg, "float", n) == 0 && n == 5) {
        return TYPE_F64;
    }

    return TYPE_INVALID;
}

int calc_str_len(Token* token) {
    char* beg = token->beg;
    char* end = token->end;

    int count = 0;
    while(beg != end) {
        count++;
        beg++;
    }
    return count + 1;
}

void fill_char_array(char* arr, Token* token, int n) {
    char* beg = token->beg;
    for (int i = 0; i < n; i++) {
        arr[i] = *beg;
        beg++;
    }
    arr[n] = '\0';
}


LLVMTypeRef codegen_get_var_type(VarType var_type, LLVMContextRef ctx) {
    switch (var_type) {
    case TYPE_I64:
        return LLVMInt64TypeInContext(ctx);
    case TYPE_I32:
        return LLVMInt32TypeInContext(ctx);
    case TYPE_I16:
        return LLVMInt16TypeInContext(ctx);
    case TYPE_I8:
        return LLVMInt8TypeInContext(ctx);
    case TYPE_UNKNOWN:
        printf("UNKNOWN");
        return NULL;
        break;
    default:
        fatal("(CODEGEN) INVALID TYPE/TYPE NOT IMPLEMENTED YET");
        return NULL; // not reachable - just want the compiler to shutup
    }
}

Node* codegen_find_name_in_scope(Scope* scope, const char* name) {
    Node* beg = scope->names;
    while (beg != NULL) {
        // TODO: only handles functions for now, need to do variables later
        switch(beg->nt) {
        case NODE_FUNC: {
            FunctionNode* func = beg->func;
            int n = calc_str_len(func->name);
            char func_name[n];
            fill_char_array(func_name, func->name, n);
            if (strcmp(func_name, name) == 0) {
                return beg;
            }
            break;
        }
        case NODE_VAR: {
            VariableNode* var = beg->var;
            int n = calc_str_len(var->identifier);
            char var_name[n];
            fill_char_array(var_name, var->identifier, n);
            if (strcmp(var_name, name) == 0) {
                return beg;
            }
            break;
        }
        default:
            fatal("Invalid Node in codegen_find_name_in_scope\n");
        }
        beg = beg->next;
    }
    return NULL;
}

LLVMValueRef codegen_var(VariableNode* var, Scope* scope, LLVMBuilderRef builder, LLVMModuleRef mod) {
    //LLVMBuildLoad2(LLVMBuilderRef, LLVMTypeRef Ty, LLVMValueRef PointerVal, const char *Name)
    LLVMContextRef ctx = LLVMGetModuleContext(mod);
    int n = calc_str_len(var->identifier);
    char name[n];
    fill_char_array(name, var->identifier, n);
    Node* node = codegen_find_name_in_scope(scope, name);
    if (node == 0) {
        printf("Unable to find \"%s\"\n", name);
        fatal("Unable to find above variable\n");
    }
    printf("TESTING: %s\n", name);

    return LLVMBuildLoad2(builder, node->var->llvm_var_type, node->var->llvm_value_ref, "");
}

LLVMValueRef codegen_constant(ConstantNode* constant, LLVMContextRef ctx) {
    // TODO: un-hardcode everything
    printf("CONSTANT: %d\n", constant->iVal);
    return LLVMConstInt(LLVMInt64TypeInContext(ctx), constant->iVal, false);
}

LLVMValueRef codegen_binop(BinOpNode* binop, Scope* scope, LLVMBuilderRef builder, LLVMModuleRef mod) {
    LLVMContextRef ctx = LLVMGetModuleContext(mod);
    LLVMValueRef lhs;
    switch(binop->lhs->nt) {
    case NODE_BINOP:
        lhs = codegen_binop(binop->lhs->binop, scope, builder, mod);
        break;
    case NODE_CONSTANT:
        lhs = codegen_constant(binop->lhs->constant, ctx);
        break;
        //TODO: other cases
    case NODE_VAR:
        lhs = codegen_var(binop->lhs->var, scope, builder, mod);
            //ret = codegen_var(expr->var, scope, builder, mod);
        break;
    default:
        printf("(CODEGEN) LHS: '%d' NODE TYPE NOT IMPLEMENTED IN EXPRESSION\n",
              binop->lhs->nt);
        print_node(binop->lhs, 0);
    }

    LLVMValueRef rhs;
    switch(binop->rhs->nt) {
    case NODE_BINOP:
        rhs = codegen_binop(binop->rhs->binop, scope, builder, mod);
        break;
    case NODE_CONSTANT:
        rhs = codegen_constant(binop->rhs->constant, ctx);
        break;
        //TODO: other cases
    case NODE_VAR:
        rhs = codegen_var(binop->lhs->var, scope, builder, mod);
            //ret = codegen_var(expr->var, scope, builder, mod);
        break;
    default:
        printf("(CODEGEN) RHS: '%d' NODE TYPE NOT IMPLEMENTED IN EXPRESSION\n",
              binop->rhs->nt);
        print_node(binop->rhs, 0);
    }

    //Token* op = binop->op;
    char op = *(binop->op->beg); // TODO: handle multi character operators

    if (op == '+')
        return LLVMBuildAdd(builder, lhs, rhs, "");
    if (op == '-')
        return LLVMBuildSub(builder, lhs, rhs, "");
    if (op == '*')
        return LLVMBuildMul(builder, lhs, rhs, "");
    if (op == '/')
        fatal("DIVIDE NOT SUPPORTED\n");
    else
        printf("%s[FATAL]:%s OPERATOR '%c' NOT SUPPORTED\n",
               CL_RED, CL_RESET,
               op);
    exit(1);
}

LLVMValueRef codegen_call(CallNode* call, Scope* scope, LLVMBuilderRef builder, LLVMModuleRef mod) {
    LLVMContextRef ctx = LLVMGetModuleContext(mod);
    int n = calc_str_len(call->name);
    char name[n];
    fill_char_array(name, call->name, n);
    Node* scoped_node = codegen_find_name_in_scope(scope->parent_scope, name);
    if (scoped_node == 0) {
        printf("Unable to find \"%s\"\n", name);
        fatal("Unable to find above function\n");
    }
    FunctionNode* fn = scoped_node->func;
    printf("TESTING: %s\n", name);
    LLVMValueRef* args  = calloc(1, sizeof(LLVMValueRef));

    LLVMTypeRef param_types[] = { LLVMVoidType() };
    LLVMTypeRef fn_type = LLVMFunctionType(LLVMVoidTypeInContext(ctx), param_types, 0, 0);
    return LLVMBuildCall2(builder, fn->llvm_fn_type, fn->llvm_value_ref, NULL, 0, "");
}

LLVMValueRef codegen_expression(Node* expr, Scope* scope, LLVMBuilderRef builder, LLVMModuleRef mod, LLVMContextRef ctx) {
    LLVMValueRef ret;
    switch(expr->nt) {
    case NODE_CALL:
        ret = codegen_call(expr->call, scope, builder, mod);
        break;
    case NODE_BINOP:
        //TODO:
        ret = codegen_binop(expr->binop, scope, builder, mod);
        break;
    case NODE_CONSTANT:
        //TODO:
        //ret = LLVMConstInt(LLVMInt64TypeInContext(ctx), expr->constant->iVal, false);
        ret = codegen_constant(expr->constant, ctx);
        break;
    case NODE_VAR:
        //TODO:
        //ret = codegen_var(VariableNode *var, Scope *scope, LLVMBuilderRef builder, LLVMModuleRef mod)
        ret = codegen_var(expr->var, scope, builder, mod);
        break;
        //fatal("(CODEGEN) VAR NOT IMPLEMENTED YET\n");
    default:
        fatal("(CODEGEN) THIS VAR TYPE IS NOT SUPPORTED IN AN EXPR");
    }
    return ret;
}

Error codegen_vardecl(VarDeclNode* var_decl, Scope* scope, LLVMBuilderRef builder, LLVMModuleRef mod, LLVMContextRef ctx) {
    //lhs
    VariableNode* scoped_var = var_decl->lhs->var->scoped_var->var;
    printf("[DEBUG]: \t\tCODEGEN VARDECL\n");
    LLVMTypeRef type = codegen_get_var_type(var_decl->type, ctx);
    int n = calc_str_len(var_decl->lhs->var->identifier);
    char name[n];
    fill_char_array(name, var_decl->lhs->var->identifier, n);
    LLVMValueRef lhs = LLVMBuildAlloca(builder, type, name);
    //rhs
    LLVMValueRef rhs = codegen_expression(var_decl->rhs, scope, builder, mod, ctx);
    //LLVMBuildStore(LLVMBuilderRef, LLVMValueRef Val, LLVMValueRef Ptr)
    LLVMBuildStore(builder, rhs, lhs);

    scoped_var->llvm_value_ref = lhs;
    scoped_var->llvm_var_type = type;
    
    return OK;
}

Error codegen_return(ReturnNode* ret, Scope* scope, LLVMBuilderRef builder, LLVMModuleRef mod, LLVMContextRef ctx) {
    if (ret == NULL) {
        LLVMBuildRet(builder, NULL);
    } else {
        LLVMValueRef ret_expr = codegen_expression(ret->ret_expr, scope, builder, mod, ctx);
        LLVMBuildRet(builder, ret_expr);
    }
    return OK;
}

Error codegen_block(BlockNode* block, LLVMBuilderRef builder, LLVMModuleRef mod, LLVMContextRef ctx) {
    printf("[DEBUG]: \tCODEGEN BLOCK\n");
    Node* current_statement = block->statements->next;
    printf("[DEBUG]: Printing Scope names\n");
    print_nodes(block->scope->names);
    bool returned = false;
    while(current_statement != NULL) {
        switch(current_statement->nt) {
        // TODO: ALL
        case NODE_VAR_DECL:
            codegen_vardecl(current_statement->var_decl, block->scope, builder, mod, ctx);
            break;
        case NODE_BINOP:
            break;
        case NODE_VAR:
            // Skip - does nothing
            break;
        case NODE_CONSTANT:
            // Skip - does nothing
            break;
        case NODE_CALL:
            codegen_call(current_statement->call, block->scope, builder, mod);
            break;
        case NODE_RETURN:
            returned = true;
            codegen_return(current_statement->ret, block->scope, builder, mod, ctx);
            break;
        default:
            break;
        }
        current_statement = current_statement->next;
    }

    // TODO: Check if it needed a return, otherwise error
    if (!returned) {
        codegen_return(NULL, block->scope, builder, mod, ctx);
    }
    
    return OK;
}

LLVMTypeRef codegen_get_type(Token* id, LLVMContextRef ctx) {
    if (id == NULL) {
        return LLVMVoidTypeInContext(ctx);
    }
    char* beg = id->beg;
    char* end = id->end;
    int n = 1;
    while (beg != end) {
        beg++;
        n++;
    }
    if (strncmp(id->beg, "i8", n) == 0 && n == 2) {
        return LLVMInt8TypeInContext(ctx);
    } else if (strncmp(id->beg, "i16", n) == 0 && n == 3) {
        return LLVMInt16TypeInContext(ctx);
    } else if (strncmp(id->beg, "i32", n) == 0 && n == 3) {
        return LLVMInt32TypeInContext(ctx);
    } else if (strncmp(id->beg, "i64", n) == 0 && n == 3) {
        return LLVMInt64TypeInContext(ctx);
    } else if (strncmp(id->beg, "u8", n) == 0 && n == 2) {
        return LLVMInt8TypeInContext(ctx);
    } else if (strncmp(id->beg, "u16", n) == 0 && n == 3) {
        return LLVMInt16TypeInContext(ctx);
    } else if (strncmp(id->beg, "u32", n) == 0 && n == 3) {
        return LLVMInt32TypeInContext(ctx);
    } else if (strncmp(id->beg, "u64", n) == 0 && n == 3) {
        return LLVMInt64TypeInContext(ctx);
    } else if (strncmp(id->beg, "f32", n) == 0 && n == 3) {
        fatal("f32 Not supported yet");
        return NULL;
    } else if (strncmp(id->beg, "f64", n) == 0 && n == 3) {
        fatal("f64 Not supported yet");
        return NULL;
    } else if (strncmp(id->beg, "int", n) == 0 && n == 3) {
        return LLVMInt64TypeInContext(ctx);
    } else if (strncmp(id->beg, "float", n) == 0 && n == 5) {
        fatal("f64 Not supported yet");
        return NULL;
    }

    fatal("(CODEGEN) Invalid Type");
    return NULL;
}

Error codegen_function(FunctionNode* func_node, FunctionNode* func_scoped, LLVMModuleRef mod) {
    // func_scoped is the copied version of the node in the Scope
    LLVMContextRef ctx = LLVMGetModuleContext(mod);
    printf("[DEBUG]: CODEGEN FUNCTION\n");
    printf("[DEBUG]: ");
    LLVMTypeRef param_types[] = { LLVMVoidType() };
    int argc = 0;
    LLVMTypeRef ret = codegen_get_type(func_node->return_types, ctx);
    //LLVMTypeRef param_types[] = {};
    LLVMTypeRef fn_type = LLVMFunctionType(ret, param_types, argc, 0);
    int n = calc_str_len(func_node->name);
    char name[n];
    fill_char_array(name, func_node->name, n);
    LLVMValueRef fn = LLVMAddFunction(mod, name, fn_type);
    LLVMBasicBlockRef entry = LLVMAppendBasicBlockInContext(ctx, fn, "entry");
    LLVMBuilderRef builder = LLVMCreateBuilderInContext(ctx);
    LLVMPositionBuilderAtEnd(builder, entry);
    //TODO : not sure what this does lol
    LLVMSetLinkage(fn, LLVMExternalLinkage);

    codegen_block(func_node->block->block, builder, mod, ctx);

    func_scoped->llvm_fn_type = fn_type;
    func_scoped->llvm_value_ref = fn;
    
    return OK;
}

Error codegen_start(Node* ast) {
    //LLVMModuleRef mod = LLVMModuleCreateWithName("my_module");
    LLVMContextRef ctx = LLVMContextCreate();
    LLVMModuleRef mod = LLVMModuleCreateWithNameInContext("atlas_module", ctx);

    Node* current_node = ast->next;
    while(current_node != NULL) {
        switch(current_node->nt) {
        case NODE_FUNC:
            codegen_function(current_node->func,
                             current_node->func->scoped_node->func,
                             mod);
        default:
            //fatal("UNSUPPORTED NODE TYPE");
            printf("[DEBUG]: UNSUPPORTED NODE TYPE\n");
        }
        
        current_node = current_node->next;
    }

    char* errors;
    LLVMTargetRef target;
    printf("%s\n", LLVMPrintModuleToString(mod));
    LLVMPrintModuleToFile(mod, "module.ll", &errors);
    return OK;
}

Node* ast_create_variable(VarType type, Token* identifier) {
    Node* ret = calloc(1, sizeof(Node));
    VariableNode* _node = calloc(1, sizeof(VariableNode));
    ret->nt = NODE_VAR;
    _node->type = type;
    _node->identifier = identifier;
    ret->var = _node;
    return ret;
}

Node* ast_create_var_decl_node(Node* lhs, Node* rhs, VarType type) {
    Node* ret = calloc(1, sizeof(Node));
    VarDeclNode* _node = calloc(1, sizeof(VarDeclNode));
    _node->lhs = lhs;
    _node->rhs = rhs;
    _node->type = type;
    ret->var_decl = _node;
    ret->nt = NODE_VAR_DECL;
    return ret;
}

int get_prec(TokenType tt) {
    switch(tt) {
    case TK_STAR:
    case TK_SLASH:
        return 2;
    case TK_PLUS:
    case TK_DASH:
        return 1;
    case TK_NEWLINE:
        return -1;
    default:
        print_error_msg("Invalid Operator");
        exit(1);
    }
}

Node* ast_create_constant(Token* token) {
    Node* ret = calloc(1, sizeof(Node));
    ConstantNode* _node = calloc(1, sizeof(ConstantNode));
    _node->iVal = atoi(token->beg);
    ret->nt = NODE_CONSTANT;
    ret->constant = _node;
    return ret;
}

Node* ast_create_binop(Node* lhs, Node* rhs, Token* op) {
    Node* ret = calloc(1, sizeof(BinOpNode));
    BinOpNode* _node = calloc(1, sizeof(BinOpNode));
    _node->lhs = lhs;
    _node->rhs = rhs;
    _node->op = op;
    ret->binop = _node;
    ret->nt = NODE_BINOP;
    return ret;
}

Node* ast_create_call(Token** token_list) {
    Node* ret = calloc(1, sizeof(Node));
    CallNode* _node = calloc(1, sizeof(CallNode));
    _node->name = *token_list;
    *token_list = (*token_list)->next;
    ret->nt = NODE_CALL;
    Node* args = NULL;
    *token_list = (*token_list)->next; // Skip the open paren
    while ((*token_list)->tt != TK_PAREN_CLOSE) {
        if ((*token_list)->tt == TK_NEWLINE) {
           fatal("(AST_GEN) MISSING CLOSING PAREN\n");
        }
        ast_create_expression(token_list, true);
        *token_list = (*token_list)->next;
    }
    print_token(*token_list);
    *token_list = (*token_list)->next;// skip paren close
    print_token(*token_list);
    ret->call = _node;
    return ret;
}


Node* ast_create_expr_prec(Token** token_list, int precedence, bool is_args) {
    Node* lhs;
    if ((*token_list)->tt == TK_IDENTIFIER
        && (*token_list)->next->tt == TK_PAREN_OPEN) {
        // Function Call
        //*token_list = (*token_list)->next; // skip the paren
        lhs = ast_create_call(token_list);
    } else if ((*token_list)->tt == TK_IDENTIFIER) {
        // Variable
        lhs = ast_create_variable(TYPE_UNKNOWN, *token_list);
        //fatal("(AST GEN) Variables in expr not implemented yet\n");
    } else {
        // Constant TODO: handle quotes 
        lhs = ast_create_constant(*token_list);
    }
    Token* lookahead = (*token_list)->next; // operator is a lookahead
    Node* rhs = NULL;
    /*
    if ((*token_list)->tt == TK_NEWLINE) {
        print_error_msg("Expected an Expression but got a NEWLINE");
        exit(1);
    }
    */
    if ((*token_list)->tt != TK_NEWLINE) {
        while(get_prec(lookahead->tt) >= precedence) {
            *token_list = (*token_list)->next; // op
            Token* op = *token_list; // operator is a lookahead
            *token_list = (*token_list)->next; // rhs
            if ((*token_list)->tt == TK_CONSTANT) {
                rhs = ast_create_constant(*token_list);
            }
            lookahead = (*token_list)->next;
            // TODO: handle left associativity
            printf("LOOKAHEAD\n");
            print_token(lookahead);
            print_token(op);
            while (get_prec(lookahead->tt) >= get_prec(op->tt)) {
                if (get_prec(lookahead->tt) > get_prec(op->tt))
                    rhs = ast_create_expr_prec(token_list, get_prec(op->tt) + 1, false);
                else
                    rhs = ast_create_expr_prec(token_list, get_prec(op->tt), false);
                *token_list = (*token_list)->next;
                if ((*token_list)->tt == TK_NEWLINE || (*token_list)->tt == TK_COMMA) {
                    break;
                }
                lookahead = (*token_list)->next;
            }
            lhs = ast_create_binop(lhs, rhs, op);
            if ((*token_list)->tt == TK_NEWLINE ||
                (*token_list)->next->tt == TK_NEWLINE ||
                (*token_list)->tt == TK_COMMA) {
                break;
            }
        }
    }
    //print_node(lhs, 0);
    return lhs;
}

Node* ast_scope_add(Scope** scope, Node* node) {
    // adds function or var to scope
    //TODO: figure out if this is even useful
    Node* beg = (*scope)->names;
    if (beg == NULL) {
        (*scope)->names = calloc(1, sizeof(Node));
        memcpy((*scope)->names, node, sizeof(Node));
        return (*scope)->names;
    } else {
        while (beg->next != NULL) {
            beg = beg->next;
        }
        //beg->next = node;
        beg->next = calloc(1, sizeof(Node));
        memcpy(beg->next, node, sizeof(Node));
        return beg->next;
    }
}

void ast_add_scope(Scope** scope, Node* add) {
    Node* beg = (*scope)->names;
    if (beg == NULL) {
        (*scope)->names = add;
    } else {
        while (beg->next != NULL) {
            beg = beg->next;
        }
        beg->next = add;
    }
}

void ast_add_node(Node** ast, Node* add) {
    Node* beg = *ast;
    while (beg->next != NULL) {
        beg = beg->next;
    }
    beg->next = add;
}

Node* ast_create_return(Token** expr) {
    Node* ret = calloc(1, sizeof(Node));
    ret->nt = NODE_RETURN;
    ReturnNode* return_node = calloc(1, sizeof(Node));
    Node* ret_expr = ast_create_expression(expr, false);
    return_node->ret_expr = ret_expr;
    ret->ret = return_node;

    return ret;
}

Node* ast_create_block(Scope** parent_scope) {
    Node* ret = calloc(1, sizeof(Node));
    BlockNode* _node = calloc(1, sizeof(BlockNode));
    Scope* scope = calloc(1, sizeof(Scope));
    Node* statements_root = calloc(1, sizeof(Node));
    statements_root->nt = NODE_ROOT;
    
    _node->scope = scope;
    _node->statements = statements_root;
    scope->parent_scope = *parent_scope;
    ret->block = _node;
    ret->nt = NODE_BLOCK;
    return ret;
}

Node* ast_create_function(Token* name,
                          Node* args,
                          Token* return_types,
                          Node* block) {
    // TODO: args and return_types
    Node* ret = calloc(1, sizeof(Node));
    FunctionNode* _node = calloc(1, sizeof(FunctionNode));
    _node->name = name;
    _node->args = args;
    _node->block = block;
    _node->return_types = return_types;
    
    ret->nt = NODE_FUNC;
    ret->func = _node;
    return ret;
}

Node* ast_create_expression(Token** token_list, bool is_args) {
    return ast_create_expr_prec(token_list, 0, is_args);
}

Error ast_create(Token** token_list,
                 Node** ast,
                 Scope** scope,
                 bool is_block) {
    Token* current_token = *token_list;
    while(current_token != NULL) {
        if (current_token->tt == TK_NEWLINE) {
            // skip
        } else if (current_token->tt == TK_IDENTIFIER && !is_block) {
            Token* name = current_token;
            // Expect Function definition
            current_token = current_token->next;
            expect(current_token, TK_PAREN_OPEN);
            // TODO: HANDLE ARGUMENTS
            Node* args = NULL;
            current_token = current_token->next;
            expect(current_token, TK_PAREN_CLOSE);

            current_token = current_token->next;
            Token* func_ret = NULL;
            if (current_token->tt == TK_ARROW) {
                //TODO: handle RETURN TYPE
                current_token = current_token->next;
                expect(current_token, TK_IDENTIFIER);
                func_ret = current_token;
                current_token = current_token->next;
            }
            
            expect(current_token, TK_CURLY_OPEN);
            current_token = current_token->next;
            //TODO: BLOCK START
            Node* block = ast_create_block(scope);
            *token_list = current_token;
            ast_create(token_list,
                       &(block->block->statements),
                       &(block->block->scope),
                      true);
            // TODO: Constants
            current_token = *token_list;
            expect(current_token, TK_CURLY_CLOSE);
            Node* func = ast_create_function(name, args, func_ret, block);
            ast_add_node(ast, func);
            func->func->scoped_node = ast_scope_add(scope, func);
            //block_scope->parent_scope = *scope;
            //ast_add_scope(scope, func);
        }

        if (is_block) {
            Token* tmp = current_token; // saving the type or DOUBLE_C
            if(current_token->tt == TK_IDENTIFIER || tmp->tt == TK_DOUBLE_C) {
                // type var decl
                current_token = current_token->next;
                //expect(current_token, TK_DOUBLE_C);
                if (current_token->tt == TK_DOUBLE_C || tmp->tt == TK_DOUBLE_C) {
                    VarType type;
                    if (tmp->tt == TK_DOUBLE_C) {
                        type = TYPE_UNKNOWN;
                    } else {
                        type = get_var_type(tmp);
                        current_token = current_token->next;
                    }
                    expect(current_token, TK_IDENTIFIER);
                    Token* identifier = current_token;
                    Node* var = ast_create_variable(type, identifier);
                    //ast_add_scope(scope, var);
                    Node* scoped_var = ast_scope_add(scope, var);
                    var->var->scoped_var = scoped_var;
                    current_token = current_token->next;
                    expect(current_token, TK_ASSIGN);
                    // TODO: EXPRESSION NODE
                    current_token = current_token->next;
                    *token_list = current_token;
                    Node* expr = ast_create_expression(token_list, false);
                    if ((*token_list)->next->tt == TK_NEWLINE) {
                        *token_list = (*token_list)->next;
                    }
                    current_token = *token_list;
                    expect(current_token, TK_NEWLINE);
                    Node* var_decl = ast_create_var_decl_node(var, expr, type);
                    ast_add_node(ast, var_decl);
                    // Create VarDeclNode
                } else if (current_token->tt == TK_ASSIGN) {
                    // var reassignment
                    Token* name = tmp;
                } else if (current_token->tt == TK_PAREN_OPEN){ 
                    // Function Call, line is an expression
                    Token* name = tmp;
                    Node* expr = ast_create_expression(token_list, false);
                    if ((*token_list)->next->tt == TK_NEWLINE) {
                        *token_list = (*token_list)->next;
                    }
                    current_token = *token_list;
                    ast_add_node(ast, expr);
                }
            } else if (current_token->tt == TK_ARROW) {
                current_token = current_token->next;
                *token_list = current_token;
                Node* ret = ast_create_return(token_list);
                current_token = *token_list;
                ast_add_node(ast, ret);
            }
        }
        current_token = current_token->next;
        if (is_block) {
            *token_list = current_token;
        }
        if (current_token == NULL || current_token->tt == TK_CURLY_CLOSE) {
            break;
        }
    }
    return OK;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        print_usage();
        exit(0);
    }

    int count;
    Token* token_list = NULL;
    Token* token_head = token_list;
    char* src = read_src(argv[1], &count);
    lexer(src, count, &token_head, &token_list);

    Node* ast_root = calloc(1, sizeof(Node));
    ast_root->nt = NODE_ROOT;
    //print_tokens(token_list);
    Scope* root_scope = calloc(1, sizeof(Scope));
    ast_create(&token_list, &ast_root, &root_scope, false);
    printf("[DEBUG]: Printing Nodes...\n");
    print_nodes(ast_root->next);
    printf("[DEBUG]: Printing Root Scope...\n");
    print_nodes(root_scope->names);

    //codegen_start_help(ast_root);
    printf("[DEBUG]: STARTING CODEGEN...\n");
    codegen_start(ast_root);
    return 0;
}
