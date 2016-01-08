#ifndef __TYPE_H__
#define __TYPE_H__

#include <string>
#include "ast.h"
#include "snippet.h"


/*******************************
 ** Type
 *******************************/

/**
 * Type should be designed as the minimum inside the class, and a bunch of helper functions.

class Type {
public:
Kind();
Raw();
}



StructureType {
Snippet();
Fields();
}

 */


struct storage_specifier {
  unsigned int is_auto     : 1;
  unsigned int is_register : 1;
  unsigned int is_static   : 1;
  unsigned int is_extern   : 1;
  unsigned int is_typedef  : 1; // not used
};
struct type_specifier {
  unsigned int is_void     : 1;
  unsigned int is_char     : 1;
  unsigned int is_short    : 1;
  unsigned int is_int      : 1;
  unsigned int is_long     : 1; // do not support long long
  unsigned int is_float    : 1;
  unsigned int is_double   : 1;
  unsigned int is_signed   : 1;
  unsigned int is_unsigned : 1;
  unsigned int is_bool     : 1; // not in C standard
};
struct type_qualifier {
  unsigned int is_const    : 1;
  unsigned int is_volatile : 1;
};
struct struct_specifier {
  unsigned int is_struct   : 1;
  unsigned int is_union    : 1;
  unsigned int is_enum     : 1;
};

typedef enum _TypeKind {
  TK_Enum,
  TK_Primitive,
  TK_Structure,
  TK_System,
  TK_Union
} TypeKind;

class Type {
public:
  Type();
  Type(const std::string& raw);
  virtual ~Type();
  std::string ToString() const;
  TypeKind Kind() const;
  std::string Raw() const;
protected:
  std::string m_raw;
  TypeKind m_kind;
  struct storage_specifier m_storage_specifier;
  struct type_specifier m_type_specifier;
  struct type_qualifier m_type_qualifier;
  struct struct_specifier m_struct_specifier;
  std::string m_id;             // identifier that left when removing qualifiers
private:
  void decompose();
};



/*******************************
 ** Variable
 *******************************/

/**
 * Variable class should also be designed as minimum, with a bunch of helper functions.

class Variable {
public:
VarName();
Type();
Kind(); //?

Dimension and Pointer information should be in Variable, not type.
}

// get input code should just need one for Variable and Type. So no such functions for Type
get_input_code()

get_input_code(Type)
get_output_code(Type)
get_input_specification(Type)

lower level helper function:
  static std::string GetDeclCode(const std::string& type_name, const std::string& var_name, int pointer_level);
  static std::string GetAllocateCode(const std::string& type_name, const std::string& var_name, int pointer_level);
  static std::string GetArrayCode(const std::string& type_name, const std::string& var_name, int dimension);


 */

class Variable {
public:
  Variable(Type type, const std::string& name);
  Variable(const std::string& type, const std::string& name);
  Variable();
  ~Variable() {}
  
  const std::string& Name() const {return m_name;}
  const Type GetType() const { return m_type;}

  operator bool();

private:
  // pugi::xml_node m_node; // the node where the variable is declared // TODO is it really used??
  Type m_type;
  std::string m_name;
  std::string m_type_str;
  // TODO pointer and dimension information should be treated as a structure?
  int m_pointer;
  int m_dimension;
};



class VariableList {
public:
  VariableList();
  ~VariableList();
  /* construct */
  void Add(Variable v);
  void Add(VariableList vars);
  /* meta */
  size_t Size() const;
  bool Empty() const;
  void Clear();
  /* add only if the name is unique */
  void AddUniqueName(Variable v);
  void AddUniqueName(VariableList vars);
  Variable LookUp(const std::string &name);
  std::vector<Variable> Variables() const;
private:
  std::vector<Variable> m_vars;
};

VariableList var_from_node(ast::Node* node);
std::string get_input_code(Variable v);


#endif
