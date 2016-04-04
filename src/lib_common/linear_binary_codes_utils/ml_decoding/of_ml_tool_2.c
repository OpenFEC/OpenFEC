/* $Id: of_ml_tool_2.c 72 2012-04-13 13:27:26Z detchart $ */
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

#include "of_ml_tool_2.h"

#ifdef OF_USE_DECODER
#ifdef OF_USE_LINEAR_BINARY_CODES_UTILS
#ifdef ML_DECODING
#include <string.h>

#ifdef OF_DEBUG
#define NB_OP_ARGS ,&(ofcb->stats_xor->nb_xor_for_ML)
#else
#define NB_OP_ARGS
#endif

of_status_t of_linear_binary_code_copy_simplified_linear_system (of_linear_binary_code_cb_t* ofcb)
{
	OF_ENTER_FUNCTION
	INT32 _col, _row;
	ofcb->remain_cols = ofcb->remain_rows = 0;
	if (ofcb->index_rows == NULL)
	{
		ofcb->index_rows = (UINT32*) of_calloc (ofcb->nb_repair_symbols, sizeof (UINT32) MEM_STATS_ARG);
	}
	if (ofcb->index_cols == NULL)
	{
		ofcb->index_cols = (UINT32*) of_calloc (ofcb->nb_total_symbols, sizeof (UINT32) MEM_STATS_ARG);
	}
	for (_col = 0;_col < ofcb->nb_total_symbols;_col++)
	{
		ofcb->index_cols[ofcb->remain_cols++] = _col;
	}
	for (_row = 0;_row < ofcb->nb_repair_symbols;_row++)
	{
		ofcb->index_rows[ofcb->remain_rows++] = _row;
	}
	ofcb->pchk_matrix_simplified = of_mod2sparse_allocate (ofcb->remain_rows, ofcb->remain_cols,ofcb->stats);
	of_mod2sparse_copyrows_opt (ofcb->pchk_matrix, ofcb->pchk_matrix_simplified, ofcb->index_rows, NULL,ofcb->stats);
	// Update the array containing the system info
	for (_row = 0 ; _row < ofcb->nb_repair_symbols ; _row++)
	{
		of_mod2entry * e;
		ofcb->tab_nb_enc_symbols_per_equ[_row] = 0;
		ofcb->tab_nb_unknown_symbols[_row] = 0;
		for (e = of_mod2sparse_first_in_row (ofcb->pchk_matrix_simplified, _row);
				!of_mod2sparse_at_end (e);
				e = of_mod2sparse_next_in_row (e))
		{
			ofcb->tab_nb_enc_symbols_per_equ[_row]++;
			ofcb->tab_nb_unknown_symbols[_row]++;
		}
	}
	OF_TRACE_LVL (1, ("%s : ok\n",__FUNCTION__))
	OF_EXIT_FUNCTION
	return OF_STATUS_OK;
}

