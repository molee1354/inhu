#include "ast.hpp"
#include "parser.hpp"
#include <llvm/IR/Instructions.h>

using namespace llvm;

// Variables for LLVM code generation
std::unique_ptr<LLVMContext> TheContext;
std::unique_ptr<Module> TheModule;
std::unique_ptr<IRBuilder<>> Builder;
std::map<std::string, Value*> NamedValues;

std::unique_ptr<llvm::FunctionPassManager> TheFPM;
std::unique_ptr<llvm::FunctionAnalysisManager> TheFAM;
std::unique_ptr<llvm::ModuleAnalysisManager> TheMAM;
std::unique_ptr<llvm::PassInstrumentationCallbacks> ThePIC;
std::unique_ptr<llvm::StandardInstrumentations> TheSI;

std::map<std::string, std::unique_ptr<PrototypeAST>> FunctionProtos;

/*
 * Helper functions for error handling
 */
std::unique_ptr<ExprAST> LogError(const char* msg) {
    fprintf(stderr, "Error: %s\n", msg);
    return nullptr;
}

std::unique_ptr<PrototypeAST> LogErrorP(const char* msg) {
    LogError(msg);
    return nullptr;
}

Value* LogErrorV(const char* msg) {
    LogError(msg);
    return nullptr;
}

static Function* getFunction(std::string Name) {
    // see if function was added to current module
    if (auto *F = TheModule->getFunction(Name))
        return F;

    // check if we can codegen decl from existing prototype
    auto FI = FunctionProtos.find(Name);
    if (FI != FunctionProtos.end())
        return FI->second->codegen();

    return nullptr;
}

/**
 * @brief NumberExprAST constructor definition
 *
 * @param Val 
 */
NumberExprAST::NumberExprAST(double Val)
    : Val(Val) {}

/**
 * @brief NumberExprAST Codegen
 *
 * @return ConstantFP
 */
Value* NumberExprAST::codegen() {
    /* APFloat(Val) --> can hold floating point constant
     * arbitrary precision
     */
    return ConstantFP::get(*TheContext, APFloat(Val));
}

/**
 * @brief VariableExprAST constructor definition
 *
 * @param Name
 */
VariableExprAST::VariableExprAST(const std::string &Name)
    : Name(Name) {}

/**
 * @brief VariableExprAST codegen
 *
 * @return Value*
 */
Value* VariableExprAST::codegen() {
    Value* V = NamedValues[Name];
    if (!V)
        return LogErrorV("Unknown variable name");
    return V;
}

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
 * @brief BinaryExprAST codegen definition
 *
 * @return Value*
 */
Value* BinaryExprAST::codegen() {
    // recursively emit code for LHS and then RHS
    Value* L = LHS->codegen();
    Value* R = RHS->codegen();

    if (!L || !R)
        return nullptr;
    
    switch (Oper) {
        case '+':
            return Builder->CreateFAdd(L, R, "addtmp");
        case '-':
            return Builder->CreateFSub(L, R, "subtmp");
        case '*':
            return Builder->CreateFMul(L, R, "multmp");
        case '/':
            return Builder->CreateFDiv(L, R, "divtmp");
        case '<':
            L = Builder->CreateFCmpULT(L, R, "cmptmp");
            return Builder->CreateUIToFP(L,
                    Type::getDoubleTy(*TheContext), "booltmp");
        default:
            break;
    }
    // if not a builtin oper, then look for a user-defined one
    Function *F = getFunction(std::string("binary") + Oper);
    assert(F && "Binary Operator not found!");

    Value* Ops[2] = {L, R};
    return Builder->CreateCall(F, Ops, "binop");
}

/**
 * @brief UnaryExprAST constructor definition
 *
 * @param OpCode 
 * @param Operand 
 */
UnaryExprAST::UnaryExprAST(char OpCode, std::unique_ptr<ExprAST> Operand)
    : OpCode(OpCode), Operand(std::move(Operand)) {}

/**
 * @brief Codegen implementation for UnarExprAST
 *
 * @return 
 */
Value* UnaryExprAST::codegen() {
    Value* OperandV = Operand->codegen();
    if (!OperandV)
        return nullptr;

    Function *F = getFunction(std::string("unary") + OpCode);
    if (!F)
        return LogErrorV("Unknown unary operator");

    return Builder->CreateCall(F, OperandV, "unop");
}

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
 * @brief CallExprAST codegen
 *
 * @return Value*
 */
