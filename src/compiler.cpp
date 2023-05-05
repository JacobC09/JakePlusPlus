#include <iostream>
#include "compiler.h"
#include "jakelang.h"
#include "interpreter.h"
#include "value.h"

Parser::Parser(const char* source) :  source(source) {
    scanner = Scanner(source);
    canAssign = false;
    hadError = false;
    currentLineNumber = 0;
}

FunctionValue Parser::compile() {
    Compiler startingCompiler = Compiler(FunctionType::Script);

    hadError = false;
    currentLineNumber = 1;
    compiler = &startingCompiler;

    advance();
    while (!isFinished()) {
        declaration();
    }

    FunctionValue function = endCompiliation();

    return hadError ? nullptr : function;
}

void Parser::advance() {
    if (hadError) return;

    previousToken = currentToken;
    
    for (;;) {
        currentToken = scanner.scanToken();

        if (currentToken.type != TokenType::Error) break;

        hadError = true;
        previousToken = currentToken;
        break;
    }
};

void Parser::consume(TokenType type, std::string msg) {
    if (currentToken.type == type) {
        advance();
        return;
    }

    errorAt(currentToken, msg);
};

bool Parser::check(TokenType type) {
    return currentToken.type == type;
}

bool Parser::match(TokenType type) {
    if (!check(type)) return false;
    advance();
    return true;
}

void Parser::errorAt(Token& token, std::string msg, bool addValue) {
    if (scanner.handledError) return;

    if (addValue) {
        printError(ExceptionType::SyntaxError, msg, token.line, std::string(token.source).c_str());
    } else {
        printError(ExceptionType::SyntaxError, msg, token.line, "");
    }

    hadError = true;
    scanner.handledError = true;
}

void Parser::error(std::string msg) {
    Parser::errorAt(previousToken, msg);
}

bool Parser::hadUnhandledError() {
    return hadError && !scanner.handledError;
}

bool Parser::isFinished() {
    return check(TokenType::EndOfFile) || check(TokenType::Error) || hadError;
}

FunctionValue Parser::endCompiliation() {
    if (hadUnhandledError()) {
        error("Invalid Syntax");
    }

    emitReturn();

    FunctionValue function = compiler->function;
    compiler = compiler->enclosing;

    #ifdef DEBUGINFO
        disassembleChunk(&function->chunk, function->name.c_str());
    #endif

    return function;
}

void Parser::emitByte(u8 byte) {
    Chunk* currentChunk = getChunk();
    
    currentChunk->bytecode.push_back(byte);    

    if (scanner.lineNumber != currentLineNumber) {
        currentChunk->lineNumbers[scanner.lineNumber] = (signed) currentChunk->bytecode.size() - 1;
        currentLineNumber = scanner.lineNumber;
    }

}

void Parser::emitConstant(Value value) {
    emitByte(OpConstant);
    emitByte(makeConstant(value));
}

void Parser::emitReturn() {
    if (compiler->type == FunctionType::Initializer) {
        emitByte(OpGetLocal);
        emitByte(0);
    } else {
        emitByte(OpNone);
    }

    emitByte(OpReturn);
}

int Parser::emitJump(u8 jumpInstruction) {
    emitByte(jumpInstruction);
    emitByte(0xff);
    emitByte(0xff);
    return (signed) getChunk()->bytecode.size() - 2;
}

int Parser::startLoop() {
    return (signed) getChunk()->bytecode.size();
}

void Parser::patchJump(int bytecodeIndex) {
    Chunk* currentChunk = getChunk();
    
    int jumpDistance = currentChunk->bytecode.size() - bytecodeIndex - 2;

    if (jumpDistance > UINT16_COUNT) {
        error("Too much code to jump over");
        return;
    }

    currentChunk->bytecode[bytecodeIndex] = jumpDistance & 0xff;
    currentChunk->bytecode[bytecodeIndex + 1] = (jumpDistance >> 8) & 0xff;
}

void Parser::emitLoop(int bytecodeIndex) {
    emitByte(OpJumpBack);

    int jumpDistance = getChunk()->bytecode.size() - bytecodeIndex + 2;

    if (jumpDistance > UINT16_COUNT) {
        error("Too much code to loop over");
        return;
    }

    emitByte(jumpDistance & 0xff);
    emitByte((jumpDistance >> 8) & 0xff);
}

