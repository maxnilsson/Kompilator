#include "STBuilder.h"

int STBuilder::build(Node* root) {
    if (!root) return 0;
    visitProgram(root);
    return errors;
}

void STBuilder::reportError(int lineno, const string& msg) {
    string full = "@error at line " + to_string(lineno) + ". " + msg;
    errorMessages.push_back(full);
    cerr << full << endl;
    ++errors;
}

string STBuilder::extractType(Node* typeNode) const {
    if (!typeNode) return "unknown";

    if (typeNode->type == "Type") {
        return typeNode->value;
    }
    if (typeNode->type == "ArrayType") {
        if (!typeNode->children.empty())
            return extractType(typeNode->children.front()) + "[]";
        return "unknown[]";
    }
    return "unknown";
}

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


void STBuilder::visitProgram(Node* node) {
    for (auto child : node->children) {
        if (child->type == "Globals")
            visitGlobals(child);
        else if (child->type == "Classes")
            visitClasses(child);
        else if (child->type == "Main")
            visitMain(child);
    }
}

void STBuilder::visitGlobals(Node* node) {
    for (auto child : node->children) {
        if (isVarDecl(child->type))
            visitVarDecl(child, VariableKind::LOCAL);  // global
    }
}

void STBuilder::visitClasses(Node* node) {
    for (auto child : node->children) {
        if (child->type == "Class")
            visitClass(child);
    }
}

void STBuilder::visitClass(Node* node) {
    const string& className = node->value;

    ClassRecord* cr = new ClassRecord(className, node->lineno);
    if (!st.insert(cr)) {
        reportError(node->lineno,
                    "Semantic error: Already declared class: '" + className + "'");
        delete cr;
    }

    st.enterScope(className, ScopeType::CLASS);

    for (auto child : node->children) {
        if (isVarDecl(child->type))
            visitVarDecl(child, VariableKind::FIELD);
        else if (child->type == "Method")
            visitMethod(child);
    }

    st.leaveScope();
}

void STBuilder::visitMethod(Node* node) {
    const string& methodName = node->value;
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
    MethodRecord* mr = new MethodRecord(methodName, returnType, node->lineno);

    if (paramsNode) {
        for (auto param : paramsNode->children) {
            string pName = param->value;
            string pType = extractType(param->children.front());
            mr->addParameter(pName, pType);
        }
    }
    if (!st.insert(mr)) {
        reportError(node->lineno,
                    "Semantic error: Already declared method: '" + methodName + "'");
        delete mr;
    }

    st.enterScope(methodName, ScopeType::METHOD);

    if (paramsNode)
        visitParams(paramsNode);

    if (blockNode)
        visitBlock(blockNode, true);

    st.leaveScope();
}

void STBuilder::visitMain(Node* node) {
    Node* blockNode = nullptr;
    for (auto child : node->children) {
        if (child->type == "Block")
            blockNode = child;
    }
    MethodRecord* mr = new MethodRecord("main", "int", node->lineno);
    if (!st.insert(mr)) {
        reportError(node->lineno,
                    "Semantic error: Already declared identifier: 'main'");
        delete mr;
    }
    st.enterScope("main", ScopeType::METHOD);

    if (blockNode)
        visitBlock(blockNode, true);

    st.leaveScope();
}

void STBuilder::visitParams(Node* node) {
    for (auto param : node->children) {
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

void STBuilder::visitBlock(Node* node, bool isMethodBody) {
    if (!isMethodBody) {
        st.enterScope("block", ScopeType::BLOCK);
    }

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

    if (isVarDecl(t)) {
        visitVarDecl(node, VariableKind::LOCAL);
        return;
    }

    if (t == "Block") {
        visitBlock(node, false);
        return;
    }

    if (t == "If") {
        auto it = node->children.begin();
        ++it; // skip first child 
        if (it != node->children.end()) {
            visitStatement(*it); 
            ++it;
        }
        if (it != node->children.end()) {
            Node* elseNode = *it;
            if (elseNode->type == "Else" && !elseNode->children.empty())
                visitStatement(elseNode->children.front());
        }
        return;
    }

    if (t == "For") {
        st.enterScope("for", ScopeType::BLOCK);

        for (auto child : node->children) {
            if (isVarDecl(child->type)) {
                visitVarDecl(child, VariableKind::LOCAL);
            } else {
               visitStatement(child);
            }
        }

        st.leaveScope();
        return;
    }

    for (auto child : node->children) {
        visitStatement(child);
    }
}

void STBuilder::visitVarDecl(Node* node, VariableKind kind) {
    const string& varName = node->value;
    bool vol = isVolatileDecl(node->type);
    Node* typeNode = node->children.front(); // First child is typeNode
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
