#pragma once
#include "common.h"

enum class TokenType {
    // Single Char
    LeftParen, RightParen,
    LeftBrace, RightBrace,
    Comma, Dot, Plus, Minus,
    Slash, Asterisk, Semicolon,

    // One or Two Char
    Bang, BangEqual,
    Equal, EqualEqual,
    Greater, GreaterEqual,
    Less, LessEqual, PlusEqual, MinusEqual, SlashEqual, AsteriskEqual,

    // literals
    Identifier, String, Number,
    
    // keywords
    And, Or, If, Else, While, For, True, False, None, Return, Print, Var, Func, Class, This, Super,

    Error, EndOfFile
};

struct Token {
    TokenType type;
    std::string_view source;
    int line;
};

bool identifiersEqual(Token* a, Token* b);

class Scanner {
public:
    int lineNumber;
    bool handledError;

    Scanner() = default;
    Scanner(const char* source);
    Token scanToken();

private:
    char advance();
    char peek();
    char peekNext();
    char isAtEnd();
    bool match(char expected);

    void skipWhiteSpace();

    Token scanNumber();
    Token scanString();
    Token scanIdentifer();
    Token makeToken(TokenType type);
    TokenType getIdentiferType();

    const char* start;
    const char* current;
    const char* source;
};

#ifdef DEBUGINFO

inline void printToken(const Token &token) {
    static const char* names[] = {
        "LeftParen", "RightParen",
        "LeftBrace", "RightBrace",
        "Comma", "Dot", "Plus", "Minus",
        "Slash", "Asterisk", "Semicolon",

        "Bang", "BangEqual",
        "Equal", "EqualEqual",
        "Greater", "GreaterEqual",
        "Less", "LessEqual", "PlusEqual", "MinusEqual", "SlashEqual", "AsteriskEqual",

        "Identifier", "String", "Number",
        
        "And", "Or", "If", "Else", "While", "For", "True", "False", "None", "Return", "Print", "Var", "Func", "Class", "This", "Super",

        "Error", "EndOfFile"
    };
 
    if (token.source.length() > 0) {
        printf("Token{type=%s, value=\'%s\'}\n", names[(int) token.type], std::string(token.source).c_str());
    } else {
        print((int) token.type);
        printf("Token{type=%s}\n", names[(int) token.type]);
    }
}

#endif