Chunk* Parser::getChunk() {
    return &compiler->function->chunk;
}

ParseRule Parser::getRule(TokenType type) {
    switch (type) {
        case TokenType::Number:
            return ParseRule {&Parser::number, NULL, Precedence::None};
        case TokenType::Identifier:
            return ParseRule {&Parser::variable, NULL, Precedence::None};
        case TokenType::String:
            return ParseRule {&Parser::string, NULL, Precedence::None};
        case TokenType::And:
            return ParseRule {NULL, &Parser::__and__, Precedence::And};
        case TokenType::Or:
            return ParseRule {NULL, &Parser::__or__, Precedence::Or};
        case TokenType::LeftParen:
            return ParseRule {&Parser::grouping, &Parser::call, Precedence::Call};
        case TokenType::True:
            return ParseRule {&Parser::literal, NULL, Precedence::None};
        case TokenType::False:
            return ParseRule {&Parser::literal, NULL, Precedence::None};
        case TokenType::None:
            return ParseRule {&Parser::literal, NULL, Precedence::None};
        case TokenType::Bang:
            return ParseRule {&Parser::unary, NULL, Precedence::None};
        case TokenType::Minus:
            return ParseRule {&Parser::unary, &Parser::binary, Precedence::Term};
        case TokenType::Plus:
            return ParseRule {NULL, &Parser::binary, Precedence::Term};
        case TokenType::Asterisk:
            return ParseRule {NULL, &Parser::binary, Precedence::Factor};
        case TokenType::Slash:
            return ParseRule {NULL, &Parser::binary, Precedence::Factor};
        case TokenType::Dot:
            return ParseRule {NULL, &Parser::dot, Precedence::Call};
        case TokenType::This:
            return ParseRule {&Parser::__this__, NULL, Precedence::None};
        case TokenType::Super:
            return ParseRule {&Parser::__super__, NULL, Precedence::None};

        case TokenType::EqualEqual:
        case TokenType::BangEqual:
            return ParseRule {NULL, &Parser::binary, Precedence::Equality};

        case TokenType::Greater:
        case TokenType::Less:
        case TokenType::GreaterEqual:
        case TokenType::LessEqual:
            return ParseRule {NULL, &Parser::binary, Precedence::Comparison};

        default:
            return ParseRule {NULL, NULL, Precedence::None};
    }

}

u8 Parser::argList() {
    u8 argc = 0;

    if (!check(TokenType::RightParen)) {
        do {
            expression();
            argc++;

            if (argc == UINT8_MAX) {
                error(formatStr("Too many arguments (max: %d)", UINT8_MAX));
                error("Too many arguments (max: %d)");
            }

        } while (match(TokenType::Comma));
    }

    consume(TokenType::RightParen, "Expected ')' after arguments");
    return argc;
}

u8 Parser::makeConstant(Value value) {
    int constant = getChunk()->addConstant(value);

    if (constant > UINT8_MAX) {
        error("Too many constants in one chunk");
    }

    return (u8) constant;
}

u8 Parser::makeIdConstant(Token* identifier) {
    return makeConstant(std::make_shared<StringObj>(std::string(identifier->source)));
}

u8 Parser::parseVariableName(std::string errorMessage) {
    consume(TokenType::Identifier, errorMessage);
    declareVariable();
    return makeIdConstant(&previousToken);
}

int Parser::findLocal(Compiler* comp, Token* name) {
    for (int index = comp->localCount - 1; index >= 0; index--) {
        if (identifiersEqual(name, &comp->locals[index].name)) {
            if (comp->locals[index].depth == -1) {
                error("Can't read a local variable in its own initializer");
            }
            return index + compiler->localStackOffset;
        }
    }

    return -1;
}

int Parser::findUpValue(Compiler* comp, Token* name) {
    if (comp->enclosing == NULL) return -1;

    int local = findLocal(comp->enclosing, name);
    if (local != -1) {
        compiler->enclosing->locals[local].isCaptured = true;
        return addUpValue(comp, (u8) local, true);
    }

    int upvalue = findUpValue(comp->enclosing, name);
    if (upvalue != -1) {
        return addUpValue(comp, (u8) upvalue, false);
    }

    return -1;
}

