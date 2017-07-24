#include "helium/parser/Visitor.h"
#include "helium/parser/NewGenerator.h"
#include "helium/parser/AST.h"
#include "helium/parser/SourceManager.h"
#include "helium/utils/StringUtils.h"

#include "helium/parser/SymbolTable.h"

#include "helium/type/Type.h"
#include <iostream>

using std::vector;
using std::string;
using std::map;
using std::set;
ASTNodeBase* last_node(std::set<ASTNodeBase*> nodes) {
  ASTNodeBase* ret = nullptr;
  for (auto *node : nodes) {
    if (!ret) ret=node;
    else {
      // should not be compound stmt, because that would make a wrong output position
      if (dynamic_cast<CompoundStmt*>(node)) continue;
      SourceLocation loc = node->getEndLoc();
      SourceLocation retloc = ret->getEndLoc();
      if (retloc < loc) {
        ret = node;
      }
    }
  }
  return ret;
}

static std::string get_input_code(ASTNodeBase* node) {
  std::string ret;
  std::set<std::string> vars = node->getDefinedVars();
  for (std::string var : vars) {
    std::string type = node->getDefinedVarType(var);
    Type *t = TypeFactory::CreateType(type);
    if (t && dynamic_cast<PrimitiveType*>(t)) {
      ret += t->GetInputCode(var) + "\n";
    }
  }
  return ret;
}

std::string get_output_code(ASTNodeBase *output_node, std::set<ASTNodeBase*> sel) {
  SymbolTable *symtbl = output_node->getASTContext()->getSymbolTable();
  std::string ret;
  if (symtbl) {
    SymbolTableEntry *entry = symtbl->getEntry(output_node);
    std::set<std::string> vars = entry->getAllVarsRecursive();
    for (std::string var : vars) {
      std::string type = entry->getTypeRecursive(var);
      ASTNodeBase *node = entry->getNodeRecursive(var);
      if (sel.count(node) == 1) {
        Type *t = TypeFactory::CreateType(type);
        if (t && dynamic_cast<PrimitiveType*>(t)) {
          ret += t->GetOutputCode(var);
        }
      }
    }
  }
  return ret;
}

void NewGenerator::setSelection(std::set<ASTNodeBase*> sel) {
  m_sel = sel;
  // m_spec = get_instrument_spec(sel);
}

