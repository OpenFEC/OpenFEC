/* $Id: of_ml_tool.h 186 2014-07-16 07:17:53Z roca $ */
/*
 * OpenFEC.org AL-FEC Library.
 * (c) Copyright 2009 - 2012 INRIA - All rights reserved
 * Contact: vincent.roca@inria.fr
 *
 * This software is governed by the CeCILL-C license under French law and
 * abiding by the rules of distribution of free software.  You can  use,
 * modify and/ or redistribute the software under the terms of the CeCILL-C
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info".
 *
 * As a counterpart to the access to the source code and  rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty  and the software's author,  the holder of the
 * economic rights,  and the successive licensors  have only  limited
 * liability.
 *
 * In this respect, the user's attention is drawn to the risks associated
 * with loading,  using,  modifying and/or developing or reproducing the
 * software by the user in light of its specific status of free software,
 * that may mean  that it is complicated to manipulate,  and  that  also
 * therefore means  that it is reserved for developers  and  experienced
 * professionals having in-depth computer knowledge. Users are therefore
 * encouraged to load and test the software's suitability as regards their
 * requirements in conditions enabling the security of their systems and/or
 * data to be ensured and,  more generally, to use and operate it in the
 * same conditions as regards security.
 *
 * The fact that you are presently reading this means that you have had
 * knowledge of the CeCILL-C license and that you accept its terms.
 */

#ifndef OF_ML_TOOL
#define OF_ML_TOOL


#ifdef OF_USE_DECODER
#ifdef OF_USE_LINEAR_BINARY_CODES_UTILS
#ifdef ML_DECODING

#ifdef OF_DEBUG
#define OP_ARGS ,UINT32* op
#define OP_ARG_VAL ,&(ofcb->stats_xor->nb_xor_for_ML)
#else
#define OP_ARGS
#define OP_ARG_VAL
#endif


/**
 * This function transforms the matrix into a triangular matrix
 *
 * @brief			triangularize the dense system
 * @param m			(IN/OUT) address of the dense matrix.
 * @param check_values		(IN/OUT)
 * @param ofcb			(IN) Linear-Binary-Code control-block.
 * @return			1 if it's OK, or 0 if an error took place.
 */
INT32	of_linear_binary_code_triangularize_dense_system (of_mod2dense *m,
							void **check_values,
							of_linear_binary_code_cb_t *ofcb);

/**
 * This function deduce the value of the missing symbols from the inverted dense matrix
 *
 * @brief			solve system with backward substitution
 * @param variables		(IN/OUT) address of the dense matrix.
 * @param constant_member
 * @param m 			(IN/OUT) address of the dense matrix.
 * @param ofcb			(IN/OUT) Linear-Binary-Code control-block.
 * @return			1 if it's OK, or 0 if an error took place.
 */
INT32	of_linear_binary_code_backward_substitution (void * variables[],
						  void * constant_member[],
						  of_mod2dense *m,
						  of_linear_binary_code_cb_t *ofcb);

/**
 * This function solves the system: first triangularize the system, then for each column,
 * do a forward elimination, then do the backward elimination.
 *
 * @brief			solves the system
 * @param m 			(IN/OUT) address of the dense matrix.
 * @param variables
 * @param constant_member	(IN/OUT)pointer to all constant members
 * @param ofcb			(IN/OUT) Linear-Binary-Code control-block.
 * @return			1 if it's OK, or 0 if an error took place.
 */
INT32	of_linear_binary_code_solve_dense_system (of_mod2dense		*m,
						void			**constant_member,
						void			**variables,
						of_linear_binary_code_cb_t	*ofcb);

/**
 * This function eliminates "1" entries in parity check matrix for each columns to make an upper triangular matrix.
 *
 * @brief			eliminates "1" entries in parity check matrix
 * @param m 			(IN/OUT) address of the dense matrix.
 * @param check_values		(IN/OUT) pointer to all constant members
 * @param ofcb			(IN/OUT) Linear-Binary-Code control-block.
 * @param col_idx		(IN) column index
 * @param stats			(IN)
 * @return			1 if it's OK, or 0 if an error took place.
 */
inline INT32	of_linear_binary_code_col_forward_elimination (of_mod2dense *m,
					void **check_values,
					of_linear_binary_code_cb_t *ofcb,
					INT32 col_idx);
/**
 * This function eliminates "1" entries in parity check matrix for each columns to make an upper triangular matrix, but choose a better pivot to minimize operations.
 *
 * @brief			eliminates "1" entries in parity check matrix
 * @param m 			(IN/OUT) address of the dense matrix.
 * @param check_values		(IN/OUT) pointer to all constant members
 * @param ofcb			(IN/OUT) Linear-Binary-Code control-block.
 * @param col_idx		(IN) column index
 * @return			1 if it's OK, or 0 if an error took place.
 */
INT32	of_linear_binary_code_col_forward_elimination_pivot_reordering (of_mod2dense *m,
									void **check_values,
									of_linear_binary_code_cb_t *ofcb,
									INT32 col_idx);

/**
 * This function update weight of a row ignoring nb_ignore values. Used by pivot reordering.
 *
 * @brief			update weight of a row ignoring nb_ignore values.
 * @param m 			(IN/OUT) address of the dense matrix.
 * @param row1			(IN) row to update
 * @param row2			(IN) second row to update
 * @param nb_ignore		(IN) number of first ignored values
 * @return			1 if it's OK, or 0 if an error took place.
 */
INT32	of_linear_binary_code_update_row_weight (of_mod2dense *m,
						INT32 row1,
						INT32 row2,
						INT32 nb_ignore);

/**
 * This function get weight of a row. Used by pivot reordering.
 *
 * @brief			get the weight of a row
 * @param m 			(IN/OUT) address of the dense matrix.
 * @param row_idx		(IN) row index
 * @return			weight of the row_idx row
 */
INT32	of_linear_binary_code_get_row_weight (of_mod2dense *m, INT32 row_idx);

/**
 * This function do permutations in parity check matrix for each columns to prepare the dense system.
 *
 * @brief			do permutations on matrix m and checkValues
 * @param m 			(IN/OUT) address of the dense matrix.
 * @param check_values		(IN/OUT) pointer to all constant members
 * @param symbol_size		(IN) Size of symbols
 * @param col_idx		(IN) column index
 * @return			1 if it's OK, or 0 if no permutations took place.
 */
INT32	of_linear_binary_code_col_forward_permutation (of_mod2dense *m,
						    void **check_values,
						    UINT32 symbol_size,
						    INT32 col_idx);
/**
 * This function prepare the dense system by making permutations on columns.
 *
 * @brief			prepare the dense system
 * @param m 			(IN/OUT) address of the dense matrix.
 * @param check_values		(IN/OUT) pointer to all constant members
 * @param symbol_size		(IN) Size of symbols
 * @return			1 if it's OK, or 0 if no permutations took place.
 */
INT32	of_linear_binary_code_preprocess_dense_system (of_mod2dense *m,
				    void **check_values,
				    UINT32 symbol_size);


#endif //ML_DECODING				   
#endif //OF_USE_LINEAR_BINARY_CODES_UTILS
#endif //OF_USE_DECODER

#endif //OF_ML_TOOL
