#pragma once

#include "tokenize.hpp"

enum ForType {
    FOR_WHILE = 0,
    FOR_EACH = 1, // not implemented yet
    FOR_LOOP = 2,
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
    NODE_CINCLUDE,
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
    bool is_const  = false;
    bool is_static = false;
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

struct CincludeNode : Node {
    Token* name;
};

struct ExpressionNode : Node {
    bool needs_paren = false;
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
        CincludeNode* cinclude;
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
    std::string mangled_name;
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
        CincludeNode* cinclude_lhs;
    };
    // RHS
    union {
        ExpressionNode* expr_rhs;
    };
};

struct Scope {
    struct Scope* prev;
    std::vector<struct VarNode*> names;
    std::vector<struct FunctionNode*> func_names;
};

struct BlockNode : Node {
    Scope* scope;
    std::vector<StatementNode*> statements;
};

void expect(Token* token, TokenType expected);
void print_tabs(int tab_level);
void print_node(Node* node, int tab_level);
void print_nodes(std::vector<Node*> nodes);
ParamNode* ast_create_param(Token* type,
                            Token* name,
                            bool is_array,
                            int ptr_level,
                            ExpressionNode* arr_size);
Token* next_token(std::vector<Token*> tokens, int* i);
std::vector<ParamNode*> ast_parse_params(std::vector<Token*> tokens, int* i);
ExpressionNode* ast_create_binop(ExpressionNode* lhs, ExpressionNode* rhs, Token* op);
ExpressionNode* ast_create_call(Token* name, std::vector<Token*> tokens, int* i);
StatementNode* ast_create_return(std::vector<Token*> tokens, int* i);
VarNode* ast_create_var(Token* identifier);
VarDeclNode* ast_create_var_decl(VarType type, Token* type_id, Token* identifier, ExpressionNode* rhs);
ExpressionNode* ast_create_variable_expr(VarType type, Token* identifier, int* i);
ExpressionNode* ast_create_constant(Token* constant_value);
ExpressionNode* ast_create_quote(Token* quote_token);
ExpressionNode* ast_create_char(Token* character);
int get_prec(TokenType tt);
Token* ast_get_lookahead(std::vector<Token*> tokens, int* i);
ExpressionNode* ast_create_array_decl(std::vector<Token*> tokens, int* i);
ExpressionNode* ast_create_subscript_node(std::vector<Token*> tokens, int* i);
ExpressionNode* ast_create_type_instantiation(std::vector<Token*> tokens, int* i);
ExpressionNode* ast_create_expr_prec(
        std::vector<Token*> tokens,
        int precedence,
        bool is_args,
        bool is_cond,
        bool is_arr,
        int* i);
ExpressionNode* ast_create_expression(std::vector<Token*> tokens, bool is_args, bool is_cond, bool is_arr, int* i);
VarType get_var_type(Token* var_type);
int ast_create_for_determine_for(std::vector<Token*> tokens, int* i);
StatementNode* ast_create_for(std::vector<Token*> tokens, int* i);
StatementNode* ast_create_if(std::vector<Token*> tokens, int* i);
BlockNode* ast_create_block(std::vector<Token*> tokens, int* i);
std::string ast_get_file_full_path(std::string filename);
TypeNode* ast_create_type_struct(std::vector<Token*> tokens, int* i);
void ast_name_mangler(FunctionNode* function);
FunctionNode* ast_create_function(std::vector<Token*> tokens, int* i);
bool is_var_decl(std::vector<Token*> tokens, int* i);
VarDeclNode* ast_handle_var_decl_lhs(std::vector<Token*> tokens, int* i);
VarDeclNode* ast_handle_var_decl(std::vector<Token*> tokens, int* i, bool has_atrs);
StatementNode* ast_create_declaration(std::vector<Token*> tokens, int* i);
std::vector<StatementNode*> ast_create(std::vector<Token*> tokens);
