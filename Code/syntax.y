%{
  #include <stdio.h>
  #include "syntaxParsingTree.h"
  #include "lex.yy.c"
  void yyerror(const char *msg);
%}

%locations

/* types */
%union {
  int type_int;
  struct node* type_node; //如果使用typedef定义了别名且在使用别名前# include了定义文件，可以使用别名，否则不能使用，要用struct。可以理解为使用struct后类型自动被当作外部类型而不是未完成类型？
}

/* tokens */
%token <type_int> INT
%token <type_int> FLOAT
%token <type_int> ID
%token <type_int> TYPE
%token SEMI COMMA
%token LC RC
%token STRUCT RETURN IF WHILE

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%right ASSIGNOP
%left  OR
%left  AND
%left  <type_int> RELOP
%left  PLUS MINUS
%left  STAR DIV
%right NOT
%left  DOT LP RP LB RB

/* non-terminals */
%type <type_node> Program   ExtDefList      ExtDef  ExtDecList
%type <type_node> Specifier StructSpecifier OptTag  Tag
%type <type_node> VarDec    FunDec         VarList ParamDec
%type <type_node> CompSt    StmtList        Stmt
%type <type_node> DefList   Def             DecList Dec
%type <type_node> Exp       Args


%%
/* High-level Definitions */
Program : ExtDefList {
    root = make_inode(Program, @$.first_line);
    node** cur = &(root->child); *cur = $1;
  }
  | ErrorSEMIList ExtDefList {
    // $$ = NULL;
    root = make_error(Program, @$.first_line);
  }
  | ErrorRCList ExtDefList {
    // $$ = NULL;
    root = make_error(Program, @$.first_line);
  }
  ;
ExtDefList: /* empty */ { 
    // $$ = NULL; 
    $$ = make_empty(ExtDefList, @$.first_line);
  }
  | ExtDef ExtDefList  {
    $$ = make_inode(ExtDefList, @$.first_line);
    node** cur = &($$->child); *cur = $1;
    cur = &((*cur)->next); *cur = $2;
  }
  | ExtDef ErrorSEMIList ExtDefList {
    // $$ = NULL;
    $$ = make_error(ExtDefList, @$.first_line);
  }
  | ExtDef ErrorRCList ExtDefList {
    // $$ = NULL;
    $$ = make_error(ExtDefList, @$.first_line);
  }
  ;
ExtDef: Specifier ExtDecList SEMI {
    $$ = make_inode(ExtDef, @$.first_line);
    node** cur = &($$->child); *cur = $1;
    cur = &((*cur)->next); *cur = $2;
    cur = &((*cur)->next); *cur = make_leaf(SEMI, @3.first_line, -1);
  }
  | Specifier SEMI {
    $$ = make_inode(ExtDef, @$.first_line);
    node** cur = &($$->child); *cur = $1;
    cur = &((*cur)->next); *cur = make_leaf(SEMI, @2.first_line, -1);
  }
  | Specifier FunDec CompSt {
    $$ = make_inode(ExtDef, @$.first_line);
    node** cur = &($$->child); *cur = $1;
    cur = &((*cur)->next); *cur = $2;
    cur = &((*cur)->next); *cur = $3;
  }
  ;
ExtDecList: VarDec {
    $$ = make_inode(ExtDecList, @$.first_line);
    node** cur = &($$->child); *cur = $1;
  }
  | VarDec COMMA ExtDecList {
    $$ = make_inode(ExtDecList, @$.first_line);
    node** cur = &($$->child); *cur = $1;
    cur = &((*cur)->next); *cur = make_leaf(COMMA, @2.first_line, -1);
    cur = &((*cur)->next); *cur = $3;
  }
  ;

/* Specifiers */
Specifier: TYPE {
    $$ = make_inode(Specifier, @$.first_line);
    node** cur = &($$->child); *cur = make_leaf(TYPE, @1.first_line, $1);
  }
  | StructSpecifier {
    $$ = make_inode(Specifier, @$.first_line);
    node** cur = &($$->child); *cur = $1;
  }
  ;