// high level
void NewGenerator::visit(TokenNode *node){
  Visitor::visit(node);
  if (m_sel.count(node) == 1) {
    std::string prog;
    prog = m_spec_pre[node];
    prog += " " + node->getText() + " ";
    prog += m_spec_post[node];
    addInnerProg(node, prog);
  }
}
void NewGenerator::visit(TranslationUnitDecl *node){
  Visitor::visit(node);
  std::vector<ASTNodeBase*> nodes = node->getDecls();
  std::string prog;
  prog = m_spec_pre[node];
  for (ASTNodeBase *n : nodes) {
    std::string sub_prog = getInnerProg(n);
    prog += sub_prog;
  }
  prog += m_spec_post[node];
  addInnerProg(node, prog);
}
void NewGenerator::visit(FunctionDecl *node){
  Visitor::visit(node);
  TokenNode *ReturnNode = node->getReturnTypeNode();
  TokenNode *NameNode = node->getNameNode();
  TokenNode *ParamNode = node->getParamNode();
  Stmt *body = node->getBody();
  std::string ret_prog = getInnerProg(ReturnNode);
  std::string name_prog = getInnerProg(NameNode);
  std::string param_prog = getInnerProg(ParamNode);
  std::string body_prog = getInnerProg(body);
  
  std::string prog;
  prog = m_spec_pre[node];
  if (!ret_prog.empty() && !name_prog.empty() && !param_prog.empty()) {
    // function header is selected
    // record the function name. This function should be called in helium_entry
    prog += ret_prog + " " + name_prog + "(" + param_prog + ")";
    prog += body_prog;
    // create a helium_entry_<func>() function
    std::string entry_func_name = "helium_entry_" + NameNode->getText();
    prog += "void " + entry_func_name + "() {\n";
    // TODO call the function with parameter, no need to instantize
    prog += "  // TODO call function " + NameNode->getText() + " with properly initialized arguments\n";
    prog += "}\n";
    // record the function
    m_entry_funcs.insert(entry_func_name);
  } else if (!body_prog.empty()) {
    // function header is not selected
    // create a helium_entry_dummy_<func>() functions
    std::string entry_func_name = "helium_entry_dummy_" + NameNode->getText();
    prog += "void " + entry_func_name + "() {\n";
    // remove return in body_prog
    utils::replace(body_prog, "return", "");
    prog += body_prog;
    prog += "}\n";
    m_entry_funcs.insert(entry_func_name);
  }
  prog += m_spec_post[node];
  addInnerProg(node, prog);
}
void NewGenerator::visit(CompoundStmt *node){
  Visitor::visit(node);
  TokenNode *lbrace = node->getLBrace();
  TokenNode *rbrace = node->getRBrace();
  std::string prog;
  prog += m_spec_pre[node];
  std::string lbrace_prog = getInnerProg(lbrace);
  if (!lbrace_prog.empty()) {
    lbrace_prog += "\n";
  }
  std::string rbrace_prog = getInnerProg(rbrace);
  if (!rbrace_prog.empty()) {
    rbrace_prog += "\n";
  }
  prog += lbrace_prog;
  for (Stmt *stmt : node->getBody()) {
    std::string sub_prog = getInnerProg(stmt);
    prog += sub_prog;
  }
  prog += rbrace_prog;
  prog += m_spec_post[node];
  addInnerProg(node, prog);
}
// condition
void NewGenerator::visit(IfStmt *node){
  Visitor::visit(node);
  TokenNode *IfNode = node->getIfNode();
  Expr *cond = node->getCond();
  Stmt *then_stmt = node->getThen();
  TokenNode *ElseNode = node->getElseNode();
  Stmt *else_stmt = node->getElse();

  std::string if_node_prog = getInnerProg(IfNode);
  std::string cond_prog = getInnerProg(cond);
  std::string then_prog = getInnerProg(then_stmt);
  std::string else_node_prog = getInnerProg(ElseNode);
  std::string else_prog = getInnerProg(else_stmt);

  std::string prog;
  prog += m_spec_pre[node];
  if (!if_node_prog.empty()) {
    // I'm adding these parenthesis back because
    // if () char a; will not compile
    prog += if_node_prog + "(" + cond_prog + ")" + then_prog + else_node_prog + else_prog;
  }
  prog += m_spec_post[node];
  addInnerProg(node, prog);
}
void NewGenerator::visit(SwitchStmt *node){
  Visitor::visit(node);
  TokenNode *SwitchNode = node->getSwitchNode();
  Expr *cond = node->getCond();
  std::vector<SwitchCase*> cases = node->getCases();
  std::string switch_node_prog = getInnerProg(SwitchNode);
  std::string cond_prog = getInnerProg(cond);
  std::string body_prog;
  for (SwitchCase *c : cases) {
    std::string case_prog = getInnerProg(c);
    body_prog += case_prog;
  }
  std::string prog;
  prog += m_spec_pre[node];
  if (!switch_node_prog.empty()) {
    prog += switch_node_prog + "(" + cond_prog + ") {" + body_prog + "}\n";
  } else {
    prog += body_prog;
  }
  prog += m_spec_post[node];
  addInnerProg(node, prog);
}
void NewGenerator::visit(CaseStmt *node){
  Visitor::visit(node);
  TokenNode *case_node = node->getCaseNode();
  Expr *cond = node->getCond();
  vector<Stmt*> body = node->getBody();
  std::string case_node_prog = getInnerProg(case_node);
  std::string cond_prog = getInnerProg(cond);
  std::string body_prog;
  for (Stmt *stmt : body) {
    body_prog += getInnerProg(stmt);
  }
  std::string prog;
  prog += m_spec_pre[node];
  if (!case_node_prog.empty()) {
    // HACK also add an empty statement because:
    // error: label at end of compound statement: expected statement
    // Prog += ": ;";
    prog += case_node_prog + " " + cond_prog + ":" + body_prog + ";";
  } else {
    prog += body_prog;
  }
  prog += m_spec_post[node];
  addInnerProg(node, prog);
}
void NewGenerator::visit(DefaultStmt *node){
  Visitor::visit(node);
  TokenNode *def_node = node->getDefaultNode();
  vector<Stmt*> body = node->getBody();
  std::string def_node_prog = getInnerProg(def_node);
  std::string body_prog;
  for (Stmt *stmt : body) {
    body_prog += getInnerProg(stmt);
  }
  std::string prog;
  prog += m_spec_pre[node];
  if (!def_node_prog.empty()) {
    prog += def_node_prog + ": " + body_prog + ";";
  } else {
    prog += body_prog;
  }
  prog += m_spec_post[node];
  addInnerProg(node, prog);
}
// loop
void NewGenerator::visit(ForStmt *node){
  Visitor::visit(node);
  TokenNode *ForNode = node->getForNode();
  Expr *init = node->getInit();
  Expr *cond = node->getCond();
  Expr *inc = node->getInc();
  Stmt *body = node->getBody();

  std::string for_node_prog = getInnerProg(ForNode);
  std::string init_prog = getInnerProg(init);
  std::string cond_prog = getInnerProg(cond);
  std::string inc_prog = getInnerProg(inc);
  std::string body_prog = getInnerProg(body);

  std::string prog;
  prog += m_spec_pre[node];
  if (!for_node_prog.empty()) {
    prog += for_node_prog + "(" + init_prog + ";" + cond_prog + ";" + inc_prog + ")" + "{" + body_prog + "}";
  } else {
    prog += body_prog;
  }
  prog += m_spec_post[node];
  addInnerProg(node, prog);
}
void NewGenerator::visit(WhileStmt *node){
  Visitor::visit(node);
  TokenNode *WhileNode = node->getWhileNode();
  Expr *cond = node->getCond();
  Stmt *body = node->getBody();

  std::string while_node_prog = getInnerProg(WhileNode);
  std::string cond_prog = getInnerProg(cond);
  std::string body_prog = getInnerProg(body);

  std::string prog;
  prog += m_spec_pre[node];
  if (!while_node_prog.empty()) {
    prog += while_node_prog + "(" + cond_prog + ")" + "{" + body_prog + "}";
  } else {
    prog += body_prog;
  }
  prog += m_spec_post[node];
  addInnerProg(node, prog);
}
void NewGenerator::visit(DoStmt *node){
  Visitor::visit(node);
  TokenNode *DoNode = node->getDoNode();
  TokenNode *WhileNode = node->getWhileNode();
  Stmt *body = node->getBody();
  Expr *cond = node->getCond();

  std::string do_node_prog = getInnerProg(DoNode);
  std::string while_node_prog = getInnerProg(WhileNode);
  std::string body_prog = getInnerProg(body);
  std::string cond_prog = getInnerProg(cond);

  std::string prog;
  prog += m_spec_pre[node];
  if (!do_node_prog.empty()) {
    prog += do_node_prog + body_prog + while_node_prog + "(" + cond_prog + ")" + ";";
  } else {
    prog += body_prog;
  }
  prog += m_spec_post[node];
  addInnerProg(node, prog);
}
// single
void NewGenerator::visit(BreakStmt *node){
  Visitor::visit(node);
  if (m_sel.count(node) == 1) {
    std::string prog;
    prog += m_spec_pre[node];
    prog += "break;\n";
    prog += m_spec_post[node];
    addInnerProg(node, prog);
  }
}
void NewGenerator::visit(ContinueStmt *node){
  Visitor::visit(node);
  if (m_sel.count(node) == 1) {
    std::string prog;
    prog += m_spec_pre[node];
    prog += "continue;\n";
    prog += m_spec_post[node];
    addInnerProg(node, prog);
  }
}
void NewGenerator::visit(ReturnStmt *node){
  Visitor::visit(node);
  TokenNode *ReturnNode = node->getReturnNode();
  Expr *expr = node->getValue();

  std::string ret_node_prog = getInnerProg(ReturnNode);
  std::string ret_value_prog = getInnerProg(expr);

  std::string prog;
  prog += m_spec_pre[node];
  if (!ret_node_prog.empty()) {
    prog += ret_node_prog + " " + ret_value_prog + "\n";
  }
  // FIXME adjust return
  // FIXME select only value
  prog += m_spec_post[node];
  addInnerProg(node, prog);
}
// expr stmt
void NewGenerator::visit(Expr *node){
  Visitor::visit(node);
  if (m_sel.count(node) == 1) {
    addInnerProg(node, node->getText());
  }
}
void NewGenerator::visit(DeclStmt *node){
  Visitor::visit(node);
  if (m_sel.count(node) == 1) {
    // TODO add input code
    std::string prog;
    prog += m_spec_pre[node];
    prog += node->getText();
    prog += m_spec_post[node];
    addInnerProg(node, prog);
  }
}
void NewGenerator::visit(ExprStmt *node){
  Visitor::visit(node);
  if (m_sel.count(node) == 1) {
    std::string prog;
    prog += m_spec_pre[node];
    prog += node->getText() + "\n";
    prog += m_spec_post[node];
    addInnerProg(node, prog);
  }
}










