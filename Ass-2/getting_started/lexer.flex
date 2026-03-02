%top{
    #include "parser.tab.hh"
    #define YY_DECL yy::parser::symbol_type yylex()
    #include "Node.h"
    
    int lexical_errors = 0; 
}
%option yylineno noyywrap nounput batch noinput stack 
%%

 /* Keywords */
"class"                 { if(USE_LEX_ONLY) printf("CLASS "); else return yy::parser::make_CLASS(); }
"main"                  { if(USE_LEX_ONLY) printf("MAIN "); else return yy::parser::make_MAIN(); }
"return"                { if(USE_LEX_ONLY) printf("RETURN "); else return yy::parser::make_RETURN(); }
"if"                    { if(USE_LEX_ONLY) printf("IF "); else return yy::parser::make_IF(); }
"else"                  { if(USE_LEX_ONLY) printf("ELSE "); else return yy::parser::make_ELSE(); }
"for"                   { if(USE_LEX_ONLY) printf("FOR "); else return yy::parser::make_FOR(); }
"break"                 { if(USE_LEX_ONLY) printf("BREAK "); else return yy::parser::make_BREAK(); }
"continue"              { if(USE_LEX_ONLY) printf("CONTINUE "); else return yy::parser::make_CONTINUE(); }
"print"                 { if(USE_LEX_ONLY) printf("PRINT "); else return yy::parser::make_PRINT(); }
"read"                  { if(USE_LEX_ONLY) printf("READ "); else return yy::parser::make_READ(); }
"length"                { if(USE_LEX_ONLY) printf("LENGTH "); else return yy::parser::make_LENGTH(); }
"void"                  { if(USE_LEX_ONLY) printf("VOID_TYPE "); else return yy::parser::make_VOID_TYPE(); }

 /* Types */
"int"                   { if(USE_LEX_ONLY) printf("INT_TYPE "); else return yy::parser::make_INT_TYPE(); }
"float"                 { if(USE_LEX_ONLY) printf("FLOAT_TYPE "); else return yy::parser::make_FLOAT_TYPE(); }
"boolean"               { if(USE_LEX_ONLY) printf("BOOL_TYPE "); else return yy::parser::make_BOOL_TYPE(); }
"volatile"              { if(USE_LEX_ONLY) printf("VOLATILE "); else return yy::parser::make_VOLATILE(); }

 /* Boolean literals */
"true"                  { if(USE_LEX_ONLY) printf("TRUE "); else return yy::parser::make_TRUE(); }
"false"                 { if(USE_LEX_ONLY) printf("FALSE "); else return yy::parser::make_FALSE(); }

 /* Multi-char operators (must come before single-char) */
":="                    { if(USE_LEX_ONLY) printf("ASSIGN "); else return yy::parser::make_ASSIGN(); }
"<="                    { if(USE_LEX_ONLY) printf("LE "); else return yy::parser::make_LE(); }
">="                    { if(USE_LEX_ONLY) printf("GE "); else return yy::parser::make_GE(); }
"!="                    { if(USE_LEX_ONLY) printf("NE "); else return yy::parser::make_NE(); }
"=="                    { if(USE_LEX_ONLY) printf("EQ "); else return yy::parser::make_EQ(); }

 /* Single-char operators */
"="                     { if(USE_LEX_ONLY) printf("EQ "); else return yy::parser::make_EQ(); }
"+"                     { if(USE_LEX_ONLY) printf("PLUSOP "); else return yy::parser::make_PLUSOP(); }
"-"                     { if(USE_LEX_ONLY) printf("MINUSOP "); else return yy::parser::make_MINUSOP(); }
"*"                     { if(USE_LEX_ONLY) printf("MULTOP "); else return yy::parser::make_MULTOP(); }
"/"                     { if(USE_LEX_ONLY) printf("DIVOP "); else return yy::parser::make_DIVOP(); }
"^"                     { if(USE_LEX_ONLY) printf("POW "); else return yy::parser::make_POW(); }
"<"                     { if(USE_LEX_ONLY) printf("LT "); else return yy::parser::make_LT(); }
">"                     { if(USE_LEX_ONLY) printf("GT "); else return yy::parser::make_GT(); }
"&&"                    { if(USE_LEX_ONLY) printf("AND "); else return yy::parser::make_AND(); }
"&"                     { if(USE_LEX_ONLY) printf("AND "); else return yy::parser::make_AND(); }
"||"                    { if(USE_LEX_ONLY) printf("OR "); else return yy::parser::make_OR(); }
"|"                     { if(USE_LEX_ONLY) printf("OR "); else return yy::parser::make_OR(); }
"!"                     { if(USE_LEX_ONLY) printf("NOT "); else return yy::parser::make_NOT(); }

 /* Delimiters */
"("                     { if(USE_LEX_ONLY) printf("LP "); else return yy::parser::make_LP(); }
")"                     { if(USE_LEX_ONLY) printf("RP "); else return yy::parser::make_RP(); }
"{"                     { if(USE_LEX_ONLY) printf("LBRACE "); else return yy::parser::make_LBRACE(); }
"}"                     { if(USE_LEX_ONLY) printf("RBRACE "); else return yy::parser::make_RBRACE(); }
"["                     { if(USE_LEX_ONLY) printf("LBRACKET "); else return yy::parser::make_LBRACKET(); }
"]"                     { if(USE_LEX_ONLY) printf("RBRACKET "); else return yy::parser::make_RBRACKET(); }
","                     { if(USE_LEX_ONLY) printf("COMMA "); else return yy::parser::make_COMMA(); }
"."                     { if(USE_LEX_ONLY) printf("DOT "); else return yy::parser::make_DOT(); }
":"                     { if(USE_LEX_ONLY) printf("COLON "); else return yy::parser::make_COLON(); }
";"                     { if(USE_LEX_ONLY) printf("SEMI "); else return yy::parser::make_SEMI(); }

 /* Newline = statement terminator in C+- */
\n                      { if(USE_LEX_ONLY) printf("NEWLINE\n"); else return yy::parser::make_NEWLINE(); }

 /* Literals */
[0-9]+\.[0-9]+          { if(USE_LEX_ONLY) printf("FLOAT(%s) ", yytext); else return yy::parser::make_FLOAT(yytext); }
[0-9]+                  { if(USE_LEX_ONLY) printf("INT(%s) ", yytext); else return yy::parser::make_INT(yytext); }

 /* Identifiers (after keywords so keywords match first) */
[a-zA-Z_][a-zA-Z0-9_]* { if(USE_LEX_ONLY) printf("ID(%s) ", yytext); else return yy::parser::make_ID(yytext); }

 /* Whitespace (ignored) */
[ \t\r]+                {}

 /* Single-line comments */
"//"[^\n]*              {}

 /* Lexical errors: skip bad character and continue */
. { 
    lexical_errors = 1; 
    fprintf(stderr, "Lexical error at line %d: Unrecognized character '%s'\n", yylineno, yytext); 
}

 /* End of file */
<<EOF>>                 { if(USE_LEX_ONLY) printf("EOF\n"); return yy::parser::make_END(); }
%%