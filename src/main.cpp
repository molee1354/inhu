#include "driver.hpp"

int main() {
    fprintf(stderr, ">>> ");
    getNextToken();

    InitializeModule();

    MainLoop();
    TheModule->print(llvm::errs(), nullptr);
    return 0;
}
