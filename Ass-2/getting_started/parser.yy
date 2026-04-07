%skeleton "lalr1.cc"
%defines
%locations
%define parse.error verbose
%define api.value.type variant
%define api.token.constructor

%code requires{
  #include <string>
  #include "Node.h"
  #define USE_LEX_ONLY false
}

%code{
  #define YY_DECL yy::parser::symbol_type yylex()
  YY_DECL;
  Node* root;
  extern int yylineno;
}

%token <std::string> ID INT FLOAT
%token CLASS MAIN RETURN IF ELSE FOR BREAK CONTINUE PRINT READ LENGTH
%token INT_TYPE FLOAT_TYPE BOOL_TYPE VOID_TYPE VOLATILE
%token TRUE FALSE
%token ASSIGN PLUSOP MINUSOP MULTOP DIVOP POW AND OR LT GT LE GE EQ NE NOT
%token COMMA DOT COLON
%token LP RP LBRACE RBRACE LBRACKET RBRACKET
%token NEWLINE
%token SEMI
%token END 0 "end of file"

/* Operator precedence (lowest to highest) */
%left OR
%left AND
%left EQ NE
%left LT GT LE GE
%left PLUSOP MINUSOP
%left MULTOP DIVOP
%right POW
%right NOT
%left DOT LBRACKET

%type <Node *> program
%type <Node *> global_var_list class_list class_decl class_body entry
%type <Node *> method_decl param_list_opt param_list
%type <Node *> stmt_block stmt_list stmt else_opt
%type <Node *> for_init_opt for_cond_opt for_update_opt
%type <Node *> var type base_type
%type <Node *> expr arg_list_opt arg_list
%type <Node *> expr_seq

%%

/* ------------------------------------------------------------------ */
/*  Program structure                                                  */
/* ------------------------------------------------------------------ */

program:
  opt_newlines global_var_list class_list entry opt_newlines END {
        root = new Node("Program", "", yylineno);
        if ($2) root->children.push_back($2);
        if ($3) root->children.push_back($3);
        root->children.push_back($4);
    }
;

opt_newlines:
  /* empty */
  | opt_newlines NEWLINE
;

global_var_list:
    /* empty */ { $$ = nullptr; }
  | global_var_list var NEWLINE {
        if ($1 == nullptr) $$ = new Node("Globals", "", yylineno);
        else $$ = $1;
        $$->children.push_back($2);
    }
;

class_list:
    /* empty */ { $$ = nullptr; }
  | class_list class_decl {
        if ($1 == nullptr) $$ = new Node("Classes", "", yylineno);
        else $$ = $1;
        $$->children.push_back($2);
    }
  | class_list NEWLINE { $$ = $1; }
;

/* ------------------------------------------------------------------ */
/*  Class                                                              */
/* ------------------------------------------------------------------ */

class_decl:
    CLASS ID LBRACE class_body RBRACE {
    $$ = new Node("Class", $2, @2.begin.line);
        if ($4) {
            for (auto child : $4->children)
                $$->children.push_back(child);
        }
    }
;

class_body:
    /* empty */ { $$ = nullptr; }
  | class_body var NEWLINE {
        if ($1 == nullptr) $$ = new Node("ClassBody", "", yylineno);
        else $$ = $1;
        $$->children.push_back($2);
    }
  | class_body method_decl {
        if ($1 == nullptr) $$ = new Node("ClassBody", "", yylineno);
        else $$ = $1;
        $$->children.push_back($2);
    }
  | class_body NEWLINE { $$ = $1; }
;

/* ------------------------------------------------------------------ */
/*  Entry point and methods                                            */
/* ------------------------------------------------------------------ */

entry:
    MAIN LP RP COLON INT_TYPE stmt_block {
    $$ = new Node("Main", "", @1.begin.line);
    $$->children.push_back(new Node("Type", "int", @5.begin.line));
        $$->children.push_back($6);
    }
;

method_decl:
    ID LP param_list_opt RP COLON type stmt_block {
    $$ = new Node("Method", $1, @1.begin.line);
        if ($3) $$->children.push_back($3);
        $$->children.push_back($6);
        $$->children.push_back($7);
    }
;

param_list_opt:
    /* empty */ { $$ = nullptr; }
  | param_list { $$ = $1; }
;

