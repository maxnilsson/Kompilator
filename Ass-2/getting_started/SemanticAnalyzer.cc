#include "SemanticAnalyzer.h"

// ═══════════════════════════════════════════════════════════════════════
//  Public entry point
// ═══════════════════════════════════════════════════════════════════════

int SemanticAnalyzer::analyze(Node* root) {
    if (!root) return 0;
    st.resetToRoot();
    scopeChildIdx.clear();
    analyzeProgram(root);
    return errors;
}

// ═══════════════════════════════════════════════════════════════════════
//  Error reporting
// ═══════════════════════════════════════════════════════════════════════

void SemanticAnalyzer::reportError(int lineno, const string& msg) {
    string full = "@error at line " + to_string(lineno) + ". Semantic error: " + msg;
    errorMessages.push_back(full);
    cerr << full << endl;
    ++errors;
}

Scope* SemanticAnalyzer::nextChildScope() {
    Scope* cur = st.getCurrentScope();
    size_t& idx = scopeChildIdx[cur];
    if (idx < cur->children.size())
        return cur->children[idx++];
    return nullptr;
}

// ═══════════════════════════════════════════════════════════════════════
//  Type helpers
// ═══════════════════════════════════════════════════════════════════════

bool SemanticAnalyzer::isPrimitive(const string& t) const {
    return t == "int" || t == "float" || t == "boolean";
}

bool SemanticAnalyzer::isNumeric(const string& t) const {
    return t == "int" || t == "float";
}

bool SemanticAnalyzer::isArrayType(const string& t) const {
    return t.size() >= 2 && t.substr(t.size() - 2) == "[]";
}

string SemanticAnalyzer::baseOfArray(const string& t) const {
    if (isArrayType(t))
        return t.substr(0, t.size() - 2);
    return t;
}

string SemanticAnalyzer::extractType(Node* typeNode) const {
    if (!typeNode) return "unknown";
    if (typeNode->type == "Type")
        return typeNode->value;
    if (typeNode->type == "ArrayType") {
        if (!typeNode->children.empty())
            return extractType(typeNode->children.front()) + "[]";
        return "unknown[]";
    }
    return "unknown";
}

bool SemanticAnalyzer::isValidType(const string& t) const {
    if (t == "int" || t == "float" || t == "boolean" || t == "void")
        return true;
    // Check for array of primitives
    if (isArrayType(t)) {
        string base = baseOfArray(t);
        return base == "int" || base == "float" || base == "boolean";
    }
    // Check if it's a declared class
    Record* rec = st.getRootScope()->lookupLocal(t);
    return rec && rec->recordType == RecordType::CLASS;
}

bool SemanticAnalyzer::isVarDecl(const string& nodeType) const {
    return nodeType == "VarDecl" || nodeType == "VarDeclInit"
        || nodeType == "VolatileVarDecl" || nodeType == "VolatileVarDeclInit";
}

// ═══════════════════════════════════════════════════════════════════════
//  Method lookup helpers
// ═══════════════════════════════════════════════════════════════════════

MethodRecord* SemanticAnalyzer::lookupMethodInClass(const string& className,
                                                     const string& methodName) {
    Scope* classScope = st.lookupClassScope(className);
    if (!classScope) return nullptr;
    Record* rec = classScope->lookupLocal(methodName);
    if (rec && rec->recordType == RecordType::METHOD)
        return static_cast<MethodRecord*>(rec);
    return nullptr;
}

string SemanticAnalyzer::getCurrentClassName() const {
    Scope* s = st.getEnclosingClassScope();
    if (s) return s->name;
    return "";
}

bool SemanticAnalyzer::containsReturn(Node* node) const {
    if (!node) return false;
    if (node->type == "Return") return true;
    for (auto child : node->children) {
        if (containsReturn(child)) return true;
    }
    return false;
}

// ═══════════════════════════════════════════════════════════════════════
//  Program / Class / Method traversal
// ═══════════════════════════════════════════════════════════════════════

void SemanticAnalyzer::analyzeProgram(Node* node) {
    // We are in root (PROGRAM) scope – mirrors STBuilder's scope walk
    int classChildIdx = 0;
    int mainChildIdx = 0;

    for (auto child : node->children) {
        if (child->type == "Globals") {
            // Check global var declarations
            for (auto gvar : child->children) {
                if (isVarDecl(gvar->type))
                    analyzeVarDecl(gvar);
            }
        }
        else if (child->type == "Classes") {
            for (auto cls : child->children) {
                if (cls->type == "Class")
                    analyzeClass(cls);
            }
        }
        else if (child->type == "Main") {
            analyzeMain(child);
        }
    }
}

