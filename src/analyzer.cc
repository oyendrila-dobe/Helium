#include "analyzer.h"
#include "utils.h"

#include "gtest/gtest.h"
#include "options.h"

/********************************
 * Helper functions
 *******************************/

std::string rel_to_string(std::string h1, std::string h2, RelKind k) {
  switch (k) {
  case RK_Equal:
    return h1 + "=" + h2;
  case RK_Less:
    return h1 + "<" + h2;
  case RK_LessEqual:
    return h1 + "<=" + h2;
  case RK_Larger:
    return h1 + ">" + h2;
  case RK_LargerEqual:
    return h1 + ">=" + h2;
  case RK_NA:
    assert(false);
  }
  assert(false);
}


/**
 * Remove NA rows
 */
static void validate(std::vector<std::string> &d1, std::vector<std::string> &d2) {
  assert(d1.size() == d2.size());
  int size = d1.size();
  std::vector<std::string> ret1,ret2;
  for (int i=0;i<size;i++) {
    if (d1[i] == "NA" || d2[i] == "NA") continue;
    ret1.push_back(d1[i]);
    ret2.push_back(d2[i]);
  }
  d1 = ret1;
  d2 = ret2;
}

static void validate(std::vector<std::string> &d) {
  std::vector<std::string> ret;
  for (std::string s : d) {
    if (s != "NA") ret.push_back(s);
  }
  d = ret;
}

static bool validate_number(std::vector<std::string> &d) {
  for (std::string s : d) {
    if (!utils::is_number(s)) return false;
  }
  return true;
}

static bool validate_addr(std::vector<std::string> &d) {
  for (std::string s : d) {
    if (s.size() < 2 || s[0] != '0' || s[1] != 'x') return false;
  }
  return true;
}

/**
 * Check if the distance of d2-d1 is constant
 * the result is d2 = d1 + b, or empty
 */
std::string check_const_dist(std::vector<std::string> d1, std::vector<std::string> d2) {
  int size = d1.size();
  // should be numbers to compare
  if (!validate_number(d1)) return "";
  if (!validate_number(d2)) return "";
  // check
  // assert(size > 0);
  if (size == 0) return "";
  int i1 = atoi(d1[0].c_str());
  int i2 = atoi(d2[0].c_str());
  int dist = i2 - i1;
  for (int i=0;i<size;i++) {
    int i1 = atoi(d1[i].c_str());
    int i2 = atoi(d2[i].c_str());
    if (i2 - i1 != dist) return "";
  }
  return std::to_string(dist);
}

/**
 * Integer
 */
std::string check_const_dist_d(std::vector<std::string> d1, std::vector<std::string> d2) {
  int size = d1.size();
  // should be numbers to compare. This should be assert
  // if (!validate_number(d1)) return "";
  // if (!validate_number(d2)) return "";
  assert(validate_number(d1));
  assert(validate_number(d2));
  // check
  // assert(size > 0);
  if (size == 0) return "";
  int i1 = atoi(d1[0].c_str());
  int i2 = atoi(d2[0].c_str());
  int dist = i2 - i1;
  for (int i=0;i<size;i++) {
    int i1 = atoi(d1[i].c_str());
    int i2 = atoi(d2[i].c_str());
    if (i2 - i1 != dist) return "";
  }
  return std::to_string(dist);
}

/**
 * Hex Address
 */
std::string check_const_dist_p(std::vector<std::string> d1, std::vector<std::string> d2) {
  int size = d1.size();
  // should be numbers to compare
  // if (!validate_number(d1)) return "";
  // if (!validate_number(d2)) return "";
  // assert address
  assert(validate_addr(d1));
  assert(validate_addr(d2));
  // check
  // assert(size > 0);
  if (size == 0) return "";
  long int p1 = strtol(d1[0].c_str(), NULL, 16);
  long int p2 = strtol(d2[0].c_str(), NULL, 16);
  // int i1 = atoi(d1[0].c_str());
  // int i2 = atoi(d2[0].c_str());
  // int dist = i2 - i1;
  long int dist = p2 - p1;
  for (int i=0;i<size;i++) {
    long int p1 = strtol(d1[i].c_str(), NULL, 16);
    long int p2 = strtol(d2[i].c_str(), NULL, 16);
    // TODO the distance is larger than something is also useful
    if (p2 - p1 != dist) return "";
  }
  return std::to_string(dist);
  // for (int i=0;i<size;i++) {
  //   int i1 = atoi(d1[i].c_str());
  //   int i2 = atoi(d2[i].c_str());
  //   if (i2 - i1 != dist) return "";
  // }
  // return std::to_string(dist);
}