param_list:
    ID COLON type {
    $$ = new Node("Params", "", @1.begin.line);
    Node* parameter = new Node("Param", $1, @1.begin.line);
        parameter->children.push_back($3);
        $$->children.push_back(parameter);
    }
  | param_list COMMA ID COLON type {
        $$ = $1;
    Node* parameter = new Node("Param", $3, @3.begin.line);
        parameter->children.push_back($5);
        $$->children.push_back(parameter);
    }
;

/* ------------------------------------------------------------------ */
/*  Statements                                                         */
/* ------------------------------------------------------------------ */

stmt_block:
    LBRACE stmt_list RBRACE {
        $$ = new Node("Block", "", yylineno);
        $$->children = $2->children;
    }
;

stmt_list:
    /* empty */ { $$ = new Node("Stmts", "", yylineno); }
  | stmt_list stmt {
        $$ = $1;
        $$->children.push_back($2);
    }
  | stmt_list NEWLINE { $$ = $1; }
;

stmt:
    stmt_block { $$ = $1; }
  | var NEWLINE { $$ = $1; }
  | expr ASSIGN expr NEWLINE {
        $$ = new Node("Assign", "", yylineno);
        $$->children.push_back($1);
        $$->children.push_back($3);
    }
  | IF LP expr RP opt_newlines stmt else_opt {
        $$ = new Node("If", "", yylineno);
        $$->children.push_back($3);
        $$->children.push_back($6);
        if ($7) $$->children.push_back($7);
    }
  | FOR LP for_init_opt COMMA for_cond_opt COMMA for_update_opt RP opt_newlines stmt {
        $$ = new Node("For", "", yylineno);
        if ($3) $$->children.push_back($3);
        if ($5) $$->children.push_back($5);
        if ($7) $$->children.push_back($7);
        $$->children.push_back($10);
    }
  | PRINT LP expr RP NEWLINE {
        $$ = new Node("Print", "", yylineno);
        $$->children.push_back($3);
    }
  | READ LP expr RP NEWLINE {
        $$ = new Node("Read", "", yylineno);
        $$->children.push_back($3);
    }
  | RETURN expr NEWLINE {
        $$ = new Node("Return", "", yylineno);
        $$->children.push_back($2);
    }
  | BREAK NEWLINE {
        $$ = new Node("Break", "", yylineno);
    }
  | CONTINUE NEWLINE {
        $$ = new Node("Continue", "", yylineno);
    }
  | expr NEWLINE {
        $$ = new Node("ExprStmt", "", yylineno);
        $$->children.push_back($1);
    }
;

else_opt:
    /* empty */ { $$ = nullptr; }
  | ELSE opt_newlines stmt {
        $$ = new Node("Else", "", yylineno);
        $$->children.push_back($3);
    }
;

/* ------------------------------------------------------------------ */
/*  For-loop parts                                                     */
/* ------------------------------------------------------------------ */

for_init_opt:
    /* empty */ { $$ = nullptr; }
  | var { $$ = $1; }
  | expr ASSIGN expr {
        $$ = new Node("Assign", "", yylineno);
        $$->children.push_back($1);
        $$->children.push_back($3);
    }
;

for_cond_opt:
    /* empty */ { $$ = nullptr; }
  | expr { $$ = $1; }
;

for_update_opt:
    /* empty */ { $$ = nullptr; }
  | expr ASSIGN expr {
        $$ = new Node("Assign", "", yylineno);
        $$->children.push_back($1);
        $$->children.push_back($3);
    }
;

/* ------------------------------------------------------------------ */
/*  Variable declarations                                              */
/* ------------------------------------------------------------------ */

var:
    ID COLON type {
    $$ = new Node("VarDecl", $1, @1.begin.line);
        $$->children.push_back($3);
    }
  | ID COLON type ASSIGN expr {
    $$ = new Node("VarDeclInit", $1, @1.begin.line);
        $$->children.push_back($3);
        $$->children.push_back($5);
    }
  | VOLATILE ID COLON type {
    $$ = new Node("VolatileVarDecl", $2, @2.begin.line);
        $$->children.push_back($4);
    }
  | VOLATILE ID COLON type ASSIGN expr {
    $$ = new Node("VolatileVarDeclInit", $2, @2.begin.line);
        $$->children.push_back($4);
        $$->children.push_back($6);
    }
;

/* ------------------------------------------------------------------ */
/*  Types                                                              */
/* ------------------------------------------------------------------ */

type:
    base_type { $$ = $1; }
  | base_type LBRACKET RBRACKET {
        $$ = new Node("ArrayType", "", @1.begin.line);
        $$->children.push_back($1);
    }
  | ID { $$ = new Node("Type", $1, @1.begin.line); }
  | VOID_TYPE { $$ = new Node("Type", "void", @1.begin.line); }
