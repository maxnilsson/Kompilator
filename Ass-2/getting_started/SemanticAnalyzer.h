#ifndef SEMANTICANALYZER_H
#define SEMANTICANALYZER_H

#include "Node.h"
#include "SymbolTable.h"
#include <string>
#include <vector>
#include <set>


class SemanticAnalyzer {
public:
    explicit SemanticAnalyzer(SymbolTable& st) : st(st), errors(0) {}
    int analyze(Node* root);

    const vector<string>& getErrors() const { return errorMessages; }

private:
    SymbolTable& st;
    int errors;
    vector<string> errorMessages;
    map<Scope*, size_t> scopeChildIdx;  // tracks next child scope to visit
    void reportError(int lineno, const string& msg);

    Scope* nextChildScope();

    bool isPrimitive(const string& t) const;
    bool isNumeric(const string& t) const;
    bool isArrayType(const string& t) const;
    string baseOfArray(const string& t) const;
    string extractType(Node* typeNode) const;
    bool isValidType(const string& t) const;

    string analyzeExpr(Node* node);

    void analyzeProgram(Node* node);
    void analyzeClass(Node* node);
    void analyzeMethod(Node* node);
    void analyzeMain(Node* node);
    void analyzeBlock(Node* node, bool isMethodBody, const string& returnType);
    void analyzeStatement(Node* node, const string& returnType, bool& hasReturn);
    void analyzeVarDecl(Node* node);

    MethodRecord* lookupMethodInClass(const string& className, const string& methodName);

    string getCurrentClassName() const;

    bool containsReturn(Node* node) const;

    bool isVarDecl(const string& nodeType) const;
};

#endif // SEMANTICANALYZER_H
