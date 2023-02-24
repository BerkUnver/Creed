// Currently not included in compilation. Just an example of what the AST might look like for now.

typedef struct AstFunc {
    char *name;
    AstVar *args;
    int arg_count;
    char *return_type;
    Statement *statements;
    int statement_count;
} AstFunc;


typedef enum StatementType {
    DECLARATION;
    ASSIGNMENT;
    INCREMENT;
    FUNCTION_CALL;
} StatementType;


typedef union StatementData {
    struct {
        char *assigned;
        AstExpr expr;
    } assignment;
} StatementData;

typedef struct Statement {
    StatementType type;
    StatementData data;
} Statement;



typedef enum AstExprType {
    EXPR_ADD,
    EXPR_SUBTRACT,
    EXPR_MULTIPLY,
    EXPR_DIVIDE,
    EXPR_MODULO,
    
    EXPR_EQUALS,
    EXPR_NOT_EQUALS,
    
    EXPR_GREATER_THAN,
    EXPR_LESS_THAN,
    EXPR_GREATER_OR_EQUAL_THAN,
    EXPR_LESS_OR_EQUAL_TO,

    EXPR_LOGICAL_AND,
    EXPR_LOGICAL_OR,
    EXPR_LOGICAL_NOT,
    
    EXPR_REFERENCE,
    EXPR_DEFERENCE,
    
    EXPR_FUNCTION_CALL,
} AstExprType;

typedef struct AstExpr {
   struct {
        AstExpr *lhs;
        AstExpr *rhs;
   } binary;
   AstExpr *unary;
} AstExpr;
