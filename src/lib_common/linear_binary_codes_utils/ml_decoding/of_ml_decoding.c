/* $Id: of_ml_decoding.c 80 2012-04-13 14:25:30Z roca $ */
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

#include "of_ml_decoding.h"

#ifdef OF_USE_DECODER

#ifdef OF_USE_LINEAR_BINARY_CODES_UTILS

#ifdef ML_DECODING

#include <string.h>

of_status_t	of_linear_binary_code_simplify_linear_system (of_linear_binary_code_cb_t* ofcb,
								const void* new_symbol,
								UINT32 new_symbol_esi)
{
	OF_ENTER_FUNCTION
	of_mod2entry *_e, *_del_me;//entry ("1") in parity check matrix and entry to delete in row/column (temp)
	INT32 _row;
	if (of_mod2sparse_empty_col (ofcb->pchk_matrix_simplified,
				     of_get_symbol_col ((of_cb_t*)ofcb,
				     new_symbol_esi)))
	{
		OF_TRACE_LVL (1, ((" nothing to do, col empty\n")))
		OF_EXIT_FUNCTION
		return OF_STATUS_OK;
	}
	if (of_is_source_symbol ((of_cb_t*)ofcb, new_symbol_esi)    && of_is_decoding_complete ((of_session_t*)ofcb)     )
	{
		// Decoding is now finished, return...
		// PRINT(("simplify_linear_system: decoding is finished!\n"))
		OF_TRACE_LVL (1, ((" decoding is finished!\n")))
		OF_EXIT_FUNCTION
		return OF_STATUS_OK;
	}
	//ofcb->nb_tmp_symbols=0;
	_e = of_mod2sparse_first_in_col (ofcb->pchk_matrix_simplified,
				      of_get_symbol_col ((of_cb_t*)ofcb, new_symbol_esi));
	while (!of_mod2sparse_at_end_col (_e))
	{
		_row = of_mod2sparse_row (_e);
		// If checkValue does not exist, create it
		if (ofcb->tab_const_term_of_equ[_row] == NULL)
		{
			if ((ofcb->tab_const_term_of_equ[_row] =
					of_malloc (ofcb->encoding_symbol_length MEM_STATS_ARG)) == NULL)
			{
				goto no_mem;
			}
			memcpy(ofcb->tab_const_term_of_equ[_row], new_symbol, ofcb->encoding_symbol_length);
		}
		else
		{
			// Inject the symbol in the partial sum
			//ofcb->tmp_tab_symbols[ofcb->nb_tmp_symbols++] = (ofcb->tab_const_term_of_equ[_row]);
#if 1
#ifdef OF_DEBUG
			of_add_to_symbol((ofcb->tab_const_term_of_equ[_row]), new_symbol, ofcb->encoding_symbol_length,
						&(ofcb->stats_xor->nb_xor_for_ML));
#else
			of_add_to_symbol((ofcb->tab_const_term_of_equ[_row]), new_symbol, ofcb->encoding_symbol_length);
#endif
#endif
		}
		// Decrease the number of unknown symbols in the equation
		ofcb->tab_nb_unknown_symbols[_row]--;	// symbol is known
		// Delete the current element
		_del_me = _e;
		_e = of_mod2sparse_next_in_col (_e);
		of_mod2sparse_delete (ofcb->pchk_matrix_simplified, _del_me);
		// If there is only one more symbol in the line, reinject it and progress like that
		if (ofcb->tab_nb_unknown_symbols[_row] == 1)
		{
			of_mod2entry * __r;
			UINT32 decoded_symbol_seqno;
			// Get the only one symbol in the equation
			__r = of_mod2sparse_first_in_row (ofcb->pchk_matrix_simplified, _row);
			decoded_symbol_seqno = of_get_symbol_esi ((of_cb_t*)ofcb, of_mod2sparse_col (__r));
			// Identify the symbol with its value, and reinject it.
			if (of_is_source_symbol ((of_cb_t*)ofcb, decoded_symbol_seqno))
			{
				if (ofcb->encoding_symbols_tab[decoded_symbol_seqno] == NULL)
				{
					if (ofcb->decoded_source_symbol_callback != NULL)
					{
						ofcb->encoding_symbols_tab[decoded_symbol_seqno] =
							ofcb->decoded_source_symbol_callback(ofcb->context_4_callback,
											ofcb->encoding_symbol_length,
											decoded_symbol_seqno);
					}
					else
					{
						ofcb->encoding_symbols_tab[decoded_symbol_seqno] =
							of_malloc (ofcb->encoding_symbol_length MEM_STATS_ARG);
					}
					if (ofcb->encoding_symbols_tab[decoded_symbol_seqno] == NULL)
					{
						goto no_mem;
					}
					// Source symbol.
					memcpy((ofcb->encoding_symbols_tab[decoded_symbol_seqno]),
						 (ofcb->tab_const_term_of_equ[_row]),
						 ofcb->encoding_symbol_length);
					of_free (ofcb->tab_const_term_of_equ[_row] MEM_STATS_ARG);
					ofcb->tab_const_term_of_equ[_row]=NULL;
					// It'll be known at the end of this step
					ofcb->tab_nb_unknown_symbols[_row]--;	// symbol is known
					of_mod2sparse_delete (ofcb->pchk_matrix_simplified, __r);
					of_linear_binary_code_simplify_linear_system (ofcb,
										ofcb->encoding_symbols_tab[decoded_symbol_seqno],
										decoded_symbol_seqno);
					ofcb->nb_source_symbol_ready++;
				}
			}
			else
			{
				// Decoded symbol is a parity symbol
				if (ofcb->encoding_symbols_tab[decoded_symbol_seqno] == NULL)
				{
					if (ofcb->decoded_repair_symbol_callback != NULL)
					{
						ofcb->encoding_symbols_tab[decoded_symbol_seqno] =
							ofcb->decoded_repair_symbol_callback(ofcb->context_4_callback,
											ofcb->encoding_symbol_length,
											decoded_symbol_seqno);
					}
					else
					{
						ofcb->encoding_symbols_tab[decoded_symbol_seqno] =
								of_malloc (ofcb->encoding_symbol_length MEM_STATS_ARG);
					}
					if (ofcb->encoding_symbols_tab[decoded_symbol_seqno] == NULL)
					{
						goto no_mem;
					}
					// copy the content...
					memcpy(ofcb->encoding_symbols_tab[decoded_symbol_seqno],
						ofcb->tab_const_term_of_equ[_row],
						ofcb->encoding_symbol_length);
					of_free (ofcb->tab_const_term_of_equ[_row] MEM_STATS_ARG);
					ofcb->tab_const_term_of_equ[_row]=NULL;
					// It'll be known at the end of this step
					ofcb->tab_nb_unknown_symbols[_row]--;	// symbol is known
					ofcb->tab_nb_equ_for_repair[decoded_symbol_seqno - ofcb->nb_source_symbols]--;
					of_mod2sparse_delete (ofcb->pchk_matrix_simplified, __r);
					of_linear_binary_code_simplify_linear_system (
						ofcb,
						ofcb->encoding_symbols_tab[decoded_symbol_seqno],
						decoded_symbol_seqno
					);
					++ (ofcb->nb_repair_symbol_ready);
				}
			}
			ofcb->remain_rows--;
		}
	}
#if 0
	if (ofcb->nb_tmp_symbols!=0)
#ifdef OF_DEBUG
		of_add_to_multiple_symbols(ofcb->tmp_tab_symbols,new_symbol,ofcb->nb_tmp_symbols,
									 ofcb->encoding_symbol_length,
									 &(ofcb->stats_xor->nb_xor_for_ML));
#else
	of_add_to_multiple_symbols(ofcb->tmp_tab_symbols,new_symbol,ofcb->nb_tmp_symbols,
								 ofcb->encoding_symbol_length);
#endif
#endif
	ofcb->remain_cols--;
	OF_EXIT_FUNCTION
	return OF_STATUS_OK;

no_mem:
	OF_PRINT_ERROR(("out of memory"))
	OF_EXIT_FUNCTION
	return OF_STATUS_FATAL_ERROR;
}


