#include "STBuilder.h"

// ═══════════════════════════════════════════════════════════════════════
//  Public entry point
// ═══════════════════════════════════════════════════════════════════════

int STBuilder::build(Node* root) {
    if (!root) return 0;
    visitProgram(root);
    return errors;
}

// ═══════════════════════════════════════════════════════════════════════
//  Error reporting
// ═══════════════════════════════════════════════════════════════════════

void STBuilder::reportError(int lineno, const string& msg) {
    string full = "@error at line " + to_string(lineno) + ". " + msg;
    errorMessages.push_back(full);
    cerr << full << endl;
    ++errors;
}

// ═══════════════════════════════════════════════════════════════════════
//  Type extraction
// ═══════════════════════════════════════════════════════════════════════

string STBuilder::extractType(Node* typeNode) const {
    if (!typeNode) return "unknown";

    if (typeNode->type == "Type") {
        // Leaf type: "int", "float", "boolean", "void", or a class name
        return typeNode->value;
    }
    if (typeNode->type == "ArrayType") {
        // ArrayType → child[0] is the base Type node
        if (!typeNode->children.empty())
            return extractType(typeNode->children.front()) + "[]";
        return "unknown[]";
    }
    return "unknown";
}

// ═══════════════════════════════════════════════════════════════════════
//  Node-type classification helpers
// ═══════════════════════════════════════════════════════════════════════

bool STBuilder::isVarDecl(const string& nodeType) const {
    return nodeType == "VarDecl" || nodeType == "VarDeclInit"
        || nodeType == "VolatileVarDecl" || nodeType == "VolatileVarDeclInit";
}

bool STBuilder::isVolatileDecl(const string& nodeType) const {
    return nodeType == "VolatileVarDecl" || nodeType == "VolatileVarDeclInit";
}

bool STBuilder::hasInit(const string& nodeType) const {
    return nodeType == "VarDeclInit" || nodeType == "VolatileVarDeclInit";
}

// ═══════════════════════════════════════════════════════════════════════
//  Program-level traversal
// ═══════════════════════════════════════════════════════════════════════

void STBuilder::visitProgram(Node* node) {
    // The PROGRAM scope is already created by the SymbolTable constructor.
    // Program children (from parser):
    //   [Globals?] [Classes?] Main
    for (auto child : node->children) {
        if (child->type == "Globals")
            visitGlobals(child);
        else if (child->type == "Classes")
            visitClasses(child);
        else if (child->type == "Main")
            visitMain(child);
    }
}

// ═══════════════════════════════════════════════════════════════════════
//  Global variables
// ═══════════════════════════════════════════════════════════════════════

void STBuilder::visitGlobals(Node* node) {
    for (auto child : node->children) {
        if (isVarDecl(child->type))
            visitVarDecl(child, VariableKind::LOCAL);  // global = top-level local
    }
}

// ═══════════════════════════════════════════════════════════════════════
//  Classes
// ═══════════════════════════════════════════════════════════════════════

void STBuilder::visitClasses(Node* node) {
    for (auto child : node->children) {
        if (child->type == "Class")
            visitClass(child);
    }
}

void STBuilder::visitClass(Node* node) {
    const string& className = node->value;

    // Insert class record into the current (program) scope
    ClassRecord* cr = new ClassRecord(className, node->lineno);
    if (!st.insert(cr)) {
        reportError(node->lineno,
                    "Semantic error: Already declared class: '" + className + "'");
        delete cr;
    }

    // Enter the class scope
    st.enterScope(className, ScopeType::CLASS);

    // Class children are directly VarDecl* and Method* (ClassBody was unwrapped)
    for (auto child : node->children) {
        if (isVarDecl(child->type))
            visitVarDecl(child, VariableKind::FIELD);
        else if (child->type == "Method")
            visitMethod(child);
    }

    st.leaveScope();
}

// ═══════════════════════════════════════════════════════════════════════
//  Methods
// ═══════════════════════════════════════════════════════════════════════

void STBuilder::visitMethod(Node* node) {
    const string& methodName = node->value;

    // Method children order (from parser):
    //   [Params?]  Type  Block
    // We need to figure out which child is which.
    Node* paramsNode = nullptr;
    Node* returnTypeNode = nullptr;
    Node* blockNode = nullptr;

    auto it = node->children.begin();
    if (it != node->children.end() && (*it)->type == "Params") {
        paramsNode = *it;
        ++it;
    }
    if (it != node->children.end() &&
        ((*it)->type == "Type" || (*it)->type == "ArrayType")) {
        returnTypeNode = *it;
        ++it;
    }
    if (it != node->children.end() && (*it)->type == "Block") {
        blockNode = *it;
    }

    string returnType = extractType(returnTypeNode);

    // Create the MethodRecord and populate its parameter list
    MethodRecord* mr = new MethodRecord(methodName, returnType, node->lineno);

    // Pre-fill parameter info into the MethodRecord (for later semantic checks)
    if (paramsNode) {
        for (auto param : paramsNode->children) {
            // Param node: value = paramName, children[0] = Type
            string pName = param->value;
            string pType = extractType(param->children.front());
            mr->addParameter(pName, pType);
        }
    }

    // Insert method record into current (class) scope
    if (!st.insert(mr)) {
        reportError(node->lineno,
                    "Semantic error: Already declared method: '" + methodName + "'");
        delete mr;
    }

    // Enter the method scope
    st.enterScope(methodName, ScopeType::METHOD);

    // Insert parameters as variable records
    if (paramsNode)
        visitParams(paramsNode);

    // Traverse the method body (top-level block does NOT create an extra scope)
    if (blockNode)
        visitBlock(blockNode, true);

    st.leaveScope();
}

