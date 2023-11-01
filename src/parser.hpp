#ifndef my_parser_hpp
#define my_parser_hpp

#include "lexer.hpp"
#include "ast.hpp"
#include <map>

extern int CurTok;
extern std::map<char, int> BinOpPrec;
extern std::map<std::string, std::unique_ptr<PrototypeAST>> FunctionProtos;


int getNextToken();
/**
 * @brief Function to be called for 'token_number' type tokens. Takes the 
 * current number value, creates a NumberExprAST node, and moves the lexer
 * to the next token.
 *
 * @return NumExprAST Node of the new result
 */
std::unique_ptr<ExprAST> ParseNumberExpr();

/**
 * @brief Function to deal with parenthetical expressions
 *
 * @return ExprAST node from parsing the expression inside the parenthesis
 */
std::unique_ptr<ExprAST> ParseParenExpr();

/**
 * @brief Function to parse an identifier expression. Deals with either a 
 * named variable or a function call
 *
 * @return ExprAST. VariableExprAST for variable, CallExprAST for func call
 */
std::unique_ptr<ExprAST> ParseIdentifierExpr();

/**
 * @brief Function to parse primary expressions, and run a valid parse 
 * function depending on the current token.
 */
std::unique_ptr<ExprAST> ParsePrimary();

/**
 * @brief Function to parse binary expressions
 *
 * @param ExprPrec Binary operator precedence
 * @param LHS LHS of the binary operation
 */
std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec,
                                       std::unique_ptr<ExprAST> LHS);

/**
 * @brief Function to parse function prototype
 *
 * @return PrototypeAST Node
 */
std::unique_ptr<PrototypeAST> ParsePrototype();

/**
 * @brief Function to parse a function definition
 *
 * @return FunctionAST Node
 */
std::unique_ptr<FunctionAST> ParseDefinition();

/**
 * @brief Function to parse an extern statement
 *
 * @return PrototypeAST Node
 */
std::unique_ptr<PrototypeAST> ParseExtern();

/**
 * @brief Function to parse arbitrary top-level functions
 *
 * @return FunctionAST Node
 */
std::unique_ptr<FunctionAST> ParseTopLevelExpr();

/**
 * @brief Function to parse an expression. An expression is a primary
 * expression followed by a sequance of binExpr or primaryExpr pairs
 *
 * @return 
 */
std::unique_ptr<ExprAST> ParseExpression();

#endif