of_status_t of_linear_binary_code_finish_decoding_with_ml (of_linear_binary_code_cb_t *ofcb)
{
	OF_ENTER_FUNCTION
	return of_linear_binary_code_finish_with_ml_decoding_on_parity_check_matrix (ofcb);
}


of_status_t of_linear_binary_code_finish_with_ml_decoding_on_parity_check_matrix (of_linear_binary_code_cb_t *ofcb)
{
	OF_ENTER_FUNCTION
	INT32	i;
	OF_TRACE_LVL (1, ("ML decoding on Parity Check Matrix\n"))
	// Matrix simplification: Inject symbols in equations
	// and store the value of the partial sum each time.
	//
	// Source Symbols
	//
	ofcb->remain_rows = ofcb->nb_repair_symbols;
	ofcb->remain_cols = ofcb->nb_source_symbols + ofcb->nb_repair_symbols;
	// Create the simplified matrix if it does not already exist
	if (ofcb->pchk_matrix_simplified == NULL)
	{
		if (of_linear_binary_code_copy_simplified_linear_system (ofcb) != OF_STATUS_OK)
		{
			if (ofcb->remain_cols == 0)
			{
				// in fact decoding is already finished!
				OF_TRACE_LVL (1, ("CreateSimplifiedLinearSystem says it's finished!\n"))
				OF_EXIT_FUNCTION
				return OF_STATUS_FAILURE;
			}
			// but here it failed! Stay in step 3...
			OF_TRACE_LVL (1, ("CreateSimplifiedLinearSystem failed, ofcb->remain_cols=%d\n",
					  ofcb->remain_cols))
			OF_EXIT_FUNCTION
			return OF_STATUS_FAILURE;
		}
	}
	for (i = 0 ; i < ofcb->nb_source_symbols ; i++)
	{
		if (ofcb->encoding_symbols_tab[i] != NULL)
		{
			if (of_linear_binary_code_simplify_linear_system (ofcb, ofcb->encoding_symbols_tab[i], i)
					!= OF_STATUS_OK)
			{
				OF_TRACE_LVL(0,("simplifying the matrix with source symbols failed\n"))
				OF_EXIT_FUNCTION
				return OF_STATUS_FAILURE;
			}
		}
	}
	//
	// Parity symbols
	//
	// Randomize the parity symbols order before injecting them.
	// It makes the decoding process more efficient...
	//srand(1234);
	UINT32	*array;
	array = (UINT32 *) of_malloc (ofcb->nb_repair_symbols * sizeof(UINT32) MEM_STATS_ARG);
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
		if (ofcb->encoding_symbols_tab[ofcb->nb_source_symbols+array[i]] != NULL)
		{
			if (of_linear_binary_code_simplify_linear_system (ofcb,
						ofcb->encoding_symbols_tab[ofcb->nb_source_symbols+array[i]],
						of_get_symbol_esi ((of_cb_t*)ofcb, array[i])) != OF_STATUS_OK)
			{
				OF_TRACE_LVL(0,("simplifying the matrix with parity symbols failed\n"))
				OF_EXIT_FUNCTION
				return OF_STATUS_FAILURE;
			}
		}
	}
	of_free (array MEM_STATS_ARG);
	array = NULL;
	OF_TRACE_LVL (1, (" ofcb->remain_rows=%d, ofcb->remain_cols=%d\n", ofcb->remain_rows, ofcb->remain_cols))