void STBuilder::visitMain(Node* node) {
    // Main children: Type("int"), Block
    Node* blockNode = nullptr;
    for (auto child : node->children) {
        if (child->type == "Block")
            blockNode = child;
    }

    // Insert a MethodRecord for main in the program scope
    MethodRecord* mr = new MethodRecord("main", "int", node->lineno);
    if (!st.insert(mr)) {
        reportError(node->lineno,
                    "Semantic error: Already declared identifier: 'main'");
        delete mr;
    }

    // Enter main's method scope
    st.enterScope("main", ScopeType::METHOD);

    if (blockNode)
        visitBlock(blockNode, true);

    st.leaveScope();
}

// ═══════════════════════════════════════════════════════════════════════
//  Parameters
// ═══════════════════════════════════════════════════════════════════════

void STBuilder::visitParams(Node* node) {
    for (auto param : node->children) {
        // Param node: type="Param", value=paramName, children[0]=Type
        const string& pName = param->value;
        string pType = extractType(param->children.front());
        bool isArr = (param->children.front()->type == "ArrayType");

        VariableRecord* vr = new VariableRecord(
            pName, pType, param->lineno, VariableKind::PARAMETER, false, isArr);

        if (!st.insert(vr)) {
            reportError(param->lineno,
                        "Semantic error: Already declared parameter: '" + pName + "'");
            delete vr;
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════
//  Blocks & Statements
// ═══════════════════════════════════════════════════════════════════════

void STBuilder::visitBlock(Node* node, bool isMethodBody) {
    // If this is NOT the method's own top-level block, create a BLOCK scope
    if (!isMethodBody) {
        st.enterScope("block", ScopeType::BLOCK);
    }

    // Visit every child statement
    for (auto child : node->children) {
        visitStatement(child);
    }

    if (!isMethodBody) {
        st.leaveScope();
    }
}

void STBuilder::visitStatement(Node* node) {
    if (!node) return;

    const string& t = node->type;

    // ── Variable declarations ────────────────────────────────────────
    if (isVarDecl(t)) {
        visitVarDecl(node, VariableKind::LOCAL);
        return;
    }

    // ── Nested block ─────────────────────────────────────────────────
    if (t == "Block") {
        visitBlock(node, false);
        return;
    }

    // ── If statement ─────────────────────────────────────────────────
    if (t == "If") {
        // Children: condition, thenStmt, [Else → elseStmt]
        auto it = node->children.begin();
        ++it; // skip condition expression
        if (it != node->children.end()) {
            visitStatement(*it);  // then-branch
            ++it;
        }
        if (it != node->children.end()) {
            // Else node: children[0] = else-stmt
            Node* elseNode = *it;
            if (elseNode->type == "Else" && !elseNode->children.empty())
                visitStatement(elseNode->children.front());
        }
        return;
    }

    // ── For loop ─────────────────────────────────────────────────────
    if (t == "For") {
        // For children can include: init (VarDecl/Assign), condition, update, body
        // We need to look for var declarations in the init part and the body.
        // The body is always the last child.
        //
        // Since for-loop init can declare a variable, we wrap the whole
        // for in a BLOCK scope so the variable is scoped to the loop.
        st.enterScope("for", ScopeType::BLOCK);

        for (auto child : node->children) {
            if (isVarDecl(child->type)) {
                visitVarDecl(child, VariableKind::LOCAL);
            } else {
                // The body statement (last child, or any nested block/stmt)
                visitStatement(child);
            }
        }

        st.leaveScope();
        return;
    }

    // ── All other statements: recurse into children looking for
    //    nested blocks / var decls (e.g. ExprStmt, Assign, Return, Print…)
    for (auto child : node->children) {
        visitStatement(child);
    }
}

// ═══════════════════════════════════════════════════════════════════════
//  Variable declarations
// ═══════════════════════════════════════════════════════════════════════

void STBuilder::visitVarDecl(Node* node, VariableKind kind) {
    const string& varName = node->value;
    bool vol = isVolatileDecl(node->type);
    // First child is always the Type node
    Node* typeNode = node->children.front();
    string varType = extractType(typeNode);
    bool isArr = (typeNode->type == "ArrayType");

    VariableRecord* vr = new VariableRecord(
        varName, varType, node->lineno, kind, vol, isArr);

    if (!st.insert(vr)) {
        string kindStr;
        switch (kind) {
            case VariableKind::FIELD:     kindStr = "variable"; break;
            case VariableKind::PARAMETER: kindStr = "parameter"; break;
            case VariableKind::LOCAL:     kindStr = "variable"; break;
        }
        reportError(node->lineno,
                    "Semantic error: Already declared " + kindStr + ": '" + varName + "'");
        delete vr;
    }
}