std::string check_constant(std::vector<std::string> d) {
  int size = (int)d.size();
  std::string last;
  for (int idx=0;idx<size;idx++) {
    // if (d[idx] == "NA") continue;
    if (last.empty()) {
      last = d[idx];
    } else {
      if (last != d[idx]) return "";
    }
  }
  // no valid row detected
  if (last.empty()) return "";
  return last;
}

/**
 * DEPRECATED use the _d and _p version instead, to handle specific types
 */
RelKind check_relation(std::vector<std::string> d1, std::vector<std::string> d2) {
  validate(d1, d2);
  assert(d1.size() == d2.size());
  int size = (int)d1.size();
  // assert(size > 0); // should have at least one data row
  if (size == 0) return RK_NA;
  // should be numbers to compare
  if (!validate_number(d1)) return RK_NA;
  if (!validate_number(d2)) return RK_NA;
  // compuare these at the same time
  bool is_equal = true;
  bool is_less = true;
  bool is_less_equal = true;
  bool is_larger = true;
  bool is_larger_equal = true;
  for (int idx=0;idx<size;idx++) {
    // if any one is not available, just discard this record
    // FIXME but what is all NAs?
    // if (d1[idx] == "NA" || d2[idx] == "NA") continue;
    int i1 = atoi(d1[idx].c_str());
    int i2 = atoi(d2[idx].c_str());
    if (i1 != i2) {
      is_equal = false;
      is_larger = false;
      is_less = false;
    }
    if (i1 < i2) {
      is_larger = false;
      is_larger_equal = false;
    }
    if (i1 > i2) {
      is_less = false;
      is_less_equal = false;
    }
  }
  // return the result in order
  if (is_equal) return RK_Equal;
  else if (is_less) return RK_Less;
  else if (is_larger) return RK_Larger;
  else if (is_less_equal) return RK_LessEqual;
  else if (is_larger_equal) return RK_LargerEqual;
  else return RK_NA;
}

RelKind check_relation_d(std::vector<std::string> d1, std::vector<std::string> d2) {
  validate(d1, d2);
  assert(d1.size() == d2.size());
  int size = (int)d1.size();
  // assert(size > 0); // should have at least one data row
  if (size == 0) return RK_NA;
  // should be numbers to compare
  // if (!validate_number(d1)) return RK_NA;
  // if (!validate_number(d2)) return RK_NA;
  assert(validate_number(d1));
  assert(validate_number(d2));
  // compuare these at the same time
  bool is_equal = true;
  bool is_less = true;
  bool is_less_equal = true;
  bool is_larger = true;
  bool is_larger_equal = true;
  for (int idx=0;idx<size;idx++) {
    int i1 = atoi(d1[idx].c_str());
    int i2 = atoi(d2[idx].c_str());
    if (i1 != i2) {
      is_equal = false;
      is_larger = false;
      is_less = false;
    }
    if (i1 < i2) {
      is_larger = false;
      is_larger_equal = false;
    }
    if (i1 > i2) {
      is_less = false;
      is_less_equal = false;
    }
  }
  // return the result in order
  if (is_equal) return RK_Equal;
  else if (is_less) return RK_Less;
  else if (is_larger) return RK_Larger;
  else if (is_less_equal) return RK_LessEqual;
  else if (is_larger_equal) return RK_LargerEqual;
  else return RK_NA;
}

