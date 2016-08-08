#include "ast_node.h"
#include "ast_common.h"
#include "utils/log.h"

using namespace ast;
For::For(XMLNode xmlnode, ASTNode* parent, AST *ast) {
  #ifdef DEBUG_AST_NODE_TRACE
  std::cout << "---- FOR" << "\n";
  #endif
  m_xmlnode = xmlnode;
  m_parent = parent;
  m_ast = ast;
  m_cond = for_get_condition_expr(xmlnode);
  m_incr = for_get_incr_expr(xmlnode);
  m_inits = for_get_init_decls_or_exprs(xmlnode);
  XMLNode blk_n = for_get_block(xmlnode);
  for (XMLNode node : block_get_nodes(blk_n)) {
    ASTNode *n  = ASTNodeFactory::CreateASTNode(node, this, ast);
    if (n) {m_children.push_back(n);}
  }
}

void For::CreateSymbolTable() {
  if (m_parent == NULL) {
    // this is root, create the default symbol table.
    m_sym_tbl = m_ast->CreateSymTbl(NULL);
  } else {
    m_sym_tbl = m_ast->CreateSymTbl(m_parent->GetSymbolTable());
  }
  /**
   * Push the symbol table
   */
  for (XMLNode init : m_inits) {
    if (kind(init) == NK_Decl) {
      Decl *decl = DeclFactory::CreateDecl(init);
      if (decl) {
        m_decls.push_back(decl);
        m_sym_tbl->AddSymbol(decl->GetName(), decl->GetType(), this);
      }
      // std::string name = decl_get_name(init);
      // std::string type = decl_get_type(init);
      // m_sym_tbl->AddSymbol(name, type, this);
    }
  }
}

For::~For() {
  for (Decl *decl : m_decls) {
    delete decl;
  }
}

void For::GetCode(std::set<ASTNode*> nodes,
                  std::string &ret, bool all) {
  // if (!nodes.empty() && nodes.count(this) == 0) return;
  bool selected = nodes.count(this) == 1;
  selected |= all;
  if (selected) {
    ret += POIOutputCode();
    ret += "for (";
    // init
    if (!m_inits.empty()) {
      for (XMLNode n : m_inits) {
        ret += get_text(n);
        ret += ",";
      }
      ret.pop_back();
    }
    ret += ";" + get_text(m_cond) + ";" + get_text(m_incr) + ")";
    // m_blk->GetCode(nodes, ret, all, selected);
    ret += "{\n";
    for (ASTNode *child : m_children) {
      child->GetCode(nodes, ret, all);
    }
    ret += "}";
    ret += POIAfterCode();
  } else {
    std::set<std::string> decls = m_ast->GetRequiredDecl(this);
    std::set<std::string> inputed_decls = m_ast->GetRequiredDeclWithInput(this);
    for (std::string decl : decls) {
      SymbolTableValue *val = m_sym_tbl->LookUp(decl);
      if (val) {
        // ret += "/*HELIUM_DECL*/";
        // ret += val->GetType() + " " + val->GetName() + ";\n";
        ret += val->GetType()->GetDeclCode(val->GetName());
      }
    }
    for (std::string decl : inputed_decls) {
      SymbolTableValue *val = m_sym_tbl->LookUp(decl);
      if (val) {
        // ret += "/*HELIUM_DECL_WITH_INPUT*/";
        // ret += val->GetType() + " " + val->GetName() + ";\n";
        ret += val->GetType()->GetDeclCode(val->GetName());
        ret += val->GetType()->GetInputCode(val->GetName());
      }
    }
  }
}

std::string For::GetLabel() {
  std::string ret;
  ret += "for (";
  // init
  if (!m_inits.empty()) {
    for (XMLNode n : m_inits) {
      ret += get_text(n);
      ret += ",";
    }
    ret.pop_back();
  }
  ret += ";" + get_text(m_cond) + ";" + get_text(m_incr) + ")";
  return ret;
}

std::set<std::string> For::GetVarIds() {
  std::set<std::string> ret;
  std::set<std::string> ids;
  ids = get_var_ids(m_cond);
  ret.insert(ids.begin(), ids.end());
  for (XMLNode init : m_inits) {
    ids = get_var_ids(init);
    ret.insert(ids.begin(), ids.end());
  }
  ids = get_var_ids(m_incr);
  ret.insert(ids.begin(), ids.end());
  return ret;
}


std::set<std::string> For::GetIdToResolve() {
  std::set<std::string> ret;
  std::set<std::string> tmp;
  tmp = extract_id_to_resolve(get_text(m_cond));
  ret.insert(tmp.begin(), tmp.end());
  tmp = extract_id_to_resolve(get_text(m_incr));
  ret.insert(tmp.begin(), tmp.end());
  for (XMLNode n : m_inits) {
    tmp = extract_id_to_resolve(get_text(n));
    ret.insert(tmp.begin(), tmp.end());
  }
  return ret;
}

ASTNode* For::LookUpDefinition(std::string id) {
  std::string code;
  for (XMLNode init : m_inits) {
    code = get_text(init);
    CheckDefKind k = check_def(code, id);
    if (k == CDK_NULL) return NULL;
    else if (k==CDK_This) return this;
  }
  code = get_text(m_cond);
  CheckDefKind k = check_def(code, id);
  if (k == CDK_NULL) return NULL;
  else if (k==CDK_This) return this;
  // I don't need to consider incr?
  if (this->PreviousSibling())
    return this->PreviousSibling()->LookUpDefinition(id);
  else if (this->GetParent())
    return this->GetParent()->LookUpDefinition(id);
  else
    return NULL;
}
