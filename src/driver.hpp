#ifndef my_driver_hpp
#define my_driver_hpp

#include "parser.hpp"

/**
 * @brief Function to handle 'def'
 */
void HandleDefinition();

/**
 * @brief Function to handle 'extern'
 */
void HandleExtern();

/**
 * @brief Function to handle top-level expressions
 */
void HandleTopLevelExpression();

/**
 * @brief The main loop of the program
 */
void MainLoop();

#endif