RelKind check_relation_p(std::vector<std::string> d1, std::vector<std::string> d2) {
  validate(d1, d2);
  assert(d1.size() == d2.size());
  int size = (int)d1.size();
  // assert(size > 0); // should have at least one data row
  if (size == 0) return RK_NA;
  // should be numbers to compare
  // if (!validate_number(d1)) return RK_NA;
  // if (!validate_number(d2)) return RK_NA;
  // assert(validate_number(d1));
  // assert(validate_number(d2));
  // compuare these at the same time
  bool is_equal = true;
  bool is_less = true;
  bool is_less_equal = true;
  bool is_larger = true;
  bool is_larger_equal = true;
  for (int idx=0;idx<size;idx++) {
    // int i1 = atoi(d1[idx].c_str());
    // int i2 = atoi(d2[idx].c_str());
    long int p1 = strtol(d1[idx].c_str(), NULL, 16);
    long int p2 = strtol(d2[idx].c_str(), NULL, 16);
    if (p1 != p2) {
      is_equal = false;
      is_larger = false;
      is_less = false;
    }
    if (p1 < p2) {
      is_larger = false;
      is_larger_equal = false;
    }
    if (p1 > p2) {
      is_less = false;
      is_less_equal = false;
    }
  }
  // return the result in order
  if (is_equal) return RK_Equal;
  else if (is_less) return RK_Less;
  else if (is_larger) return RK_Larger;
  else if (is_less_equal) return RK_LessEqual;
  else if (is_larger_equal) return RK_LargerEqual;
  else return RK_NA;
}


/********************************
 * The Analyzer Class
 *******************************/


/**
 * Special headers:
 * - HELIUM_TEST_SUCCESS: the return code
 * - HELIUM_POI: whether the POI right before POI is printed
 */
Analyzer::Analyzer(std::string csv_file) {
  std::ifstream is;
  is.open(csv_file);
  assert(is.is_open());
  // I'm going to collect some summary metrics for the CSV file
  int total = 0;
  int reach_poi_test_success = 0;
  int reach_poi_test_failure = 0;
  int no_reach_poi_test_success = 0;
  int no_reach_poi_test_failure = 0;
  if (is.is_open()) {
    std::string line;
    getline(is, line);
    m_header = utils::split(line, ',');
    assert(m_header.size() >= 2);
    assert(m_header[0] == "HELIUM_POI");
    assert(m_header[1] == "HELIUM_TEST_SUCCESS");
    while (getline(is, line)) {
      // I need to remove some invalid rows
      // the rows I want is:
      // HELIUM_POI: true
      // HELIUM_TEST_SUCCESS: false or true
      std::vector<std::string> row = utils::split(line, ',');
      total++;
      if (row[0] == "true") {
        // reach POI
        if (row[1] == "true") {
          reach_poi_test_success++;
        } else {
          m_raw_data.push_back(row);
          reach_poi_test_failure++;
        }
      } else {
        if (row[1] == "true") {
          no_reach_poi_test_success++;
        } else {
          no_reach_poi_test_failure++;
        }
      }
    }
    is.close();
  } else {
    assert(false);
  }
  // std::cout << "Valid data: " << m_raw_data.size()  << "\n";
  if (PrintOption::Instance()->Has(POK_CSVSummary)) {
    std::cerr << "------ CSV summary ------" << "\n";
    std::cout << "| total records: " << total  << "\n";
    std::cout << "| reach POI, return zero: " << reach_poi_test_success  << "\n";
    std::cout << "| reach POI, return non-zero: " << reach_poi_test_failure  << "\n";
    std::cout << "| no reach POI, return zero: " << no_reach_poi_test_success  << "\n";
    std::cout << "| no reach POI, return non-zero: " << no_reach_poi_test_failure  << "\n";
    std::cout << "-------------------------"  << "\n";
  }
  // std::cout << "raw data:"  << "\n";
  // std::cout << m_raw_data.size()  << "\n";
  // for (auto &v : m_raw_data) {
  //   for (auto s : v) {
  //     std::cout << s << " ";
  //   }
  //   std::cout << "\n";
  // }
  // std::cout << "header size: "  << "\n";
  // std::cout << m_header.size()  << "\n";
  
  // process the raw data
  for (int i=0;i<(int)m_header.size();++i) {
    std::string header = m_header[i];
    for (int j=0;j<(int)m_raw_data.size();++j) {
      m_data_m[header].push_back(m_raw_data[j][i]);
    }
  }
  // std::cout << m_data_m.size()  << "\n";
  // construct i header and o header
  for (std::string h : m_header) {
    assert(h.size() > 0);
    if (h[0] == 'I') {
      m_i_header.push_back(h);
    } else if (h[0] == 'O') {
      m_o_header.push_back(h);
    } else {
      // No code here. It might be HELIUM_TEST_SUCCESS
    }
  }
}


