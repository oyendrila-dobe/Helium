#include <pugixml.hpp>
#include <iostream>
#include "snippet.h"
#include "utils.h"

using namespace utils;

/*******************************
 ** ctags
 *******************************/

CtagsEntry::CtagsEntry(const tagEntry* const entry) {
  m_name = entry->name;
  m_file = entry->file;
  m_line = entry->address.lineNumber;
  m_pattern = entry->address.pattern;
  m_type = *(entry->kind);
  if (m_file.find("/") != std::string::npos) {
    m_simple_filename = m_file.substr(m_file.rfind("/")+1);
  }
}

static tagFile *tag_file;
/**
 * Init tag file.
 * If tag_file is not NULL, free it, and then 
 * @param[in] filename
 * @side modify static variable tag_file, which is the ctags handle.
 * Must be called before ctags_parse.
 */
void ctags_load(const std::string& filename) {
  if (tag_file != NULL) tagsClose(tag_file);
  tagFileInfo *info = (tagFileInfo*)malloc(sizeof(tagFileInfo));
  tag_file = tagsOpen(filename.c_str(), info);
  if (info->status.opened != true) {
    assert(false);
  }
  free(info);
}

/**
 * Parse name in tag file.
 * @return a vector of CtagsEntry
 */
std::vector<CtagsEntry> ctags_parse(const std::string& name) {
  assert(tag_file != NULL);
  tagEntry *tag_entry = (tagEntry*)malloc(sizeof(tagEntry));
  std::vector<CtagsEntry> entries;
  tagResult result = tagsFind(tag_file, tag_entry, name.c_str(), TAG_FULLMATCH);
  while (result == TagSuccess) {
    if (tag_entry->kind) {
      entries.push_back(CtagsEntry(tag_entry));
    }
    result = tagsFindNext(tag_file, tag_entry);
  }
  return entries;
}
  


/*******************************
 ** ctags_type enum related
 *******************************/

SnippetKind char_to_ctags_type(char t) {
  switch (t) {
  case 'f': return SK_Function;
  case 's': return SK_Structure;
  case 'g': return SK_Enum;
  case 'u': return SK_Union;
  case 'd': return SK_Define;
  case 'v': return SK_Variable;
  case 'e': return SK_EnumMember;
  case 't': return SK_Typedef;
  case 'c': return SK_Const;
  case 'm': return SK_Member;
  default: assert(false);
  }
}

std::set<SnippetKind> string_to_ctags_types(std::string s) {
  std::set<SnippetKind> types;
  for (auto it=s.begin();it!=s.end();it++) {
    types.insert(char_to_ctags_type(*it));
  }
  return types;
}

char ctags_type_to_char(SnippetKind t) {
  switch (t) {
  case SK_Function : return 'f';
  case SK_Structure : return 's';
  case SK_Enum : return 'g';
  case SK_Union : return 'u';
  case SK_Define : return 'd';
  case SK_Variable : return 'v';
  case SK_EnumMember : return 'e';
  case SK_Typedef : return 't';
  case SK_Const : return 'c';
  case SK_Member : return 'm';
  default: assert(false);
  }
}

std::string ctags_types_to_string(std::set<SnippetKind> types) {
  std::string s;
  for (auto it=types.begin();it!=types.end();it++) {
    s += ctags_type_to_char(*it);
  }
  return s;
}

/*******************************
 ** Snippet
 *******************************/


/*******************************
 ** create snippet
 *******************************/

/**
 * Query query on code.
 * return the first matching.
 */
std::string query_code_first(const std::string& code, const std::string& query) {
  pugi::xml_document doc;
  string2xml(code, doc);
  pugi::xml_node root_node = doc.document_element();
  pugi::xml_node node = root_node.select_node(query.c_str()).node();
  return node.child_value();
}
/**
 * Query "query" on "code".
 * Return all matching.
 * Will not use get_text_content, but use child_value() for a xml tag.
 * Only support tag value currently, not attribute value.
 */
std::vector<std::string> query_code(const std::string& code, const std::string& query) {
  std::vector<std::string> result;
  pugi::xml_document doc;
  string2xml(code, doc);
  pugi::xml_node root_node = doc.document_element();
  pugi::xpath_node_set nodes = root_node.select_nodes(query.c_str());
  for (auto it=nodes.begin();it!=nodes.end();it++) {
    pugi::xml_node node = it->node();
    result.push_back(node.child_value());
  }
  return result;
}


