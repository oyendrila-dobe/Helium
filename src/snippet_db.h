#ifndef SNIPPET_DB_H
#define SNIPPET_DB_H
#include "common.h"
#include "snippet.h"
#include <sqlite3.h>

// filename, linenumber, (keyword, kind)s
class SnippetMeta {
public:
  SnippetMeta() {}
  SnippetMeta(std::string f, int l) : filename(f), linum(l) {}
  void AddSignature(std::string key, SnippetKind k) {
    signature[key].insert(k);
  }
  bool HasKind(SnippetKind k) {
    for (auto sig : signature) {
      if (sig.second.count(k) == 1) return true;
    }
    return false;
  }
  /**
   * Get one key.
   */
  std::string GetKey() {
    assert(!signature.empty());
    return signature.begin()->first;
  }
  std::string filename;
  int linum=0;
  std::map<std::string, std::set<SnippetKind> > signature;
};

/**
 * This class is a singleton.
 * The database must be loaded, or created, before calling any methods.
 */
class SnippetDB {
public:
  static SnippetDB *Instance() {
    if (!m_instance) {
      m_instance = new SnippetDB();
    }
    return m_instance;
  }
  void Load(std::string folder);
  void Create(std::string tagfile, std::string output_folder);
  std::set<int> LookUp(std::string key, std::set<SnippetKind> kinds={});
  std::set<int> LookUp(std::set<std::string> keys, std::set<SnippetKind> kinds={});
  SnippetMeta GetMeta(int snippet_id);
  std::string GetCode(int snippet_id);
  std::set<int> RemoveDup(std::set<int> snippet_ids);
  std::set<int> GetAllDep(std::set<int> snippet_ids);
  std::set<int> GetDep(int id);
  std::vector<int> SortSnippets(std::set<int> snippets);

  /**
   * Print methods
   */
  void PrintCG();
private:
  std::vector<int> queryInt(const char *query);
  std::vector<std::string> queryStr(const char *query);
  std::vector<std::pair<int, std::string> > queryIntStr(const char *query);
  std::vector<std::pair<std::string, int> > queryStrInt(const char *query);
  std::vector<std::pair<std::string, char> > queryStrChar(const char *query);
  std::vector<std::pair<int, int> > queryIntInt(const char *query);

  // TODO Don't know how to use template to do this.
  // template <typename T1, typename T2> std::vector<std::pair<T1, T2> > queryTT(char *query);

  /**
   * Customized queries
   */
  std::map<std::string, int> queryFunctions();

  int insertSnippet(Snippet *s);
  void createTable();
  void createDep();
  void createCG();
  std::map<std::string, std::set<std::string> > constructCG(std::map<std::string, int> &all_functions);
  void createHeaderDep();

  /**
   * The following 2 fields should be initiliazed by Load or Create, otherwise the code is invalid
   */
  std::string m_db_folder;
  sqlite3 *m_db = NULL;
  /**
   * Cache
   */
  std::map<int, SnippetMeta> m_snippet_cache;
  std::map<int, std::string> m_snippet_code_cache;
  SnippetDB() {}
  static SnippetDB *m_instance;
};

// void create_callgraph();
// void create_callgraph(sqlite3 *db);

#endif /* SNIPPET-DB_H */