int Parser::addUpValue(Compiler* comp, u8 index, bool isLocal) {
    int upValueCount = comp->function->upValueCount;

    for (int upValueIndex = 0; upValueIndex < upValueCount; upValueIndex++) {
        UpValue* upValue = &compiler->upValues[upValueIndex];
        if (upValue->index == index && upValue->isLocal == isLocal) {
            return upValueIndex;
        }
    }

    if (upValueCount >= UINT8_COUNT) {
        error("Too many up values in one function");
        return 0;
    }

    comp->upValues[upValueCount] = UpValue {index, isLocal};

    return comp->function->upValueCount++;
}

void Parser::addLocal(Token name) {
    compiler->locals[compiler->localCount++] = Local {name, -1};
}

void Parser::declareVariable() {
    if (compiler->scopeDepth == 0) return;

    for (int index = compiler->localCount - 1; index >= 0; index--) {
        Local &local = compiler->locals[index];

        if (compiler->scopeDepth > local.depth && index != -1)
            break;

        if (identifiersEqual(&local.name, &previousToken)) {
            error("There is already a variable with the same name in this scope");
            break;
        }
    }

    addLocal(previousToken);
}

void Parser::namedVariable(Token name) {
    u8 getOp, setOp;

    int arg = findLocal(compiler, &name);

    if (arg != -1) {
        getOp = OpGetLocal;
        setOp = OpSetLocal;
    } else if ((arg = findUpValue(compiler, &name)) != -1) {
        getOp = OpGetUpValue;
        setOp = OpSetUpValue;
    } else {
        getOp = OpGetGlobal;
        setOp = OpSetGlobal;
        arg = makeIdConstant(&name);
    }

    bool isAssignment = canAssign && name.type != TokenType::This && (check(TokenType::Equal) || check(TokenType::PlusEqual) || check(TokenType::MinusEqual) ||
        check(TokenType::AsteriskEqual) || check(TokenType::SlashEqual));

    if (isAssignment) {
        if (match(TokenType::Equal)) {
            expression();
            emitByte(setOp);
            emitByte((u8) arg);
        } else {
            TokenType type = currentToken.type;
            advance();
            emitByte(getOp);
            emitByte((u8) arg);
            expression();

            switch (type) {
                case TokenType::PlusEqual:
                    emitByte(OpAdd);
                    break;
                case TokenType::MinusEqual:
                    emitByte(OpSubtract);
                    break;
                case TokenType::AsteriskEqual:
                    emitByte(OpMultiply);
                    break;
                case TokenType::SlashEqual:
                    emitByte(OpDivide);
                    break;
                default:
                    break;
            }

            emitByte(setOp);
            emitByte((u8) arg);
        }
    } else {
        emitByte(getOp);
        emitByte((u8) arg);
    }
}

void Parser::defineVariable(int global) {
    if (compiler->scopeDepth > 0) {
        markInitialized();
        return;
    }
    
    emitByte(OpDefineGlobal);
    emitByte(global);
}

void Parser::markInitialized() {
    if (compiler->scopeDepth > 0) {
        compiler->locals[compiler->localCount - 1].depth = compiler->scopeDepth;
        return;
    }
}

void Parser::__and__() {
    int endJump = emitJump(OpJumpIfFalse);
    emitByte(OpPop);
    parsePrecedence(Precedence::And);
    patchJump(endJump);
}

void Parser::__or__() {
    int endJump = emitJump(OpJumpIfTrue);
    emitByte(OpPop);
    parsePrecedence(Precedence::Or);
    patchJump(endJump);
}

void Parser::__this__() {
    if (currentClass == NULL) {
        error("Can't use 'this' outside of a class");
        return;
    }

    variable();
}

