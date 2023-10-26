#include "ast.hpp"

/**
 * @brief NumberExprAST constructor definition
 *
 * @param Val 
 */
NumberExprAST::NumberExprAST(double Val)
    : Val(Val) {}

/**
 * @brief VariableExprAST constructor definition
 *
 * @param Name 
 */
VariableExprAST::VariableExprAST(const std::string &Name)
    : Name(Name) {}

/**
 * @brief BinaryExprAST constructor definition
 *
 * @param Oper 
 * @param LHS 
 * @param RHS 
 */
BinaryExprAST::BinaryExprAST(char Oper,
                             std::unique_ptr<ExprAST> LHS,
                             std::unique_ptr<ExprAST> RHS) 
    : Oper(Oper) , LHS(std::move(LHS)), RHS(std::move(RHS)) {}

/**
 * @brief CallExprAST constructor definition
 *
 * @param Callee 
 * @param Args 
 */
CallExprAST::CallExprAST(const std::string &Callee,
                         std::vector<std::unique_ptr<ExprAST>> Args)
    : Callee(Callee), Args(std::move(Args)) {}

/**
 * @brief PrototypeAST constructor definition
 *
 * @param Name 
 * @param Args 
 */
PrototypeAST::PrototypeAST(const std::string &Name,
                           std::vector<std::string> Args)
    : Name(Name) , Args(std::move(Args)) {}

/**
 * @brief Getting the name of the function prototype
 *
 * @return std::string & Name of the prototype
 */
const std::string &PrototypeAST::getName() const { return Name; }

/**
 * @brief FunctionAST constructor definition
 *
 * @param PrototypeAST 
 * @param Proto 
 * @param Body 
 */
FunctionAST::FunctionAST(std::unique_ptr<PrototypeAST> Proto,
                         std::unique_ptr<ExprAST> Body)
    : Proto(std::move(Proto)), Body(std::move(Body)) {}