Value* CallExprAST::codegen() {
    // look up name in global module table
    Function* CalleeF = getFunction(Callee);
    if (!CalleeF)
        return LogErrorV("Unknown function referred");

    if (CalleeF->arg_size() != Args.size())
        return LogErrorV("Incorrect number of arguments passed");

    std::vector<Value*> ArgsV;
    for (unsigned i = 0, e = Args.size(); i != e; ++i) {
        ArgsV.push_back(Args[i]->codegen());
        if (!ArgsV.back())
            return nullptr;
    }
    return Builder->CreateCall(CalleeF, ArgsV, "calltmp");
}

/**
 * @brief 'If' expression constructor
 *
 * @param ExprAST 
 * @param Cond 
 * @param Then 
 * @param Else 
 */
IfExprAST::IfExprAST(std::unique_ptr<ExprAST> Cond,
              std::unique_ptr<ExprAST> Then,
              std::unique_ptr<ExprAST> Else)
    : Cond(std::move(Cond)), Then(std::move(Then)), Else(std::move(Else)) {}

/**
 * @brief 'If' expression codegen() implementation
 *
 * @return 
 */
Value* IfExprAST::codegen() {
    Value* CondV = Cond->codegen();
    if (!CondV)
        return nullptr;

    CondV = Builder->CreateFCmpONE(CondV,
                                   ConstantFP::get(*TheContext, APFloat(0.0)),
                                   "ifcond");
    Function* TheFunction = Builder->GetInsertBlock()->getParent();

    BasicBlock* ThenBB = BasicBlock::Create(*TheContext, "then", TheFunction);
    BasicBlock* ElseBB = BasicBlock::Create(*TheContext, "else");
    BasicBlock* MergeBB = BasicBlock::Create(*TheContext, "ifcont");

    Builder->CreateCondBr(CondV, ThenBB, ElseBB);

    Builder->SetInsertPoint(ThenBB);
    Value* ThenV = Then->codegen();
    if (!ThenV)
        return nullptr;

    Builder->CreateBr(MergeBB);
    ThenBB = Builder->GetInsertBlock();

    TheFunction->insert(TheFunction->end(), ElseBB);
    Builder->SetInsertPoint(ElseBB);

    Value* ElseV = Else->codegen();
    if (!ElseV)
        return nullptr;

    Builder->CreateBr(MergeBB);
    ElseBB = Builder->GetInsertBlock();

    TheFunction->insert(TheFunction->end(), MergeBB);
    Builder->SetInsertPoint(MergeBB);
    PHINode* PN = Builder->CreatePHI(Type::getDoubleTy(*TheContext), 2, "iftmp");
    PN->addIncoming(ThenV,ThenBB);
    PN->addIncoming(ElseV,ElseBB);
    return PN;
}

/**
 * @brief 'for' expression constructor
 *
 * @param VarName 
 * @param Start 
 * @param End 
 * @param Step 
 * @param Body 
 */
ForExprAST::ForExprAST(const std::string &VarName,
                       std::unique_ptr<ExprAST> Start,
                       std::unique_ptr<ExprAST> End,
                       std::unique_ptr<ExprAST> Step,
                       std::unique_ptr<ExprAST> Body)
    : VarName(VarName), Start(std::move(Start)), End(std::move(End)),
      Step(std::move(Step)), Body(std::move(Body)) {}