of_status_t of_linear_binary_code_create_simplified_linear_system (of_linear_binary_code_cb_t* ofcb)
{
	OF_ENTER_FUNCTION
	INT32 _col, _row;
	of_mod2sparse	*__pchk_matrix_simplified_cols;
	ofcb->remain_cols = ofcb->remain_rows = 0;
	if (ofcb->index_rows == NULL)
	{
		ofcb->index_rows = (UINT32 *) of_calloc (ofcb->nb_repair_symbols, sizeof (UINT32) MEM_STATS_ARG);
	}
	
	UINT32 *index_rows = (UINT32*) of_malloc(ofcb->nb_repair_symbols * sizeof(UINT32) MEM_STATS_ARG);
	UINT32 *index_cols = (UINT32*) of_malloc(ofcb->nb_total_symbols  * sizeof(UINT32) MEM_STATS_ARG);
	
	//UINT32 index_rows[ofcb->nb_repair_symbols];
	//UINT32 index_cols[ofcb->nb_total_symbols];
	
	if (ofcb->index_cols == NULL)
	{
		ofcb->index_cols =
		(UINT32 *) of_calloc (ofcb->nb_total_symbols - ofcb->nb_repair_symbol_ready - ofcb->nb_source_symbol_ready, sizeof (UINT32) MEM_STATS_ARG);
	}
	// Get the index of cols to copy from the original matrix
	for (_col = 0 ; _col < ofcb->nb_total_symbols ; _col++)
	{
		if (!of_mod2sparse_empty_col (ofcb->pchk_matrix_simplified, _col))
		{
			index_cols[_col] = ofcb->remain_cols;
			//printf("ofcb->index_cols[%d]= %d\n",ofcb->remain_cols,__col);
			ofcb->index_cols[ofcb->remain_cols++] = _col;
		}
	}
	// Get the index of rows to copy from the original matrix
	for (_row = 0 ; _row < ofcb->nb_repair_symbols ; _row++)
	{
		if (!of_mod2sparse_empty_row (ofcb->pchk_matrix_simplified, _row))
		{
			index_rows[_row] = ofcb->remain_rows;
			ofcb->index_rows[ofcb->remain_rows++] = _row;
		}
	}
	// If the initial matrix is completely simplified
	if (ofcb->remain_cols == 0)
	{
		of_free (ofcb->index_rows MEM_STATS_ARG);
		ofcb->index_rows = NULL;
		of_free (ofcb->index_cols MEM_STATS_ARG);
		ofcb->index_cols = NULL;
		OF_PRINT_LVL (1, ("Failure: ofcb->remain_cols==0\n"))
		return OF_STATUS_FAILURE;
	}
	else
	{
		if (ofcb->remain_rows < ofcb->remain_cols)
		{
			// But there is not enough rows compared to cols
			OF_PRINT_LVL (1, ("Failure: ofcb->remain_rows (%d) < ofcb->remain_cols (%d)\n",
					  ofcb->remain_rows, ofcb->remain_cols))
			return OF_STATUS_FAILURE;
		}
	}
	OF_TRACE_LVL (1, ("%dx%d; already decoded symbols: src=%d, parity=%d\n",
			  ofcb->remain_rows,
			  ofcb->remain_cols,
			  ofcb->nb_source_symbol_ready,
			  ofcb->nb_repair_symbol_ready))
	__pchk_matrix_simplified_cols = of_mod2sparse_allocate (ofcb->nb_repair_symbols, ofcb->remain_cols,ofcb->stats);
	OF_TRACE_LVL (1, ("Simplified Matrix: %dx%d; already decoded symbols: src=%d, parity=%d  nb_row=%d  nb_col=%d\n",
			  ofcb->remain_rows,
			  ofcb->remain_cols,
			  ofcb->nb_source_symbol_ready,
			  ofcb->nb_repair_symbol_ready,
			  ofcb->remain_rows,
			  ofcb->remain_cols))
	/*of_mod2sparse_copycols_opt (ofcb->pchk_matrix_simplified,
				    __pchk_matrix_simplified_cols,
				    ofcb->index_cols,ofcb->stats);
	of_mod2sparse_free (ofcb->pchk_matrix_simplified,ofcb->stats);*/
	
	of_mod2sparse_copy_filled_matrix(ofcb->pchk_matrix_simplified,__pchk_matrix_simplified_cols,index_rows,index_cols,ofcb->stats);
	//mod2sparse_copy_filled_rows_and_cols(ofcb->pchk_matrix_simplified,__pchk_matrix_simplified_cols,ofcb->stats);
	of_free (ofcb->pchk_matrix_simplified MEM_STATS_ARG);	/* of_mod2sparse_free does not free it! */
	ofcb->pchk_matrix_simplified = __pchk_matrix_simplified_cols;
	//of_mod2sparse_free(__pchk_matrix_simplified_cols, ofcb->stats);
	__pchk_matrix_simplified_cols = NULL;
	/*ofcb->pchk_matrix_simplified = NULL;
	ofcb->pchk_matrix_simplified = of_mod2sparse_allocate (ofcb->remain_rows, ofcb->remain_cols,ofcb->stats);
	of_mod2sparse_copyrows_opt (__pchk_matrix_simplified_cols,
				    ofcb->pchk_matrix_simplified,
				    ofcb->index_rows, NULL,ofcb->stats);

	of_mod2sparse_free(__pchk_matrix_simplified_cols, ofcb->stats);
	of_free(__pchk_matrix_simplified_cols MEM_STATS_ARG);
	__pchk_matrix_simplified_cols = NULL;*/
	
	of_free(index_rows MEM_STATS_ARG);
	of_free(index_cols MEM_STATS_ARG);
	
#if !defined(ML_DECODING)
	of_mod2sparse_free (ofcb->pchk_matrix,ofcb->stats);
	of_free (ofcb->pchk_matrix MEM_STATS_ARG);	/* of_mod2sparse_free does not free it! */
	ofcb->pchk_matrix = NULL;
	if (ofcb->tab_nb_unknown_symbols != NULL)
	{
		of_free (ofcb->tab_nb_unknown_symbols MEM_STATS_ARG);
		ofcb->tab_nb_unknown_symbols = NULL;
	}
	of_free (ofcb->tab_nb_enc_symbols_per_equ MEM_STATS_ARG);
	ofcb->tab_nb_enc_symbols_per_equ = NULL;
	of_free (ofcb->tab_nb_equ_for_repair MEM_STATS_ARG);
	ofcb->tab_nb_equ_for_repair = NULL;
#endif
	OF_TRACE_LVL (1, ("ok\n"))
	OF_EXIT_FUNCTION
	return OF_STATUS_OK;
}

