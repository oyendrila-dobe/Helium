#ifndef SEGMENT_H
#define SEGMENT_H

#include "common.h"
#include "parser/ast.h"
#include "parser/cfg.h"

class Selection {
};


class Segment {
public:
  Segment() {}
  ~Segment() {}
  Segment(const Segment &q);
  Segment(ASTNode *astnode);
  Segment(CFGNode *cfgnode);

  /**
   * Modification
   */
  void Merge(Segment *s);
  void Add(CFGNode *node, bool inter=false);
  void Remove(CFGNode *node);
  void Remove(std::set<CFGNode*> nodes);
  // if the "new" set contains a branch, remove it
  // return true if removed
  bool RemoveNewBranch();
  // TODO remove branch based on the problematic one
  void SetHead(CFGNode *head);
  CFGNode* Head() {return m_head;}
  std::set<CFGNode*> New() {return m_new;}
  /**
   * Retrieve information
   */
  std::set<CFGNode*> GetSelection() {return m_selection;}

  // TODO nodes in the CFG that contains m_new
  std::set<CFGNode*> GetNodesForNewFunction();
  bool ContainNode(CFGNode *node);
  // void Visualize(bool open=true);

  /**
   * Code generating
   */
  void ResolveInput();
  void GenCode();
  std::string GetMain() {return m_main;}
  std::string GetSupport() {return m_support;}
  std::string GetMakefile() {return m_makefile;}
  // std::map<std::string, Type*> GetInputs() {return m_inputs;}
  std::vector<Variable*> GetInputs() {return m_inputs;}
  // getopt specification string
  std::string GetOpt();
  bool IsValid() {return m_valid;}
  static void SetPOI(CFGNode *poi) {m_poi = poi;}
private:

  static CFGNode *m_poi;
  bool m_valid = true;

  std::set<CFGNode*> m_selection;
  // the head should always be a single node
  CFGNode *m_head = NULL;
  // the new nodes should be a lot due to:
  // 1. the grammar completion process might add more
  // 2. the merge might bring more
  std::set<CFGNode*> m_new;
  // the callsite nodes. These nodes cannot be removed, otherwise resulting invalid segment.
  std::set<CFGNode*> m_callsites;


  // inputs
  std::vector<Variable*> m_inputs;
  
  // std::map<std::string, Type*> m_inputs;
  // std::map<std::string, Type*> m_inputs_without_decl;

  std::string m_main;
  std::string m_support;
  std::string m_makefile;
};


#endif /* SEGMENT_H */
