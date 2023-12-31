#include <cctype>
#include <cstdio>
#include <llvm/ADT/StringExtras.h>
#include <memory>
#include <vector>
#include "ast.hpp"
#include "lexer.hpp"
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
        case token_if:
            return ParseIfExpr();
        case token_for:
            return ParseForExpr();
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

        // this is a binop
        int BinOp = CurTok;
        getNextToken();
        auto RHS = ParseUnary(); // parse unary oper after binop
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

std::unique_ptr<ExprAST> ParseUnary() {
    // if current token is not an oper, then it is a primary expr
    if (!isascii(CurTok) || CurTok == '(' || CurTok == ',')
        return ParsePrimary();

    // reading a unary oper
    int OpChar = CurTok;
    getNextToken();
    if (auto Operand = ParseUnary())
        return std::make_unique<UnaryExprAST>(OpChar, std::move(Operand));
    return nullptr;
}

std::unique_ptr<PrototypeAST> ParsePrototype(bool isExtern) {
    std::string FnName;

    unsigned Kind = 0; // 0 -> identifier, 1 -> unary, 2 -> binary
    unsigned BinaryPrecedence = 30;

    switch (CurTok) {
        default:
            return LogErrorP("Expected function name in prototype");
        case token_identifier:
            FnName = IdentifierStr;
            Kind = 0;
            getNextToken();
            break;
        case token_unary:
            int UnaryToken;
            getNextToken();
            if (CurTok != '{')
                return LogErrorP("Expected opening '{' for unary definitions");
            getNextToken();
            if (!isascii(CurTok))
                return LogErrorP("Expected unary operator");
            else
                UnaryToken = CurTok;
            getNextToken();
            if (CurTok != '}')
                return LogErrorP("Expected closing '}' for unary definitions");
            FnName = "unary";
            FnName += (char)UnaryToken;
            Kind = 1;
            getNextToken();
            break;
        case token_binary:
            int BinaryToken;
            getNextToken();
            if (CurTok != '{')
                return LogErrorP("Expected opening '{' for binary definitions");
            getNextToken();
            if (!isascii(CurTok))
                return LogErrorP("Expected binary operator");
            else
                BinaryToken = CurTok;
            getNextToken();
            if (CurTok == ':') {
                getNextToken();
                if (CurTok != token_number)
                    return LogErrorP("Expected precdence number after ':'");
                if (NumVal < 1 || NumVal > 100)
                    return LogErrorP("Invalid Precedence: precedence must be between 1 and 100");
                BinaryPrecedence = (unsigned)NumVal;
                getNextToken();
            }
            if (CurTok != '}')
                return LogErrorP("Expected closing '}' for binary definitions");
            FnName = "binary";
            FnName += (char)BinaryToken;
            Kind = 2;
            getNextToken();

            break;
    }

    if (CurTok != '(')
        return LogErrorP("Expected '(' in prototype");

    // read argsname list
    std::vector<std::string> ArgNames;
    /* while (getNextToken() == token_identifier) {
        ArgNames.push_back(IdentifierStr);
    } */
    if (CurTok != ')') {
        do {
            getNextToken();
            ArgNames.push_back(IdentifierStr);
        } while (getNextToken() == ',');
    }

    if (CurTok != ')')
        return LogErrorP("Expected ')' in prototype");

    // success
    getNextToken(); // eat ')'

    if (!isExtern) {
        if (CurTok != token_as) {
            return LogErrorP("Expected 'as' after prototype");
        }
        getNextToken(); // eat 'as'
    }

    // verify right number of names for oper
    if (Kind && ArgNames.size() != Kind)
        return LogErrorP("Invalid number of operands for operator");

    return std::make_unique<PrototypeAST>(FnName, std::move(ArgNames),
                                          Kind != 0, BinaryPrecedence);
}

std::unique_ptr<FunctionAST> ParseDefinition() {
    getNextToken(); // eat function keyword
    auto Proto = ParsePrototype(false);
    if (!Proto)
        return nullptr;

    if (auto E = ParseExpression())
        return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
    return nullptr;
}

std::unique_ptr<ExprAST> ParseIfExpr() {
    getNextToken();
    auto Cond = ParseExpression();
    if (!Cond)
        return nullptr;

    if (CurTok != token_then)
        return LogError("Expected 'then'");
    getNextToken();

    auto Then = ParseExpression();
    if (!Then)
        return nullptr;

    if (CurTok != token_else)
        return LogError("Expected 'else'");

    getNextToken();
    auto Else = ParseExpression();
    if (!Else)
        return nullptr;
    return std::make_unique<IfExprAST>(std::move(Cond), std::move(Then),
                                       std::move(Else));
}

std::unique_ptr<ExprAST> ParseForExpr() {
    getNextToken(); // eat for
    
    if (CurTok != token_identifier)
        return LogError("Expected identifier after 'for'");

    std::string IdName = IdentifierStr;
    getNextToken(); // get identifier
    
    if (CurTok != '=')
        return LogError("Expected variable assignment in 'for' loop");
    getNextToken();

    auto Start = ParseExpression();
    if (!Start)
        return nullptr;
    if (CurTok != ',')
        return LogError("Expected ',' after 'for' loop variable");
    getNextToken();

    auto End = ParseExpression();
    if (!End)
        return nullptr;

    // optional step value
    std::unique_ptr<ExprAST> Step;
    if (CurTok == ',') {
        getNextToken();
        Step = ParseExpression();
        if (!Step)
            return nullptr;
    }

    if (CurTok != token_do)
        return LogError("Expected 'do' after 'for'");
    getNextToken();

    auto Body = ParseExpression();
    if (!Body)
        return nullptr;

    return std::make_unique<ForExprAST>(IdName, std::move(Start),
                                        std::move(End), std::move(Step),
                                        std::move(Body));
}

std::unique_ptr<PrototypeAST> ParseExtern() {
    getNextToken(); // just eat token
    return ParsePrototype(true);
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
    auto LHS = ParseUnary();
    if (!LHS)
        return nullptr;
    return ParseBinOpRHS(0, std::move(LHS));
}

