#include "driver.hpp"
#include <memory>

using namespace llvm;

extern std::unique_ptr<LLVMContext> TheContext;
extern std::unique_ptr<Module> TheModule;
extern std::unique_ptr<IRBuilder<>> Builder;
extern std::map<std::string, Value*> NamedValues;

ExitOnError ExitOnErr;
std::unique_ptr<orc::KaleidoscopeJIT> TheJIT;

void InitializeModuleAndManagers() {
    TheContext = std::make_unique<LLVMContext>();
    TheModule = std::make_unique<Module>("My JIT", *TheContext);
    TheModule->setDataLayout(TheJIT->getDataLayout());

    // creating a new builder for the module
    Builder = std::make_unique<IRBuilder<>>(*TheContext);

    // creating new pass and analysis managers
    TheFPM = std::make_unique<FunctionPassManager>();
    TheFAM = std::make_unique<FunctionAnalysisManager>();
    TheMAM = std::make_unique<ModuleAnalysisManager>();
    ThePIC = std::make_unique<PassInstrumentationCallbacks>();
    TheSI = std::make_unique<StandardInstrumentations>(*TheContext, true);

    // adding transform passes
    TheFPM->addPass(InstCombinePass()); // 'peephole' optimizations
    TheFPM->addPass(ReassociatePass()); // reassociate expr
    TheFPM->addPass(GVNPass());         // eliminate common subexpressions
    TheFPM->addPass(SimplifyCFGPass()); // simplify control flow graph

    // register analysis passes used in the transforming passes
    TheFAM->registerPass([&] {return AAManager(); });
    TheFAM->registerPass([&] {return AssumptionAnalysis(); });
    TheFAM->registerPass([&] {return DominatorTreeAnalysis(); });
    TheFAM->registerPass([&] {return LoopAnalysis(); });
    TheFAM->registerPass([&] {return MemoryDependenceAnalysis(); });
    TheFAM->registerPass([&] {return MemorySSAAnalysis(); });
    TheFAM->registerPass([&] {return OptimizationRemarkEmitterAnalysis(); });
    TheFAM->registerPass([&] {
            return OuterAnalysisManagerProxy<ModuleAnalysisManager, Function>(*TheMAM); 
    });
    TheFAM->registerPass([&] {return TargetIRAnalysis(); });
    TheFAM->registerPass([&] {return TargetLibraryAnalysis(); });
    TheMAM->registerPass([&] {return ProfileSummaryAnalysis(); });
}

void HandleDefinition() {
  if (auto FnAST = ParseDefinition()) {
    if (auto *FnIR = FnAST->codegen()) {
      fprintf(stderr, "Read function definition:");
      FnIR->print(errs());
      fprintf(stderr, "\n");
      ExitOnErr(TheJIT->addModule(
                  orc::ThreadSafeModule(std::move(TheModule),
                                        std::move(TheContext))));
      InitializeModuleAndManagers();
    }
  } else {
    // Skip token for error recovery.
    getNextToken();
  }
}

void HandleExtern() {
  if (auto ProtoAST = ParseExtern()) {
    if (auto *FnIR = ProtoAST->codegen()) {
      fprintf(stderr, "Read extern: ");
      FnIR->print(errs());
      fprintf(stderr, "\n");
      FunctionProtos[ProtoAST->getName()] = std::move(ProtoAST);
    }
  } else {
    // Skip token for error recovery.
    getNextToken();
  }
}

void HandleTopLevelExpression() {
// Evaluate a top-level expression into an anonymous function.
    if (auto FnAST = ParseTopLevelExpr()) {
        if (FnAST->codegen()) {

            auto RT = TheJIT->getMainJITDylib().createResourceTracker();
            auto TSM = orc::ThreadSafeModule(std::move(TheModule), std::move(TheContext));

            ExitOnErr(TheJIT->addModule(std::move(TSM), RT));
            InitializeModuleAndManagers();

            auto ExprSymbol = ExitOnErr(TheJIT->lookup("__anon_expr"));

            double (*FP)() = ExprSymbol.getAddress().toPtr<double (*)()>();
            fprintf(stderr, "Evaluated to %f\n", FP());
            ExitOnErr(RT->remove());
        }
    } else {
        // Skip token for error recovery.
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
