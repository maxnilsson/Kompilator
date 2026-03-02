#ifndef STBUILDER_H
#define STBUILDER_H

#include "Node.h"
#include "SymbolTable.h"
#include <string>
#include <vector>

/**
 * STBuilder  –  Single left-to-right AST traversal that populates a SymbolTable.
 *
 * Usage:
 *     SymbolTable st;
 *     STBuilder builder(st);
 *     int errors = builder.build(root);   // root = AST from parser
 *
 * After build() returns, the SymbolTable is fully populated with all
 * declarations found in the program.  The return value is the number
 * of semantic errors detected during the declaration phase (e.g.
 * duplicate identifiers).
 */
class STBuilder {
public:
    explicit STBuilder(SymbolTable& st) : st(st), errors(0) {}

    /** Entry point – traverse the whole AST and return the error count. */
    int build(Node* root);

    /** Access the collected error messages. */
    const vector<string>& getErrors() const { return errorMessages; }

private:
    SymbolTable& st;
    int errors;
    vector<string> errorMessages;

    // ── Helpers ──────────────────────────────────────────────────────

    /** Report a semantic error (duplicate declaration). */
    void reportError(int lineno, const string& msg);

    /** Extract the type string from a Type / ArrayType node. */
    string extractType(Node* typeNode) const;

    /** Check whether a node type represents a variable declaration. */
    bool isVarDecl(const string& nodeType) const;

    /** Check whether a node type represents a volatile variable declaration. */
    bool isVolatileDecl(const string& nodeType) const;

    /** Check whether a node's declaration includes an initialiser. */
    bool hasInit(const string& nodeType) const;

    // ── Traversal methods (one per grammar construct) ────────────────

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

#endif // STBUILDER_H
