#include <string>
#include <stack>
#include <vector>
#include "work.hpp"

void collect_work(vertex u, graph& g, std::stack<work_item>& s,
                  vector<rewrite_fun>const& check, string history, bool all) {
  bool opt = true;

  for (unsigned int i = 0; i != check.size(); ++i) {
    if (check[i](u, g)) {
      work_item w(new graph(g), i, u, history + "__" +
          boost::lexical_cast<string>(u) + "_" +
          boost::lexical_cast<string>(i), 0);
      s.push(w);
      opt = false;
    }
  }

  if (all || opt) {
    work_item w(new graph(g), -1, u, history + "__" +
        boost::lexical_cast<string>(u) + "-", 0);
    s.push(w);
  }
}
