# ExpL / OExpL Compiler

This repository contains my stage-wise work on building an **ExpL compiler**, extended with object-oriented features in **OExpL**, from Stage 0 through Stage 8.

The implementation was developed by following the **EXPL NIT-C roadmap and documentation**. These resources were used as the learning roadmap and technical reference; they are not part of this repository.

## Project Overview

The project builds a compiler incrementally, starting with code generation for arithmetic expressions and progressing to functions, user-defined types, objects, inheritance, and subtype polymorphism.

## Stage-wise Implementation

| Stage | Topic | Main concepts |
|---|---|---|
| 0 | Installation | Initial setup and understanding of the compiler environment |
| 1 | Code Generation for Arithmetic Expressions | Translating arithmetic expressions into target code |
| 2 | Introduction to Static Storage Allocation | Static memory allocation and variable storage |
| 3 | Adding Flow Control Statements | Conditional statements, loops, and control-flow code generation |
| 4 | User-Defined Variables and Arrays | User-defined variables, arrays, and their storage/access |
| 5 | Adding Functions | Function definitions, calls, arguments, and activation records |
| 6 | User-Defined Types and Dynamic Memory Allocation | User-defined types, type-related operations, and dynamic allocation |
| 7 | Adding Objects — Data Encapsulation | Classes, objects, fields, methods, and encapsulation |
| 8 | Inheritance and Sub-type Polymorphism | Inheritance, method overriding, virtual function tables, and dynamic method dispatch |

## Repository Contents

- Stage-wise compiler implementation for Stages 0–8.
- Related source files, test programs, and notes maintained as part of my implementation.
- No copy of the original EXPL/XSM environment or third-party environment components.

The exact build and run procedure depends on the environment and tools used for the corresponding stage. Refer to the official installation and simulator documentation to set up a compatible environment separately.

## Building and Running

The compiler uses **LEX/Flex** and **YACC/Bison**. Install the required tools first (Debian/Ubuntu):

```bash
sudo apt update
sudo apt install flex bison build-essential
```

### 1. Build the compiler

Open the directory for the stage you want to build. For stages that contain a LEX (`.l`) file and a YACC (`.y`) file, run the following commands, replacing the example filenames with the actual filenames in that stage:

```bash
cd path/to/Stage_8

# Generate the parser and token header
yacc -d parser.y

# Generate the lexical analyzer
lex lexer.l

# Compile both generated C files into the compiler executable
gcc lex.yy.c y.tab.c -o compiler
```

Run `yacc` before `lex` when the LEX file includes `y.tab.h`. If a stage uses different source filenames or a different build setup, adjust the commands accordingly.

### 2. Build the label translator

From Stage 3 onward, the generated XSM assembly may contain symbolic labels such as `L0`, `MAIN`, and function labels. These labels must be translated into instruction addresses before the XSM program is executed. The label translator is a separate LEX program. In the directory containing your label-translation source file (shown here as `label.l`), run:

```bash
# Generate the label translator's C source
lex label.l

# Compile the generated C source into an executable
gcc lex.yy.c -o label
```

The official label-translation documentation describes this as a two-pass process: first build a label-to-address table, then replace labels in `JMP`, `JZ`, `JNZ`, and `CALL` instructions with their corresponding addresses. The implementation details and file names depend on your `label.l` program.

### 3. Compile an ExpL/OExpL program

For a compiler that reads its source program from standard input:

```bash
./compiler < program.expl
```

The generated XSM file name and location depend on the output settings in that stage's implementation. For stages that generate code containing symbolic labels, keep that output as an intermediate file (for example, `intermediate.xsm`) until label translation is complete.

### 4. Translate labels and run the generated XSM code

If the label translator reads from standard input and writes the translated code to standard output, run it as follows (replace the example file names with the names used by your implementation):

```bash
./label < intermediate.xsm > final.xsm
```

After translation, `final.xsm` should contain numeric target addresses instead of symbolic labels. If your `label.l` program opens input/output files by name inside its own code, run `./label` according to that implementation instead of using input/output redirection. Stages that do not generate symbolic labels do not need this step.

After label translation (when required), run the generated XSM code from the XSM environment directory. First install and build the XSM simulator separately, and make sure the required runtime library is available locally in that environment:

```bash
./xsm -l library1.lib -e final.xsm
```

The XSM simulator can be built from its own environment directory with `make`. The XFS interface is initialized separately:

```bash
cd path/to/xsm_expl
make
cd ../xfs-interface
./init
```

> **Note:** The commands above illustrate the usual LEX/YACC and XSM workflow. File names, generated output paths, and runtime-library requirements can vary between stages. The simulator is not included in this repository; obtain them separately and follow their respective license terms.

## Environment and Third-Party Files

The EXPL/XSM environment, simulator, supplied libraries, and other third-party setup files are **not included** in this repository. They are intentionally omitted because their redistribution is subject to separate licensing and permission requirements. Please obtain any required components from their official sources and follow the license terms that apply to each component.

## References and Acknowledgements

This work follows the learning roadmap and uses the technical documentation provided by **SILCNITC, Department of Computer Science and Engineering, National Institute of Technology Calicut**.

- **Roadmap:** [EXPL NIT-C — Roadmap](https://silcnitc.github.io/expl-docs/roadmap/)
- **Documentation:** [EXPL NIT-C — Documentation](https://silcnitc.github.io/expl-docs/)
- **License information:** [EXPL NIT-C — License](https://silcnitc.github.io/expl-docs/about/license/)

According to the official license page, SILCNITC material by Dr. Murali Krishnan K is licensed under the **Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0)** license. Any material from those resources that is reproduced or adapted must be attributed and used in accordance with that license. This attribution does not change the license of the original material or grant additional permissions for third-party files.