void SemanticAnalyzer::analyzeClass(Node* node) {
    const string& className = node->value;

    // Enter the class scope – use next child scope (mirrors STBuilder order)
    Scope* classScope = nextChildScope();
    if (!classScope || classScope->name != className) {
        // Fallback: try looking up by name
        classScope = st.lookupClassScope(className);
    }
    if (!classScope) return;
    st.setCurrentScope(classScope);

    for (auto child : node->children) {
        if (isVarDecl(child->type))
            analyzeVarDecl(child);
        else if (child->type == "Method")
            analyzeMethod(child);
    }

    st.setCurrentScope(classScope->parent);
}

void SemanticAnalyzer::analyzeMethod(Node* node) {
    const string& methodName = node->value;

    // Get the next child scope (METHOD scope for this method)
    Scope* parentScope = st.getCurrentScope();
    Scope* methodScope = nextChildScope();
    if (!methodScope) return;
    st.setCurrentScope(methodScope);

    // Parse children to find return type and block
    Node* returnTypeNode = nullptr;
    Node* blockNode = nullptr;
    auto it = node->children.begin();
    if (it != node->children.end() && (*it)->type == "Params") ++it;  // skip params
    if (it != node->children.end() &&
        ((*it)->type == "Type" || (*it)->type == "ArrayType")) {
        returnTypeNode = *it;
        ++it;
    }
    if (it != node->children.end() && (*it)->type == "Block")
        blockNode = *it;

    string returnType = extractType(returnTypeNode);

    // Analyze the body
    if (blockNode) {
        bool hasReturn = false;
        analyzeBlock(blockNode, true, returnType);

        // Check for missing return in non-void methods
        if (returnType != "void") {
            if (!containsReturn(blockNode)) {
                reportError(node->lineno,
                    "Missing return statement in non-void method '" + methodName + "'");
            }
        }
    }

    st.setCurrentScope(parentScope);
}

void SemanticAnalyzer::analyzeMain(Node* node) {
    // Get the next child scope (METHOD scope for main)
    Scope* parentScope = st.getCurrentScope();
    Scope* mainScope = nextChildScope();
    if (!mainScope) return;
    st.setCurrentScope(mainScope);

    Node* blockNode = nullptr;
    for (auto child : node->children) {
        if (child->type == "Block")
            blockNode = child;
    }

    if (blockNode)
        analyzeBlock(blockNode, true, "int");

    st.setCurrentScope(parentScope);
}

// ═══════════════════════════════════════════════════════════════════════
//  Block & Statement analysis
// ═══════════════════════════════════════════════════════════════════════

void SemanticAnalyzer::analyzeBlock(Node* node, bool isMethodBody,
                                     const string& returnType) {
    Scope* parentScope = st.getCurrentScope();

    if (!isMethodBody) {
        // Get the next child scope (BLOCK scope)
        Scope* blockScope = nextChildScope();
        if (blockScope)
            st.setCurrentScope(blockScope);
    }

    bool hasReturn = false;
    for (auto child : node->children) {
        if (hasReturn) {
            reportError(child->lineno,
                "Unreachable statement after return");
            break;  // report once
        }
        analyzeStatement(child, returnType, hasReturn);
    }

    if (!isMethodBody)
        st.setCurrentScope(parentScope);
}

