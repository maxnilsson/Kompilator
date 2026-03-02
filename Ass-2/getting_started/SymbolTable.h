#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H

#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>

using namespace std;

// ═══════════════════════════════════════════════════════════════════════
//  Enumerations
// ═══════════════════════════════════════════════════════════════════════

/** The four-level scope hierarchy: Program → Class → Method → Block */
enum class ScopeType {
    PROGRAM,   // Top-level (global variables + class declarations + main)
    CLASS,     // Inside a class body
    METHOD,    // Inside a method (or main) body
    BLOCK      // Nested block (if/for/standalone { })
};

/** Distinguishes the three kinds of symbol table records. */
enum class RecordType {
    CLASS,
    METHOD,
    VARIABLE
};

/** Sub-classification for variables. */
enum class VariableKind {
    FIELD,      // Class field
    PARAMETER,  // Method parameter
    LOCAL       // Local variable (in method or block)
};

// ─── Enum → String helpers ───────────────────────────────────────────

inline string scopeTypeToString(ScopeType st) {
    switch (st) {
        case ScopeType::PROGRAM: return "Program";
        case ScopeType::CLASS:   return "Class";
        case ScopeType::METHOD:  return "Method";
        case ScopeType::BLOCK:   return "Block";
    }
    return "Unknown";
}

inline string recordTypeToString(RecordType rt) {
    switch (rt) {
        case RecordType::CLASS:    return "Class";
        case RecordType::METHOD:   return "Method";
        case RecordType::VARIABLE: return "Variable";
    }
    return "Unknown";
}

inline string variableKindToString(VariableKind vk) {
    switch (vk) {
        case VariableKind::FIELD:     return "Field";
        case VariableKind::PARAMETER: return "Parameter";
        case VariableKind::LOCAL:     return "Local";
    }
    return "Unknown";
}

// ═══════════════════════════════════════════════════════════════════════
//  Records  –  one concrete struct per identifier category
// ═══════════════════════════════════════════════════════════════════════

/**
 * Base record stored in every scope's symbol map.
 *
 *   name   – the identifier text
 *   type   – semantic meaning depends on subclass:
 *              ClassRecord    → class name (same as `name`)
 *              MethodRecord   → return type ("int", "void", …)
 *              VariableRecord → declared type ("int", "float", "int[]", …)
 *   lineno – source line where the identifier was declared
 */
struct Record {
    string     name;
    string     type;
    int        lineno;
    RecordType recordType;

    Record(const string& n, const string& t, int line, RecordType rt)
        : name(n), type(t), lineno(line), recordType(rt) {}

    virtual ~Record() = default;
    virtual string toString() const;
};

// ─────────────────────────────────────────────────────────────────────

/** Record for a class declaration. */
struct ClassRecord : public Record {
    ClassRecord(const string& className, int line)
        : Record(className, className, line, RecordType::CLASS) {}

    string toString() const override;
};

// ─────────────────────────────────────────────────────────────────────

/**
 * Record for a method (or `main`) declaration.
 *
 *   type (inherited)  – the return type
 *   parameters        – ordered list of (name, type) pairs
 */
struct MethodRecord : public Record {
    vector<pair<string, string>> parameters;   // (paramName, paramType)

    MethodRecord(const string& methodName, const string& returnType, int line)
        : Record(methodName, returnType, line, RecordType::METHOD) {}

    /** Register a formal parameter.  Returns false if a duplicate name. */
    bool addParameter(const string& pName, const string& pType);

    string toString() const override;
};

// ─────────────────────────────────────────────────────────────────────

/**
 * Record for a variable (field, parameter or local).
 *
 *   type (inherited) – declared type (e.g. "int", "boolean", "MyClass", "int[]")
 *   varKind          – FIELD / PARAMETER / LOCAL
 *   isVolatile       – declared with the `volatile` keyword
 *   isArray          – true when the type is an array (e.g. "int[]")
 */
struct VariableRecord : public Record {
    VariableKind varKind;
    bool         isVolatile;
    bool         isArray;

    VariableRecord(const string& varName, const string& varType, int line,
                   VariableKind kind, bool vol = false, bool arr = false)
        : Record(varName, varType, line, RecordType::VARIABLE),
          varKind(kind), isVolatile(vol), isArray(arr) {}

    string toString() const override;
};

// ═══════════════════════════════════════════════════════════════════════
//  Scope  –  one node in the scope tree
// ═══════════════════════════════════════════════════════════════════════

class Scope {
public:
    string           name;       // human-readable label (class name, method name, …)
    ScopeType        scopeType;
    int              level;      // nesting depth (0 = PROGRAM)
    Scope*           parent;
    vector<Scope*>   children;
    map<string, Record*> symbols;

    Scope(const string& n, ScopeType st, int lvl, Scope* par)
        : name(n), scopeType(st), level(lvl), parent(par) {}

    ~Scope();

    /** Insert a record.  Returns true on success, false if the name already exists. */
    bool insert(Record* record);

    /** Look up an identifier in *this scope only*. */
    Record* lookupLocal(const string& id) const;

    /** Look up an identifier walking up through ancestor scopes. */
    Record* lookup(const string& id) const;
};

// ═══════════════════════════════════════════════════════════════════════
//  SymbolTable  –  the public API used by the semantic-analysis pass
// ═══════════════════════════════════════════════════════════════════════

class SymbolTable {
private:
    Scope* root;           // The PROGRAM scope (always exists)
    Scope* current;        // The scope we are currently inside

    // Internal helpers for visualisation
    void printScope(const Scope* scope, int indent) const;
    void generateDotContent(const Scope* scope, int& nodeId, ostream& out) const;

public:
    SymbolTable();
    ~SymbolTable();

    // ── Scope management ─────────────────────────────────────────────

    /** Create a new child scope under the current scope and enter it. */
    void enterScope(const string& name, ScopeType type);

    /** Leave the current scope (move up to the parent).
     *  Does nothing if already at the PROGRAM scope. */
    void leaveScope();

    // ── Symbol operations ────────────────────────────────────────────

    /** Insert a record into the *current* scope.
     *  Returns true on success, false on duplicate. */
    bool insert(Record* record);

    /** Look up an identifier starting from the current scope upward. */
    Record* lookup(const string& name) const;

    /** Look up an identifier in the current scope only (no parent walk). */
    Record* lookupLocal(const string& name) const;

    // ── Accessors ────────────────────────────────────────────────────

    Scope*    getCurrentScope() const { return current; }
    Scope*    getRootScope()    const { return root;    }
    ScopeType getCurrentScopeType() const { return current->scopeType; }

    /** Set current scope directly (used by semantic-analysis pass). */
    void setCurrentScope(Scope* s) { current = s; }

    /** Reset the traversal pointer back to root. */
    void resetToRoot() { current = root; }

    /** Find the class scope that is a child of root with the given name. */
    Scope* lookupClassScope(const string& className) const;

    /** Walk up from current scope to find the enclosing CLASS scope. */
    Scope* getEnclosingClassScope() const;

    /** Walk up from current scope to find the enclosing METHOD scope. */
    Scope* getEnclosingMethodScope() const;

    // ── Visualisation ────────────────────────────────────────────────

    /** Pretty-print the whole symbol table to stdout. */
    void printTable() const;

    /** Write a Graphviz DOT file representing the scope tree. */
    void generateDot(const string& filename = "symboltable.dot") const;
};

#endif // SYMBOLTABLE_H
