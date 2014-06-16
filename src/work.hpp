#ifndef WORK_HPP_
#define WORK_HPP_

#include "syntax.hpp"
#include <stack>

struct work_item {
	work_item(graph* g, int rewrite_rule, vertex u, string history, 
			int nesting)
		: rewrite_rule(rewrite_rule), u(u), g(g), history(history),
		nesting(nesting) { }

	// can go after old stack is gone.
	// maybe not -- evaluate partitioning needs
	int rewrite_rule;
	vertex u;

	// needed for new stack
	graph* g;
	string history;
	int nesting;

	void del() {
		history.clear();
		delete g;
	}
};

void collect_work(vertex u, graph& g, std::stack<work_item>& s,
		vector<rewrite_fun>const& check, string history, bool all);
#endif /*WORK_HPP_*/
