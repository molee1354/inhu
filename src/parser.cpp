#include <cctype>
#include <cstdio>
#include <memory>
#include <vector>
#include "parser.hpp"

using namespace llvm;

/*
 * CurTok - the current token the parser is looking at
 * getNextToken() - reads another token from the lexer and updates
 * CurTok with its results.
 */
int CurTok;
extern double NumVal;

int getNextToken() {
    return CurTok = gettok();
}

/**
 * @brief Map to hold the precedence of the supported binary operators
 */
  // Install standard binary operators.
  // 1 is lowest precedence.
std::map<char, int> BinOpPrec = {
    { '<', 10 },
    { '+', 20 },
    { '-', 20 },
    { '/', 40 },
    { '*', 40 }
};

/**
 * @brief Function to get the precedence of a binary operation
 *
 * @return int A comparable precedence value from the BinOpPrecedence map
 */
static int GetTokenPrecedence() {
    if (!isascii(CurTok)) return -1;

    int TokenPrec = BinOpPrec[CurTok];
    if (TokenPrec <= 0) return -1;
    return TokenPrec;
}

std::unique_ptr<ExprAST> ParseNumberExpr() {
    auto Result = std::make_unique<NumberExprAST>(NumVal);
    getNextToken();
    return std::move(Result);
}

std::unique_ptr<ExprAST> ParseParenExpr() {
    getNextToken(); // eat left paren

    // recursive : ParseExpression calls ParseParenExpr
    auto V = ParseExpression(); 
    if (!V) 
        return nullptr;

    if (CurTok != ')')
        return LogError("Expected ')'");
    getNextToken(); // eat right paren
    return V;
}

std::unique_ptr<ExprAST> ParseIdentifierExpr() {
    std::string IdName = IdentifierStr;

    getNextToken(); // eat identifier

    // if it's a variable call
    if (CurTok != '(')
        return std::make_unique<VariableExprAST>(IdName);

    // dealing with function calls
    getNextToken();
    std::vector<std::unique_ptr<ExprAST>> Args;
    if (CurTok != ')') {
        while (true) {
            if (auto Arg = ParseExpression()) 
                Args.push_back(std::move(Arg));
            else
                return nullptr;

            // end args list
            if (CurTok == ')')
                break;

            // looking for comma separators between args
            if (CurTok != ',')
                return LogError("Expected ')' or ',' in argument list");
            getNextToken();
        }
    }
    getNextToken(); // eat )
    return std::make_unique<CallExprAST>(IdName, std::move(Args));
}

std::unique_ptr<ExprAST> ParsePrimary() {
    switch (CurTok) {
        default:
            return LogError("Unknown token when expecting an expression.");
        case token_identifier:
            return ParseIdentifierExpr();
        case token_number:
            return ParseNumberExpr();
        case '(':
            return ParseParenExpr();
    }
}

std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec,
                                       std::unique_ptr<ExprAST> LHS) {
    while (true) {
        int TokenPrec = GetTokenPrecedence();

        // if token precedence for CurTok is lesser than the expression at hand,
        // then there's nothing to do
        if (TokenPrec < ExprPrec)
            return LHS;

        int BinOp = CurTok;
        getNextToken();
        auto RHS = ParsePrimary();
        if (!RHS)
            return nullptr;

        // If BinOp binds less tightly with RHS than the operator after RHS, let
        // the pending operator take RHS as its LHS.
        int NextTokenPrec = GetTokenPrecedence();
        if (TokenPrec < NextTokenPrec) {
            RHS = ParseBinOpRHS(TokenPrec+1, std::move(RHS));
            if (!RHS)
                return nullptr;
        }

        // merge LHS, RHS
        LHS = std::make_unique<BinaryExprAST>(BinOp,
                                              std::move(LHS),
                                              std::move(RHS));
    }
}

std::unique_ptr<PrototypeAST> ParsePrototype() {
    if (CurTok != token_identifier)
        return LogErrorP("Expected function name in prototype");

    std::string FnName = IdentifierStr;
    getNextToken();

    if (CurTok != '(')
        return LogErrorP("Expected '(' in prototype");

    // read argsname list
    std::vector<std::string> ArgNames;
    while (getNextToken() == token_identifier)
        ArgNames.push_back(IdentifierStr);
    if (CurTok != ')')
        return LogErrorP("Expected ')' in prototype");
    // success
    getNextToken(); // eat ')'
    return std::make_unique<PrototypeAST>(FnName, std::move(ArgNames));
}

std::unique_ptr<FunctionAST> ParseDefinition() {
    getNextToken(); // eat function keyword
    auto Proto = ParsePrototype();
    if (!Proto)
        return nullptr;

    if (auto E = ParseExpression())
        return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
    return nullptr;
}

std::unique_ptr<PrototypeAST> ParseExtern() {
    getNextToken(); // just eat token
    return ParsePrototype();
}

std::unique_ptr<FunctionAST> ParseTopLevelExpr() {
    if (auto E = ParseExpression()) {
        // make anonymous Proto
        auto Proto = std::make_unique<PrototypeAST>("__anon_expr",
                                                    std::vector<std::string>());
        return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
    }
    return nullptr;
}

std::unique_ptr<ExprAST> ParseExpression() {
    auto LHS = ParsePrimary();
    if (!LHS)
        return nullptr;
    return ParseBinOpRHS(0, std::move(LHS));
}