of_status_t of_linear_binary_code_apply_gauss_pivoting (of_linear_binary_code_cb_t* ofcb,
				     void * encoding_symbols_tab[])
{
	OF_ENTER_FUNCTION
	INT32 __current_row, __col;//,count;
	of_mod2entry * __pivot = NULL;
	of_mod2sparse * __swapMatrix = of_mod2sparse_allocate (1, ofcb->remain_cols,ofcb->stats);
	of_mod2entry ** __links = (of_mod2entry **) of_calloc (ofcb->remain_cols, sizeof (of_mod2entry *) MEM_STATS_ARG);
	of_mod2entry ** __parsing = (of_mod2entry **) of_calloc (ofcb->remain_cols, sizeof (of_mod2entry *) MEM_STATS_ARG);
	__current_row = 0;
	for (__col = 0 ; __col < ofcb->remain_cols ; __col++)
	{
		of_mod2entry * __e;
		// Find the first line with the higher coefficient on the diagonal,
		// and swap it with the current line indexed by "__col".
		if (!of_mod2sparse_empty_row (ofcb->pchk_matrix_simplified, __current_row))
		{
			__e = of_mod2sparse_first_in_row (ofcb->pchk_matrix_simplified, __current_row);
			if (of_mod2sparse_col (__e) > __col)
			{
				if (__links != NULL)
				{
					if (__links[__col] != NULL)
					{
						__e = of_mod2sparse_next_in_col (__links[__col]);
					}
					else
					{
						__e = of_mod2sparse_first_in_col (ofcb->pchk_matrix_simplified,
										  __col);
					}
				}
				else
				{
					__e = of_mod2sparse_first_in_col (ofcb->pchk_matrix_simplified,
									  __col);
				}
				// Search for the first line which appears to be valid
				while (!of_mod2sparse_at_end_col (__e) && of_mod2sparse_row (__e) <= __current_row)
				{
					__e = of_mod2sparse_next_in_col (__e);
				}
			}
			else
				if (of_mod2sparse_col (__e) < __col)
				{
					OF_PRINT_ERROR(("[++-] Error in Gauss pivoting\n"))
					return OF_STATUS_FAILURE;
				}
		}
		else
		{
			if (__links != NULL)
			{
				if (__links[__col] != NULL)
				{
					__e = of_mod2sparse_next_in_col (__links[__col]);
				}
				else
				{
					__e = of_mod2sparse_first_in_col (ofcb->pchk_matrix_simplified, __col);
				}
			}
			else
			{
				__e = of_mod2sparse_first_in_col (ofcb->pchk_matrix_simplified, __col);
			}
			while (!of_mod2sparse_at_end_col (__e) && of_mod2sparse_row (__e) <= __current_row)
			{
				__e = of_mod2sparse_next_in_col (__e);
			}
			of_free (ofcb->tab_const_term_of_equ[ofcb->index_rows[__current_row]] MEM_STATS_ARG);
		}
		if (!of_mod2sparse_at_end_col (__e))
		{
			// Swap indexes for partial sum also
			int __row;
			int __idx;
			__row = of_mod2sparse_row (__e);
			// Found the next line to swap with
			if (__row > __current_row)
			{
				ofcb->tab_nb_unknown_symbols[__current_row] =
					of_mod2sparse_swap_rows (
						ofcb->pchk_matrix_simplified,
						__current_row,
						__row,
						__swapMatrix,
						__links,
						__parsing,
						ofcb->stats
					);
				// Swap index of checkValues
				__idx  				 = ofcb->index_rows[__current_row];
				ofcb->index_rows[__current_row]  = ofcb->index_rows[__row];
				ofcb->index_rows[__row]          =  __idx;
				__pivot = of_mod2sparse_first_in_row (ofcb->pchk_matrix_simplified,
								      __current_row);
			}
			else
				if (__row == __current_row)
				{
					ofcb->tab_nb_unknown_symbols[__current_row] =
						of_mod2sparse_swap_rows (
							ofcb->pchk_matrix_simplified,
							__current_row,
							__row,
							__swapMatrix,
							__links,
							__parsing,
							ofcb->stats
						);
					__pivot = __e;
				}
				else
				{
					__pivot = NULL;
					__e = & (ofcb->pchk_matrix_simplified->cols[__col]);
				}
			if (__pivot != NULL)
			{
				// Position __e on the expected line.
				__e = of_mod2sparse_next_in_col (__pivot);
			}
			// While there is line with the same higher coefficient, xor them with the
			// swapped line, now indexed by __col.
			//ofcb->nb_tmp_symbols=0;
			while (!of_mod2sparse_at_end_col (__e))
			{
				__row = of_mod2sparse_row (__e);
				ofcb->tab_nb_unknown_symbols[__row] =
					of_mod2sparse_xor_rows (
						ofcb->pchk_matrix_simplified,
						__current_row,
						__row,
						__links,
						__parsing,
						ofcb->stats
					);
				if (ofcb->tab_const_term_of_equ[ofcb->index_rows[__row]] != NULL
						&& ofcb->tab_const_term_of_equ[ofcb->index_rows[__current_row]] != NULL)
				{
					//ofcb->tmp_tab_symbols[ofcb->nb_tmp_symbols++] = (ofcb->tab_const_term_of_equ[ofcb->index_rows[__row]]);
#if 1
					of_add_to_symbol
					(
						(ofcb->tab_const_term_of_equ[ofcb->index_rows[__row]]),
						(ofcb->tab_const_term_of_equ[ofcb->index_rows[__current_row]])
						, ofcb->encoding_symbol_length NB_OP_ARGS);
#endif
				}
				else
					if (ofcb->tab_const_term_of_equ[ofcb->index_rows[__row]] == NULL
							&& ofcb->tab_const_term_of_equ[ofcb->index_rows[__current_row]] != NULL)
					{
						ofcb->tab_const_term_of_equ[ofcb->index_rows[__row]] = of_malloc (
									ofcb->encoding_symbol_length MEM_STATS_ARG);
						memcpy (
							(ofcb->tab_const_term_of_equ[ofcb->index_rows[__row]]),
							(ofcb->tab_const_term_of_equ[ofcb->index_rows[__current_row]]),
							ofcb->encoding_symbol_length
						);
					}
				__e = of_mod2sparse_next_in_col (__pivot);
			}
#if 0
			if (ofcb->nb_tmp_symbols!=0)
#ifdef OF_DEBUG
				of_add_to_multiple_symbols(ofcb->tmp_tab_symbols,(ofcb->tab_const_term_of_equ[ofcb->index_rows[__current_row]]),ofcb->nb_tmp_symbols,
										   ofcb->encoding_symbol_length,
										   &(ofcb->stats_xor->nb_xor_for_ML));
#else
			of_add_to_multiple_symbols(ofcb->tmp_tab_symbols,(ofcb->tab_const_term_of_equ[ofcb->index_rows[__current_row]]),ofcb->nb_tmp_symbols,
									   ofcb->encoding_symbol_length);
#endif
#endif
		}
		if (!of_mod2sparse_empty_row (ofcb->pchk_matrix_simplified, __current_row)
				&& of_mod2sparse_col (__pivot) == __col)
		{
			++__current_row;
		}
		else
			if (of_mod2sparse_empty_row (ofcb->pchk_matrix_simplified, __current_row))
			{
				of_free (ofcb->tab_const_term_of_equ[ofcb->index_rows[__current_row]] MEM_STATS_ARG);
				ofcb->tab_const_term_of_equ[ofcb->index_rows[__current_row]] = NULL;
			}
	}
	
	//
	// Free spaces not used anymore
	//
	of_mod2sparse_free (__swapMatrix,ofcb->stats);
	of_free (__swapMatrix MEM_STATS_ARG);
	__swapMatrix = NULL;
	of_free (__links MEM_STATS_ARG);
	__links = NULL;
	of_free (__parsing MEM_STATS_ARG);
	__parsing = NULL;
	OF_TRACE_LVL (1, ("%s : ok\n",__FUNCTION__))
	OF_EXIT_FUNCTION
	return OF_STATUS_OK;
}

