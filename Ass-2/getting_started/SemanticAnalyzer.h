#ifndef SEMANTICANALYZER_H
#define SEMANTICANALYZER_H

#include "Node.h"
#include "SymbolTable.h"
#include <string>
#include <vector>
#include <set>

/**
 * SemanticAnalyzer – Second AST pass that performs full semantic checking.
 *
 * Assumes the SymbolTable has already been fully populated by STBuilder.
 * Performs:
 *   - Undeclared identifier checks
 *   - Type checking (expressions, assignments, returns, conditions)
 *   - Method call validation (argument count & types)
 *   - Array usage validation (index type, .length only on arrays)
 *   - Unreachable statement detection
 *   - Undefined type checks for variable declarations
 */
class SemanticAnalyzer {
public:
    explicit SemanticAnalyzer(SymbolTable& st) : st(st), errors(0) {}

    /** Entry point – analyse the whole AST and return error count. */
    int analyze(Node* root);

    const vector<string>& getErrors() const { return errorMessages; }

private:
    SymbolTable& st;
    int errors;
    vector<string> errorMessages;
    map<Scope*, size_t> scopeChildIdx;  // tracks next child scope to visit

    // ── Error reporting ──────────────────────────────────────────────
    void reportError(int lineno, const string& msg);

    /** Get the next unvisited child scope of the current scope. */
    Scope* nextChildScope();

    // ── Type helpers ─────────────────────────────────────────────────
    bool isPrimitive(const string& t) const;
    bool isNumeric(const string& t) const;
    bool isArrayType(const string& t) const;
    string baseOfArray(const string& t) const;
    string extractType(Node* typeNode) const;
    bool isValidType(const string& t) const;

    // ── Expression type inference ────────────────────────────────────
    /** Returns the inferred type of an expression, or "error" on failure. */
    string analyzeExpr(Node* node);

    // ── Statement analysis ───────────────────────────────────────────
    void analyzeProgram(Node* node);
    void analyzeClass(Node* node);
    void analyzeMethod(Node* node);
    void analyzeMain(Node* node);
    void analyzeBlock(Node* node, bool isMethodBody, const string& returnType);
    void analyzeStatement(Node* node, const string& returnType, bool& hasReturn);
    void analyzeVarDecl(Node* node);

    // ── Method call resolution ───────────────────────────────────────
    /** Look up a method in a class scope and return its MethodRecord, or nullptr. */
    MethodRecord* lookupMethodInClass(const string& className, const string& methodName);

    /** Find the class that the current scope belongs to (for "self" calls). */
    string getCurrentClassName() const;

    /** Recursively check if a block/statement contains a return statement. */
    bool containsReturn(Node* node) const;

    // ── Helper: check if node type is a var decl ─────────────────────
    bool isVarDecl(const string& nodeType) const;
};

#endif // SEMANTICANALYZER_H
