#include "driver.hpp"
#include <memory>

using namespace llvm;
extern ExitOnError ExitOnErr;
extern std::unique_ptr<orc::KaleidoscopeJIT> TheJIT;

// library functions that can be 'extern'd from user code


#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

extern "C" DLLEXPORT double putchard(double X) {
    fputc((char)X, stderr);
    return 0;
}

extern "C" DLLEXPORT double printd(double X) {
    fprintf(stderr, "%f\n", X);
    return 0;
}

int main() {
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();

    fprintf(stderr, ">>> ");
    getNextToken();

    TheJIT = ExitOnErr(orc::KaleidoscopeJIT::Create());

    InitializeModuleAndManagers();

    MainLoop();
    return 0;
}
