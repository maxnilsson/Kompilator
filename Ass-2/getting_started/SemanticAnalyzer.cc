#include "SemanticAnalyzer.h"

//public
int SemanticAnalyzer::analyze(Node* root) {
    if (!root) return 0;
    st.resetToRoot();
    scopeChildIdx.clear();
    analyzeProgram(root);
    return errors;
}


void SemanticAnalyzer::reportError(int lineno, const string& msg) { // error
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


void SemanticAnalyzer::analyzeProgram(Node* node) {
    // We are in root - PROGRAM scope
    int classChildIdx = 0;
    int mainChildIdx = 0;

    for (auto child : node->children) {
        if (child->type == "Globals") {
            // Checking global var declarations
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

    Scope* classScope = nextChildScope();
    if (!classScope || classScope->name != className) {
        // tryinfg looking up by name
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
    //next child scope
    Scope* parentScope = st.getCurrentScope();
    Scope* methodScope = nextChildScope();
    if (!methodScope) return;
    st.setCurrentScope(methodScope);

    // Parse children 
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


void SemanticAnalyzer::analyzeBlock(Node* node, bool isMethodBody,
                                     const string& returnType) {
    Scope* parentScope = st.getCurrentScope();

    if (!isMethodBody) { 
        Scope* blockScope = nextChildScope();
        if (blockScope)
            st.setCurrentScope(blockScope);
    }

    bool hasReturn = false;
    for (auto child : node->children) {
        if (hasReturn) {
            reportError(child->lineno,
                "Unreachable statement after return");
            break;  
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

    if (isVarDecl(t)) {
        analyzeVarDecl(node);
        return;
    }

    if (t == "Block") {
        analyzeBlock(node, false, returnType);
        return;
    }

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

    if (t == "If") {
        auto it = node->children.begin();
        Node* condition = *it; ++it;

        string condType = analyzeExpr(condition);
        if (condType != "error" && condType != "boolean") {
            reportError(condition->lineno,
                "If condition must be boolean, got " + condType);
        }

        // then
        if (it != node->children.end()) {
            bool branchReturn = false;
            analyzeStatement(*it, returnType, branchReturn);
            ++it;
        }
        // else
        if (it != node->children.end()) {
            Node* elseNode = *it;
            if (elseNode->type == "Else" && !elseNode->children.empty()) {
                bool branchReturn = false;
                analyzeStatement(elseNode->children.front(), returnType, branchReturn);
            }
        }
        return;
    }

    if (t == "For") {
        // Enter the for BLOCK scope
        Scope* parentScope = st.getCurrentScope();
        Scope* forScope = nextChildScope();
        if (forScope) st.setCurrentScope(forScope);

        // Process children the last child is the body, others is init/cond/update
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
                    // For-loop condition should be boolean if exists
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

    if (t == "Print") {
        if (!node->children.empty())
            analyzeExpr(node->children.front());
        return;
    }
    
    if (t == "Read") {
        if (!node->children.empty())
            analyzeExpr(node->children.front());
        return;
    }

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

    if (t == "Break" || t == "Continue") {
        return;
    }

    if (t == "ExprStmt") {
        if (!node->children.empty())
            analyzeExpr(node->children.front());
        return;
    }
    for (auto child : node->children) {
        bool dummy = false;
        analyzeStatement(child, returnType, dummy);
    }
}

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

string SemanticAnalyzer::analyzeExpr(Node* node) {
    if (!node) return "error";

    const string& t = node->type;

    if (t == "Int")   return "int";
    if (t == "Float") return "float";
    if (t == "True" || t == "False") return "boolean";

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

    if (t == "Not") {
        string operandType = analyzeExpr(node->children.front());
        if (operandType != "error" && operandType != "boolean") {
            reportError(node->lineno,
                "Operator '!' requires boolean operand, got " + operandType);
        }
        return "boolean";
    }

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

        // float 
        if (leftType == "float" || rightType == "float")
            return "float";
        return "int";
    }

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

    if (t == "ArrayLength") {
        string arrType = analyzeExpr(node->children.front());
        if (arrType != "error" && !isArrayType(arrType)) {
            reportError(node->lineno,
                "'.length' can only be used on arrays, not on " + arrType);
        }
        return "int";
    }

    if (t == "ArrayInit") {
        auto it = node->children.begin();
        Node* baseTypeNode = *it; ++it;
        string baseType = extractType(baseTypeNode);

        // Checking each element expression
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

    if (t == "MethodCall") {
        const string& methodName = node->value;

        // Check if this is a constructor call, class name used as method
        Record* classRec = st.getRootScope()->lookupLocal(methodName);
        if (classRec && classRec->recordType == RecordType::CLASS) {
            for (auto child : node->children) {
                if (child->type == "Args") {
                    for (auto arg : child->children)
                        analyzeExpr(arg);
                }
            }
            return methodName;  // returns class type
        }

        string className = getCurrentClassName();
        MethodRecord* mr = nullptr;
        if (!className.empty())
            mr = lookupMethodInClass(className, methodName);

        if (!mr) {
            Record* rec = st.getCurrentScope()->lookup(methodName);
            if (rec && rec->recordType == RecordType::METHOD) {
                mr = static_cast<MethodRecord*>(rec);
            }
        }

        if (!mr) {
            reportError(node->lineno,
                "Undeclared method '" + methodName + "'");
            for (auto child : node->children)
                if (child->type == "Args")
                    for (auto arg : child->children)
                        analyzeExpr(arg);
            return "error";
        }

        // arguments
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

    if (t == "ObjectMethodCall") {
        const string& methodName = node->value;

        auto it = node->children.begin();
        Node* objExpr = *it; ++it;

        string objType = analyzeExpr(objExpr);

        vector<string> argTypes;
        if (it != node->children.end() && (*it)->type == "Args") {
            for (auto arg : (*it)->children)
                argTypes.push_back(analyzeExpr(arg));
        }

        if (objType == "error")
            return "error";

        // Object must be a class type not primitive, not array
        if (isPrimitive(objType) || isArrayType(objType)) {
            reportError(node->lineno,
                "Cannot call method '" + methodName + "' on type " + objType);
            return "error";
        }

        MethodRecord* mr = lookupMethodInClass(objType, methodName);
        if (!mr) {
            reportError(node->lineno,
                "Class '" + objType + "' has no method '" + methodName + "'");
            return "error";
        }

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

    if (t == "Paren" && !node->children.empty()) {
        return analyzeExpr(node->children.front());
    }

    return "error";
}