void Parser::__super__() {
    if (currentClass == NULL) {
        error("Can't use 'super' outside of a class.");
    } else if (!currentClass->hasSuperClass) {
        error("Can't use 'super' in a class with no superclass.");
    }

    consume(TokenType::Dot, "Expect '.' after 'super'.");
    consume(TokenType::Identifier, "Expect superclass method name.");
    u8 name = makeIdConstant(&previousToken);
    
    namedVariable(Token{TokenType::Identifier, "this"});
    namedVariable(Token{TokenType::Identifier, "super"});
    emitByte(OpGetSuper);
    emitByte(name);

}

void Parser::number() {
    double value;

    if (previousToken.source.front() == '.') {
        std::string literal = std::string(previousToken.source);
        value = std::stod("0." + literal);
    } else {
        value = std::stod(std::string(previousToken.source));
    }

    emitConstant(NUMBER_VAL(value));
}

void Parser::variable() {
    namedVariable(previousToken);
}

void Parser::string() {
    emitConstant(std::make_shared<StringObj>(std::string(previousToken.source.data() + 1, previousToken.source.size() - 2)));
}

void Parser::literal() {
    switch (previousToken.type) {
        case TokenType::True:
            emitByte(OpTrue);
            break;
        case TokenType::False:
            emitByte(OpFalse);
            break;
        case TokenType::None:
            emitByte(OpNone);
            break;
        default:
            return;
    }
}

void Parser::grouping() {
    expression();
    consume(TokenType::RightParen, "Expected a closing parenthesis");
}

void Parser::call() {
    u8 argc = argList();
    emitByte(OpCall);
    emitByte(argc);
}

void Parser::dot() {
    consume(TokenType::Identifier, "Expected identifier after '.'");

    u8 id = makeIdConstant(&previousToken);

    if (canAssign && match(TokenType::Equal)) {
        expression();
        emitByte(OpSetProperty);
        emitByte(id);
    } else if (match(TokenType::LeftParen)) {
        u8 argc = argList();
        emitByte(OpInvoke);
        emitByte(id);
        emitByte(argc);
    } else {
        emitByte(OpGetProperty);
        emitByte(id);
    }
}

void Parser::binary() {
    TokenType operatorType = previousToken.type;

    ParseRule rule = getRule(operatorType);
    parsePrecedence((Precedence) ((int) rule.precedence + 1));

    switch (operatorType) {
        case TokenType::Plus:
            emitByte(OpAdd);
            break;
        case TokenType::Minus:
            emitByte(OpSubtract);
            break;
        case TokenType::Asterisk:
            emitByte(OpMultiply);
            break;
        case TokenType::Slash:
            emitByte(OpDivide);
            break;
        case TokenType::EqualEqual:
            emitByte(OpEqual);
            break;
        case TokenType::BangEqual:
            emitByte(OpNotEqual);
            break;
        case TokenType::Greater:
            emitByte(OpGreater);
            break;
        case TokenType::GreaterEqual:
            emitByte(OpGreaterEqual);
            break;
        case TokenType::Less:
            emitByte(OpLess);
            break;
        case TokenType::LessEqual:
            emitByte(OpLessEqual);
            break;
        default:
            break;
    }
}

void Parser::unary() {
    switch (previousToken.type) {
        case TokenType::Minus:
            parsePrecedence(Precedence::Unary);
            emitByte(OpNegate);
            break;

        case TokenType::Bang:
            parsePrecedence(Precedence::Equality);
            emitByte(OpNot);
            break;
        
        default: 
            return;
    }
}

void Parser::parsePrecedence(Precedence precedence) {
    advance();

    ParseFn prefix = getRule(previousToken.type).prefix;
    
    if (prefix == NULL) {
        error("Expected an expression");
        return;
    }

    canAssign = precedence <= Precedence::Assignment;

    (this->*prefix)();  // TODO: Add function parameter for can assign to enable block expressions that may change global can assign value to true 

    while (precedence <= getRule(currentToken.type).precedence) {
        advance();
        ParseFn infix = getRule(previousToken.type).infix;
        (this->*infix)();
    }
    
    bool isAssignment = check(TokenType::Equal) || check(TokenType::PlusEqual) || check(TokenType::MinusEqual) ||
        check(TokenType::AsteriskEqual) || check(TokenType::SlashEqual);

    if (precedence <= Precedence::Assignment && isAssignment) {
        error("Invalid assignment target");
        advance();
    }
}

