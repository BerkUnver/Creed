#include <stdlib.h>
#include <stdbool.h>
#include "lexer.h"
#include "parser.h"

bool parse_expr(Lexer *lexer, AstExpr *expr) {
    switch (lexer_token_peek_type(lexer))
    {
        case TOKEN_LITERAL_INT: {
            Token token = lexer_token_get(lexer);
            
            *expr = (AstExpr) {
                .line_start = token.line_idx,
                .line_end = token.line_idx,
                .char_start = token.char_idx,
                .char_end = token.char_idx + token.len,
                .parenthesized = false,
                .type = EXPR_LITERAL_INT,
                .data.literal_int = token.data.literal_int
            };

            token_free(&token);
            return true;
        }

        default: return false;
    }
}

void expr_free(AstExpr *expr) {
    // Add code to free subexprs
}
