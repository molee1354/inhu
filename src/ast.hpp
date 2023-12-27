#ifndef my_ast_hpp
#define my_ast_hpp
    
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "llvm_headers.hpp"

extern std::unique_ptr<llvm::LLVMContext> TheContext;
extern std::unique_ptr<llvm::Module> TheModule;
extern std::unique_ptr<llvm::IRBuilder<>> Builder;
extern std::map<std::string, llvm::Value*> NamedValues;

extern std::unique_ptr<llvm::FunctionPassManager> TheFPM;
extern std::unique_ptr<llvm::FunctionAnalysisManager> TheFAM;
extern std::unique_ptr<llvm::ModuleAnalysisManager> TheMAM;
extern std::unique_ptr<llvm::PassInstrumentationCallbacks> ThePIC;
extern std::unique_ptr<llvm::StandardInstrumentations> TheSI;

/**
 * @class ExprAst
 * @brief Base class for all expression nodes
 *
 */
class ExprAST{
public:
    virtual ~ExprAST() = default;
    virtual llvm::Value* codegen() = 0; // emit IR for that node
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
    llvm::Value* codegen() override;
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
    llvm::Value* codegen() override;
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
    llvm::Value* codegen() override;
};

/**
 * @class UnaryExprAST
 * @brief Expression class for unary operations
 *
 */
class UnaryExprAST : public ExprAST {
    char OpCode;
    std::unique_ptr<ExprAST> Operand;
public:
    UnaryExprAST(char OpCode, std::unique_ptr<ExprAST> Operand);
    llvm::Value* codegen() override;
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
    llvm::Value* codegen() override;
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
    bool IsOperator;
    unsigned Precedence;

public:
    PrototypeAST(const std::string &Name, std::vector<std::string> Args,
                 bool isOperator = false, unsigned Prec = 0);

    /* the 'const' after function name indicates that
     * the state of the object will not be changed by
     * calling this function. The function just gets the Name.
     */
    llvm::Function* codegen();
    const std::string &getName() const;

    bool isUnaryOp() const;
    bool isBinaryOp() const;
    char getOperatorName() const;
    unsigned getBinaryPrecedence() const;
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
    llvm::Function* codegen();
};

/**
 * @class IfExprAST
 * @brief Class to represent a 'if' conditional expression
 *
 */
class IfExprAST: public ExprAST {
    std::unique_ptr<ExprAST> Cond, Then, Else;
public:
    IfExprAST(std::unique_ptr<ExprAST> Cond,
              std::unique_ptr<ExprAST> Then,
              std::unique_ptr<ExprAST> Else);

    llvm::Value* codegen() override;
};

/**
 * @class ForExprAST
 * @brief Class to represent a 'for' loop expression
 *
 */
class ForExprAST: public ExprAST {
    std::string VarName;
    std::unique_ptr<ExprAST> Start, End, Step, Body;

public:
    ForExprAST(const std::string &VarName,
               std::unique_ptr<ExprAST> Start,
               std::unique_ptr<ExprAST> End,
               std::unique_ptr<ExprAST> Step,
               std::unique_ptr<ExprAST> Body);
    llvm::Value* codegen() override;
};

/**
 * @brief Function to display log errors for expression nodes
 *
 * @param msg Error message
 */
std::unique_ptr<ExprAST> LogError(const char* msg);

/**
 * @brief Function to display log errors for Prototype nodes
 *
 * @param msg Error message
 */
std::unique_ptr<PrototypeAST> LogErrorP(const char* msg);

/**
 * @brief Function to log errors for Value nodes
 *
 * @param msg Error message
 */
llvm::Value* LogErrorV(const char* msg);

#endif
