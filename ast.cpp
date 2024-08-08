#include <vector>
#include <string>
#include <iostream>

#include "global.hpp"
#include "error.hpp"
#include "ast.hpp"
#include "tokenize.hpp"

void expect(Token* token, TokenType expected) {
    if (token->tt != expected) {
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
        Token* name = current_token;
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
        Token* type = current_token;
        current_token = next_token(tokens, i);
        //Token* name = current_token;
        //current_token = next_token(tokens, i);
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
    case TK_PTR_DEREFERENCE:
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
    } else if (current_token->tt == TK_PAREN_OPEN) {
        current_token = next_token(tokens, i);
        //lhs = ast_create_expr_prec(tokens, get_prec(TK_PAREN_OPEN),
        //                           false, false, false, i);
        lhs = ast_create_expression(tokens, false, false, false, i);
        lhs->needs_paren = true;
        current_token = next_token(tokens, i);
        expect(current_token, TK_PAREN_CLOSE);
        //current_token = next_token(tokens, i);
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
    } else if (is_arr && (current_token->tt == TK_COMMA)
               || (lookahead->tt == TK_COMMA)) {
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
    print_token(current_token);
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
        current_token = tokens[*i];
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
    current_token = tokens[*i];
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
        } else if (tt == TK_CINCLUDE) {
            current_token = next_token(tokens, i); // skip cinclude token
            // Current token should be a string
            expect(current_token, TK_QUOTE);
            StatementNode* statement = new StatementNode;
            CincludeNode* cinclude = new CincludeNode;
            cinclude->name = current_token;

            statement->nt = NODE_CINCLUDE;
            statement->cinclude_lhs = cinclude;
            statements.push_back(statement);
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
    expect(current_token, TK_TYPE); // type keyword
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
        Token* name = current_token;
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
        Token* type = current_token;
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

void ast_name_mangler(FunctionNode* function) {
    std::string og_name = function->token->token;

    std::string type;
    if (function->return_types == NULL) {
        type = "v";
    } else if (function->return_types->token == "i64") {
        type = "x";
    } else if (function->return_types->token == "u64") {
        type = "y";
    } else if (function->return_types->token == "i32") {
        type = "i";
    } else if (function->return_types->token == "u32") {
        type = "j";
    } else if (function->return_types->token == "i16") {
        type = "s";
    } else if (function->return_types->token == "u16") {
        type = "t";
    } else if (function->return_types->token == "i8") {
        type = "Dh";
    } else if (function->return_types->token == "u8") {
        type = "h";
    } else {
        type = std::to_string(function->return_types->token.length())
            + function->return_types->token;
    }
    
    function->mangled_name = "Z_" + std::to_string(og_name.length())
                             + og_name + type;
}

FunctionNode* ast_create_function(std::vector<Token*> tokens, int* i) {
    FunctionNode* ret = new FunctionNode;

    // start with name
    Token* current_token = tokens[*i];
    Token* name = current_token;
    ret->nt = NODE_FUNC;
    current_token = next_token(tokens, i);
    expect(current_token, TK_FN); 
    current_token = next_token(tokens, i); // skip fn
    
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
    if(ret->token->token != "main") {
        ast_name_mangler(ret);
    } else {
        ret->mangled_name = "main";
    }
    function_table.push_back(ret);
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
    expect(current_token, TK_DOUBLE_C);
    current_token = next_token(tokens, i); // skip DOUBLE_C
    Token* id = current_token;
    expect(current_token, TK_IDENTIFIER);
    bool is_array = false;
    int ptr_level = 0;
    ExpressionNode* arr_size = NULL; // incase the var is an array
    current_token = next_token(tokens, i); // skip DOUBLE_C
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
    Token* type = current_token;
    current_token = next_token(tokens, i);
    VarDeclNode* lhs = ast_create_var_decl(get_var_type(type), type, id, NULL);
    lhs->lhs->ptr_level = ptr_level;
    lhs->lhs->is_array = is_array;
    lhs->lhs->arr_size = arr_size;
    return lhs;
}

VarDeclNode* ast_handle_var_decl(std::vector<Token*> tokens, int* i, bool has_atrs) {
    Token* current_token = tokens[*i]; // update token
    bool is_static, is_const = false;
    if(has_atrs) {
        // collect attributes first
        while(current_token->tt != TK_DOUBLE_C) {
            if (current_token->tt == TK_COMMA) {
                current_token = next_token(tokens, i);
                continue;
            }
            expect(current_token, TK_IDENTIFIER);
            if (current_token->token == "static") {
                is_static = true;
            } else if (current_token->token == "const") {
                is_const = true;
            } else {
                std::string err = "Invalid attribute: " + current_token->token;
                print_error_msg(err);
                exit(1);
            }
            current_token = next_token(tokens, i);
        }
    }
    VarDeclNode* lhs = ast_handle_var_decl_lhs(tokens, i);
    lhs->is_static = is_static;
    lhs->is_const = is_const;
    current_token = tokens[*i]; // update token
    expect(current_token, TK_ASSIGN);
    current_token = next_token(tokens, i);
    ExpressionNode* rhs = ast_create_expression(tokens, false, false, false, i);
    lhs->rhs = rhs;
    if (lhs->lhs->is_array && rhs->nt == NODE_SUBSCRIPT) {
        rhs->subscript->is_declaration = true;
    }
    return lhs;
}

StatementNode* ast_create_declaration(std::vector<Token*> tokens, int* i) {
    StatementNode* stmt = new StatementNode;
    // assume starts at the beginning of the line
    for (; *i < tokens.size(); (*i)++) {
        Token* current_token = tokens[*i];
        TokenType tt = tokens[*i]->tt;
        int save_beg = *i; // incase its an expression
        if(tt == TK_DOUBLE_C) {
            // var_decl
            VarDeclNode* var_decl = ast_handle_var_decl(tokens, i, false);
            StatementNode* statement = new StatementNode;
            statement->nt = NODE_VAR_DECL;
            statement->vardecl_lhs = var_decl;
            statement->expr_rhs = var_decl->rhs;
            return statement;
        } else if (tt == TK_CINCLUDE) {
            current_token = next_token(tokens, i); // skip cinclude token
            // Current token should be a string
            expect(current_token, TK_QUOTE);
            StatementNode* statement = new StatementNode;
            CincludeNode* cinclude = new CincludeNode;
            cinclude->name = current_token;

            statement->nt = NODE_CINCLUDE;
            statement->cinclude_lhs = cinclude;
            return statement;
        } else if (current_token->tt == TK_IDENTIFIER) {
            Token* lookahead = ast_get_lookahead(tokens, i);
            if (lookahead->tt == TK_COMMA || lookahead->tt == TK_DOUBLE_C) {
                // var_decl
                VarDeclNode* var_decl = ast_handle_var_decl(tokens, i, true);
                StatementNode* statement = new StatementNode;
                statement->nt = NODE_VAR_DECL;
                statement->vardecl_lhs = var_decl;
                statement->expr_rhs = var_decl->rhs;
                return statement;
            } else if (lookahead->tt == TK_FN) {
                // Function
                FunctionNode* fn = ast_create_function(tokens, i);
                StatementNode* stmt = new StatementNode;
                stmt->nt = NODE_FUNC;
                stmt->func_lhs = fn;
                return stmt;
            } else if (lookahead->tt == TK_TYPE) {
                // Type struct
                //TODO
                TypeNode* type_struct = ast_create_type_struct(tokens, i);
                StatementNode* stmt = new StatementNode;
                stmt->nt = NODE_TYPE;
                stmt->type_lhs = type_struct;
                return stmt;
            } else {
                // Expression
                // TODO: probably need to fix this later
                StatementNode* statement = new StatementNode;
                ExpressionNode* stmt_expr = ast_create_expression(tokens, false, false, false, i);
                statement->nt = stmt_expr->nt;
                statement->expr_lhs = stmt_expr;
                return statement;
            }
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
            //TODO:
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