#ifdef IL_SUPPORT
	of_mod2sparse_print_bitmap(ofcb->pchk_matrix_simplified);
#endif
	if (of_linear_binary_code_create_simplified_linear_system (ofcb) != OF_STATUS_OK)
	{
		OF_TRACE_LVL(0, ("Create Simplified Linear System failed\n"))
		OF_EXIT_FUNCTION
		return OF_STATUS_FAILURE;
	}
#ifdef IL_SUPPORT
	//of_mod2sparse_print_bitmap(ofcb->pchk_matrix_simplified);
#endif
	of_mod2dense	*dense_pck_matrix_simplified;
	//of_mod2dense	*m;

	dense_pck_matrix_simplified = of_mod2dense_allocate (ofcb->pchk_matrix_simplified->n_rows,
							     ofcb->pchk_matrix_simplified->n_cols,
							     ofcb->stats);
	of_mod2sparse_to_dense (ofcb->pchk_matrix_simplified, dense_pck_matrix_simplified);
#ifdef DEBUG
	struct timeval	gdtv0;	/* start */
	struct timeval	gdtv1;	/* end */
	struct timeval	gdtv_delta;/* difference tv1 - tv0 */
#endif
	INT32		*column_idx;
	void		** checkValues;
	void		** variable_member;

#ifdef DEBUG
	gettimeofday (&gdtv0, NULL);
	OF_TRACE_LVL (1, ("gauss_decoding_start=%ld.%ld\n", gdtv0.tv_sec, gdtv0.tv_usec))
