#include "driver.hpp"

int main() {
    fprintf(stderr, ">>> ");
    getNextToken();

    MainLoop();
    return 0;
}