StructSpecifier: STRUCT OptTag LC DefList RC {
    $$ = make_inode(StructSpecifier, @$.first_line);
    node** cur = &($$->child); *cur = make_leaf(STRUCT, @1.first_line, -1);
    cur = &((*cur)->next); *cur = $2;
    cur = &((*cur)->next); *cur = make_leaf(LC, @3.first_line, -1);
    cur = &((*cur)->next); *cur = $4;
    cur = &((*cur)->next); *cur = make_leaf(RC, @5.first_line, -1);
  }
  | STRUCT Tag {
    $$ = make_inode(StructSpecifier, @$.first_line);
    node** cur = &($$->child); *cur = make_leaf(STRUCT, @1.first_line, -1);
    cur = &((*cur)->next); *cur = $2;
  }
  ;
OptTag: /* empty */ { 
    // $$ = NULL; 
    $$ = make_empty(OptTag, @$.first_line);
  } // ?
  | ID {
    $$ = make_inode(OptTag, @$.first_line);
    node** cur = &($$->child); *cur = make_leaf(ID, @1.first_line, $1);
  }
  ;
Tag: ID {
    $$ = make_inode(Tag, @$.first_line);
    node** cur = &($$->child); *cur = make_leaf(ID, @1.first_line, $1);
  }
  ;

/* Declarations */
VarDec: ID {
    $$ = make_inode(VarDec, @$.first_line);
    node** cur = &($$->child); *cur = make_leaf(ID, @1.first_line, $1);
  }
  | VarDec LB INT RB  {
    $$ = make_inode(VarDec, @$.first_line);
    node** cur = &($$->child); *cur = $1;
    cur = &((*cur)->next); *cur = make_leaf(LB, @2.first_line, -1);
    cur = &((*cur)->next); *cur = make_leaf(INT, @3.first_line, $3);
    cur = &((*cur)->next); *cur = make_leaf(RB, @4.first_line, -1);
  }
  ;
FunDec: ID LP VarList RP {
    $$ = make_inode(FunDec, @$.first_line);
    node** cur = &($$->child); *cur = make_leaf(ID, @1.first_line, $1);
    cur = &((*cur)->next); *cur = make_leaf(LP, @2.first_line, -1);
    cur = &((*cur)->next); *cur = $3;
    cur = &((*cur)->next); *cur = make_leaf(RP, @4.first_line, -1);
  }
  | ID LP RP {
    $$ = make_inode(FunDec, @$.first_line);
    node** cur = &($$->child); *cur = make_leaf(ID, @1.first_line, $1);
    cur = &((*cur)->next); *cur = make_leaf(LP, @2.first_line, -1);
    cur = &((*cur)->next); *cur = make_leaf(RP, @3.first_line, -1);
  } 
  | ID LP error RP{
    $$ = make_error(FunDec, @$.first_line);
    errorFlag = 1;
  }
  ;
VarList: ParamDec COMMA VarList {
    $$ = make_inode(VarList, @$.first_line);
    node** cur = &($$->child); *cur = $1;
    cur = &((*cur)->next); *cur = make_leaf(COMMA, @2.first_line, -1);
    cur = &((*cur)->next); *cur = $3;
  }
  | ParamDec {
    $$ = make_inode(VarList, @$.first_line);
    node** cur = &($$->child); *cur = $1;
  }
  ;
ParamDec: Specifier VarDec {
    $$ = make_inode(ParamDec, @$.first_line);
    node** cur = &($$->child); *cur = $1;
    cur = &((*cur)->next); *cur = $2;
  }
  ;

/* Statements */
CompSt: LC DefList StmtList RC {
    $$ = make_inode(CompSt, @$.first_line);
    node** cur = &($$->child); *cur = make_leaf(LC, @1.first_line, -1);
    cur = &((*cur)->next); *cur = $2;
    cur = &((*cur)->next); *cur = $3;
    cur = &((*cur)->next); *cur = make_leaf(RC, @4.first_line, -1);
  }
  | LC ErrorSEMIList DefList StmtList RC {
    // $$ = NULL;
    $$ = make_error(CompSt, @$.first_line);
  }
  | LC DefList ErrorRCList StmtList RC {
    $$ = make_error(CompSt, @$.first_line);
  }
  | LC ErrorSEMIList DefList ErrorRCList StmtList RC {
    $$ = make_error(CompSt, @$.first_line);
  }
  ;
