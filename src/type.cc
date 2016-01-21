#include "type.h"
#include "common.h"
#include "utils.h"
#include "resolver.h"
#include "snippet.h"

#include <gtest/gtest.h>

using namespace utils;
using namespace ast;

/*******************************
 ** helper
 *******************************/

static bool
search_and_remove(std::string &s, boost::regex reg) {
  if (boost::regex_search(s, reg)) {
    s = boost::regex_replace<boost::regex_traits<char>, char>(s, reg, "");
    return true;
  }
  return false;
}

static int
count_and_remove(std::string &s, char c) {
  int count = std::count(s.begin(), s.end(), c);
  if (count) {
    s.erase(std::remove(s.begin(), s.end(), c), s.end());
  }
  return count;
}

static void
fill_storage_specifier(std::string& name, struct storage_specifier& specifier) {
  specifier.is_auto     = search_and_remove(name, boost::regex("\\bauto\\b"))     ? 1 : 0;
  specifier.is_register = search_and_remove(name, boost::regex("\\bregister\\b")) ? 1 : 0;
  specifier.is_static   = search_and_remove(name, boost::regex("\\bstatic\\b"))   ? 1 : 0;
  specifier.is_extern   = search_and_remove(name, boost::regex("\\bextern\\b"))   ? 1 : 0;
  // specifier.is_typedef     = search_and_remove(name, typedef_regex)     ? 1 : 0;
}

static void
fill_type_specifier(std::string& name, struct type_specifier& specifier) {
  specifier.is_void     = search_and_remove(name, boost::regex("\\bvoid\\b"))     ? 1 : 0;
  specifier.is_char     = search_and_remove(name, boost::regex("\\bchar\\b"))     ? 1 : 0;
  specifier.is_short    = search_and_remove(name, boost::regex("\\bshort\\b"))    ? 1 : 0;
  specifier.is_int      = search_and_remove(name, boost::regex("\\bint\\b"))      ? 1 : 0;
  specifier.is_long     = search_and_remove(name, boost::regex("\\blong\\b"))     ? 1 : 0;
  specifier.is_float    = search_and_remove(name, boost::regex("\\bfloat\\b"))    ? 1 : 0;
  specifier.is_double   = search_and_remove(name, boost::regex("\\bdouble\\b"))   ? 1 : 0;
  specifier.is_signed   = search_and_remove(name, boost::regex("\\bsigned\\b"))   ? 1 : 0;
  specifier.is_unsigned = search_and_remove(name, boost::regex("\\bunsigned\\b")) ? 1 : 0;
  specifier.is_bool     = search_and_remove(name, boost::regex("\\bbool\\b"))     ? 1 : 0;
}
static void
fill_type_qualifier(std::string& name, struct type_qualifier& qualifier) {
  qualifier.is_const    = search_and_remove(name, boost::regex("\\bconst\\b"))    ? 1 : 0;
  qualifier.is_volatile = search_and_remove(name, boost::regex("\\bvolatile\\b")) ? 1 : 0;
}

static void
fill_struct_specifier(std::string& name, struct struct_specifier& specifier) {
  specifier.is_struct = search_and_remove(name, boost::regex("\\bstruct\\b")) ? 1 : 0;
  specifier.is_enum   = search_and_remove(name, boost::regex("\\benum\\b"))   ? 1 : 0;
  specifier.is_union  = search_and_remove(name, boost::regex("\\bunion\\b"))  ? 1 : 0;
}

bool is_primitive(std::string s) {
  if (s.find('[') != std::string::npos) {
    s = s.substr(0, s.find('['));
  }
  count_and_remove(s, '*');
  struct storage_specifier storage_specifier;
  struct type_specifier type_specifier;
  struct type_qualifier type_qualifier;
  struct struct_specifier struct_specifier;
  fill_storage_specifier(s, storage_specifier);
  fill_type_specifier(s, type_specifier);
  fill_type_qualifier(s, type_qualifier);
  fill_struct_specifier(s, struct_specifier);
  trim(s);
  if (s.empty()) return true;
  return false;
}