of_status_t of_linear_binary_code_inject_symbol_in_triangular_system (of_linear_binary_code_cb_t* ofcb,
						   void * encoding_symbol_tab[],
						   void * new_symbol,
						   INT32 simplified_col)
{
	OF_ENTER_FUNCTION
	of_mod2entry * __e;
	of_mod2entry * __delMe;
	of_mod2entry * __pivot;
	INT32 new_symbol_esi;
	INT32 __current_row;
	OF_TRACE_LVL (1, ("-> inject_symbol_in_triangular_system:\n"))
	// Is it a known symbol that has een injected or not...
	if (of_mod2sparse_empty_col (ofcb->pchk_matrix_simplified, simplified_col))
	{
		OF_TRACE_LVL (1, ("<- inject_symbol_in_triangular_system: nothing to do, col empty\n"))
		return OF_STATUS_OK;
	}
	new_symbol_esi = of_get_symbol_esi ((of_cb_t*)ofcb, ofcb->index_cols[simplified_col]);
	if (of_is_source_symbol ((of_cb_t*)ofcb, new_symbol_esi) && of_is_decoding_complete ((of_session_t*)ofcb))
	{
		// Decoding is now finished, return...
		OF_TRACE_LVL (1, ("inject_symbol_in_triangular_system: decoding is finished!\n"))
		return OF_STATUS_OK;
	}
	// Inject the decoded/received symbol in the simplified matrix
	__e = of_mod2sparse_first_in_col (ofcb->pchk_matrix_simplified, simplified_col);
	while (!of_mod2sparse_at_end_col (__e))
	{
		__current_row = of_mod2sparse_row (__e);
		// Verify if the partial sum has already been allocated, and either add or copy the data
		// to the partial sum
		if (ofcb->tab_const_term_of_equ[ofcb->index_rows[__current_row]] == NULL)
		{
			ofcb->tab_const_term_of_equ[ofcb->index_rows[__current_row]] =
				of_calloc (1, ofcb->encoding_symbol_length MEM_STATS_ARG);
			// Copy data now
			memcpy (
				(ofcb->tab_const_term_of_equ[ofcb->index_rows[__current_row]]),
				(new_symbol),
				ofcb->encoding_symbol_length);
		}
		else
		{
			of_add_to_symbol (
				(ofcb->tab_const_term_of_equ[ofcb->index_rows[__current_row]]),
				(new_symbol)
				, ofcb->encoding_symbol_length NB_OP_ARGS);
		}
		ofcb->tab_nb_unknown_symbols[__current_row]--;
		// Delete the current element
		__delMe = __e;
		__e = of_mod2sparse_next_in_col (__e);
		of_mod2sparse_delete (ofcb->pchk_matrix_simplified, __delMe);
		// Only one 1 in the line and on the line, we can identify the symbol
		if (ofcb->tab_nb_unknown_symbols[__current_row] == 1)
		{
			int tmp_seq_no;
			int __pivot_col;
			// The pivot is the first element in the row
			__pivot = of_mod2sparse_first_in_row (ofcb->pchk_matrix_simplified, __current_row);
			__pivot_col = of_mod2sparse_col (__pivot);
			tmp_seq_no = of_get_symbol_esi ((of_cb_t*)ofcb,
							ofcb->index_cols[of_mod2sparse_col (__pivot) ]);
			if (of_is_source_symbol ((of_cb_t*)ofcb, tmp_seq_no))
			{
				if (encoding_symbol_tab[tmp_seq_no] == NULL)
				{
					encoding_symbol_tab[tmp_seq_no] =
							of_malloc (ofcb->encoding_symbol_length MEM_STATS_ARG);
					memcpy (
						(encoding_symbol_tab[tmp_seq_no]),
						(ofcb->tab_const_term_of_equ[ofcb->index_rows[__current_row]]),
						ofcb->encoding_symbol_length
					);
					of_free (ofcb->tab_const_term_of_equ[ofcb->index_rows[__current_row]] MEM_STATS_ARG);
					ofcb->nb_source_symbol_ready++;
					ofcb->tab_nb_unknown_symbols[__current_row]--;
					of_mod2sparse_delete (ofcb->pchk_matrix_simplified, __pivot);
					of_linear_binary_code_inject_symbol_in_triangular_system (ofcb,
									    encoding_symbol_tab,
									    encoding_symbol_tab[tmp_seq_no],
									    __pivot_col
									   );
				}
			}
			else
			{
				if (ofcb->nb_source_symbol_ready < ofcb->nb_source_symbols)
				{
					if (ofcb->encoding_symbols_tab[tmp_seq_no] == NULL)
					{
						ofcb->encoding_symbols_tab[tmp_seq_no] = of_malloc ( ofcb->encoding_symbol_length MEM_STATS_ARG);
						memcpy (
							(ofcb->encoding_symbols_tab[tmp_seq_no]),
							(ofcb->tab_const_term_of_equ[ofcb->index_rows[__current_row]]),
							ofcb->encoding_symbol_length
						);
						of_free (ofcb->tab_const_term_of_equ[ofcb->index_rows[__current_row]] MEM_STATS_ARG);
						ofcb->nb_repair_symbol_ready++;
						ofcb->tab_nb_unknown_symbols[__current_row]--;
						of_mod2sparse_delete (ofcb->pchk_matrix_simplified, __pivot);
						// If it is needed to reinject the symbol...
						of_linear_binary_code_inject_symbol_in_triangular_system (ofcb,
										    encoding_symbol_tab,
										    ofcb->encoding_symbols_tab[tmp_seq_no],
										    __pivot_col
										   );
					}
				}
			}
		}
	}
	OF_TRACE_LVL (1, ("<- inject_symbol_in_triangular_system: ok\n"))
	OF_EXIT_FUNCTION
	return OF_STATUS_OK;
}

