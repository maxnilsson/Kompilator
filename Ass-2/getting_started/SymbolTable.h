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

enum class ScopeType {
    PROGRAM,   //global variable and class
    CLASS, 
    METHOD,
    BLOCK   // if, for 
};

enum class RecordType {
    CLASS,
    METHOD,
    VARIABLE
};

enum class VariableKind {
    FIELD,      // Class
    PARAMETER,  // Method parameter
    LOCAL       // variable
};

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

struct ClassRecord : public Record {
    ClassRecord(const string& className, int line)
        : Record(className, className, line, RecordType::CLASS) {}

    string toString() const override;
};


struct MethodRecord : public Record {
    vector<pair<string, string>> parameters; 

    MethodRecord(const string& methodName, const string& returnType, int line)
        : Record(methodName, returnType, line, RecordType::METHOD) {}

    bool addParameter(const string& pName, const string& pType);

    string toString() const override;
};


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


class Scope {
public:
    string           name;  
    ScopeType        scopeType;
    int              level;      
    Scope*           parent;
    vector<Scope*>   children;
    map<string, Record*> symbols;

    Scope(const string& n, ScopeType st, int lvl, Scope* par)
        : name(n), scopeType(st), level(lvl), parent(par) {}

    ~Scope();

    bool insert(Record* record);
    Record* lookupLocal(const string& id) const;
    // identifier
    Record* lookup(const string& id) const;
};

class SymbolTable {
private:
    Scope* root;          
    Scope* current; 

    void printScope(const Scope* scope, int indent) const;
    void generateDotContent(const Scope* scope, int& nodeId, ostream& out) const;

public:
    SymbolTable();
    ~SymbolTable();

    // creates child
    void enterScope(const string& name, ScopeType type);
    void leaveScope();

    bool insert(Record* record);

    Record* lookup(const string& name) const;

    // identifier
    Record* lookupLocal(const string& name) const;

    Scope*    getCurrentScope() const { return current; }
    Scope*    getRootScope()    const { return root;    }
    ScopeType getCurrentScopeType() const { return current->scopeType; }

    void setCurrentScope(Scope* s) { current = s; }
    void resetToRoot() { current = root; }

    // Find class scope 
    Scope* lookupClassScope(const string& className) const;

    // From current scope to scope.
    Scope* getEnclosingClassScope() const;
    Scope* getEnclosingMethodScope() const;

    void printTable() const;
    void generateDot(const string& filename = "symboltable.dot") const;
};

#endif 
