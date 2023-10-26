#include <string>
#include "lexer.hpp"

std::string IdentifierStr;
double NumVal;

int gettok() {
    static int LastChar = ' ';

    // whitespace
    while (isspace(LastChar)) 
        LastChar = getchar();

    // identifier strings
    if (isalpha(LastChar)) {
        IdentifierStr = LastChar;
        while ( isalnum(( LastChar = getchar() )) )
            IdentifierStr += LastChar;
        
        if (IdentifierStr == "def") {
            return token_def;
        }
        if (IdentifierStr == "extern") {
            return token_extern;
        }
        return token_identifier;
    }

    // numbers --> int and float
    if (isdigit(LastChar) || LastChar == '.') {
        std::string NumStr;
        do{
            NumStr += LastChar;
            LastChar = getchar();
        } while (isdigit(LastChar) || LastChar == '.');

        NumVal = strtod(NumStr.c_str(), nullptr);
        return token_number;
    }

    // comments
    if (LastChar == '#') {
        do
            LastChar = getchar();
        while (LastChar != EOF &&
               LastChar != '\n' && LastChar != '\r');
        if (LastChar != EOF)
            return gettok();

    }
    if (LastChar == EOF)
        return token_eof;

    int ThisChar = LastChar;
    LastChar = getchar();
    return ThisChar;
}
