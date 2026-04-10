#ifndef STBUILDER_H
#define STBUILDER_H

#include "Node.h"
#include "SymbolTable.h"
#include <string>
#include <vector>

class STBuilder {
public:
    explicit STBuilder(SymbolTable& st) : st(st), errors(0) {}

    int build(Node* root);

    const vector<string>& getErrors() const { return errorMessages; }

private:
    SymbolTable& st;
    int errors;
    vector<string> errorMessages;

    void reportError(int lineno, const string& msg);
    string extractType(Node* typeNode) const;
    bool isVarDecl(const string& nodeType) const;

    bool isVolatileDecl(const string& nodeType) const;     // Check node type volatile variable

    bool hasInit(const string& nodeType) const;    // Check node declaration

    void visitProgram(Node* node);
    void visitGlobals(Node* node);
    void visitClasses(Node* node);
    void visitClass(Node* node);
    void visitMethod(Node* node);
    void visitMain(Node* node);
    void visitParams(Node* node);
    void visitBlock(Node* node, bool isMethodBody);
    void visitStatement(Node* node);
    void visitVarDecl(Node* node, VariableKind kind);
};

#endif 