of_status_t of_linear_binary_code_solve_triangular_system (of_linear_binary_code_cb_t* ofcb,
					void * encoding_symbol_tab[])
{
	OF_ENTER_FUNCTION
	INT32 __current_row;
	INT32 new_symbol_esi;
	of_mod2entry * __pivot;
	of_mod2entry * __delMe;
	of_mod2entry * __e;
	INT32 __col;
	for (__col = ofcb->remain_cols - 1 ; __col >= 0 ; __col--)
	{
		new_symbol_esi = of_get_symbol_esi ((of_cb_t*)ofcb, ofcb->index_cols[__col]);
		__current_row = __col;
		if (!of_mod2sparse_empty_row (ofcb->pchk_matrix_simplified, __current_row))
		{
			// Verify if the parital sum has already been allocated
			if (ofcb->tab_const_term_of_equ[ofcb->index_rows[__current_row]] == NULL)
			{
				ofcb->tab_const_term_of_equ[ofcb->index_rows[__current_row]] =
					of_calloc (1, ofcb->encoding_symbol_length MEM_STATS_ARG);
			}
			__pivot = of_mod2sparse_first_in_row (ofcb->pchk_matrix_simplified, __current_row);
			if (of_mod2sparse_col (__pivot) == __col)
			{
				new_symbol_esi = of_get_symbol_esi ((of_cb_t*)ofcb, ofcb->index_cols[__current_row]);
			}
			else
			{
				if (of_mod2sparse_col (__pivot) > __col)
				{
					new_symbol_esi = of_get_symbol_esi ((of_cb_t*)ofcb, ofcb->index_cols[of_mod2sparse_col (__pivot) ]);
				}
			}
			__e = __pivot;
			__e = of_mod2sparse_next_in_row (__e);
			// Inject all other term of the equation to solve
			while (!of_mod2sparse_at_end (__e))
			{
				INT32  tmp_symbol_esi;
				tmp_symbol_esi = of_get_symbol_esi ((of_cb_t*)ofcb, ofcb->index_cols[of_mod2sparse_col (__e) ]);
				if (of_is_source_symbol ((of_cb_t*)ofcb, tmp_symbol_esi))
				{
					if (ofcb->encoding_symbols_tab[tmp_symbol_esi] != NULL)
					{
						of_add_to_symbol((ofcb->tab_const_term_of_equ[ofcb->index_rows[__current_row]]),
								(ofcb->tab_const_term_of_equ[tmp_symbol_esi]), ofcb->encoding_symbol_length NB_OP_ARGS);
						ofcb->tab_nb_unknown_symbols[__current_row]--;
						__delMe = __e;
						__e = of_mod2sparse_next_in_row (__e);
						of_mod2sparse_delete (ofcb->pchk_matrix_simplified, __delMe);
					}
					else
					{
						__e = of_mod2sparse_next_in_row (__e);
					}
				}
				else
				{
					if (ofcb->nb_source_symbol_ready < ofcb->nb_source_symbols)
					{
						if (ofcb->encoding_symbols_tab[tmp_symbol_esi] != NULL)
						{
							of_add_to_symbol (
								(ofcb->tab_const_term_of_equ[ofcb->index_rows[__current_row]]),
								(ofcb->tab_const_term_of_equ[tmp_symbol_esi])
								, ofcb->encoding_symbol_length NB_OP_ARGS);
							ofcb->tab_nb_unknown_symbols[__current_row]--;
							__delMe = __e;
							__e = of_mod2sparse_next_in_row (__e);
							of_mod2sparse_delete (ofcb->pchk_matrix_simplified, __delMe);
						}
						else
						{
							__e = of_mod2sparse_next_in_row (__e);
						}
					}
				}
			}
			if (ofcb->tab_nb_unknown_symbols[__current_row] == 1)
			{
				INT32 __current_col;
				bool __inject;
				__pivot = of_mod2sparse_first_in_row (ofcb->pchk_matrix_simplified, __current_row);
				__current_col = of_mod2sparse_col (__pivot);
				new_symbol_esi = of_get_symbol_esi ((of_cb_t*)ofcb, ofcb->index_cols[of_mod2sparse_col (__pivot) ]);
				if (of_is_source_symbol ((of_cb_t*)ofcb, new_symbol_esi))
				{
					if (ofcb->encoding_symbols_tab[new_symbol_esi] == NULL)
					{
						ofcb->encoding_symbols_tab[new_symbol_esi] = of_calloc (1, ofcb->encoding_symbol_length MEM_STATS_ARG);
						memcpy((ofcb->encoding_symbols_tab[new_symbol_esi]),
							 (ofcb->tab_const_term_of_equ[ofcb->index_rows[__current_row]]),
							 ofcb->encoding_symbol_length);
						of_free (ofcb->tab_const_term_of_equ[ofcb->index_rows[__current_row]] MEM_STATS_ARG);
						++ofcb->nb_source_symbol_ready;
						ofcb->tab_nb_unknown_symbols[__current_row]--;
						__inject =
							(!of_mod2sparse_at_end_col (of_mod2sparse_next_in_col (__pivot)))
							? true : false;
						of_mod2sparse_delete (ofcb->pchk_matrix_simplified, __pivot);
						// Inject only there exists 1 in the same col under my rows
						if (__inject == true)
						{
							of_linear_binary_code_inject_symbol_in_triangular_system (
								ofcb,
								encoding_symbol_tab,
								ofcb->encoding_symbols_tab[new_symbol_esi],
								__current_col);
						}
					}
				}
				else
				{
					if (ofcb->nb_source_symbol_ready < ofcb->nb_source_symbols)
					{
						if (ofcb->encoding_symbols_tab[new_symbol_esi] == NULL)
						{
							ofcb->encoding_symbols_tab[new_symbol_esi] = of_calloc (1, ofcb->encoding_symbol_length MEM_STATS_ARG);
							memcpy((ofcb->encoding_symbols_tab[new_symbol_esi]),
								 (ofcb->tab_const_term_of_equ[ofcb->index_rows[__current_row]]),
								 ofcb->encoding_symbol_length);
							of_free (ofcb->tab_const_term_of_equ[ofcb->index_rows[__current_row]] MEM_STATS_ARG);
							++ofcb->nb_repair_symbol_ready;
							ofcb->tab_nb_unknown_symbols[__current_row]--;
							__inject =
								(!of_mod2sparse_at_end_col (of_mod2sparse_next_in_col (__pivot)))
								? true : false;
							of_mod2sparse_delete (ofcb->pchk_matrix_simplified, __pivot);
							// Inject only there exists 1 in the same col under my rows
							if (__inject == true)
							{
								of_linear_binary_code_inject_symbol_in_triangular_system (ofcb,
												    ofcb->encoding_symbols_tab,
												    ofcb->encoding_symbols_tab[new_symbol_esi],
												    __current_col
												   );
							}
						}
					}
				}
			}
		}
	}
	OF_TRACE_LVL (1, ("ok\n"))
	OF_EXIT_FUNCTION
	return OF_STATUS_OK;
}