Snippet::Snippet(const CtagsEntry& entry) {
  /**
   * 1. get code
   * 2. get signature
   */
  SnippetKind type = char_to_ctags_type(entry.GetType());
  switch(type) {
  case SK_Function: {
    m_code = get_func_code(entry);
    m_sig.emplace(entry.GetName(), SK_Function);
    break;
  }
  case SK_Structure: {
    m_code = get_struct_code(entry);
    m_sig.emplace(entry.GetName(), SK_Structure);
    break;
  }
  case SK_Enum: {
    m_code = get_enum_code(entry);
    m_sig.emplace(entry.GetName(), SK_Enum);
    // FIXME TEST this!!! HEBI can I just use this, without the detailed "block"?
    std::vector<std::string> members = query_code(m_code, "//enum/decl/name");
    for (std::string m : members) {
      m_sig.emplace(m, SK_EnumMember);
    }
    break;
  }
  case SK_Union: {
    m_code = get_union_code(entry);
    m_sig.emplace(entry.GetName(), SK_Union);
    break;
  }
  case SK_Define: {
    m_code = get_def_code(entry);
    m_sig.emplace(entry.GetName(), SK_Define);
    break;
  }
  case SK_Variable: {
    m_code = get_var_code(entry);
    m_sig.emplace(entry.GetName(), SK_Variable);
    break;
  }
  case SK_EnumMember: {
    m_code = get_enum_code(entry);
    std::vector<std::string> members = query_code(m_code, "//enum/decl/name");
    for (std::string m : members) {
      m_sig.emplace(m, SK_EnumMember);
    }
    // enum name
    std::string name = query_code_first(m_code, "//enum/name");
    if (!name.empty()) m_sig.emplace(name, SK_Enum);
    // possibly typedef
    name = query_code_first(m_code, "//typedef/name");
    if (!name.empty()) m_sig.emplace(name, SK_Typedef);
    break;
  }
  case SK_Typedef: {
    m_code = get_typedef_code(entry);
    // tyepdef
    std::string name = query_code_first(m_code, "//typedef/name");
    if (!name.empty()) m_sig.emplace(name, SK_Typedef);
    // TODO NOW also needs to test if it is a struct, union, or enum
    break;
  }
  // case SK_Const:
  //   m_code = get_const_code(entry);
  // case SK_Member:
  //   m_code = get_mem_code(entry);
  default:
    // should we reach here?
    // null snippet?
    m_code = "";
  }
}



/**
 * Get all types pairs of this snippet.

 This should be a multimap
 {
 "conn": SK_Structure,
 "conn": SK_Typedef
 }

 {
 "ctags_type": SK_Enum,
 "SK_Const": SK_EnumMember,
 "SK_Member": SK_EnumMember
 }

 The snippet will only be indexed by the name, e.g. "conn".

 typedef struct {
 } ALIAS_NAME;

 Then the signature is {"ALIAS_NAME", SK_Typedef} ? but actually it is a structure.

 每个模块只专心做一件事，所以这个不必理会，直接按照typedef来说。
 因为大体上用到snippet只是要知道其code，以及能够管理dependency和lookup。
 只有Type init时才会考虑到其member，只有在其是函数时才会考虑到其declaration。
 因此，写一些help函数来判断一个typedf是什么，就行了。

*/
snippet_signature
Snippet::GetSignature() const {
  return m_sig;
}
/**
 * @param[in] name only get the types of the id(key) "name"
 */
std::set<SnippetKind>
Snippet::GetSignature(const std::string& name) {
  std::pair <snippet_signature::iterator, snippet_signature::iterator> ret; 
  std::set<SnippetKind> types;
  ret = m_sig.equal_range(name);
  for (auto it=ret.first; it!=ret.second;it++) {
    types.insert(it->second);
  }
  return types;
}

/**
 * Get all keys of m_sig. Which is all keywords to index this snippet in registry
 */
std::set<std::string> Snippet::GetSignatureKey() const {
  std::set<std::string> result;
  for (auto sig : m_sig) {
    result.insert(sig.first);
  }
  return result;
}

/**
 * True if this snippet has a key "name", whose type has a map to ONE of "types"
 */
bool
Snippet::SatisfySignature(const std::string& name, std::set<SnippetKind> types) {
  std::pair <snippet_signature::iterator, snippet_signature::iterator> ret;
  ret = m_sig.equal_range(name);
  for (auto it=ret.first; it!=ret.second;it++) {
    if (types.find(it->second) != types.end()) {
      return true;
    }
  }
  return false;
}

Snippet::~Snippet() {}
// used only for print purpose! Human readable.
std::string Snippet::GetName() const {
  return "NO NAME";
}



/*******************************
 ** Functions for get code from file based on ctags entry
 *******************************/

/*
 * use depth-first-search for the first pos:line attribute
 * return -1 if no pos:line attr found
 */
int
get_element_line(pugi::xml_node node) {
  // check if pos:line is enabled on this xml
  pugi::xml_node root = node.root();
  if (!root.child("unit").attribute("xmlns:pos")) {
    std::cerr<<"position is not enabled in srcml"<<std::endl;
    exit(1);
    return -1;
  }
  // the node itself has pos:line attr, just use it
  if (node.attribute("pos:line")) {
    return atoi(node.attribute("pos:line").value());
  } else {
    pugi::xml_node n = node.select_node("//*[@pos:line]").node();
    if (n) {
      return atoi(n.attribute("pos:line").value());
    }
  }
  return -1;
}