/**
 * Instrumentor
 */

void Instrumentor::pre(ASTNodeBase*node) {
  if (m_last == node) {
    m_spec_post[node] += get_output_code(node, m_sel);
  }
}


// high level
void Instrumentor::visit(TokenNode *node) {
  pre(node);
  Visitor::visit(node);
}
void Instrumentor::visit(TranslationUnitDecl *node) {
  pre(node);
  Visitor::visit(node);
}
void Instrumentor::visit(FunctionDecl *node) {
  pre(node);
  TokenNode *param = node->getParamNode();
  if (param && m_sel.count(param) == 1) {
    std::string code = get_input_code(param);
    if (!code.empty()) {
      // add code to post of compound stmt
      Stmt *body = node->getBody();
      CompoundStmt *comp_stmt = dynamic_cast<CompoundStmt*>(body);
      assert(comp_stmt);
      TokenNode *lbrace = comp_stmt->getLBrace();
      assert(lbrace);
      assert(m_sel.count(lbrace) == 1);
      m_spec_post[lbrace] += code;
    }
  }
  // output
  if (m_sel.count(node) == 1) {
    // output should be at the end of the functions
    Stmt *body = node->getBody();
    CompoundStmt *comp_stmt = dynamic_cast<CompoundStmt*>(body);
    assert(comp_stmt);
    TokenNode *rbrace = comp_stmt->getRBrace();
    // get all variables
    std::string code = get_output_code(rbrace, m_sel);
    m_spec_pre[rbrace] += code;
  } else {
    // output should be the last of the nodes
    // m_last = nullptr;
    // TODO this should be set for each functiondecl
    m_last = last_node(m_sel);
  }
  Visitor::visit(node);
  
}
void Instrumentor::visit(CompoundStmt *node) {
  pre(node);
  Visitor::visit(node);
}
// condition
void Instrumentor::visit(IfStmt *node) {
  pre(node);
  Visitor::visit(node);
}
void Instrumentor::visit(SwitchStmt *node) {
  pre(node);
  Visitor::visit(node);
}
void Instrumentor::visit(CaseStmt *node) {
  pre(node);
  Visitor::visit(node);
}
void Instrumentor::visit(DefaultStmt *node) {
  pre(node);
  Visitor::visit(node);
}
// loop
void Instrumentor::visit(ForStmt *node) {
  pre(node);
  Visitor::visit(node);
}
void Instrumentor::visit(WhileStmt *node) {
  pre(node);
  Visitor::visit(node);
}
void Instrumentor::visit(DoStmt *node) {
  pre(node);
  Visitor::visit(node);
}
// single
void Instrumentor::visit(BreakStmt *node) {
  pre(node);
  Visitor::visit(node);
}
void Instrumentor::visit(ContinueStmt *node) {
  pre(node);
  Visitor::visit(node);
}
void Instrumentor::visit(ReturnStmt *node) {
  pre(node);
  Visitor::visit(node);
}
// expr stmt
void Instrumentor::visit(Expr *node) {
  pre(node);
  Visitor::visit(node);
}
void Instrumentor::visit(DeclStmt *node) {
  pre(node);
  std::string code = get_input_code(node);
  if (!code.empty()) {
    m_spec_post[node] += code;
  }
  Visitor::visit(node);
}
void Instrumentor::visit(ExprStmt *node) {
  pre(node);
  Visitor::visit(node);
}
