#pragma once

#include <string>
#include <vector>

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
  TK_CINCLUDE,
  TK_PTR_DEREFERENCE,
  TK_DOT_CURLY,
};

struct Token {
    int line;
    int column;
    TokenType tt;
    std::string token;
};


std::vector<Token*> tokenize (std::string src);
TokenType get_tt(char c);
const char* get_tt_str(TokenType tt);
void print_token(Token* token);
void print_tokens (std::vector<Token*> tokens);
std::string read_file (std::string filename);