void SemanticAnalyzer::analyzeStatement(Node* node, const string& returnType,
                                         bool& hasReturn) {
    if (!node) return;
    const string& t = node->type;

    // ── Variable declaration ─────────────────────────────────────────
    if (isVarDecl(t)) {
        analyzeVarDecl(node);
        return;
    }

    // ── Nested block ─────────────────────────────────────────────────
    if (t == "Block") {
        analyzeBlock(node, false, returnType);
        return;
    }

    // ── Assignment: expr := expr ─────────────────────────────────────
    if (t == "Assign") {
        auto it = node->children.begin();
        Node* lhs = *it; ++it;
        Node* rhs = *it;

        string lhsType = analyzeExpr(lhs);
        string rhsType = analyzeExpr(rhs);

        if (lhsType != "error" && rhsType != "error" && lhsType != rhsType) {
            reportError(node->lineno,
                "Type mismatch in assignment: cannot assign " + rhsType + " to " + lhsType);
        }
        return;
    }

    // ── If statement ─────────────────────────────────────────────────
    if (t == "If") {
        auto it = node->children.begin();
        Node* condition = *it; ++it;

        string condType = analyzeExpr(condition);
        if (condType != "error" && condType != "boolean") {
            reportError(condition->lineno,
                "If condition must be boolean, got " + condType);
        }

        // then-branch
        if (it != node->children.end()) {
            bool branchReturn = false;
            analyzeStatement(*it, returnType, branchReturn);
            ++it;
        }
        // else-branch
        if (it != node->children.end()) {
            Node* elseNode = *it;
            if (elseNode->type == "Else" && !elseNode->children.empty()) {
                bool branchReturn = false;
                analyzeStatement(elseNode->children.front(), returnType, branchReturn);
            }
        }
        return;
    }

    // ── For loop ─────────────────────────────────────────────────────
    if (t == "For") {
        // Enter the for's BLOCK scope
        Scope* parentScope = st.getCurrentScope();
        Scope* forScope = nextChildScope();
        if (forScope) st.setCurrentScope(forScope);

        // Process children: the last child is the body, others are init/cond/update
        auto children = node->children;
        for (size_t i = 0; i < children.size(); ++i) {
            auto it2 = children.begin();
            advance(it2, i);
            Node* child = *it2;

            if (isVarDecl(child->type)) {
                analyzeVarDecl(child);
            } else if (child->type == "Assign") {
                // for init or update assignment
                auto ait = child->children.begin();
                Node* lhs = *ait; ++ait;
                Node* rhs = *ait;
                string lhsT = analyzeExpr(lhs);
                string rhsT = analyzeExpr(rhs);
                if (lhsT != "error" && rhsT != "error" && lhsT != rhsT) {
                    reportError(child->lineno,
                        "Type mismatch in assignment: cannot assign " + rhsT + " to " + lhsT);
                }
            } else if (child->type == "Block") {
                // body block
                analyzeBlock(child, false, returnType);
            } else {
                // condition expression or body statement
                if (i < children.size() - 1) {
                    // This is likely the condition
                    string condType = analyzeExpr(child);
                    // For-loop condition should be boolean (if provided)
                    if (condType != "error" && condType != "boolean") {
                        reportError(child->lineno,
                            "For-loop condition must be boolean, got " + condType);
                    }
                } else {
                    // body statement (non-block)
                    bool dummy = false;
                    analyzeStatement(child, returnType, dummy);
                }
            }
        }

        if (forScope) st.setCurrentScope(parentScope);
        return;
    }

    // ── Print statement ──────────────────────────────────────────────
    if (t == "Print") {
        if (!node->children.empty())
            analyzeExpr(node->children.front());
        return;
    }

    // ── Read statement ───────────────────────────────────────────────
    if (t == "Read") {
        if (!node->children.empty())
            analyzeExpr(node->children.front());
        return;
    }

    // ── Return statement ─────────────────────────────────────────────
    if (t == "Return") {
        hasReturn = true;
        if (!node->children.empty()) {
            string exprType = analyzeExpr(node->children.front());
            if (exprType != "error" && returnType != "error" && exprType != returnType) {
                reportError(node->lineno,
                    "Return type mismatch: expected " + returnType + ", got " + exprType);
            }
        }
        return;
    }

    // ── Break / Continue ─────────────────────────────────────────────
    if (t == "Break" || t == "Continue") {
        return;
    }

    // ── Expression statement ─────────────────────────────────────────
    if (t == "ExprStmt") {
        if (!node->children.empty())
            analyzeExpr(node->children.front());
        return;
    }

    // ── Fallback: recurse ────────────────────────────────────────────
    for (auto child : node->children) {
        bool dummy = false;
        analyzeStatement(child, returnType, dummy);
    }
}

// ═══════════════════════════════════════════════════════════════════════
//  Variable declaration analysis (type validity + initializer type)
// ═══════════════════════════════════════════════════════════════════════

