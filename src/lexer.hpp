#ifndef my_lexer_hpp
#define my_lexer_hpp

#include <memory>

enum Token {
    token_eof        = -1,

    token_def        = -2,
    token_extern     = -3,
    token_as         = -4,

    token_identifier = -5,
    token_number     = -6,

    token_if         = -7,
    token_then       = -8,
    token_else       = -9,

    token_for        = -10,
    token_do         = -11,

    token_binary     = -12,
    token_unary      = -13
};

extern std::string IdentifierStr;
extern double NumVal;

int gettok();

#endif