of_status_t of_linear_binary_code_launch_ml_decoding (of_linear_binary_code_cb_t* ofcb,
				   void * encoding_symbol_tab[])
{
	OF_ENTER_FUNCTION
	INT32 i;
	if ( (ofcb->nb_source_symbol_ready + ofcb->nb_repair_symbol_ready) < ofcb->nb_source_symbols)
		return OF_STATUS_ERROR;
	if (ofcb->pchk_matrix_simplified == NULL)
	{
		if (of_linear_binary_code_copy_simplified_linear_system (ofcb) == OF_STATUS_ERROR)
		{
			if (ofcb->remain_cols == 0)
			{
				// in fact decoding is already finished!
				OF_TRACE_LVL (1, ("create_simplified_linear_system says it's finished!\n"))
				return OF_STATUS_OK;
			}
			// but here it failed! Stay in step 3...
			OF_TRACE_LVL (1, ("<- DecodingStepWithSymbol: create_simplified_linear_system failed, ofcb->remain_cols=%d\n",
					  ofcb->remain_cols))
			return OF_STATUS_FAILURE;
		}
	}
	for (i = 0 ; i < ofcb->nb_source_symbols ; i++)
	{
		if (encoding_symbol_tab[i] != NULL)
		{
			if (of_linear_binary_code_simplify_linear_system (ofcb,
							    encoding_symbol_tab[i],
							    i) == OF_STATUS_ERROR)
			{
				OF_TRACE_LVL (1, ("simplifying the matrix with source symbols failed\n"))
				return OF_STATUS_FAILURE;
			}
		}
	}
	// Parity symbols
	// Randomize the parity symbols order before injecting them.
	// It makes the decoding process more efficient...
	INT32	*array;
	array = (INT32 *) of_calloc (ofcb->nb_repair_symbols, sizeof (INT32) MEM_STATS_ARG);
	for (i = 0 ; i < ofcb->nb_repair_symbols ; i++)
	{
		array[i] = i;
	}
	for (i = 0 ; i < ofcb->nb_repair_symbols ; i++)
	{
		INT32	backup;
		INT32	randInd;
		backup = array[i];
		randInd = rand() % ofcb->nb_repair_symbols;
		array[i] = array[randInd];
		array[randInd] = backup;
	}
	// Inject parity symbols following the random order given by array
	for (i = 0 ; i < ofcb->nb_repair_symbols ; i++)
	{
		if (ofcb->encoding_symbols_tab[array[i] + ofcb->nb_source_symbols] != NULL)
		{
			if (of_linear_binary_code_simplify_linear_system (ofcb,
							    ofcb->encoding_symbols_tab[array[i] + ofcb->nb_source_symbols],
							    of_get_symbol_esi ((of_cb_t*)ofcb, array[i])) == OF_STATUS_ERROR)
			{
				OF_PRINT_ERROR(("simplifying the matrix with parity symbols failed\n"))
				return OF_STATUS_FAILURE;
			}
		}
	}
	of_free (array MEM_STATS_ARG);
	array = NULL;
	// Solve the system of linear equations:
	//  - simplify matrix
	//  - perform gauss pivoting
	//  - solve gauss system
	of_linear_binary_code_create_simplified_linear_system (ofcb);
#ifdef OF_DEBUG
	if (ofcb->nb_source_symbols < 100)
	{
		OF_TRACE_LVL (1, ("ofcb->pchk_matrix_simplified:\n"))
		of_mod2sparse_printf (stdout, ofcb->pchk_matrix_simplified);
		OF_TRACE_LVL (1, ("\n"))
	}
#endif
	of_mod2dense *dense_pck_matrix_simplified;
	dense_pck_matrix_simplified = of_mod2dense_allocate (ofcb->pchk_matrix_simplified->n_rows,
							     ofcb->pchk_matrix_simplified->n_cols,
							     ofcb->stats);
	of_mod2sparse_to_dense (ofcb->pchk_matrix_simplified,
				dense_pck_matrix_simplified);
	OF_TRACE_LVL (1, ("Matrix m1 triangularizes or inverted \n\n"))
	of_mod2dense_print (stdout, dense_pck_matrix_simplified);
	OF_TRACE_LVL (1, ("\n"))
	fflush (stdout);
	// Gauss Pivoting to obtain a triangular system
	if (of_linear_binary_code_apply_gauss_pivoting (ofcb, encoding_symbol_tab) == OF_STATUS_ERROR)
	{
		OF_TRACE_LVL (1, ("<- DecodingStepWithSymbol: apply_gauss_pivoting failed\n"))
		// Stay in step 3...
		return OF_STATUS_FAILURE;
	}

	
#ifdef OF_DEBUG
	if (ofcb->nb_source_symbols < 100)
	{
		OF_TRACE_LVL (1, ("ofcb->pchk_matrix_simplified:\n"))
		of_mod2sparse_printf (stdout, ofcb->pchk_matrix_simplified);
		OF_TRACE_LVL (1, ("\n"))
	}
#endif
	// Solving the triangular system
	if (of_linear_binary_code_solve_triangular_system (ofcb, encoding_symbol_tab) == OF_STATUS_ERROR)
	{
		OF_TRACE_LVL (1, ("<- DecodingStepWithSymbol: solve_triangular_system failed\n"))
		// Stay in step 3...
		OF_EXIT_FUNCTION
		return OF_STATUS_FAILURE;
	}
	OF_EXIT_FUNCTION
	return OF_STATUS_OK;
}