/**
 * OK, now I COMPUTE the invairants, instead of GET
 * 1. constants, e.g. a == NULL
 * 2. relations, e.g. a < b
 */
std::vector<std::string> Analyzer::GetInvariants() {
  std::vector<std::string> ret;
  // invariants cares about the Out variables
  for (int i=0;i<(int)m_o_header.size();++i) {
    // check constant
    std::string res = checkConstant(m_o_header[i]);
    if (!res.empty()) {
      ret.push_back(res);
    }
    // pairwise comparison
    for (int j=i+1;j<(int)m_o_header.size();++j) {
      // compare i and j
      std::string res = checkRelation(m_o_header[i], m_o_header[j]);
      if (!res.empty()) {
        ret.push_back(res);
      }
      // if (kind != RK_NA) {
      //   std::string s = rel_to_string(m_o_header[i], m_o_header[j], kind);
      //   ret.push_back(s);
      // }
    }
  }
  return ret;
}

/**
 * Transfer functions capture the relation from Input to Output
 */
std::vector<std::string> Analyzer::GetTransferFunctions() {
  std::vector<std::string> ret;
  for (int i=0;i<(int)m_i_header.size();i++) {
    for (int o=0;o<(int)m_o_header.size();o++) {
      std::string trans = checkTransfer(m_i_header[i], m_o_header[o]);
      if (!trans.empty()) {
        ret.push_back(trans);
      }
    }
  }
  return ret;
}

/**
 * Pre conditions are invariants for Input variables
 */
std::vector<std::string> Analyzer::GetPreConditions() {
  std::vector<std::string> ret;
  // invariants cares about the Out variables
  for (int i=0;i<(int)m_i_header.size();++i) {
    // check constant
    std::string res = checkConstant(m_i_header[i]);
    if (!res.empty()) {
      ret.push_back(res);
    }
    // for pre-conditions, we need to check for each column, against the discovered constants
    std::vector<std::string> vs = checkDiscoveredConstants(m_i_header[i]);
    ret.insert(ret.end(), vs.begin(), vs.end());
    // not necessary to check the pairwise comparison actually, because the input parameters are huge ...
    // e.g. when the input variable is a char**
    // pairwise comparison
    for (int j=i+1;j<(int)m_i_header.size();++j) {
      // compare i and j
      std::string res = checkRelation(m_i_header[i], m_i_header[j]);
      if (!res.empty()) {
        ret.push_back(res);
      }
      // if (kind != RK_NA) {
      //   std::string s = rel_to_string(m_i_header[i], m_i_header[j], kind);
      //   ret.push_back(s);
      // }
    }
  }
  return ret;
}

/********************************
 * Private Methods
 *******************************/


/**
 * Return the ready formula, not just the constant
 */
std::string Analyzer::checkConstant(std::string header) {
  // std::cout << "checking constant: " << header  << "\n";
  std::vector<std::string> data = m_data_m[header];
  validate(data);
  if (data.size() < 2) return "";
  std::string ret;
  std::string res = check_constant(data);
  if (!res.empty()) {
    ret += header + "=" + res + " conf:" + std::to_string(data.size());
    if (header[1] == 'd') {
      // add discovered constants. This is very likely to be a buffer size.
      // this is important for precondition generation
      m_discovered_constants.insert(atoi(res.c_str()));
    }
  }
  return ret;
}

