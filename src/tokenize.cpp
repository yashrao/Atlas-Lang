#include <vector>
#include <iostream>
#include <fstream>

#include "error.hpp"
#include "tokenize.hpp"
#include "global.hpp"


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


void print_token(Token* token) {
    if (global_state->debug) {
        std::string message = std::string("") + "TOKEN" + "<" + std::string(get_tt_str(token->tt)) + "> " 
            + "(" + std::to_string(token->line) + "," + std::to_string(token->column) + ")" 
            + ": " + token->token + "\n";
        log_print(message);
    }
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
    } else if (word == "cinclude") {
        return TK_CINCLUDE;
    } else if (word == "fn") {
        return TK_FN;
    } else {
        return TK_IDENTIFIER;
    }
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
    } else if (tt == TK_CINCLUDE) {
        return "TK_CINCLUDE";
    } else if (tt == TK_DOT_CURLY) {
        return "TK_DOT_CURLY";
    } else {
        return "TK_INVALID";
    }
}