INT32 of_linear_binary_code_invert_dense_system (of_linear_binary_code_cb_t* ofcb,
				of_mod2dense *m	/* The matrix to find the inverse of (destroyed) */)
{
	OF_ENTER_FUNCTION
	of_mod2word *s, *t;
	INT32 i, j, k, n, p, w, k0, b0;
	void * tmp_buffer;
#ifdef COL_ORIENTED
	n = of_mod2dense_rows (m);
	p = of_mod2dense_cols (m);
	w = m->n_words;
	/* for each line*/
	for (i = 0; i < n; i++)
	{
		k0 = i >> of_mod2_wordsize_shift; // word index of the ith bit
		b0 = i & of_mod2_wordsize_mask;  // bit index of the ith bit in the k0-th word
		/* search for the first non-null element */
		for (j = i; j < p; j++)
		{
			if (of_mod2_getbit (m->col[j][k0], b0))
				break;
		}
		// now j is the index of the first non null element in the line i
		if (j == p)
			return 0;
		if (j != i)
		{
			// swap column i an j the two matrices
			t = m->col[i];
			m->col[i] = m->col[j];
			m->col[j] = t;
		}
		for (j = i; j < p; j++)
		{
			if (j != i && of_mod2_getbit (m->col[j][k0], b0))
			{
				// for all the column were the ith element is non null
				s = m->col[j];
				t = m->col[i];
				for (k = k0; k < w; k++)
					s[k] ^= t[k]; // add column i with coluln j
			}
		}
	}
#else
	n = of_mod2dense_cols (m);
	p = of_mod2dense_rows (m);
	w = m->n_words;
	/* for each line*/
	for (i = 0; i < n; i++)
	{
		k0 = i >> of_mod2_wordsize_shift; // word index of the ith bit
		b0 = i & of_mod2_wordsize_mask;  // bit index of the ith bit in the k0-th word
		/* search for the first non-null element */
		for (j = i; j < p; j++)
		{
			if (of_mod2_getbit (m->row[j][k0], b0))
				break;
		}
		// now j is the index of the first non null element in the line i
		if (j == p)
			return 0;
		if (j != i)
		{
			// swap column i an j the two matrices
			t = m->row[i];
			m->row[i] = m->row[j];
			m->row[j] = t;
			//  swap the partial sum
			tmp_buffer = ofcb->tab_const_term_of_equ[ofcb->index_rows[i]];
			ofcb->tab_const_term_of_equ[ofcb->index_rows[i]] = ofcb->tab_const_term_of_equ[ofcb->index_rows[j]];
			ofcb->tab_const_term_of_equ[ofcb->index_rows[j]] = tmp_buffer;
		}
		for (j = 0; j < p; j++)
			//for (j = i; j<p; j++)
		{
			if (j != i && of_mod2_getbit (m->row[j][k0], b0))
			{
				// for all the column were the ith element is non null
				s = m->row[j];
				t = m->row[i];
				for (k = k0; k < w; k++)
					s[k] ^= t[k]; // add column i with coluln j
				if (ofcb->tab_const_term_of_equ[ofcb->index_rows[i]] != NULL)
				{
					// if the buffer of line i is NULL there is nothing to Add
					// add the ofcb->tab_const_term_of_equ of the i to the m_checkValue of line j
					if (ofcb->tab_const_term_of_equ[ofcb->index_rows[j]] == NULL)
					{
							ofcb->tab_const_term_of_equ[ofcb->index_rows[j]] = of_calloc (1, ofcb->encoding_symbol_length MEM_STATS_ARG);
						// Copy data now
						memcpy (
							(ofcb->tab_const_term_of_equ[ofcb->index_rows[j]]),
							(ofcb->tab_const_term_of_equ[ofcb->index_rows[i]]),
							ofcb->encoding_symbol_length);

					} // if (ofcb->tab_const_term_of_equ[ofcb->index_rows[__current_row]] == NULL)
					else
					{
						of_add_to_symbol (
							(ofcb->tab_const_term_of_equ[ofcb->index_rows[j]]),
							(ofcb->tab_const_term_of_equ[ofcb->index_rows[i]])
							, ofcb->encoding_symbol_length NB_OP_ARGS);
					} // if (ofcb->tab_const_term_of_equ[ofcb->index_rows[__current_row]] == NULL) ... else ...
				}
			}
		}
	}
#endif
	OF_EXIT_FUNCTION
	return 1;
}

