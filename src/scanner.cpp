#include "common.h"
#include "scanner.h"
#include "jakelang.h"

bool identifiersEqual(Token* a, Token* b) {
    return a->source == b->source;
}

Scanner::Scanner(const char* source) : source(source), handledError(false) {
    lineNumber = 1;
    current = source;
    start = source;
}

char Scanner::advance() {
    current++;
    return current[-1];
}

char Scanner::peek() {
    return *current;
}

char Scanner::peekNext() {
    if (isAtEnd()) return '\0';

    return current[1];
}

char Scanner::isAtEnd() {
    return *current == '\0';
}

bool Scanner::match(char expected) {
    if (isAtEnd())
        return false;
    
    if (*current != expected) return false;

    current++;
    return true; 
}

void Scanner::skipWhiteSpace() {
    for (;;) {
        switch (peek()) {
            case '\r':
            case '\t':
            case ' ':
                advance();
                break;
            case '\n':
                lineNumber++;
                advance();
                break;
            
            case '/':
                if (peekNext() == '/') {
                    while (peek() != '\n' and !isAtEnd()) advance();
                } else {
                    return;
                }

                break;

            default:
                return;
        }
    }
}

Token Scanner::makeToken(TokenType type) {
    return Token {type, std::string_view(start, (int) (current - start)), lineNumber};
}

Token Scanner::scanNumber() {
    while (isdigit(peek())) advance();

    if (peek() == '.') {
        advance();
        while (isdigit(peek())) advance();
    }

    return makeToken(TokenType::Number);
}

Token Scanner::scanString() {
    char startingChar = current[-1];

    while (peek() != startingChar) {
        if (peek() == '\n' || isAtEnd()) {
            printError(ExceptionType::SyntaxError, "String literal does not end", lineNumber, "");
            handledError = true;
            return makeToken(TokenType::Error);
        }

        advance();
    }
        

    advance();
    return makeToken(TokenType::String);
}

Token Scanner::scanIdentifer() {
    while (isalpha(peek()) || isdigit(peek()) || peek() == '_') advance();
    return makeToken(getIdentiferType());
}

Token Scanner::scanToken() {
    skipWhiteSpace();

    start = current;

    if (isAtEnd())
        return makeToken(TokenType::EndOfFile);

    char c = advance();

    if (isdigit(c))
        return scanNumber();

    if (isalpha(c) || c == '_')
        return scanIdentifer();

    if (c == '\"' || c == '\'')
        return scanString();

    switch (c) {
        case '(': return makeToken(TokenType::LeftParen);
        case ')': return makeToken(TokenType::RightParen);
        case '{': return makeToken(TokenType::LeftBrace);
        case '}': return makeToken(TokenType::RightBrace);
        case ',': return makeToken(TokenType::Comma);
        case ';': return makeToken(TokenType::Semicolon);

        case '+':
            return makeToken(match('=') ? TokenType::PlusEqual : TokenType::Plus);
        case '-':
            return makeToken(match('=') ? TokenType::MinusEqual : TokenType::Minus);
        case '/':
            return makeToken(match('=') ? TokenType::SlashEqual : TokenType::Slash);
        case '*':
            return makeToken(match('=') ? TokenType::AsteriskEqual : TokenType::Asterisk);
        case '!':
            return makeToken(match('=') ? TokenType::BangEqual : TokenType::Bang);
        case '=':
            return makeToken(match('=') ? TokenType::EqualEqual : TokenType::Equal);
        case '>':
            return makeToken(match('=') ? TokenType::GreaterEqual : TokenType::Greater);
        case '<':
            return makeToken(match('=') ? TokenType::LessEqual : TokenType::Less);
            
        case '.':
            if (isdigit(peek()))
                return scanNumber();
            return makeToken(TokenType::Dot);
        
        default:
            return makeToken(TokenType::Error);
    }
};

TokenType Scanner::getIdentiferType() {
    std::string_view str = std::string_view(start, (int) (current - start));

    if (str == "if") {
        return TokenType::If;
    } else if (str == "and") {
        return TokenType::And;
    } else if (str == "or") {
        return TokenType::Or;
    } else if (str == "if") {
        return TokenType::If;
    } else if (str == "else") {
        return TokenType::Else;
    } else if (str == "while") {
        return TokenType::While;
    } else if (str == "for") {
        return TokenType::For;
    } else if (str == "true") {
        return TokenType::True;
    } else if (str == "false") {
        return TokenType::False;
    } else if (str == "none") {
        return TokenType::None;
    } else if (str == "return") {
        return TokenType::Return;
    } else if (str == "print") {
        return TokenType::Print;
    } else if (str == "var") {
        return TokenType::Var;
    } else if (str == "func") {
        return TokenType::Func;
    } else if (str == "class") {
        return TokenType::Class;
    } else if (str == "this") {
        return TokenType::This;
    } else if (str == "super") {
        return TokenType::Super;
    }

    return TokenType::Identifier;
}