void Parser::expression() {
    parsePrecedence(Precedence::Assignment);
}

void Parser::beginScope() {
    compiler->scopeDepth++;
}

void Parser::endScope() {
    compiler->scopeDepth--;

    while (compiler->locals[compiler->localCount - 1].depth > compiler->scopeDepth && compiler->localCount > 0) {
        
        if (compiler->locals[compiler->localCount - 1].isCaptured) {
            emitByte(OpCloseUpValue);
        } else {
            emitByte(OpPop);
        }
        compiler->localCount--;
    }
}

void Parser::block() {
    advance();

    while (!check(TokenType::RightBrace) && !isFinished()) {
        declaration();
    }

    consume(TokenType::RightBrace, "Expected '}' after block");
}

void Parser::function(FunctionType type) {
    Compiler funcCompiler = Compiler(type);
    funcCompiler.enclosing = compiler;
    funcCompiler.function->name = std::string(previousToken.source);
    compiler = &funcCompiler;

    beginScope(); 
    consume(TokenType::LeftParen, "Expected '(' after function name");
    
    if (!check(TokenType::RightParen)) {
        do {
            compiler->function->argc++;
            if (compiler->function->argc > 255) {
                error("Can't have more than 255 parameters");
            }

            u8 constant = parseVariableName("Expect parameter name");
            defineVariable(constant);

        } while (match(TokenType::Comma));
    }

    consume(TokenType::RightParen, "Expected ')' after parameters");
    
    if (!check(TokenType::LeftBrace))
        errorAt(currentToken, "Expected '{' before function body");

    block();

    FunctionValue function = endCompiliation();

    emitByte(OpClosure);
    emitByte(makeConstant(function));

    for (int index = 0; index < function->upValueCount; index++) {
        emitByte(funcCompiler.upValues[index].isLocal ? 1 : 0);
        emitByte(funcCompiler.upValues[index].index);
    }
}

void Parser::method() {
    consume(TokenType::Identifier, "Expected method name");
    u8 constant = makeIdConstant(&previousToken);

    FunctionType type = previousToken.source == constructorName ? FunctionType::Initializer : FunctionType::Method;
    function(type);

    emitByte(OpMethod);
    emitByte(constant);
}

void Parser::expressionStatement() {
    expression();
    emitByte(OpPop);
    consume(TokenType::Semicolon, "Expected ';' after expression");
}

void Parser::printStatement() {
    advance();
    expression();
    emitByte(OpPrint);
    consume(TokenType::Semicolon, "Expected ';' after print statement");
}

void Parser::returnStatement() {
    if (compiler->type == FunctionType::Script) {
        error("Cannot return from top level of code");
        return;
    }

    advance();

    if (match(TokenType::Semicolon)) {
        emitReturn();
    } else if (compiler->type == FunctionType::Initializer) {
        error("Can't return a value from an initializer");
    } else {
        expression();
        emitByte(OpReturn);
        consume(TokenType::Semicolon, "Expected ';' after return statement");
    }
}

void Parser::ifStatement() {
    advance();
    consume(TokenType::LeftParen, "Expected '(' before condition");
    expression();
    consume(TokenType::RightParen, "Expected ')' after condition");

    int ifJump = emitJump(OpJumpIfFalse);
    emitByte(OpPop);
    statement();

    int elseJump = emitJump(OpJump);
    
    patchJump(ifJump);
    emitByte(OpPop);

    if (match(TokenType::Else))
        statement();
    
    patchJump(elseJump);
}

void Parser::whileLoop() {
    int loopStart = startLoop();
    advance();
    consume(TokenType::LeftParen, "Expected '(' before condition");
    expression();
    consume(TokenType::RightParen, "Expected ')' after condition");
    
    int exitJump = emitJump(OpJumpIfFalse);
    emitByte(OpPop);
    statement();
    emitLoop(loopStart);

    patchJump(exitJump);
    emitByte(OpPop);
}

