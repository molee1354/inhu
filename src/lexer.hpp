#ifndef my_lexer_hpp
#define my_lexer_hpp

#include <memory>

enum Token {
    token_eof = -1,

    token_def = -2,
    token_extern = -3,

    token_identifier = -4,
    token_number = -5,
};

extern std::string IdentifierStr;
extern double NumVal;

int gettok();

#endif
