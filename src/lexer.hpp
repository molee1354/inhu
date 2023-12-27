#ifndef my_lexer_hpp
#define my_lexer_hpp

#include <memory>

enum Token {
    token_eof = -1,

    token_def = -2,
    token_extern = -3,

    token_identifier = -4,
    token_number = -5,

    token_if = -6,
    token_then = -7,
    token_else = -8,

    token_for = -9,
    token_do = -10,

    token_binary = -11,
    token_unary = -12
};

extern std::string IdentifierStr;
extern double NumVal;

int gettok();

#endif