RelKind check_against_constant(std::vector<std::string> data, int a) {
  assert(validate_number(data));
  std::vector<std::string> aa(data.size(), std::to_string(a));
  RelKind kind = check_relation_d(data, aa);
  return kind;
}

/**
 * Check the column, against a set of discovered constants, in m_discovered_constants
 * This is used for precondition generation, and the constants are discovered in invariants generation phase
 */
std::vector<std::string> Analyzer::checkDiscoveredConstants(std::string header) {
  std::vector<std::string> ret;
  if (m_discovered_constants.empty()) return ret;
  // std::cout << "m discovered constants: " << m_discovered_constants.size()  << "\n";
  // it should be numbers
  if (header[1] != 'd') return ret;
  std::vector<std::string> data = m_data_m[header];
  validate(data);
  if (data.size() < 2) return ret;
  for (int a : m_discovered_constants) {
    RelKind k = check_against_constant(data, a);
    if (k != RK_NA) {
      std::string res = rel_to_string(header, std::to_string(a), k);
      res += " conf: " + std::to_string(data.size());
      ret.push_back(res);
    }
  }
  return ret;
}


std::string Analyzer::checkRelation(std::string h1, std::string h2) {
  // std::cout << "checking relation: " << h1 << " vs. " << h2  << "\n";
  std::vector<std::string> d1 = m_data_m[h1];
  std::vector<std::string> d2 = m_data_m[h2];
  // only check when the type mtches
  // if (h1[1] != h2[1]) return RK_NA;
  validate(d1, d2);
  if (d1.size() < 2) return "";
  RelKind kind;
  if (h1[1] == 'd' && h2[1] == 'd') {
    kind = check_relation_d(d1, d2);
  } else if (h1[1] == 'p' && h2[1] == 'p') {
    kind = check_relation_p(d1, d2);
  } else {
    // std::cerr << "type does not match: " << h1[1] << ":" << h2[1] << "\n";
    kind = RK_NA;
  }
  std::string ret;
  if (kind != RK_NA) {
    ret = rel_to_string(h1, h2, kind);
    ret += " conf: " + std::to_string(d1.size());
  }
  return ret;
  // return check_relation(d1, d2, h1[1]);
}

std::string Analyzer::checkTransfer(std::string h1, std::string h2) {
  // std::cout << "checking transfer: " << h1 << " vs. " << h2  << "\n";
  std::vector<std::string> d1 = m_data_m[h1];
  std::vector<std::string> d2 = m_data_m[h2];
  // only compare when type matches
  // if (h1[1] != h2[1]) return "";
  validate(d1, d2);
  if (d1.size() < 2) return "";
  // check the distance is constant or not
  std::string dist;
  if (h1[1] == 'd' && h2[1] == 'd') {
    dist = check_const_dist_d(d1, d2);
  } else if (h1[1] == 'p' && h2[1] == 'p') {
    dist = check_const_dist_p(d1, d2);
  } else {
    // std::cerr << "type does not match: " << h1[1] << ":" << h2[1] << "\n";
  }
  std::string ret;
  if (!dist.empty()) {
    if (dist == "0") {
      ret = h2 + "=" + h1;
    } else {
      ret = h2 + "=" + h1 + "+" + dist;
    }
    ret += " conf:" + std::to_string(d1.size());
  }
  return ret;
}

/********************************
 * TESTs
 *******************************/

/**
 * Disabled because has a path
 */
TEST(AnalyzerTestCase, DISABLED_CSVTest) {
  Analyzer analyzer("/Users/hebi/tmp/b.csv");
  std::vector<std::string> invs = analyzer.GetInvariants();
  std::vector<std::string> pres = analyzer.GetPreConditions();
  std::vector<std::string> trans = analyzer.GetTransferFunctions();
  std::cout << "invariants:"  << "\n";
  for (std::string s : invs) {
    std::cout << s  << "\n";
  }
  std::cout << "pre conditions:"  << "\n";
  for (std::string s : pres) {
    std::cout << s  << "\n";
  }
  std::cout << "transfer functions:"  << "\n";
  for (std::string s : trans) {
    std::cout << s  << "\n";
  }
}