INT32 of_linear_binary_code_triangularize_dense_system_with_canvas (of_linear_binary_code_cb_t* ofcb,
						 of_mod2dense *m,
						 void * encoding_symbol_tab[])
{
	OF_ENTER_FUNCTION
	of_mod2word *s, *t;
	INT32 i, j, k, n, p, w, k0, b0;
	void * tmp_buffer;
#ifdef COL_ORIENTED

#else
	n = of_mod2dense_cols (m);
	p = of_mod2dense_rows (m);
	w = m->n_words;
	/* for each line*/
	for (i = 0; i < n; i++)
	{
		k0 = i >> of_mod2_wordsize_shift; // word index of the ith bit
		b0 = i & of_mod2_wordsize_mask;  // bit index of the ith bit in the k0-th word
		/* search for the pivot */
	for (j = i; j < p; j++)
	{
		if (of_mod2_getbit (m->row[j][k0], b0))
			break;
	}
		// now j is the index of the first non null element in the line i
		if (j == p)
			return 0;
		if (j != i)
		{
			// swap column i an j the two matrices
			t = m->row[i];
			m->row[i] = m->row[j];
			m->row[j] = t;
			//  swap the partial sum
			tmp_buffer = encoding_symbol_tab[i]; // ofcb->tab_const_term_of_equ[ofcb->index_rows[i]];
			encoding_symbol_tab[i] = encoding_symbol_tab[j]; // ofcb->tab_const_term_of_equ[ofcb->index_rows[i]]= ofcb->tab_const_term_of_equ[ofcb->index_rows[j]];
			encoding_symbol_tab[j] = tmp_buffer; //ofcb->tab_const_term_of_equ[ofcb->index_rows[j]] = tmp_buffer;
		}
		for (j = 0; j < p; j++)
			//for (j = i; j<p; j++)
		{
			if (j != i && of_mod2_getbit (m->row[j][k0], b0))
			{
				// for all the column were the ith element is non null
				s = m->row[j];
				t = m->row[i];
				for (k = k0; k < w; k++)
					s[k] ^= t[k]; // add column i with coluln j
				if (encoding_symbol_tab[i] /*ofcb->tab_const_term_of_equ[ofcb->index_rows[i]]*/ != NULL)
				{
					// if the buffer of line i is NULL there is nothing to Add
					// add the ofcb->tab_const_term_of_equ of the i to the m_checkValue of line j
					if (encoding_symbol_tab[j] == NULL)
					{
						ofcb->tab_const_term_of_equ[ofcb->index_rows[j]] = of_calloc (1, ofcb->encoding_symbol_length MEM_STATS_ARG);
						// Copy data now
						memcpy (
							(encoding_symbol_tab[j]),  //GetBufferPtrOnly(ofcb->tab_const_term_of_equ[ofcb->index_rows[j]]),
							(encoding_symbol_tab[i]),  //GetBuffer(ofcb->tab_const_term_of_equ[ofcb->index_rows[i]]),
							ofcb->encoding_symbol_length);
					}
					else
					{
						of_add_to_symbol (
							encoding_symbol_tab[j],//GetBufferPtrOnly(ofcb->tab_const_term_of_equ[ofcb->index_rows[j]]),
							encoding_symbol_tab[i]//GetBuffer(ofcb->tab_const_term_of_equ[ofcb->index_rows[i]])
							, ofcb->encoding_symbol_length NB_OP_ARGS);
					}
				}
			}
		}
	}
#endif
	OF_EXIT_FUNCTION
	return 1;
}


INT32 of_linear_binary_code_forward_substitution (of_linear_binary_code_cb_t* ofcb,
				void * encoding_symbol_tab[],
				of_mod2dense* m,
				void ** check_values)
{
	OF_ENTER_FUNCTION
	INT32 i, n_rows, n_cols, k0, b0, symbol_esi;
	n_cols = m->n_cols;
	n_rows = m->n_rows;
	for (i = 0; i < n_cols; i++)
	{
		k0 = i >> of_mod2_wordsize_shift; // word index of the ith bit
		b0 = i & of_mod2_wordsize_mask;  // bit index of the ith bit in the k0-th word
		if (of_mod2_getbit (m->row[i][k0], b0))
		{
			symbol_esi = of_get_symbol_esi ((of_cb_t*)ofcb, ofcb->index_cols[i]);
			if (of_is_source_symbol ((of_cb_t*)ofcb, symbol_esi))
			{
				encoding_symbol_tab[symbol_esi] = of_malloc (ofcb->encoding_symbol_length MEM_STATS_ARG);
				// Copy data now
				memcpy (
					(encoding_symbol_tab[symbol_esi]),
					(check_values[i]),
					ofcb->encoding_symbol_length);
				++ofcb->nb_source_symbol_ready;
			}
		}
		else
		{
			// the matrix is not Identity
			OF_EXIT_FUNCTION
			return 0;
		}
	}
	OF_EXIT_FUNCTION
	return 1;
}

#endif //ML_DECODING
#endif //OF_USE_LINEAR_BINARY_CODES_UTILS
#endif //OF_USE_DECODER
