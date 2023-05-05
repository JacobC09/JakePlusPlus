#pragma once
#include "common.h"
#include "scanner.h"
#include "interpreter.h"

class Parser;

enum class Precedence {
    None,
    Assignment,  // =
    Or,          // or
    And,         // and
    Equality,    // == !=
    Comparison,  // < > <= >=
    Term,        // + -
    Factor,      // * /
    Unary,       // ! -
    Call,        // . ()
    Primary
};

enum class FunctionType {
    Function,
    Script,
    Method,
    Initializer
};

typedef void (Parser::*ParseFn)();  

struct ParseRule {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
};

struct Local {
    Token name;
    int depth;
    bool isCaptured = false;
};

struct UpValue {
    u8 index;
    bool isLocal;
};

class Compiler {
public:
    int localCount;
    int scopeDepth;
    int localStackOffset;
    Local locals[UINT8_COUNT];
    UpValue upValues[UINT8_COUNT];
    FunctionValue function;
    FunctionType type;
    Compiler* enclosing;

    Compiler(FunctionType type);
};

class ClassCompiler {
public:
    ClassCompiler* enclosing;
    bool hasSuperClass;
};

class Parser {
public:
    Parser(const char* source);

    FunctionValue compile();

private:
    bool hadError;
    bool canAssign;
    int currentLineNumber;
    const char* source;
    Token currentToken;
    Token previousToken;
    Scanner scanner;
    Compiler* compiler = nullptr;
    ClassCompiler* currentClass = nullptr;

    // Token Functions
    
    void advance();
    void consume(TokenType type, std::string msg);
    bool check(TokenType type);
    bool match(TokenType type);

    void errorAt(Token& token, std::string msg, bool addValue=false);
    void error(std::string msg);
    bool hadUnhandledError();
    bool isFinished();

    FunctionValue endCompiliation();
    void emitByte(u8 byte);
    void emitConstant(Value value);
    void emitReturn();
    int emitJump(u8 jumpInstruction);
    int startLoop();
    void patchJump(int bytecodeIndex);
    void emitLoop(int bytecodeIndex);

    Chunk* getChunk();
    ParseRule getRule(TokenType type);
    u8 argList();
    u8 makeConstant(Value value);
    u8 makeIdConstant(Token* identifier);
    u8 parseVariableName(std::string errorMessage);
    int findLocal(Compiler* comp, Token* name);
    int findUpValue(Compiler* comp, Token* name);
    int addUpValue(Compiler* comp, u8 index, bool isLocal);
    void addLocal(Token name);
    void declareVariable();
    void namedVariable(Token name);
    void defineVariable(int global);
    void markInitialized();

    void __and__();
    void __or__();
    void __this__();
    void __super__();
    void number();
    void variable();
    void string();
    void literal();
    void grouping();
    void call();
    void dot();
    void unary();
    void binary();

    void expression();
    void parsePrecedence(Precedence precedence);

    void beginScope();
    void endScope();
    void block();

    void function(FunctionType type);
    void method();

    void expressionStatement();
    void printStatement();
    void returnStatement();
    void ifStatement();
    void whileLoop();
    void forLoop();
    void statement();
    
    void varDeclaration(); 
    void funcDeclaration();
    void classDeclaration();
    void declaration();
};
