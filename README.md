# INHU

INHU is an LLVM language frontend that is implemented based on this [tutorial](https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/index.html). Along with a simple, intuitive syntax INHU supports JIT compilation so that you can enjoy all the cool features of a compiled language *AND* have a REPL.

## Features and Syntax

INHU has a simple and intuitive syntax that should be relatively easy and familiar.

### Syntax

#### Semicolons

INHU was built with both single-line and multiline code syntax in mind. Because of that, semicolons are required only at the *very end* of statements, definitions, and/or expressions.

#### Functions

Functions in INHU are defined using the `def` and `as` keyword.

```python
def add_one(num) as num+1;
```

Here, we have a function named `foo` that takes in a single argument and returns a number value that is "+1" of the argument passed in.

INHU does not have a `return` keyword. Instead, the expression bottommost line (unless nested in a control-flow statement) will be computed and returned instead.

##### Extern Functions

You can call standard library functions by declaring them using the `extern` keyword.

```python
extern sin(theta);

sin(3.14);  # Evaluates to 0.00159
```

#### Conditionals

Conditionals in INHU are written as such:

```python
if condition then
    # do something
else if condition then
    # do something else
else
    # do something even more different ;
```

This is how a conditional would look inside a function:

```python
def is_less_than(left, right) as
    if left < right then
        1
    else
        0;
```

INHU does not have boolean support. Instead, `true` values evaluate to `1.0` and `false` values evaluate to `0.0`.

#### Loops

INHU only supports *for* loops, since *while* loops hold some disadvantages like readability, efficiency, etc. The syntax for *for* loops in INHU is as follows:

```python
for start, end, step do
    # do something
    ;
```

The `step` incrementor part of the function may be omitted, which then the compiler would default to an increment value of  `1.0`.

This is how a for-loop would look inside a function:

```python
def print_stars(num) as
    for i = 1, i < n, 1.0 do
        putchard(42); # ascii for '*'
```

### Unique Features

INHU syntax is very extensible. You can define your own unary/binary operators out of the box, which makes it so that you can implement your language in any way you want.

To define your own unary operator, you can use the predefined `unary` keyword function:

```python
# unary 'not'
def unary{!} (u) as if v then 0 else 1;
```

Binary operators can be defined similarly by using the `binary`. The novelty here is that you can set their precedence values as well!

```python
# binary 'and' 
def binary{&:6} (left, right) as
    if !LHS then
        0
    else
        !!RHS;
```

In both `unary` and `binary` operation definitions, inside the curly braces the first argument is the symbol itself. The following optional argument for `binary` definitions is its precedence value.

Here is a precedence values list for reference. A higher precedence means that the operation will be evaluated first.

| Operation | Symbol | Precedence |
| --- | :---: | :---: |
| Less-than | `<` | `10` |
| Addition | `+` | `20` |
| Subtraction | `-` | `20` |
| Division | `/` | `40` |
| Multiplication | `*` | `40` |

It should be noted that all precedence values must land between `1` and `100`.

## Setup

Setting up INHU on your machine is simple.

### Downloading the Precompiled Binary

You can download the precompiled binary [here](https://raw.githubusercontent.com/molee1354/inhu/master/bin/inhu).

There isn't a Windows executable at the moment because I've yet to overcome some skill issues when it comes to C++. It should come in due time.

### Building from Source

To build INHU from source, you should make sure that you have LLVM-17 ready. Depending on the system, the newest LLVM version available may be an older version than the required version. In that case, LLVM should be built from source.

#### Building LLVM and Clang from Source

More details on how to do this can be found [here](https://clang.llvm.org/get_started.html).

- Make sure the required software packages are present:
  - `CMake` (version >= 3.20.0)
  - `python` (version >= 3.6)
  - `zlib` (version >= 1.2.3.4)
  - `GNU Make` (version 3.47, 3.79.1)
- Clone the LLVM project

    ```shell
    git clone --depth=1 https://github.com/llvm/llvm-project.git 
    ```

    This will create a shallow clone, which should be enough for what we need it for.
- Build LLVM and Clang with `CMake` with the following commands

    ```shell
    cd llvm-project
    mkdir build
    cd build
    cmake -DLLVM_ENABLE_PROJECTS=clang -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles" ../llvm
    ```
  
    This generates the build files for LLVM and Clang in release mode. After that is done, run either

    ```shell
    make
    ```

    to build normally or

    ```shell
    make $(nproc)
    ```

    to build with multiple cores.

This should take quite a while, but once that is done, you should be ready to build INHU from source.

#### Building INHU from Source

To build INHU from source, simply run

```shell
make
```

in the terminal and the INHU binary should compile to the `./bin/` directory.
