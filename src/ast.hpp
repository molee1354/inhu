#ifndef my_ast_hpp
#define my_ast_hpp
    
#include <memory>
#include <string>
#include <vector>

/**
 * @class ExprAst
 * @brief Base class for all expression nodes
 *
 */
class ExprAST{
public:
    virtual ~ExprAST() = default;
};

/**
 * @class NumberExprAST
 * @brief Expression class for referencing a number
 *
 */
class NumberExprAST : public ExprAST {
    double Val;
public: 
    NumberExprAST(double Val);
};

/**
 * @class VariableExprAST
 * @brief Expression class for referencing a variable
 *
 */
class VariableExprAST : public ExprAST {
    std::string Name;
public:
    VariableExprAST(const std::string &Name);
};

/**
 * @class BinaryExprAST
 * @brief Expression class for binary operations
 *
 */
class BinaryExprAST : public ExprAST {
    char Oper;
    std::unique_ptr<ExprAST> LHS, RHS;
public:
    BinaryExprAST(char Oper, std::unique_ptr<ExprAST> LHS,
                             std::unique_ptr<ExprAST> RHS);
};

/**
 * @class CallExprAST
 * @brief Expression class for function calls
 *
 */
class CallExprAST : public ExprAST {
    std::string Callee; // name of function called
    std::vector<std::unique_ptr<ExprAST>> Args;  // arguments

public:
    CallExprAST(const std::string &Callee,
                std::vector<std::unique_ptr<ExprAST>> Args);
};

/**
 * @class PrototypeAST
 * @brief Represents the prototype for a function. Captures the name and its
 * argument names
 *
 */
class PrototypeAST {
    std::string Name;
    std::vector<std::string> Args;

public:
    PrototypeAST(const std::string &Name, std::vector<std::string> Args);

    /* the 'const' after function name indicates that
     * the state of the object will not be changed by
     * calling this function. The function just gets the Name.
     */
    const std::string &getName() const;
};

/**
 * @class FunctionAST
 * @brief Class to represent a function definition
 *
 */
class FunctionAST {
    std::unique_ptr<PrototypeAST> Proto;
    std::unique_ptr<ExprAST> Body;

public:
    FunctionAST(std::unique_ptr<PrototypeAST> Proto,
                std::unique_ptr<ExprAST> Body);
};

#endif