Value* ForExprAST::codegen() {
    Value* StartVal = Start->codegen();
    if (!StartVal)
        return nullptr;

    // create new basic block for loop header
    Function* TheFunction = Builder->GetInsertBlock()->getParent();
    BasicBlock* PreheaderBB = Builder->GetInsertBlock();
    BasicBlock* LoopBB = BasicBlock::Create(*TheContext, "loop", TheFunction);

    // set explicit fallthrough from current block to the loopBB
    Builder->CreateBr(LoopBB);
    Builder->SetInsertPoint(LoopBB);

    // PHI node
    PHINode* Variable = Builder->CreatePHI(Type::getDoubleTy(*TheContext), 2, VarName);
    Variable->addIncoming(StartVal, PreheaderBB);

    // variable == phi node in the loop. If loop variable is already an existing
    // variable, save it to 'OldVal'
    Value* OldVal = NamedValues[VarName];
    NamedValues[VarName] = Variable;

    // emit loop body
    if (!Body->codegen()) 
        return nullptr;

    // emit step value
    Value* StepVal = nullptr;
    if (Step) {
        StepVal = Step->codegen();
        if (!StepVal)
            return nullptr;
    } else {
        StepVal = ConstantFP::get(*TheContext, APFloat(1.0));
    }

    Value* NextVar = Builder->CreateFAdd(Variable, StepVal, "nextvar");

    // computing the end condition
    Value* EndCond = End->codegen();
    if (!EndCond)
        return nullptr;

    // convert cond to bool by comparing neq to 0.0
    EndCond = Builder->CreateFCmpONE(
            EndCond, ConstantFP::get(*TheContext, APFloat(0.0)), "loopcond");
    
    // creating an 'after loop' block and inserting it
    BasicBlock* LoopEndBB = Builder->GetInsertBlock();
    BasicBlock* AfterBB =
        BasicBlock::Create(*TheContext, "afterloop", TheFunction);

    // insert conditional branch into the end of LoopEndBB
    Builder->CreateCondBr(EndCond, LoopBB, AfterBB);

    // setting insert point @ AfterBB s.t. new code will be inserted there
    Builder->SetInsertPoint(AfterBB);

    Variable->addIncoming(NextVar, LoopEndBB);

    // restore 'OldVar'
    if (OldVal)
        NamedValues[VarName] = OldVal;
    else
        NamedValues.erase(VarName);

    // for expr always returns 0.0
    return Constant::getNullValue(Type::getDoubleTy(*TheContext));
}
/**
 * @brief PrototypeAST constructor definition
 *
 * @param Name 
 * @param Args 
 */
PrototypeAST::PrototypeAST(const std::string &Name,
                           std::vector<std::string> Args,
                           bool IsOperator,
                           unsigned Prec)
    : Name(Name) , Args(std::move(Args)),
      IsOperator(IsOperator), Precedence(Prec) {}

/**
 * @brief PrototypeAST codegen
 *
 * @return Function*
 */
Function* PrototypeAST::codegen() {
    // make function type: int(int, int) ...
    std::vector<Type*> Doubles(Args.size(), Type::getDoubleTy(*TheContext));
    FunctionType* FuncType = FunctionType::get(Type::getDoubleTy(*TheContext),
                                               Doubles, false);
    Function* Func = Function::Create(FuncType,
                                      Function::ExternalLinkage,
                                      Name, TheModule.get());

    // set names for all arguments
    unsigned Idx = 0;
    for (auto &Arg : Func->args())
        Arg.setName(Args[Idx++]);
    
    return Func;
}

/**
 * @brief Getting the name of the function prototype
 *
 * @return std::string & Name of the prototype
 */
const std::string &PrototypeAST::getName() const { return Name; }

/**
 * @brief Function to determine if an operator is unary
 *
 * @return bool True if operator is unary
 */
bool PrototypeAST::isUnaryOp() const {
    return IsOperator && Args.size() == 1;
}

/**
 * @brief Function to determine if an operator is binary
 *
 * @return bool True if operator is binary
 */
bool PrototypeAST::isBinaryOp() const {
    return IsOperator && Args.size() == 2;
}

/**
 * @brief Function to get the name of the operator
 *
 * @return char Character of operator
 */
char PrototypeAST::getOperatorName() const {
    assert(isUnaryOp() || isBinaryOp());
    return Name[Name.size()-1];
}

/**
 * @brief Function to get the operator precedence
 *
 * @return unsigned Comparable number value of operator
 */
unsigned PrototypeAST::getBinaryPrecedence() const { return Precedence; }

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

Function* FunctionAST::codegen() {
    auto &P = *Proto;
    FunctionProtos[Proto->getName()] = std::move(Proto);
    Function* TheFunction = getFunction(P.getName());

    if (!TheFunction)
        return nullptr;

    // if binary operator, create precedence entry
    if (P.isBinaryOp())
        BinOpPrec[P.getOperatorName()] = P.getBinaryPrecedence();

    // create a new basic block to start insertion into
    BasicBlock* BBlock = BasicBlock::Create(*TheContext, "entry", TheFunction);
    Builder->SetInsertPoint(BBlock);

    // record the function arguments in the NamedValues map
    NamedValues.clear();
    for (auto &Arg : TheFunction->args())
        NamedValues[std::string(Arg.getName())] = &Arg;

    if (Value* RetVal = Body->codegen()) {
        // finish function
        Builder->CreateRet(RetVal);

        // validate generated code
        verifyFunction(*TheFunction);
        TheFPM->run(*TheFunction, *TheFAM);
        return TheFunction;
    }

    // error case reading body -> remove function
    TheFunction->eraseFromParent();
    return nullptr;
}