/*******************************
 ** Type
 *******************************/

Type::Type(const std::string& raw)
  : m_raw(raw) {
  decompose(raw);
  if (m_id.empty()) {
    m_kind = TK_Primitive;
  }
  else if (SystemResolver::Instance()->Has(m_id)) {
    std::string new_type = SystemResolver::Instance()->ResolveType(m_id);
    if (!new_type.empty()) {
      std::string tmp = raw;
      tmp.replace(tmp.find(m_id), m_id.length(), new_type);
      decompose(tmp);
      m_kind = TK_Primitive;
    } else {
      m_kind = TK_System;
    }
  } else {
    std::set<Snippet*> snippets = SnippetRegistry::Instance()->Resolve(m_id);
    // TODO check type
    // TODO store snippet pointer
    if (snippets.empty()) {
      m_kind = TK_Unknown;
    } else {
      m_kind = TK_Snippet;
    }
  }
  // FIXME will cause crash if I run unit tests without setting system resolver
  // else if (SystemResolver::Instance()->Has(m_id)) m_kind = TK_System;
  // TODO local type: struct, union, or enum?
}

void Type::decompose(std::string tmp) {
  m_dimension = std::count(tmp.begin(), tmp.end(), '[');
  if (tmp.find('[') != std::string::npos) {
    tmp = tmp.substr(0, tmp.find('['));
  }
  m_pointer = count_and_remove(tmp, '*');
  m_name = tmp;                 // get name right after removing * and []
  fill_storage_specifier(tmp, m_storage_specifier);
  fill_type_specifier(tmp, m_type_specifier);
  fill_type_qualifier(tmp, m_type_qualifier);
  fill_struct_specifier(tmp, m_struct_specifier);
  trim(tmp);
  
  m_id = tmp;
  if (!m_id.empty()) {
    m_name = m_id;
  }
}

std::string Type::Name() const {
  return m_name;
}


/*******************************
 ** Variable
 *******************************/


Variable::Variable() {}
Variable::Variable(Type type, const std::string& name)
: m_type(type), m_name(name) {
}


Variable::Variable(const std::string& type, const std::string& name) {
  m_name = name;
  m_type = Type(type);
}
Variable::operator bool() {
  if (!m_name.empty()) return true;
  return false;
}


/**
 * Get newly defined/declared variables in node.
 */
VariableList var_from_node(ast::Node node) {
  VariableList vars;
  std::map<std::string, std::string> plain_vars;
  switch (kind(node)) {
  case ast::NK_Function: {
    // plain_vars = dynamic_cast<ast::FunctionNode&>(node).ParamList();
    NodeList params = function_get_params(node);
    for (Node param : params) {
      // FIXME this may be just part of it. Or very long.
      std::string type = param_get_type(param);
      std::string name = param_get_name(param);
      vars.push_back(Variable(type, name));
    }
    break;
  }
  case ast::NK_DeclStmt: {
    // from name to type
    NodeList decls = decl_stmt_get_decls(node);
    for (Node decl : decls) {
      std::string type = decl_get_type(decl);
      std::string name = decl_get_name(decl);
      vars.push_back(Variable(type, name));
    }
    break;
  }
  case ast::NK_For: {
    NodeList init_decls = for_get_init_decls(node);
    for (Node decl : init_decls) {
      std::string type = decl_get_type(decl);
      std::string name = decl_get_name(decl);
      vars.push_back(Variable(type, name));
    }
    // std::map<std::string, std::string> for_vars = for_get_init_detail(node);
    // for (auto &m : for_vars) {
    //   vars.push_back(Variable(m.first, m.second));
    // }
    break;
  }
  default: {}
  }
  return vars;
}

/**
 * Get decl code for a pointer type
 */
std::string
get_decl_code(const std::string& type_name, const std::string& var_name, int pointer_level) {
  return type_name + std::string(pointer_level, '*')+ " " + var_name+";\n";
}