#endif
	if ((column_idx = (INT32 *) of_malloc
				(of_mod2dense_cols (dense_pck_matrix_simplified) * sizeof (INT32) MEM_STATS_ARG))
			== NULL)
	{
		goto no_mem;
	}
	for (i = 0;i < of_mod2dense_cols (dense_pck_matrix_simplified);i++)
	{
		column_idx[i] = i;
	}
	if ((checkValues = (void **) of_malloc
				(of_mod2dense_rows (dense_pck_matrix_simplified) * sizeof (void*) MEM_STATS_ARG))
			== NULL)
	{
		goto no_mem;
	}
	for (i = 0;i < of_mod2dense_rows (dense_pck_matrix_simplified);i++)
	{
		checkValues[i] = ofcb->tab_const_term_of_equ[ofcb->index_rows[i]];
		ofcb->tab_const_term_of_equ[ofcb->index_rows[i]] = NULL;
	}
	if ((variable_member = (void **) of_calloc
				(of_mod2dense_cols (dense_pck_matrix_simplified), sizeof (void*) MEM_STATS_ARG))
			== NULL)
	{
		goto no_mem;
	}
	if (!of_linear_binary_code_solve_dense_system (dense_pck_matrix_simplified,
				    checkValues,
				    variable_member,
				    ofcb))
	{
		OF_TRACE_LVL(0,("SolveDenseSystem failed !! \n"))
		OF_TRACE_LVL (1, ("Triangularize_Dense_System  failed\n"))
		OF_EXIT_FUNCTION
		goto error;
	}
	else
	{
		OF_TRACE_LVL (1, ("SolveDenseSystem success !! \n"))
	}
	// the system has been solved
	// store the result in the canvas
	/* first, we compute the number of repair found in ML. */
	UINT32 nb_computed_repair_in_ml = ofcb->nb_repair_symbols - ofcb->nb_repair_symbol_ready ;
	/* the first nb_computed_repair_in_ml symbols found in ML are repair symbols. So, we don't
	 * need it.
	 */
	for (i = 0; i < nb_computed_repair_in_ml; i++)
	{
		if (variable_member[i] != NULL)
			of_free(variable_member[i] MEM_STATS_ARG);
	}
	/* the other symbols found in ML are source symbols. So, we copy pointers. */
	for (i = 0; i < ofcb->nb_source_symbols; i++)
	{
		if (ofcb->encoding_symbols_tab[i] == NULL)
		{
			ofcb->encoding_symbols_tab[i] = variable_member[nb_computed_repair_in_ml];
			nb_computed_repair_in_ml++;
		}
	}
#ifdef DEBUG
	if (of_is_decoding_complete (ofcb))
	{
		OF_TRACE_LVL (1, ("ML decoding successful\n"))
	}

	gettimeofday (&gdtv1, NULL);
	timersub (&gdtv1, &gdtv0, &gdtv_delta);
	OF_TRACE_LVL (1, ("gauss_decoding_end=%ld.%ld   gauss_decoding_time=%ld.%06ld \n",
			    gdtv1.tv_sec, gdtv1.tv_usec,
			    gdtv_delta.tv_sec, gdtv_delta.tv_usec))
#endif
	of_free(checkValues MEM_STATS_ARG);
	checkValues = NULL;
	of_free(variable_member MEM_STATS_ARG);
	variable_member = NULL;
	of_free(column_idx MEM_STATS_ARG);
	column_idx = NULL;
	of_mod2dense_free(dense_pck_matrix_simplified , ofcb->stats);
	dense_pck_matrix_simplified = NULL;
	
	OF_EXIT_FUNCTION
	return OF_STATUS_OK;

error:
	of_free(checkValues MEM_STATS_ARG);
	checkValues = NULL;
	of_free(variable_member MEM_STATS_ARG);
	variable_member = NULL;
	of_free(column_idx MEM_STATS_ARG);
	column_idx = NULL;
	of_mod2dense_free(dense_pck_matrix_simplified , ofcb->stats);
	dense_pck_matrix_simplified = NULL;
	return OF_STATUS_FAILURE;

no_mem:
	OF_PRINT_ERROR(("out of memory"))
	OF_EXIT_FUNCTION
	return OF_STATUS_FATAL_ERROR;
}




#endif //ML_DECODING

#endif //OF_USE_LINEAR_BINARY_CODES_UTILS

#endif //OF_USE_DECODER
