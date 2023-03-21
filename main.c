#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>

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
    TYPE_F8,
    TYPE_F16,
    TYPE_F32,
    TYPE_F64,
    TYPE_INVALID
} VarType;

typedef enum NodeType {
    NODE_FUNC = 200,
    NODE_BINOP,
    NODE_CONSTANT,
    NODE_VAR,
    NODE_BLOCK,
    NODE_ROOT
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

typedef struct BlockNode {
    
} BlockNode;

typedef struct FunctionNode {

} FunctionNode;

typedef struct Node {
    struct Node* next;
    NodeType nt;
    union {
        FunctionNode* func;
        BlockNode* block;
        AssignNode* assign;
        BinOpNode* binop;
        VariableNode* var;
        ConstantNode* constant;
    };
} Node;

static Error OK = (Error) {COMPILER, NULL};

void print_usage() {
    printf("OI, this is the usage\n");
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
    }
}

void print_error_msg(const char* error) {
    printf("%s[ERROR]:%s %s\n",
            CL_RED, CL_RESET,
            error);
}

void print_node(Node* node, int tab_level) {
    printf("[DEBUG]: ");
    print_tabs(tab_level);
    printf("%s└─%s", CL_YELLOW, CL_RESET);
    printf(" ");
    switch(node->nt) {
    case NODE_BINOP:
        printf("BinOpNode <%c>", *(node->binop->op->beg));
        printf("\n");
        print_node(node->binop->lhs, tab_level + 1);
        print_node(node->binop->rhs, tab_level + 1);
        break;
    case NODE_CONSTANT:
        printf("ConstantNode <%d>", node->constant->iVal);
        break;
    default:
        print_error_msg("Something has horribly gone wrong");
        break;
    }
    printf("\n");
}

void fatal(const char* string) {
    printf("%s", string);
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
        fatal("Could not open the file");
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
    } 

    return TYPE_INVALID;
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

Node* ast_create_expr_prec(Token** token_list, int precedence) {
    Node* lhs = ast_create_constant(*token_list);
    Token* lookahead = (*token_list)->next; // operator is a lookahead
    Node* rhs = NULL;
    if ((*token_list)->tt == TK_NEWLINE) {
        print_error_msg("Expected an Expression but got a NEWLINE");
        exit(1);
    }
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
                rhs = ast_create_expr_prec(token_list, get_prec(op->tt) + 1);
            else
                rhs = ast_create_expr_prec(token_list, get_prec(op->tt));
            *token_list = (*token_list)->next;
            if ((*token_list)->tt == TK_NEWLINE) {
                break;
            }
            lookahead = (*token_list)->next;
        }
        lhs = ast_create_binop(lhs, rhs, op);
        if ((*token_list)->tt == TK_NEWLINE || (*token_list)->next->tt == TK_NEWLINE) {
            break;
        }
    }
    print_node(lhs, 0);
    return lhs;
}

Node* ast_create_expression(Token** token_list) {
    return ast_create_expr_prec(token_list, 0);
}

Error ast_create(Token** token_list, Node** ast, bool is_block) {
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
            *token_list = current_token;
            ast_create(token_list, ast, true);
            // TODO: Constants
            current_token = *token_list;
            expect(current_token, TK_CURLY_CLOSE);
        }

        if (is_block) {
            if(current_token->tt == TK_IDENTIFIER) {
                // type var decl
                Token* tmp = current_token;
                VarType type = get_var_type(current_token);
                current_token = current_token->next;
                //expect(current_token, TK_DOUBLE_C);
                if (current_token->tt == TK_DOUBLE_C) {
                    // Var decl
                    current_token = current_token->next;
                    expect(current_token, TK_IDENTIFIER);
                    Token* identifier = current_token;
                    Node* var = ast_create_variable(type, identifier);
                    current_token = current_token->next;
                    expect(current_token, TK_ASSIGN);
                    // TODO: EXPRESSION NODE
                    current_token = current_token->next;
                    *token_list = current_token;
                    Node* expr = ast_create_expression(token_list);
                    if ((*token_list)->next->tt == TK_NEWLINE) {
                        *token_list = (*token_list)->next;
                    }
                    current_token = *token_list;
                    expect(current_token, TK_NEWLINE);
                    // Create VarDeclNode
                } else if (current_token->tt == TK_ASSIGN) {
                    // var reassignment
                    Token* name = tmp;
                }

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
    print_tokens(token_list);
    ast_create(&token_list, &ast_root, false);
}
