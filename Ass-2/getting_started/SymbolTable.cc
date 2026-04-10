#include "SymbolTable.h"

string Record::toString() const {
    return "[Record] " + name + " : " + type + " (line " + to_string(lineno) + ")";
}

string ClassRecord::toString() const {
    return "[Class] " + name + " (line " + to_string(lineno) + ")";
}


bool MethodRecord::addParameter(const string& pName, const string& pType) {
    // duplicate parameter
    for (const auto& p : parameters) {
        if (p.first == pName)
            return false;
    }
    parameters.emplace_back(pName, pType);
    return true;
}

string MethodRecord::toString() const {
    ostringstream oss;
    oss << "[Method] " << name << "(";
    for (size_t i = 0; i < parameters.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << parameters[i].first << ":" << parameters[i].second;
    }
    oss << ") : " << type << " (line " << lineno << ")";
    return oss.str();
}

string VariableRecord::toString() const {
    ostringstream oss;
    oss << "[Variable/" << variableKindToString(varKind) << "] "
        << name << " : " << type;
    if (isVolatile) oss << " (volatile)";
    oss << " (line " << lineno << ")";
    return oss.str();
}

Scope::~Scope() {

    for (Scope* child : children)
        delete child;

    for (auto& pair : symbols)
        delete pair.second;
}

bool Scope::insert(Record* record) {
    if (symbols.find(record->name) != symbols.end())
        return false;  
    symbols[record->name] = record;
    return true;
}

Record* Scope::lookupLocal(const string& id) const {
    auto it = symbols.find(id);
    if (it != symbols.end())
        return it->second;
    return nullptr;
}

Record* Scope::lookup(const string& id) const {
    // Current scope then search tree
    Record* rec = lookupLocal(id);
    if (rec)
        return rec;
    if (parent)
        return parent->lookup(id);
    return nullptr;
}


SymbolTable::SymbolTable() {
    root    = new Scope("Program", ScopeType::PROGRAM, 0, nullptr);
    current = root;
}

SymbolTable::~SymbolTable() {
    delete root;  
}


void SymbolTable::enterScope(const string& name, ScopeType type) {
    Scope* child = new Scope(name, type, current->level + 1, current);
    current->children.push_back(child);
    current = child;
}

void SymbolTable::leaveScope() {
    if (current->parent)
        current = current->parent;
}

bool SymbolTable::insert(Record* record) {
    return current->insert(record);
}

Record* SymbolTable::lookup(const string& name) const {
    return current->lookup(name);
}

Record* SymbolTable::lookupLocal(const string& name) const {
    return current->lookupLocal(name);
}

Scope* SymbolTable::lookupClassScope(const string& className) const {
    for (Scope* child : root->children) {
        if (child->scopeType == ScopeType::CLASS && child->name == className)
            return child;
    }
    return nullptr;
}

Scope* SymbolTable::getEnclosingClassScope() const {
    Scope* s = current;
    while (s) {
        if (s->scopeType == ScopeType::CLASS)
            return s;
        s = s->parent;
    }
    return nullptr;
}

Scope* SymbolTable::getEnclosingMethodScope() const {
    Scope* s = current;
    while (s) {
        if (s->scopeType == ScopeType::METHOD)
            return s;
        s = s->parent;
    }
    return nullptr;
}

void SymbolTable::printTable() const {
    cout << "\n";
    cout << "╔══════════════════════════════════════════════════════════╗\n";
    cout << "║                    SYMBOL  TABLE                        ║\n";
    cout << "╚══════════════════════════════════════════════════════════╝\n";
    printScope(root, 0);
    cout << "\n";
}

void SymbolTable::printScope(const Scope* scope, int indent) const {
    string pad(indent * 2, ' ');

    cout << pad << "┌── Scope: " << scope->name
         << "  [" << scopeTypeToString(scope->scopeType) << "]"
         << "  (level " << scope->level << ")\n";

    if (scope->symbols.empty()) {
        cout << pad << "│   (no symbols)\n";
    } else {
        for (const auto& entry : scope->symbols) {
            cout << pad << "│   " << entry.second->toString() << "\n";
        }
    }

    for (const Scope* child : scope->children) {
        printScope(child, indent + 1);
    }

    cout << pad << "└──\n";
}

void SymbolTable::generateDot(const string& filename) const {
    ofstream out(filename);
    if (!out.is_open()) {
        cerr << "Error: could not open " << filename << " for writing.\n";
        return;
    }

    int nodeId = 0;
    out << "digraph SymbolTable {\n";
    out << "  rankdir=TB;\n";
    out << "  node [shape=record, fontname=\"Courier New\", fontsize=10];\n";
    out << "  edge [arrowhead=vee];\n\n";

    generateDotContent(root, nodeId, out);

    out << "}\n";
    out.close();

    cout << "\nSymbol-table DOT file written to " << filename
         << ".  Use 'dot -Tpdf " << filename << " -o symboltable.pdf' to render.\n";
}

void SymbolTable::generateDotContent(const Scope* scope, int& nodeId,
                                     ostream& out) const {
    int myId = nodeId++;

    ostringstream label;
    label << "{" << scopeTypeToString(scope->scopeType)
          << ": " << scope->name;

    for (const auto& entry : scope->symbols) {
        string desc = entry.second->toString();
        for (auto& c : desc) {
            if (c == '<' || c == '>' || c == '{' || c == '}')
                c = ' ';
        }
        label << " | " << desc;
    }
    label << "}";

    out << "  n" << myId << " [label=\"" << label.str() << "\"];\n";

    for (const Scope* child : scope->children) {
        int childId = nodeId;  
        generateDotContent(child, nodeId, out);
        out << "  n" << myId << " -> n" << childId << ";\n";
    }
}