;

base_type:
    INT_TYPE { $$ = new Node("Type", "int", @1.begin.line); }
  | FLOAT_TYPE { $$ = new Node("Type", "float", @1.begin.line); }
  | BOOL_TYPE { $$ = new Node("Type", "boolean", @1.begin.line); }
;

/* ------------------------------------------------------------------ */
/*  Expressions                                                        */
/* ------------------------------------------------------------------ */

expr:
    expr OR expr {
        $$ = new Node("Or", "", yylineno);
        $$->children.push_back($1);
        $$->children.push_back($3);
    }
  | expr AND expr {
        $$ = new Node("And", "", yylineno);
        $$->children.push_back($1);
        $$->children.push_back($3);
    }
  | expr LT expr {
        $$ = new Node("LessThan", "", yylineno);
        $$->children.push_back($1);
        $$->children.push_back($3);
    }
  | expr GT expr {
        $$ = new Node("GreaterThan", "", yylineno);
        $$->children.push_back($1);
        $$->children.push_back($3);
    }
  | expr LE expr {
        $$ = new Node("LessEqual", "", yylineno);
        $$->children.push_back($1);
        $$->children.push_back($3);
    }
  | expr GE expr {
        $$ = new Node("GreaterEqual", "", yylineno);
        $$->children.push_back($1);
        $$->children.push_back($3);
    }
  | expr EQ expr {
        $$ = new Node("Equal", "", yylineno);
        $$->children.push_back($1);
        $$->children.push_back($3);
    }
  | expr NE expr {
        $$ = new Node("NotEqual", "", yylineno);
        $$->children.push_back($1);
        $$->children.push_back($3);
    }
  | expr PLUSOP expr {
        $$ = new Node("Add", "", yylineno);
        $$->children.push_back($1);
        $$->children.push_back($3);
    }
  | expr MINUSOP expr {
        $$ = new Node("Sub", "", yylineno);
        $$->children.push_back($1);
        $$->children.push_back($3);
    }
  | expr MULTOP expr {
        $$ = new Node("Mul", "", yylineno);
        $$->children.push_back($1);
        $$->children.push_back($3);
    }
  | expr DIVOP expr {
        $$ = new Node("Div", "", yylineno);
        $$->children.push_back($1);
        $$->children.push_back($3);
    }
  | expr POW expr {
        $$ = new Node("Pow", "", yylineno);
        $$->children.push_back($1);
        $$->children.push_back($3);
    }
  | expr LBRACKET expr RBRACKET {
        $$ = new Node("ArrayAccess", "", yylineno);
        $$->children.push_back($1);
        $$->children.push_back($3);
    }
  | expr DOT LENGTH {
        $$ = new Node("ArrayLength", "", yylineno);
        $$->children.push_back($1);
    }
  | ID LP arg_list_opt RP {
        $$ = new Node("MethodCall", $1, yylineno);
        if ($3) $$->children.push_back($3);
    }
  | expr DOT ID LP arg_list_opt RP {
        $$ = new Node("ObjectMethodCall", $3, yylineno);
        $$->children.push_back($1);
        if ($5) $$->children.push_back($5);
    }
  | INT { $$ = new Node("Int", $1, yylineno); }
  | FLOAT { $$ = new Node("Float", $1, yylineno); }
  | ID { $$ = new Node("ID", $1, yylineno); }
  | TRUE { $$ = new Node("True", "", yylineno); }
  | FALSE { $$ = new Node("False", "", yylineno); }
  | base_type LBRACKET expr_seq RBRACKET {
        $$ = new Node("ArrayInit", "", yylineno);
        $$->children.push_back($1);
        $$->children.push_back($3);
    }
  | NOT expr {
        $$ = new Node("Not", "", yylineno);
        $$->children.push_back($2);
    }
  | LP expr RP {
        $$ = $2;
    }
;

expr_seq:
    expr {
        $$ = new Node("ExprList", "", yylineno);
        $$->children.push_back($1);
    }
  | expr_seq COMMA expr {
        $$ = $1;
        $$->children.push_back($3);
    }
;

arg_list_opt:
    /* empty */ { $$ = nullptr; }
  | arg_list { $$ = $1; }
;

arg_list:
    expr {
        $$ = new Node("Args", "", yylineno);
        $$->children.push_back($1);
    }
  | arg_list COMMA expr {
        $$ = $1;
        $$->children.push_back($3);
    }
;

%%