/*
 * The last pos:line in the current node element
 * Useful to track the range of the current AST node.
 */
int
get_element_last_line(pugi::xml_node node) {
  pugi::xml_node root = node.root();
  if (!root.child("unit").attribute("xmlns:pos")) {
    std::cerr<<"position is not enabled in srcml"<<std::endl;
    exit(1);
    return -1;
  }
  pugi::xml_node n = node.select_node("(//*[@pos:line])[last()]").node();
  
  // pugi::xpath_node_set nodes = node.select_nodes("//*[@pos:line]");
  // pugi::xml_node last_node = nodes[nodes.size()-1].node();
  // return atoi(last_node.attribute("pos:line").value());
  
  if (n) {
    return atoi(n.attribute("pos:line").value());
  } else if (node.attribute("pos:line")) {
    return atoi(node.attribute("pos:line").value());
  }
  return -1;
}

std::string get_text_content(pugi::xml_node node);

/**
 * Get code as string of <tag_name> node in filename that encloses line_number
 */
std::string get_code_enclosing_line(const std::string& filename, int line_number, std::string tag_name) {
  pugi::xml_document doc;
  file2xml(filename, doc);
  pugi::xml_node root =doc.document_element();
  std::string query = "//" + tag_name;
  pugi::xpath_node_set nodes = root.select_nodes(query.c_str());
  for (auto it=nodes.begin();it!=nodes.end();it++) {
    pugi::xml_node node = it->node();
    int first_line = get_element_line(node);
    int last_line = get_element_last_line(node);
    // FIXME the equal is necessary? Be precise.
    if (first_line <= line_number && last_line >= line_number) {
      return get_text_content(node);
    }
  }
  return "";
}

/**
 * Use filename and line number to match a <funciton> that contains that line.
 */
std::string get_func_code(const CtagsEntry& entry) {
  int line_number = entry.GetLineNumber();
  std::string filename = entry.GetFileName();
  return get_code_enclosing_line(filename, line_number, "function");
}

/**
 * Use filename and line number to match a <enum> that contains the line.
 * FIXME the enum member may be of a anonymouse enum, within another struct.
 * FIXME The line number containing technique should be tested.
 */
std::string get_enum_code(const CtagsEntry& entry) {
  int line_number = entry.GetLineNumber();
  std::string filename = entry.GetFileName();
  std::string enum_code = get_code_enclosing_line(filename, line_number, "enum");
  std::string typedef_code = get_code_enclosing_line(filename, line_number, "typedef");
  if (!typedef_code.empty()) {
    return typedef_code;
  } else {
    return enum_code;
  }

}


std::string get_def_code(const CtagsEntry& entry) {
                            // std::string filename, int line) {
  int line_number = entry.GetLineNumber();
  std::string filename = entry.GetFileName();
  return get_code_enclosing_line(filename, line_number, "cpp:define");
}

std::string get_typedef_code(const CtagsEntry& entry) {
  int line_number = entry.GetLineNumber();
  std::string filename = entry.GetFileName();
  return get_code_enclosing_line(filename, line_number, "typedef");
}

/**
 * If the struct is a typedef, I should not just get the <struct> tag, but the <typedef> tag.
 * Because 1) only get the struct is not syntax-valid(miss ;);
 * and 2) the typedef ID will need another snippet, which will cause the same code appears in different places.
 * 
 * The baseline of Snippet class is that, the same code can NEVER be in different places(Snippets).
 */
std::string get_struct_code(const CtagsEntry& entry) {
  int line_number = entry.GetLineNumber();
  std::string filename = entry.GetFileName();
  std::string struct_code =  get_code_enclosing_line(filename, line_number, "struct");
  std::string typedef_code = get_code_enclosing_line(filename, line_number, "typedef");
  /*
    If there's also a <typedef> enclosing this line, than we use it,
    because we don't want same code in different Snippets.
  */
  if (!typedef_code.empty()) {
    return typedef_code;
  } else {
    return struct_code;
  }
}

/**
 * FIXME same as above
 */
std::string get_union_code(const CtagsEntry& entry) {
  int line_number = entry.GetLineNumber();
  std::string filename = entry.GetFileName();
  std::string union_code =  get_code_enclosing_line(filename, line_number, "union");
  std::string typedef_code = get_code_enclosing_line(filename, line_number, "typedef");
  if (!typedef_code.empty()) {
    return typedef_code;
  } else {
    return union_code;
  }
}

/**
 * FIXME the decl_stmt that enclose the line may contains other variable!!
 */
std::string get_var_code(const CtagsEntry& entry) {
  int line_number = entry.GetLineNumber();
  std::string filename = entry.GetFileName();
  return get_code_enclosing_line(filename, line_number, "decl_stmt");
}