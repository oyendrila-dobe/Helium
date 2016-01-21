#include "resolver.h"
#include "utils.h"

#include <gtest/gtest.h>

using namespace ast;

TEST(resolver_test_case, io_resolver_test) {
  Doc doc;
  const char* raw = R"prefix(
	u_char *eom, *cp, *cp1, *rdatap;
	u_int class, type, dlen;
	int n,len=0;
	u_int32_t ttl;
	u_char data[MAXDATA*2];   /* sizeof data = 2 (2 * 1025 + 5*4) = 4140 */
	HEADER *hp = (HEADER *)msg;

if (!ns_nameok((char *)data, class, NULL)) {
            printf("Name not ok!\n");
            hp->rcode = 1;
            return (-1);
          }
)prefix";
  utils::string2xml(raw, doc);
  NodeList if_nodes = find_nodes(doc, NK_If);
  ASSERT_EQ(if_nodes.size(), 1);
  VariableList result;
  resolver::get_undefined_vars(if_nodes[0], result);
  // for (Variable var : result) {
  //   std::cout <<var.Name()  << ":";
  //   std::cout <<var.GetType().ToString()  << "\n";
  // }
  ASSERT_EQ(result.size(), 3);
  EXPECT_EQ(look_up(result, "data").GetType().ToString(), "u_char[]");
  EXPECT_EQ(look_up(result, "class").GetType().ToString(), "u_int");
  EXPECT_EQ(look_up(result, "hp").GetType().ToString(), "HEADER*");
  
}
