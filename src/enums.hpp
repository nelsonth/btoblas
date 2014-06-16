//
//  enums.hpp
//  btoblas
//

/* 
 These enumerations had to come to a separate file because of the 
 dependencies between syntax.hpp and graph.hpp.  Structures in each of
 those files need these definitions fairly early.
 */

#ifndef btoblas_enums_hpp
#define btoblas_enums_hpp

enum op_code {	deleted = 0x0, 
    trans = 0x1, 
    negate_op = 0x2, 
    add = 0x4, 
    subtract = 0x8, 
    multiply = 0x10, 
    sum = 0x20,
    sumto = 0x40,
    input = 0x80, 
    output = 0x100, 
    temporary = 0x200,
    get_row = 0x400, 
    get_column = 0x800, 
    get_element = 0x1000, 
    get_row_from_column = 0x2000, 
    get_column_from_row = 0x4000, 
    store_row = 0x8000, 
    store_add_row = 0x10000, 
    store_add_column = 0x20000,
    store_column = 0x40000, 
    store_element = 0x80000,
    store_add_element = 0x100000, 
    partition_cast = 0x200000, 
    squareroot = 0x400000, 
    divide = 0x800000 };

#define OP_2OP_ARITHMATIC (add | subtract | multiply | divide)
#define OP_1OP_ARITHMATIC (squareroot)
#define OP_ANY_ARITHMATIC (OP_2OP_ARITHMATIC | OP_1OP_ARITHMATIC) 
#define OP_STORE		  (store_row | store_add_row | store_add_column \
| store_column | store_element | store_add_element)
#define OP_GET (get_row | get_column | get_element | get_row_from_column | get_column_from_row)

enum kind { unknown, row, column, scalar, parallel_reduce };
// there are uses of type where the storage format does not matter.  in these
// cases use any.
enum storage { any, general, triangular, compressed, coordinate };
enum uplo_t {uplo_null, upper, lower};
enum diag_t {diag_null, unit, nonunit} ;
enum Way { m=0, n=1, k=2};

#endif
