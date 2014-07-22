/*
 * opcode.h
 *
*/

#ifndef OPCODE_H_
#define OPCODE_H_

#include <string>
#include "sip.h"

namespace sip {
//codes for where clauses
enum where_code_t {
	where_eq = 0,
	where_geq = 1,
	where_leq = 2,
	where_gt = 3,
	where_lt = 4,
	where_neq = 5
};
/**opcodes and optable entry contents
 *
 * An optable entry contains the following fields
 * opcode, op0, op1, op2, selector[MAX_RANK], line_number
 *
* Below, the contents of the entry for each opcode is indicated
 * Unused slots are indicate with _
*
* X-Macros are used to define the opcodes as an enum and a enum-to-string function.
*
*/

#define SIP_OPCODES \
SIPOP(goto_op,100,"goto",false)\
SIPOP(jump_if_zero_op,101,"jump_if_zero",false)\
SIPOP(stop_op,102,"stop",false)\
SIPOP(call_op,103,"call",false)\
SIPOP(return_op,104,"return",false)\
SIPOP(execute_op,105,"execute",false)\
SIPOP(do_op,106,"do",false)\
SIPOP(enddo_op,107,"enddo",false)\
SIPOP(dosubindex_op,108,"dosubindex",false)\
SIPOP(enddosubindex_op,109,"enddosubindex",false)\
SIPOP(exit_op,110,"exit",false)\
SIPOP(where_op,111,"where",false)\
SIPOP(pardo_op,112,"pardo",false)\
SIPOP(endpardo_op,113,"endpardo",false)\
SIPOP(begin_pardo_section_op,114,"begin_pardo_section",false)\
SIPOP(end_pardo_section_op,115,"end_pardo_section",false)\
SIPOP(sip_barrier_op,116,"sip_barrier",false)\
SIPOP(broadcast_static_op,117,"broadcast_static",false)\
SIPOP(push_block_selector_op,118,"push_block_selector",false)\
SIPOP(allocate_op,119,"allocate",false)\
SIPOP(deallocate_op,120,"deallocate",false)\
SIPOP(allocate_contiguous_op,121,"allocate_contiguous",false)\
SIPOP(deallocate_contiguous_op,122,"deallocate_contiguous",false)\
SIPOP(get_op,123,"get",false)\
SIPOP(put_accumulate_op,124,"put_accumulate",false)\
SIPOP(put_replace_op,125,"put_replace",false)\
SIPOP(create_op,126,"create",false)\
SIPOP(delete_op,127,"delete",false)\
SIPOP(int_load_value_op,128,"int_load_value",false)\
SIPOP(int_load_literal_op,129,"int_load_literal",false)\
SIPOP(int_store_op,130,"int_store",false)\
SIPOP(index_load_value_op,131,"index_load_value",false)\
SIPOP(int_add_op,132,"int_add",false)\
SIPOP(int_subtract_op,133,"int_subtract",false)\
SIPOP(int_multiply_op,134,"int_multiply",false)\
SIPOP(int_divide_op,135,"int_divide",false)\
SIPOP(int_equal_op,136,"int_equal",false)\
SIPOP(int_nequal_op,137,"int_nequal",false)\
SIPOP(int_ge_op,138,"int_ge",false)\
SIPOP(int_le_op,139,"int_le",false)\
SIPOP(int_gt_op,140,"int_gt",false)\
SIPOP(int_lt_op,141,"int_lt",false)\
SIPOP(int_neg_op,142,"int_neg",false)\
SIPOP(cast_to_int_op,143,"cast_to_int",false)\
SIPOP(scalar_load_value_op,144,"scalar_load_value",false)\
SIPOP(scalar_store_op,145,"scalar_store",false)\
SIPOP(scalar_add_op,146,"scalar_add",false)\
SIPOP(scalar_subtract_op,147,"scalar_subtract",false)\
SIPOP(scalar_multiply_op,148,"scalar_multiply",false)\
SIPOP(scalar_divide_op,149,"scalar_divide",false)\
SIPOP(scalar_exp_op,150,"scalar_exp",false)\
SIPOP(scalar_eq_op,151,"scalar_eq",false)\
SIPOP(scalar_ne_op,152,"scalar_ne",false)\
SIPOP(scalar_ge_op,153,"scalar_ge",false)\
SIPOP(scalar_le_op,154,"scalar_le",false)\
SIPOP(scalar_gt_op,155,"scalar_gt",false)\
SIPOP(scalar_lt_op,156,"scalar_lt",false)\
SIPOP(scalar_neg_op,157,"scalar_neg",false)\
SIPOP(scalar_sqrt_op,158,"scalar_sqrt",false)\
SIPOP(cast_to_scalar_op,159,"cast_to_scalar",false)\
SIPOP(collective_sum_op,160,"collective_sum",false)\
SIPOP(assert_same_op,161,"assert_same",false)\
SIPOP(tensor_op,162,"tensor",false)\
SIPOP(block_copy_op,163,"block_copy",false)\
SIPOP(block_permute_op,164,"block_permute",false)\
SIPOP(fill_block_op,165,"fill_block",false)\
SIPOP(scale_block_op,166,"scale_block",false)\
SIPOP(accumulate_scalar_into_block_op,167,"accumulate_scalar_into_block",false)\
SIPOP(block_add_op,168,"block_add",false)\
SIPOP(block_subtract_op,169,"block_subtract",false)\
SIPOP(block_contract_op,170,"block_contract",false)\
SIPOP(block_contract_accumulate_op,171,"block_contract_accumulate",false)\
SIPOP(block_contract_to_scalar_op,172,"block_contract_to_scalar",false)\
SIPOP(block_load_scalar_op,173,"block_load_scalar",false)\
SIPOP(slice_op,174,"slice",false)\
SIPOP(insert_op,175,"insert",false)\
SIPOP(string_load_literal_op,176,"string_load_literal",false)\
SIPOP(print_string_op,177,"print_string",false)\
SIPOP(println_op,178,"println",false)\
SIPOP(print_index_op,179,"print_index",false)\
SIPOP(print_scalar_op,180,"print_scalar",false)\
SIPOP(print_int_op,181,"print_int",false)\
SIPOP(gpu_on_op,182,"gpu_on",false)\
SIPOP(gpu_off_op,183,"gpu_off",false)\
SIPOP(gpu_allocate_op,184,"gpu_allocate",false)\
SIPOP(gpu_free_op,185,"gpu_free",false)\
SIPOP(gpu_put_op,186,"gpu_put",false)\
SIPOP(gpu_get_op,187,"gpu_get",false)\
SIPOP(gpu_get_int_op,188,"gpu_get_int",false)\
SIPOP(gpu_put_int_op,189,"gpu_put_int",false)\
SIPOP(set_persistent_op,190,"set_persistent",false)\
SIPOP(restore_persistent_op,191,"restore_persistent",false)\
SIPOP(idup_op,192,"idup",false)\
SIPOP(iswap_op,193,"iswap",false)\
SIPOP(sswap_op,194,"sswap",false)\
SIPOP(invalid_op,195,"invalid",false)\

enum opcode_t {
#define SIPOP(e,n,t,p) e = n,
				SIP_OPCODES
#undef SIPOP
				last_op
			};

			/**
			 * Converts an opcode to it's string equivalent
			 * @param
			 * @return
			 */
			std::string opcodeToName(opcode_t);
			/**
			 * Converts an integer to an opcode
			 * @param
			 * @return
			 */
			opcode_t intToOpcode(int);
			/**
			 * Whether a certain opcode is printable
			 * @param
			 * @return
			 */
			bool printableOpcode(opcode_t);
			} /* namespace sip */
#endif /* OPCODE_H_ */
