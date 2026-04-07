%top{
    #include "parser.tab.hh"
    #define YY_DECL yy::parser::symbol_type yylex()
    #include "Node.h"
    extern int yylineno;
    extern int yyleng;

    int lexical_errors = 0; 

    static yy::location make_loc() {
        yy::location loc;
        loc.begin.line = yylineno;
        loc.begin.column = 1;
        loc.end.line = yylineno;
        loc.end.column = yyleng > 0 ? yyleng : 1;
        return loc;
    }
}
%option yylineno noyywrap nounput batch noinput stack 
%%

 /* Keywords */
"class"                 { if(USE_LEX_ONLY) printf("CLASS "); else return yy::parser::make_CLASS(make_loc()); }
"main"                  { if(USE_LEX_ONLY) printf("MAIN "); else return yy::parser::make_MAIN(make_loc()); }
"return"                { if(USE_LEX_ONLY) printf("RETURN "); else return yy::parser::make_RETURN(make_loc()); }
"if"                    { if(USE_LEX_ONLY) printf("IF "); else return yy::parser::make_IF(make_loc()); }
"else"                  { if(USE_LEX_ONLY) printf("ELSE "); else return yy::parser::make_ELSE(make_loc()); }
"for"                   { if(USE_LEX_ONLY) printf("FOR "); else return yy::parser::make_FOR(make_loc()); }
"break"                 { if(USE_LEX_ONLY) printf("BREAK "); else return yy::parser::make_BREAK(make_loc()); }
"continue"              { if(USE_LEX_ONLY) printf("CONTINUE "); else return yy::parser::make_CONTINUE(make_loc()); }
"print"                 { if(USE_LEX_ONLY) printf("PRINT "); else return yy::parser::make_PRINT(make_loc()); }
"read"                  { if(USE_LEX_ONLY) printf("READ "); else return yy::parser::make_READ(make_loc()); }
"length"                { if(USE_LEX_ONLY) printf("LENGTH "); else return yy::parser::make_LENGTH(make_loc()); }
"void"                  { if(USE_LEX_ONLY) printf("VOID_TYPE "); else return yy::parser::make_VOID_TYPE(make_loc()); }

 /* Types */
"int"                   { if(USE_LEX_ONLY) printf("INT_TYPE "); else return yy::parser::make_INT_TYPE(make_loc()); }
"float"                 { if(USE_LEX_ONLY) printf("FLOAT_TYPE "); else return yy::parser::make_FLOAT_TYPE(make_loc()); }
"boolean"               { if(USE_LEX_ONLY) printf("BOOL_TYPE "); else return yy::parser::make_BOOL_TYPE(make_loc()); }
"volatile"              { if(USE_LEX_ONLY) printf("VOLATILE "); else return yy::parser::make_VOLATILE(make_loc()); }

 /* Boolean literals */
"true"                  { if(USE_LEX_ONLY) printf("TRUE "); else return yy::parser::make_TRUE(make_loc()); }
"false"                 { if(USE_LEX_ONLY) printf("FALSE "); else return yy::parser::make_FALSE(make_loc()); }

 /* Multi-char operators (must come before single-char) */
":="                    { if(USE_LEX_ONLY) printf("ASSIGN "); else return yy::parser::make_ASSIGN(make_loc()); }
"<="                    { if(USE_LEX_ONLY) printf("LE "); else return yy::parser::make_LE(make_loc()); }
">="                    { if(USE_LEX_ONLY) printf("GE "); else return yy::parser::make_GE(make_loc()); }
"!="                    { if(USE_LEX_ONLY) printf("NE "); else return yy::parser::make_NE(make_loc()); }
"=="                    { if(USE_LEX_ONLY) printf("EQ "); else return yy::parser::make_EQ(make_loc()); }

 /* Single-char operators */
"="                     { if(USE_LEX_ONLY) printf("EQ "); else return yy::parser::make_EQ(make_loc()); }
"+"                     { if(USE_LEX_ONLY) printf("PLUSOP "); else return yy::parser::make_PLUSOP(make_loc()); }
"-"                     { if(USE_LEX_ONLY) printf("MINUSOP "); else return yy::parser::make_MINUSOP(make_loc()); }
"*"                     { if(USE_LEX_ONLY) printf("MULTOP "); else return yy::parser::make_MULTOP(make_loc()); }
"/"                     { if(USE_LEX_ONLY) printf("DIVOP "); else return yy::parser::make_DIVOP(make_loc()); }
"^"                     { if(USE_LEX_ONLY) printf("POW "); else return yy::parser::make_POW(make_loc()); }
"<"                     { if(USE_LEX_ONLY) printf("LT "); else return yy::parser::make_LT(make_loc()); }
">"                     { if(USE_LEX_ONLY) printf("GT "); else return yy::parser::make_GT(make_loc()); }
"&&"                    { if(USE_LEX_ONLY) printf("AND "); else return yy::parser::make_AND(make_loc()); }
"&"                     { if(USE_LEX_ONLY) printf("AND "); else return yy::parser::make_AND(make_loc()); }
"||"                    { if(USE_LEX_ONLY) printf("OR "); else return yy::parser::make_OR(make_loc()); }
"|"                     { if(USE_LEX_ONLY) printf("OR "); else return yy::parser::make_OR(make_loc()); }
"!"                     { if(USE_LEX_ONLY) printf("NOT "); else return yy::parser::make_NOT(make_loc()); }

 /* Delimiters */
"("                     { if(USE_LEX_ONLY) printf("LP "); else return yy::parser::make_LP(make_loc()); }
")"                     { if(USE_LEX_ONLY) printf("RP "); else return yy::parser::make_RP(make_loc()); }
"{"                     { if(USE_LEX_ONLY) printf("LBRACE "); else return yy::parser::make_LBRACE(make_loc()); }
"}"                     { if(USE_LEX_ONLY) printf("RBRACE "); else return yy::parser::make_RBRACE(make_loc()); }
"["                     { if(USE_LEX_ONLY) printf("LBRACKET "); else return yy::parser::make_LBRACKET(make_loc()); }
"]"                     { if(USE_LEX_ONLY) printf("RBRACKET "); else return yy::parser::make_RBRACKET(make_loc()); }
","                     { if(USE_LEX_ONLY) printf("COMMA "); else return yy::parser::make_COMMA(make_loc()); }
"."                     { if(USE_LEX_ONLY) printf("DOT "); else return yy::parser::make_DOT(make_loc()); }
":"                     { if(USE_LEX_ONLY) printf("COLON "); else return yy::parser::make_COLON(make_loc()); }
";"                     { if(USE_LEX_ONLY) printf("SEMI "); else return yy::parser::make_SEMI(make_loc()); }

 /* Newline = statement terminator in C+- */
\n                      { if(USE_LEX_ONLY) printf("NEWLINE\n"); else return yy::parser::make_NEWLINE(make_loc()); }

 /* Literals */
[0-9]+\.[0-9]+          { if(USE_LEX_ONLY) printf("FLOAT(%s) ", yytext); else return yy::parser::make_FLOAT(yytext, make_loc()); }
[0-9]+                  { if(USE_LEX_ONLY) printf("INT(%s) ", yytext); else return yy::parser::make_INT(yytext, make_loc()); }

 /* Identifiers (after keywords so keywords match first) */
[a-zA-Z_][a-zA-Z0-9_]* { if(USE_LEX_ONLY) printf("ID(%s) ", yytext); else return yy::parser::make_ID(yytext, make_loc()); }

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
<<EOF>>                 { if(USE_LEX_ONLY) printf("EOF\n"); return yy::parser::make_END(make_loc()); }
%%