StmtList: /* empty */ { 
    // $$ = NULL; 
    $$ = make_empty(StmtList, @$.first_line);
  }
  | Stmt StmtList {
    $$ = make_inode(StmtList, @$.first_line);
    node** cur = &($$->child); *cur = $1;
    cur = &((*cur)->next); *cur = $2;
  }
  | Stmt ErrorSEMIList StmtList {
    // $$ = NULL;
    $$ = make_error(StmtList, @$.first_line);
  }
  | Stmt ErrorRCList StmtList {
    $$ = make_error(StmtList, @$.first_line);
  }// stmt 也用以RC结尾的！！
  ;
Stmt: Exp SEMI {
    $$ = make_inode(Stmt, @$.first_line);
    node** cur = &($$->child); *cur = $1;
    cur = &((*cur)->next); *cur = make_leaf(SEMI, @2.first_line, -1);
  }
  | CompSt {
    $$ = make_inode(Stmt, @$.first_line);
    $$->child = $1;
  }
  | RETURN Exp SEMI {
    $$ = make_inode(Stmt, @$.first_line);
    node** cur = &($$->child); *cur = make_leaf(RETURN, @1.first_line, -1);
    cur = &((*cur)->next); *cur = $2;
    cur = &((*cur)->next); *cur = make_leaf(SEMI, @3.first_line, -1);
  }
  | IF LP Exp RP Stmt %prec LOWER_THAN_ELSE {
    $$ = make_inode(Stmt, @$.first_line);
    node** cur = &($$->child); *cur = make_leaf(IF, @1.first_line, -1);
    cur = &((*cur)->next); *cur = make_leaf(LP, @2.first_line, -1);
    cur = &((*cur)->next); *cur = $3;
    cur = &((*cur)->next); *cur = make_leaf(RP, @4.first_line, -1);
    cur = &((*cur)->next); *cur = $5;
  }
  | IF LP Exp RP Stmt ELSE Stmt{
    $$ = make_inode(Stmt, @$.first_line);
    node** cur = &($$->child); *cur = make_leaf(IF, @1.first_line, -1);
    cur = &((*cur)->next); *cur = make_leaf(LP, @2.first_line, -1);
    cur = &((*cur)->next); *cur = $3;
    cur = &((*cur)->next); *cur = make_leaf(RP, @4.first_line, -1);
    cur = &((*cur)->next); *cur = $5;
    cur = &((*cur)->next); *cur = make_leaf(ELSE, @6.first_line, -1);
    cur = &((*cur)->next); *cur = $7;
  }
  | WHILE LP Exp RP Stmt {
    $$ = make_inode(Stmt, @$.first_line);
    node** cur = &($$->child); *cur = make_leaf(WHILE, @1.first_line, -1);
    cur = &((*cur)->next); *cur = make_leaf(LP, @2.first_line, -1);
    cur = &((*cur)->next); *cur = $3;
    cur = &((*cur)->next); *cur = make_leaf(RP, @4.first_line, -1);
    cur = &((*cur)->next); *cur = $5;
  }
  | IF LP error RP Stmt %prec LOWER_THAN_ELSE {
    $$ = make_error(Stmt, @$.first_line);
    errorFlag = 1;
  }
  | IF LP error RP Stmt ELSE Stmt{
    $$ = make_error(Stmt, @$.first_line);
    errorFlag = 1;
  }
  | WHILE LP error RP Stmt {
    $$ = make_error(Stmt, @$.first_line);
    errorFlag = 1;
  }
  | error SEMI {
    $$ = make_error(Stmt, @$.first_line);
    errorFlag = 1;
  }
  ;


