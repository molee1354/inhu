#include "driver.hpp"

void HandleDefinition() {
    if (ParseDefinition()) {
        fprintf(stderr, "Parsed a function definition.\n");
    } else {
        // skip token for error recovery
        getNextToken();
    }
}

void HandleExtern() {
    if (ParseExtern()) {
        fprintf(stderr, "Parsed an extern\n");
    } else {
        // skip token for error recovery
        getNextToken();
    }
}

void HandleTopLevelExpression() {
    if (ParseTopLevelExpr()) {
        fprintf(stderr, "Parsed a top-level expression\n");
    } else {
        // skip token for error recovery
        getNextToken();
    }
}

void MainLoop() {
    while (true) {
        fprintf(stderr, ">>> ");
        switch (CurTok) {
            case token_eof:
                return;
            case ';': // ignore top-level semicolons
                getNextToken();
                break;
            case token_def:
                HandleDefinition();
                break;
            case token_extern:
                HandleExtern();
                break;
            default:
                HandleTopLevelExpression();
                break;
        }
    }
}
