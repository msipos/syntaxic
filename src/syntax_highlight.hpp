#ifndef SYNTAXIC_SYNTAX_HIGHLIGHT_HPP
#define SYNTAXIC_SYNTAX_HIGHLIGHT_HPP

#define SH_NONE 0

/** Identifier. */
#define SH_IDENT 1

/** Reserved keyword. */
#define SH_KEYWORD 2

/** Builtin types, values, methods, etc. */
#define SH_OTHER_KEYWORD 3

/** Operator. */
#define SH_OPERATOR 4

/** Preprocessor. */
#define SH_PREPROCESSOR 5

/** Important identifier (let's say, maybe a class or function name). */
#define SH_IMPORTANT_IDENT 6

/** String. */
#define SH_STRING 7

/** Number. */
#define SH_NUMBER 8

/** Comment. */
#define SH_COMMENT 9

/** Error. */
#define SH_ERROR 10

#endif