void Parser::forLoop() {
    advance();
    beginScope();
    consume(TokenType::LeftParen, "Expected '(' after 'for'");
    
    // Initializer Clause
    if (check(TokenType::Var)) {
        varDeclaration();
    } else if (!match(TokenType::Semicolon)) {
        expressionStatement();
    }

    // Condition Clause
    int exitJump = -1;
    int loopStart = startLoop();

    if (!match(TokenType::Semicolon)) {
        expression();
        consume(TokenType::Semicolon, "Expected ';' after loop conditon");

        exitJump = emitJump(OpJumpIfFalse);
        emitByte(OpPop);
    }

    // Increment Clause
    if (!match(TokenType::RightParen)) {
        int bodyJump = emitJump(OpJump);
        int incrementStart = startLoop();

        expression();
        emitByte(OpPop);
        consume(TokenType::RightParen, "Expected ')' after increment clause");

        emitLoop(loopStart);
        loopStart = incrementStart;
        patchJump(bodyJump);
    }

    statement();
    emitLoop(loopStart);

    if (exitJump != -1) {
        patchJump(exitJump);
        emitByte(OpPop);
    }

    endScope();
}

void Parser::statement() {
    switch (currentToken.type) {
        case TokenType::If:
            ifStatement();
            break;

        case TokenType::While:
            whileLoop();
            break;
        
        case TokenType::For:
            forLoop();
            break;

        case TokenType::Return:
            returnStatement();
            break;

        case TokenType::Print:
            printStatement();
            break;
        
        case TokenType::LeftBrace:
            beginScope();
            block();
            endScope();
            break;

        case TokenType::Semicolon:
            advance();
            break;

        default:
            expressionStatement();
            break;
    }
}

void Parser::varDeclaration() {
    advance();
    u8 global = parseVariableName("Invalid variable name");

    if (match(TokenType::Equal)) {
        expression();
    } else {
        emitByte(OpNone);
    }

    consume(TokenType::Semicolon, "Expected ';' after variable declaration");
    defineVariable(global);
} 

void Parser::funcDeclaration() {
    advance();
    u8 global = parseVariableName("Expected function name");
    markInitialized();
    function(FunctionType::Function);
    defineVariable(global);
}

void Parser::classDeclaration() {
    advance();
    consume(TokenType::Identifier, "Expected class name");
    
    Token className = previousToken;
    u8 nameConstant = makeIdConstant(&previousToken);
    
    declareVariable();
    
    emitByte(OpClass);
    emitByte(nameConstant);
    defineVariable(nameConstant);

    ClassCompiler classCompiler;
    classCompiler.enclosing = currentClass;
    classCompiler.hasSuperClass = false;
    currentClass = &classCompiler;
    
    canAssign = false; // TODO: This should be removed when parameter can assign is implimented

    if (match(TokenType::Less)) {
        consume(TokenType::Identifier, "Expected base class name");

        if (identifiersEqual(&previousToken, &className)) {
            error("A class can't inherit from itself");
        }

        variable();

        beginScope();
        addLocal(Token{TokenType::Identifier, "super"});
        markInitialized();

        namedVariable(className);
        emitByte(OpInherit);

        classCompiler.hasSuperClass = true;
    }


    namedVariable(className);
    consume(TokenType::LeftBrace, "Expected '{' before class body");

    while (!isFinished() && !check(TokenType::RightBrace)) {
        method();
    }

    consume(TokenType::RightBrace, "Expected '}' after class body");
    emitByte(OpPop);
    
    if (classCompiler.hasSuperClass) {
        endScope();
    }


    currentClass = currentClass->enclosing;
}

void Parser::declaration() {
    switch (currentToken.type) {
        case TokenType::Var:
            varDeclaration();
            break;

        case TokenType::Func:
            funcDeclaration();
            break;
        
        case TokenType::Class:
            classDeclaration();
            break;

        default:
            statement();
            break;
    }
}

// Compiler

Compiler::Compiler(FunctionType type) : type(type) {
    scopeDepth = 0;
    function = std::make_shared<FunctionObj>();
    enclosing = NULL;

    if (type == FunctionType::Function) {
        localCount = 0;
        localStackOffset = 1;
    } else {
        localCount = 1;
        localStackOffset = 0;
        locals[0] = Local{Token{TokenType::Identifier, "this"}, 0};
    }
}
