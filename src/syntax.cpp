#include <iostream>
#include "syntax.hpp"

string type_to_string(const type &t)
{

  string tmp;
  switch (t.k) {
	  case unknown: 
	  {
	  	tmp = "?";
	  	break;
	  }
	  case row:
	  {
	  	tmp = "row<";
	  	break;
	  }
	  case column:
	  {
	  	tmp = "col<";
	  	break;
	  }
	  case scalar:
	  {
	  	tmp = "scl";
	  	break;
	  }
      case parallel_reduce:
      {
          tmp = "pr<";
          break;
      }
	  default:
	  {
	  	tmp = "????";
	  }
  }
	  if (t.t) {
	  	tmp += type_to_string(*(t.t)) + ">";
	  }
	  return tmp;
}

string uplo_to_string(uplo_t uplo) {
    if (uplo == upper)
        return "upper";
    else if (uplo == lower)
        return "lower";
    return "";
}

string attribute_to_string(sg_iterator_t *iter) {
    if (iter->format == triangular) {
        triangular_itr *attrib = (triangular_itr*)iter->attributes;
        return "; " + uplo_to_string(attrib->uplo);
    }
    return "";
}

string format_to_name(storage s) {
    switch(s) {
        case general:
            return "general";
        case triangular:
            return "triangular";
        default:
            return "unknown";
    }
}

string op_to_name(op_code op) {
	switch (op) {
		case trans:
			return "trans";
		case negate_op:
			return "negate";
		case squareroot:
			return "squareroot";
		case add:
			return "add";
		case subtract:
			return "sub";
		case multiply:
			return "mul";
		case divide:
			return "div";
		case sum:
			return "sum";
		case input: 
			return "input";
		case output: 
			return "output";
		case temporary:
			return "tmp";
		case get_element: 
			return "get_element";
		case get_row: 
			return "get_row";
		case get_column:
			return "get_column";
		case get_row_from_column: 
			return "get_row_from_column";
		case get_column_from_row:
			return "get_column_from_row";
		case store_element: 
			return "store_element";
		case store_add_element: 
			return "store_add_element";
		case store_row: 
			return "store_row";
		case store_add_row:
			return "store_add_row";
		case store_column:
			return "store_column";
		case store_add_column:
			return "store_add_column";
		case sumto:
			return "sumto";
		case partition_cast:
			return "cast";
		default:
			return "";
	}
}

string op_to_string(op_code op) {
	switch (op) {
		case trans:
			return "'";
		case negate_op:
			return "-";
		case squareroot:
			return "sqrt";
		case add:
			return "+";
		case subtract:
			return "-";
		case multiply:
			return "*";
		case divide:
			return "/";
		case sum:
			return "Sum";
		case input: 
			return "in";
		case output: 
			return "out";
		case temporary:
			return "tmp";
		case get_element: 
			return "(i)";
		case get_row: 
			return "(i,:)";
		case get_column:
			return "(:,j)";
		case get_row_from_column: 
			return "(*i,:)";
		case get_column_from_row:
			return "(:,*j)";
		case store_element: 
			return "(i)<-";
		case store_add_element: 
			return "(i)+=";
		case store_row: 
			return "(i,:)<-";
		case store_add_row:
			return "(i,:)+<-";
		case store_column:
			return "(:,j)<-";
		case store_add_column:
			return "(:,j)+<-";
		case sumto:
			return "sumto";
		case partition_cast:
			return "cast";
		default:
			return "";
	}
}

string vertex_info::to_string(vertex i) const { 
  string shape = "ellipse";
  string style = "filled";
  string color = (eval == evaluate) ? "red" : "gray";
  string dim1 = t.dim.dim;
  
  return "[label=\"" + boost::lexical_cast<string>(i) + ". " + label
    + ":" + op_to_string(op) + ":" + type_to_string(t)
    + "\\n" + format_to_name(t.s) + ";" + uplo_to_string(t.uplo) +
    + "\\n" + dim1 + ":" + t.dim.step + "\""
    + ",shape=" + shape + ",color=" + color + ",style=" + style + ",width=1.75];";
}


void variable::print() { std::cout << name; }

void scalar_in::print() { std::cout << val; }

void stmt::print() {
  std::cout << lhs << " = ";
  rhs->print();
}

void operation::print() {
	std::cout << "(";
	switch (op) {
		case trans:
			operands[0]->print();
			std::cout << "'";
			break;
		case negate_op:
			std::cout << "-";
			operands[0]->print();
			break;
		case squareroot:
			std::cout << "squareroot(";
			operands[0]->print();
			std::cout << ")";
			break;
		case add:
			operands[0]->print();
			std::cout << "+";
			operands[1]->print();
			break;
		case subtract:
			operands[0]->print();
			std::cout << "-";
			operands[1]->print();
			break;
		case multiply:
			operands[0]->print();
			std::cout << "*";
			operands[1]->print();
			break;
		case divide:
			operands[0]->print();
			std::cout << "/";
			operands[1]->print();
			break;
		case sum:
			std::cout << "Sum";
			operands[0]->print();
			break;
		case input:
			std::cout << "in";
			break;
		case output:
			std::cout << "out";
			break;
		case temporary:
			std::cout << "tmp";
			break;
		case get_row:
			std::cout << "get_row";
			break;
		case get_column:
			std::cout << "get_column";
			break;
		case get_row_from_column:
			std::cout << "get_row_from_column";
			break;
		case get_column_from_row:
			std::cout << "get_column_from_row";
			break;
		case get_element:
			std::cout << "get_element";
			break;
		case store_row:
			std::cout << "store_row";
			break;
		case store_column:
			std::cout << "store_column";
			break;
		case store_add_row:
			std::cout << "store_add_row";
			break;
		case store_add_column:
			std::cout << "store_add_column";
			break;
		case store_element:
			std::cout << "store_element";
			break;
		case store_add_element:
			std::cout << "store_add_element";
			break;
		case partition_cast:
			std::cout << "cast";
			break;
        case sumto:
            std::cout << "sumto";
            break;
        case deleted:
            std::cout << "deleted";
            break;
	};
	std::cout << ")";
}

void print_program(vector<stmt*>& p) {
  for (unsigned int i = 0; i != p.size(); ++i) {
    p[i]->print();
    std::cout << std::endl;
  }
}

