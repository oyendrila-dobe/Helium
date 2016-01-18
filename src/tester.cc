#include "tester.h"
#include <fstream>
#include "utils.h"
#include "config.h"

using namespace utils;

static int
myrand(int low, int high) {
  double d = rand();
  d /= RAND_MAX;
  return low + (high-low)*d;
}

Tester::Tester(const std::string &executable, SPU spu)
: m_executable(executable), m_spu(spu), m_success(false) {
  srand(time(0));
  // the first random number is highly related to the time, so we don't use it
  rand();
}

Tester::~Tester() {}

std::string
get_input_by_spec(std::string spec) {
  std::vector<std::string> specs = split(spec, ',');
  std::string text;
  int size = 1;
  if (specs.size() == 0) return "";
  // array
  if (specs[0] == "size") {
    size = myrand(1, 10);
    text += std::to_string(size);
    text += " ";
    specs.erase(specs.begin());
  }
  for (int i=0;i<size;i++) {
    for (auto it=specs.begin();it!=specs.end();it++) {
      if (it->find("_") == std::string::npos) continue;
      int low = atoi(it->substr(0, it->find("_")).c_str());
      int high = atoi(it->substr(it->find("_")+1).c_str());
      text += std::to_string(myrand(low, high)) + " ";
    }
  }
  return text;
}

std::string
Tester::generateInput() {
  // get input specification
  VariableList inv = m_spu.GetInputVariables();
  std::string text;
  // for (auto it=inv.begin();it!=inv.end();it++) {
  //   std::string input_spec = (*it)->GetInputSpecification();
  //   text += get_input_by_spec(input_spec);
  // }
  // random & pair
  return text;
}

void
Tester::Test() {
  // generate input
  std::string input = generateInput();
  // run program
  std::string cmd;
  // right now, hard code to timeout 2 seconds
  int status;
  std::string result = exec(m_executable.c_str(), input.c_str(), &status, 2);
  if (status == 0) {
    m_success = true;
  } else {
    m_success = false;
  }
  // get output
  VariableList outv = m_spu.GetOutputVariables();
  int size = outv.Size();
  std::vector<std::string> results = split(result);
  std::string outcsv = Config::Instance()->GetString("output-folder") + "/out.csv";
  std::ofstream os;
  os.open(outcsv);
  if (os.is_open()) {
    int count = 0;
    // for (auto it=outv.begin();it!=outv.end();it++) {
    //   os << (*it)->GetName();
    //   if (++count<size) {
    //     os << ",";
    //   }
    // }
    os << "\n";
    count = 0;
    for (auto it=results.begin();it!=results.end();it++) {
      count++;
      os << *it;
      if (count ==size) {
        count = 0;
        os << '\n';
      } else {
        os << ',';
      }
    }
    m_success = true;
    os.close();
  }
}

bool Tester::Success() {
  return m_success;
}

void
Tester::WriteCSV() {}