static std::string
qualify_var_name(const std::string& varname) {
  std::string tmp = varname;
  tmp.erase(std::remove(tmp.begin(), tmp.end(), '.'), tmp.end());
  tmp.erase(std::remove(tmp.begin(), tmp.end(), '>'), tmp.end());
  tmp.erase(std::remove(tmp.begin(), tmp.end(), '-'), tmp.end());
  return tmp;
}
/**
 * Only get the allocate code(malloc, assign), but no decl code.
 */
std::string
get_allocate_code(const std::string& type_name, const std::string& var_name, int pointer_level) {
  std::string code;
  std::string var_tmp = qualify_var_name(var_name) + "_tmp";
  code += type_name + "* " + var_tmp + " = (" + type_name + "*)malloc(sizeof(" + type_name + "));\n";
  code += var_name + " = " + std::string(pointer_level-1, '&') + var_tmp + ";\n";
  return code;
}

/**
 * Get input code for a Variable.

If it is a pointer, allocate memory.
TODO If it is an array, need a loop, and array size.

variable contains the name, and its type.
Type contains the type str, the modifier(* &),
and [].

 */

std::string get_input_code(Variable v) {
  return get_input_code(v.GetType(), v.Name());
}

std::string get_input_code(Type type, const std::string& var) {
  std::string result;
  switch (type.Kind()) {
  case TK_Primitive: {
    /*******************************
     * Primitive type
     *******************************/
    if (type.m_type_specifier.is_int) {
      if (type.Pointer() > 0) {
        result += get_allocate_code(type.Name(), var, type.Pointer());
        // TODO free it to shutup valgrind
      } else {
        result += type.Raw() + " " + var + ";\n";
        result += "scanf(\"%d\", &"+var+");";
      }
    } else if (type.m_type_specifier.is_long) {
      if (type.Pointer() > 0) {
        result += get_allocate_code(type.Name(), var, type.Pointer());
        // TODO free it to shutup valgrind
      } else {
        result += type.Raw() + " " + var + ";\n";
        result += "scanf(\"%ld\", &"+var+");";
      }
    } else if (type.m_type_specifier.is_char) {
      if (type.Pointer() == 0) {
        result += type.Raw() + " " + var + ";\n";
        result += "scanf(\"%c\", &"+var+");";
      } else if (type.Pointer() == 1) {
        result += "scanf(\"%d\", &helium_size);\n";
        result += type.Raw() + " "+var+";\n";
        result += "if (helium_size == 0) {\n";
        result += "  " + var + " = NULL;\n";
        result += "} else {\n";
        result += "  " + var + " = ("+type.Raw()+")malloc(sizeof("+type.Name()+")*helium_size);\n";
        result += "  scanf(\"%s\", "+var+");\n}";
      }
    }
    break;
  }
  default: {
    result += "Unimplemented.";
  }
  }

  return result;
}



/*******************************
 ** Helper function for Variable
 *******************************/



// std::string
// Type::GetArrayCode(const std::string& type_name, const std::string& var_name, int dimension) {
//   // TODO only support 1 dimension!!!
//   std::string code;
//   std::string size_var = var_name + "_size";
//   // std::string
//   code += "int " + size_var + ";\n";
//   code += "scanf(\"%d\", &"+size_var + ");\n";
//   code += type_name + " " + var_name + "[" + size_var + "];\n";
//   // TODO init code
//   // code += "for (int i=0;i<" + size_var + ";i++) {\n";
//   // code += "   scanf()"
//   return code;
// }


/*******************************
 ** VariableList
 *******************************/


Variable look_up(const VariableList &vars, const std::string& name) {
  for (Variable v : vars) {
    if (v.Name() == name) {
      return v;
    }
  }
  return Variable();
}

/**
 * Add var to vars only if the name of var does not appear in vars.
 */
void add_unique(VariableList &vars, Variable var) {
  std::string name = var.Name();
  if (!var) return;
  for (auto it=vars.begin();it!=vars.end();) {
    if (it->Name() == var.Name()) {
      it = vars.erase(it);
    } else {
      ++it;
    }
  }
  vars.push_back(var);
}