/* Local Definitions */
DefList: /* empty */ { 
    // $$ = NULL; 
    $$ = make_empty(DefList, @$.first_line);
  }
  | Def DefList {
    $$ = make_inode(DefList, @$.first_line);
    node** cur = &($$->child); *cur = $1;
    cur = &((*cur)->next); *cur = $2;
  }
  | Def ErrorSEMIList DefList {
    // $$ = NULL;
    $$ = make_error(DefList, @$.first_line);
  }
  ;
Def: Specifier DecList SEMI {
    $$ = make_inode(Def, @$.first_line);
    node** cur = &($$->child); *cur = $1;
    cur = &((*cur)->next); *cur = $2;
    cur = &((*cur)->next); *cur = make_leaf(SEMI, @3.first_line, -1);
  }
  ;
DecList: Dec {
    $$ = make_inode(DecList, @$.first_line);
    $$->child = $1;
  }
  | Dec COMMA DecList {
    $$ = make_inode(DecList, @$.first_line);
    node** cur = &($$->child); *cur = $1;
    cur = &((*cur)->next); *cur = make_leaf(COMMA, @2.first_line, -1);
    cur = &((*cur)->next); *cur = $3;
  }
  ;
Dec: VarDec {
    $$ = make_inode(Dec, @$.first_line);
    node** cur = &($$->child); *cur = $1;
  }
  | VarDec ASSIGNOP Exp {
    $$ = make_inode(Dec, @$.first_line);
    node** cur = &($$->child); *cur = $1;
    cur = &((*cur)->next); *cur = make_leaf(ASSIGNOP, @2.first_line, -1);
    cur = &((*cur)->next); *cur = $3;
  }
  ;