void SemanticAnalyzer::analyzeVarDecl(Node* node) {
    const string& varName = node->value;
    Node* typeNode = node->children.front();
    string varType = extractType(typeNode);

    // Check that the declared type is valid
    if (!isValidType(varType)) {
        reportError(node->lineno,
            "Undefined type '" + varType + "'");
    }

    // If there's an initializer, check its type
    bool hasInit = (node->type == "VarDeclInit" || node->type == "VolatileVarDeclInit");
    if (hasInit && node->children.size() >= 2) {
        auto it = node->children.begin(); ++it;
        string initType = analyzeExpr(*it);
        if (initType != "error" && varType != "error" && initType != varType) {
            reportError(node->lineno,
                "Type mismatch in initialization of '" + varName +
                "': expected " + varType + ", got " + initType);
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════
//  Expression type inference
// ═══════════════════════════════════════════════════════════════════════

string SemanticAnalyzer::analyzeExpr(Node* node) {
    if (!node) return "error";

    const string& t = node->type;

    // ── Literals ─────────────────────────────────────────────────────
    if (t == "Int")   return "int";
    if (t == "Float") return "float";
    if (t == "True" || t == "False") return "boolean";

    // ── Identifier ───────────────────────────────────────────────────
    if (t == "ID") {
        Record* rec = st.getCurrentScope()->lookup(node->value);
        if (!rec) {
            reportError(node->lineno,
                "Undeclared identifier '" + node->value + "'");
            return "error";
        }
        if (rec->recordType == RecordType::VARIABLE)
            return rec->type;
        if (rec->recordType == RecordType::CLASS)
            return rec->name;
        return rec->type;
    }

    // ── Not (!expr) ──────────────────────────────────────────────────
    if (t == "Not") {
        string operandType = analyzeExpr(node->children.front());
        if (operandType != "error" && operandType != "boolean") {
            reportError(node->lineno,
                "Operator '!' requires boolean operand, got " + operandType);
        }
        return "boolean";
    }

    // ── Arithmetic operators: +, -, *, /, ^ ──────────────────────────
    if (t == "Add" || t == "Sub" || t == "Mul" || t == "Div" || t == "Pow") {
        auto it = node->children.begin();
        string leftType = analyzeExpr(*it); ++it;
        string rightType = analyzeExpr(*it);

        if (leftType == "error" || rightType == "error")
            return "error";

        if (!isNumeric(leftType) || !isNumeric(rightType)) {
            reportError(node->lineno,
                "Operator '" + t + "' requires numeric operands, got "
                + leftType + " and " + rightType);
            return "error";
        }

        // float promotion
        if (leftType == "float" || rightType == "float")
            return "float";
        return "int";
    }

    // ── Comparison operators: <, >, <=, >= ───────────────────────────
    if (t == "LessThan" || t == "GreaterThan" ||
        t == "LessEqual" || t == "GreaterEqual") {
        auto it = node->children.begin();
        string leftType = analyzeExpr(*it); ++it;
        string rightType = analyzeExpr(*it);

        if (leftType == "error" || rightType == "error")
            return "boolean";

        if (!isNumeric(leftType) || !isNumeric(rightType)) {
            reportError(node->lineno,
                "Comparison operator requires numeric operands, got "
                + leftType + " and " + rightType);
        }
        return "boolean";
    }

    // ── Equality operators: =, != ────────────────────────────────────
    if (t == "Equal" || t == "NotEqual") {
        auto it = node->children.begin();
        string leftType = analyzeExpr(*it); ++it;
        string rightType = analyzeExpr(*it);

        if (leftType == "error" || rightType == "error")
            return "boolean";

        if (leftType != rightType) {
            reportError(node->lineno,
                "Equality operator requires same types, got "
                + leftType + " and " + rightType);
        }
        return "boolean";
    }

    // ── Logical operators: &, | ──────────────────────────────────────
    if (t == "And" || t == "Or") {
        auto it = node->children.begin();
        string leftType = analyzeExpr(*it); ++it;
        string rightType = analyzeExpr(*it);

        if (leftType == "error" || rightType == "error")
            return "boolean";

        if (leftType != "boolean" || rightType != "boolean") {
            string op = (t == "And") ? "&" : "|";
            reportError(node->lineno,
                "Operator '" + op + "' requires boolean operands, got "
                + leftType + " and " + rightType);
        }
        return "boolean";
    }

    // ── Array access: expr[expr] ─────────────────────────────────────
    if (t == "ArrayAccess") {
        auto it = node->children.begin();
        string arrType = analyzeExpr(*it); ++it;
        string idxType = analyzeExpr(*it);

        if (arrType != "error" && !isArrayType(arrType)) {
            reportError(node->lineno,
                "Array access on non-array type " + arrType);
            return "error";
        }

        if (idxType != "error" && idxType != "int") {
            reportError(node->lineno,
                "Array index must be int, got " + idxType);
        }

        if (arrType != "error")
            return baseOfArray(arrType);
        return "error";
    }

    // ── Array length: expr.length ────────────────────────────────────
    if (t == "ArrayLength") {
        string arrType = analyzeExpr(node->children.front());
        if (arrType != "error" && !isArrayType(arrType)) {
            reportError(node->lineno,
                "'.length' can only be used on arrays, not on " + arrType);
        }
        return "int";
    }

    // ── Array initialiser: baseType[ expr, expr, ... ] ───────────────
    if (t == "ArrayInit") {
        auto it = node->children.begin();
        Node* baseTypeNode = *it; ++it;
        string baseType = extractType(baseTypeNode);

        // Check each element expression
        if (it != node->children.end()) {
            Node* exprList = *it;
            for (auto elem : exprList->children) {
                string elemType = analyzeExpr(elem);
                if (elemType != "error" && elemType != baseType) {
                    reportError(elem->lineno,
                        "Array element type mismatch: expected " + baseType + ", got " + elemType);
                }
            }
        }
        return baseType + "[]";
    }

    // ── Self method call: ID( args ) ─────────────────────────────────
    if (t == "MethodCall") {
        const string& methodName = node->value;

        // Check if this is a constructor call (class name used as method)
        Record* classRec = st.getRootScope()->lookupLocal(methodName);
        if (classRec && classRec->recordType == RecordType::CLASS) {
            // Constructor call – no arguments expected in basic C+-
            // but we still analyze the args
            for (auto child : node->children) {
                if (child->type == "Args") {
                    for (auto arg : child->children)
                        analyzeExpr(arg);
                }
            }
            return methodName;  // returns the class type
        }

        // Otherwise, look up method in current class
        string className = getCurrentClassName();
        MethodRecord* mr = nullptr;
        if (!className.empty())
            mr = lookupMethodInClass(className, methodName);

        if (!mr) {
            // Try looking up in any accessible scope
            Record* rec = st.getCurrentScope()->lookup(methodName);
            if (rec && rec->recordType == RecordType::METHOD) {
                mr = static_cast<MethodRecord*>(rec);
            }
        }

        if (!mr) {
            reportError(node->lineno,
                "Undeclared method '" + methodName + "'");
            // Still analyze args
            for (auto child : node->children)
                if (child->type == "Args")
                    for (auto arg : child->children)
                        analyzeExpr(arg);
            return "error";
        }

        // Check arguments
        vector<string> argTypes;
        for (auto child : node->children) {
            if (child->type == "Args") {
                for (auto arg : child->children)
                    argTypes.push_back(analyzeExpr(arg));
            }
        }

        if (argTypes.size() != mr->parameters.size()) {
            reportError(node->lineno,
                "Method '" + methodName + "' expects " + to_string(mr->parameters.size())
                + " argument(s), got " + to_string(argTypes.size()));
        } else {
            for (size_t i = 0; i < argTypes.size(); ++i) {
                if (argTypes[i] != "error" &&
                    argTypes[i] != mr->parameters[i].second) {
                    reportError(node->lineno,
                        "Argument " + to_string(i+1) + " of method '" + methodName
                        + "' expects " + mr->parameters[i].second
                        + ", got " + argTypes[i]);
                }
            }
        }

        return mr->type;
    }

    // ── Object method call: expr.ID( args ) ──────────────────────────
    if (t == "ObjectMethodCall") {
        const string& methodName = node->value;

        auto it = node->children.begin();
        Node* objExpr = *it; ++it;

        string objType = analyzeExpr(objExpr);

        // Analyze arguments regardless
        vector<string> argTypes;
        if (it != node->children.end() && (*it)->type == "Args") {
            for (auto arg : (*it)->children)
                argTypes.push_back(analyzeExpr(arg));
        }

        if (objType == "error")
            return "error";

        // Object must be a class type (not primitive, not array)
        if (isPrimitive(objType) || isArrayType(objType)) {
            reportError(node->lineno,
                "Cannot call method '" + methodName + "' on type " + objType);
            return "error";
        }

        // Look up method in the class
        MethodRecord* mr = lookupMethodInClass(objType, methodName);
        if (!mr) {
            reportError(node->lineno,
                "Class '" + objType + "' has no method '" + methodName + "'");
            return "error";
        }

        // Check argument count and types
        if (argTypes.size() != mr->parameters.size()) {
            reportError(node->lineno,
                "Method '" + objType + "." + methodName + "' expects "
                + to_string(mr->parameters.size()) + " argument(s), got "
                + to_string(argTypes.size()));
        } else {
            for (size_t i = 0; i < argTypes.size(); ++i) {
                if (argTypes[i] != "error" &&
                    argTypes[i] != mr->parameters[i].second) {
                    reportError(node->lineno,
                        "Argument " + to_string(i+1) + " of method '"
                        + objType + "." + methodName + "' expects "
                        + mr->parameters[i].second + ", got " + argTypes[i]);
                }
            }
        }

        return mr->type;
    }

    // ── Parenthesised expression – should have been unwrapped by parser
    //    but handle just in case ──────────────────────────────────────
    if (t == "Paren" && !node->children.empty()) {
        return analyzeExpr(node->children.front());
    }

    // ── Fallback: unknown expression node ────────────────────────────
    return "error";
}