/* Expressions */
Exp: Exp ASSIGNOP Exp {
    $$ = make_inode(Exp, @$.first_line); 
    node** cur = &($$->child); *cur = $1;
    cur = &((*cur)->next); *cur = make_leaf(ASSIGNOP, @2.first_line, -1); 
    cur = &((*cur)->next); *cur = $3;
  }
  | Exp AND Exp {
    $$ = make_inode(Exp, @$.first_line); 
    node** cur = &($$->child); *cur = $1;
    cur = &((*cur)->next); *cur = make_leaf(AND, @2.first_line, -1); 
    cur = &((*cur)->next); *cur = $3;
  }
  | Exp OR Exp     {
    $$ = make_inode(Exp, @$.first_line); 
    node** cur = &($$->child); *cur = $1;
    cur = &((*cur)->next); *cur = make_leaf(OR, @2.first_line, -1); 
    cur = &((*cur)->next); *cur = $3;
  }
  | Exp RELOP Exp  {
    $$ = make_inode(Exp, @$.first_line); 
    node** cur = &($$->child); *cur = $1;
    cur = &((*cur)->next); *cur = make_leaf(RELOP, @2.first_line, $2); 
    cur = &((*cur)->next); *cur = $3;
  }
  | Exp PLUS Exp  {
    $$ = make_inode(Exp, @$.first_line); 
    node** cur = &($$->child); *cur = $1;
    cur = &((*cur)->next); *cur = make_leaf(PLUS, @2.first_line, -1); 
    cur = &((*cur)->next); *cur = $3;
  }
  | Exp MINUS Exp {
    $$ = make_inode(Exp, @$.first_line); 
    node** cur = &($$->child); *cur = $1;
    cur = &((*cur)->next); *cur = make_leaf(MINUS, @2.first_line, -1); 
    cur = &((*cur)->next); *cur = $3;
  }
  | Exp STAR Exp {
    $$ = make_inode(Exp, @$.first_line); 
    node** cur = &($$->child); *cur = $1;
    cur = &((*cur)->next); *cur = make_leaf(STAR, @2.first_line, -1); 
    cur = &((*cur)->next); *cur = $3;
  }
  | Exp DIV Exp {
    $$ = make_inode(Exp, @$.first_line); 
    node** cur = &($$->child); *cur = $1;
    cur = &((*cur)->next); *cur = make_leaf(DIV, @2.first_line, -1); 
    cur = &((*cur)->next); *cur = $3;
  }
  | LP Exp RP {
    $$ = make_inode(Exp, @$.first_line); 
    node** cur = &($$->child); *cur = make_leaf(LP, @1.first_line, -1); 
    cur = &((*cur)->next); *cur = $2; 
    cur = &((*cur)->next); *cur = make_leaf(RP, @3.first_line, -1); 
  }
  | MINUS Exp {
    $$ = make_inode(Exp, @$.first_line); 
    node** cur = &($$->child); *cur = make_leaf(MINUS, @1.first_line, -1); 
    cur = &((*cur)->next); *cur = $2;
  }
  | NOT Exp {
    $$ = make_inode(Exp, @$.first_line); 
    node** cur = &($$->child); *cur = make_leaf(NOT, @1.first_line, -1); 
    cur = &((*cur)->next); *cur = $2;
  }
  | ID LP Args RP {
    $$ = make_inode(Exp, @$.first_line); 
    node** cur = &($$->child); *cur = make_leaf(ID, @1.first_line,$1); 
    cur = &((*cur)->next); *cur = make_leaf(LP, @2.first_line, -1); 
    cur = &((*cur)->next); *cur = $3;
    cur = &((*cur)->next); *cur = make_leaf(RP, @4.first_line, -1); 
  }
  | ID LP RP {
    $$ = make_inode(Exp, @$.first_line); 
    node** cur = &($$->child); *cur = make_leaf(ID, @1.first_line,$1); 
    cur = &((*cur)->next); *cur = make_leaf(LP, @2.first_line, -1); 
    cur = &((*cur)->next); *cur = make_leaf(RP, @3.first_line, -1); 
  }
  | Exp LB Exp RB {
    $$ = make_inode(Exp, @$.first_line); 
    node** cur = &($$->child); *cur = $1; 
    cur = &((*cur)->next); *cur = make_leaf(LB, @2.first_line, -1); 
    cur = &((*cur)->next); *cur = $3; 
    cur = &((*cur)->next); *cur = make_leaf(RB, @4.first_line, -1); 
  }
  | Exp DOT ID {
    $$ = make_inode(Exp, @$.first_line); 
    node** cur = &($$->child); *cur = $1; 
    cur = &((*cur)->next); *cur = make_leaf(DOT, @2.first_line, -1); 
    cur = &((*cur)->next); *cur = make_leaf(ID, @3.first_line, $3);
  }
  | ID  {
    $$ = make_inode(Exp, @$.first_line); 
    $$->child = make_leaf(ID, @1.first_line,$1);
  }
  | INT {
    $$ = make_inode(Exp, @$.first_line); 
    $$->child = make_leaf(INT, @1.first_line,$1);
  }
  | FLOAT { 
    $$ = make_inode(Exp, @$.first_line); 
    $$->child = make_leaf(FLOAT, @1.first_line,$1);
  }
  ;
Args: Exp COMMA Args {
    $$ = make_inode(Args, @$.first_line); 
    node** cur = &($$->child); *cur = $1; 
    cur = &((*cur)->next); *cur = make_leaf(COMMA, @2.first_line, -1);
    cur = &((*cur)->next); *cur = $3; 
  }
  | Exp {
    $$ = make_inode(Args, @$.first_line); 
    $$->child = $1;
  }
  ;

/* errors */
ErrorSEMIList: 
  | error SEMI ErrorSEMIList { errorFlag = 1; }
  ;
ErrorRCList: 
  | error RC ErrorRCList { errorFlag = 1; }
  ;
//ErrorRPList: 
//  | error RP ErrorRPList { errorFlag = 1; }
//  ;


%%
void yyerror(const char *msg) {
  if (!lexError){
    if (!lineError){
      printf("Error type B at Line %d: errorMsg: %s, column: %d, text: '%s'\n", yylineno, msg, yycolumn, yytext);
      lineError = 1;
    }
  } else {
    lexError = 0;
  }
}