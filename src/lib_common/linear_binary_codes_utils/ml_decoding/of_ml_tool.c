/* $Id: of_ml_tool.c 72 2012-04-13 13:27:26Z detchart $ */
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

#include "of_ml_tool.h"

#ifdef OF_USE_DECODER
#ifdef OF_USE_LINEAR_BINARY_CODES_UTILS
#ifdef ML_DECODING

#include <math.h>
#include <string.h>



INT32	of_linear_binary_code_solve_dense_system (of_mod2dense *m,
			       void ** constant_member,
			       void **variables,
			       of_linear_binary_code_cb_t *ofcb)
{
	OF_ENTER_FUNCTION
	if (!of_linear_binary_code_triangularize_dense_system (m, constant_member, ofcb))
	{
		OF_TRACE_LVL(0,("SolveDenseSystem: Triangularize_Dense_System  failed\n"))
        //of_mod2dense_print_bitmap(m);
		OF_EXIT_FUNCTION
		return 0;
	}
    int i,j,nb=0;
    for (i=0;i<of_mod2dense_rows(m);i++)
    {
        for (j=0;j<of_mod2dense_cols(m);j++)
        {
            if (of_mod2dense_get (m, i, j))
                nb++;
        }
    }
    //printf("nb entries:%i\n",nb);
	//of_mod2dense_print_bitmap(m);
	if (!of_linear_binary_code_backward_substitution (variables, constant_member, m, ofcb))
	{
		OF_TRACE_LVL(0,("SolveDenseSystem: Backward_Substitution  failed\n"))
		OF_EXIT_FUNCTION
		return 0;
	}
	OF_EXIT_FUNCTION
	return 1;
}

INT32	of_linear_binary_code_preprocess_dense_system (of_mod2dense *m,
				    void **check_values,
				    UINT32 symbol_size)
{
	OF_ENTER_FUNCTION
	INT32 i, n, p, w;
	n = of_mod2dense_cols (m);
	p = of_mod2dense_rows (m);
	w = m->n_words;
	/* for each line*/
	for (i = 0; i < n; i++)
	{
		if (!of_linear_binary_code_col_forward_permutation (m, check_values, symbol_size, i))
		{
			//return 0;
		}
		if (i > 20)
		{
			OF_EXIT_FUNCTION
			return 1;
		}
		if (i % (int) ceil (n / 10) == 0)
		{
			//of_mod2dense_print_bitmap(m);
		}
	}
	OF_EXIT_FUNCTION
	return 1;
}

INT32	of_linear_binary_code_triangularize_dense_system (of_mod2dense *m,
				       void **check_values,
				       of_linear_binary_code_cb_t *ofcb)
{
	OF_ENTER_FUNCTION
	INT32 i,  n, p, w;
	n = of_mod2dense_cols (m);
	p = of_mod2dense_rows (m);
	w = m->n_words;
	/* for each line*/
	for (i = 0; i < n; i++)
	{
		if (!of_linear_binary_code_col_forward_elimination (m, check_values, ofcb, i))
		{
			OF_EXIT_FUNCTION
			return 0;
		}
	}
	OF_EXIT_FUNCTION
	return 1;
}


inline INT32	of_linear_binary_code_col_forward_elimination (	of_mod2dense		*m,
							void			**check_values,
							of_linear_binary_code_cb_t	*ofcb,
							INT32			col_idx)
{
	OF_ENTER_FUNCTION
	of_mod2word *s, *t;
	INT32 i, j, k, n, p, w, k0, b0,symbol_size=ofcb->encoding_symbol_length;
	void * tmp_buffer;
	n = of_mod2dense_cols (m);
	p = of_mod2dense_rows (m);
	w = m->n_words;
	i = col_idx;
	//printf("of_col_forward_elimination of col %d\n",i);
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
	{
		OF_EXIT_FUNCTION
		return 0;
	}
	if (j != i)
	{
		//printf("swap rows %i and %i\n",j,i);
		// swap columns i and j in the two matrices
		t = m->row[i];
		m->row[i] = m->row[j];
		m->row[j] = t;
		//  swap the partial sum
		tmp_buffer = check_values[i];
		check_values[i] = check_values[j];
		check_values[j] = tmp_buffer;
		//of_mod2dense_print(stdout,m);
	}

	ofcb->nb_tmp_symbols=0;
	for (j = i + 1; j < p; j++)
	{
		if (of_mod2_getbit (m->row[j][k0], b0))
		{
			//printf("\n\nxor in %i, %i\n",j,i);
			// for all the column were the ith element is non null
			s = m->row[j];
			t = m->row[i];
			s += k0;
			t += k0;
			for (k=k0;k<w;k++)
			{
				*s ^=*t;
				s++;
				t++;
			}
			//of_mod2dense_print(stdout,m);
			/*for (k = k0; k < w; k++)
				s[k] ^= t[k]; // add column i with coluln j*/
			if (check_values[i] != NULL)
			{
				// if the buffer of line i is NULL there is nothing to Add
				// add the checkValues of the i to the m_checkValue of line j
				if (check_values[j] == NULL)
				{
					check_values[j] = of_malloc (symbol_size MEM_STATS_ARG);
				
					// Copy data now
					memcpy (check_values[j],
						check_values[i],
						symbol_size);
				} // if (checkValues[m_index_rows[__current_row]] == NULL)
				else
				{
					ofcb->tmp_tab_symbols[ofcb->nb_tmp_symbols++] = check_values[j];
#if 0
					of_add_to_symbol (check_values[j],
							  check_values[i],
							  symbol_size OP_ARG_VAL);
#endif
				} // if (checkValues[m_index_rows[__current_row]] == NULL) ... else ...
			}
			else
			{
				check_values[i] = of_calloc (1, symbol_size MEM_STATS_ARG);
				//memset (check_values[i], 0, symbol_size);
			}
		}
	}
#if 1
	if (ofcb->nb_tmp_symbols!=0)
		if (ofcb->nb_tmp_symbols == 1)
			of_add_to_symbol (ofcb->tmp_tab_symbols[0],
							  check_values[i],
							  symbol_size OP_ARG_VAL);
		else
#ifdef OF_DEBUG
		of_add_to_multiple_symbols(ofcb->tmp_tab_symbols,check_values[i],ofcb->nb_tmp_symbols,
								   ofcb->encoding_symbol_length,
								   &(ofcb->stats_xor->nb_xor_for_ML));
#else
	of_add_to_multiple_symbols(ofcb->tmp_tab_symbols,check_values[i],ofcb->nb_tmp_symbols,
							   ofcb->encoding_symbol_length);
#endif
#endif
	OF_EXIT_FUNCTION
	return 1;
}


INT32	of_linear_binary_code_col_forward_elimination_pivot_reordering (of_mod2dense		*m,
									void			**check_values,
									of_linear_binary_code_cb_t	*ofcb,
									INT32			col_idx)
{
	OF_ENTER_FUNCTION
	of_mod2word *s, *t;
	INT32 i, j, k, n, p, w, k0, b0,symbol_size = ofcb->encoding_symbol_length;
	void * tmp_buffer;
	n = of_mod2dense_cols (m);
	p = of_mod2dense_rows (m);
	w = m->n_words;
	i = col_idx;
	//printf("of_col_forward_elimination of col %d\n",i);
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
	{
		OF_EXIT_FUNCTION
		return 0;
	}
	if (j != i)
	{
		// swap column i an j the two matrices
		t = m->row[i];
		m->row[i] = m->row[j];
		m->row[j] = t;
		//  swap the partial sum
		tmp_buffer = check_values[i];
		check_values[i] = check_values[j];
		check_values[j] = tmp_buffer;
	}
	//for (j = 0; j<p; j++)
	for (j = i; j < p; j++)
	{
		if (j != i && of_mod2_getbit (m->row[j][k0], b0))
		{			
			// for all the row were the ith element is non null
			s = m->row[j];
			t = m->row[i];
			for (k = k0; k < w; k++)
			{
				s[k] ^= t[k]; // add row i with row j
			}
			if (check_values[i] != NULL)
			{
				// if the buffer of line i is NULL there is nothing to Add
				// add the checkValues of the i to the m_checkValue of line j
				if (check_values[j] == NULL)
				{
					check_values[j] =  of_malloc (symbol_size MEM_STATS_ARG);
					//printf("malloc %d\n",symbolSize);
					// Copy data now
					memcpy (
						check_values[j],
						check_values[i],
						symbol_size);
					//MEM_STORE(checkValues[j]);
				} // if (checkValues[m_index_rows[__current_row]] == NULL)
				else
				{
					of_add_to_symbol (
						check_values[j],
						check_values[i],
						symbol_size OP_ARG_VAL);
				} // if (checkValues[m_index_rows[__current_row]] == NULL) ... else ...
			}
			else
			{
				check_values[i] = of_malloc (symbol_size MEM_STATS_ARG);
				memset (check_values[i], 0, symbol_size);
			}
		}
	}
	OF_EXIT_FUNCTION
	return 1;
}

/* Deduce the value of the missing symbols from the inverted dense matrix */
INT32	of_linear_binary_code_backward_substitution (void * variables[],
				  void * constant_member[],
				  of_mod2dense *m,
				  of_linear_binary_code_cb_t *ofcb)
{
	OF_ENTER_FUNCTION
	INT32 i, j, n, p, w, k0, b0;
	n = of_mod2dense_cols (m);
	p = of_mod2dense_rows (m);
	w = m->n_words;
	for (i = n - 1;i >= 0;i--)
	{
		if (variables[i] == NULL)
		{
			k0 = i >> of_mod2_wordsize_shift; // word index of the ith bit
			b0 = i & of_mod2_wordsize_mask;  // bit index of the ith bit in the k0-th word
			ASSERT(of_mod2_getbit (m->row[i][k0], b0))
			/*if (!of_mod2_getbit (m->row[i][k0], b0))
			{
				// check if the diagonal is non null
				OF_PRINT_LVL (1, ("diagonal element %d is null !! \n ", i))
				OF_EXIT_FUNCTION
				return 0;
			}*/
			// the missing source symbol in col i is equal to the sum of P'_i and all the
			// source symbol represented in row i
			//printf("rebuilding source symbol %d with col %d\n",col_index[i],i);
            variables[i] = constant_member[i];
            constant_member[i] = NULL;

			ofcb->nb_tmp_symbols=0;
			for (j = i + 1; j < n;j++)
			{
				k0 = j >> of_mod2_wordsize_shift; // word index of the ith bit
				b0 = j & of_mod2_wordsize_mask;  // bit index of the ith bit in the k0-th word
				/* search for the non-null element  in the row i*/
				if (of_mod2_getbit (m->row[i][k0], b0))
				{
					ofcb->tmp_tab_symbols[ofcb->nb_tmp_symbols++] = variables[j];
#if 0
					of_add_to_symbol (
						variables[i],//GetBufferPtrOnly(m_checkValues[m_index_rows[j]]),
						variables[j],//GetBuffer(m_checkValues[m_index_rows[i]])
						symbol_size OP_ARG_VAL);
#endif
				}
			}
			//printf("%i\n",ofcb->nb_tmp_symbols);
#if 1
			if (ofcb->nb_tmp_symbols!=0)
#ifdef OF_DEBUG
				of_add_from_multiple_symbols(variables[i],ofcb->tmp_tab_symbols,ofcb->nb_tmp_symbols,
										   ofcb->encoding_symbol_length,
										   &(ofcb->stats_xor->nb_xor_for_ML));
#else
			of_add_from_multiple_symbols(variables[i],(const void**)ofcb->tmp_tab_symbols,ofcb->nb_tmp_symbols,
									   ofcb->encoding_symbol_length);
#endif
#endif
		}
        else
        {
            OF_PRINT_ERROR(("Backward_Substitution_mt: ERROR variables[%d] should be NULL\n", i))
        }
	}
	OF_EXIT_FUNCTION
	return 1;
}

INT32	of_linear_binary_code_col_forward_permutation (of_mod2dense *m,
				    void **check_values,
				    UINT32 symbol_size,
				    INT32 col_idx)
{
	OF_ENTER_FUNCTION
	of_mod2word  *t;
	INT32 i, j, n, p, w, k0, b0;
	void * tmp_buffer;
	n = of_mod2dense_cols (m);
	p = of_mod2dense_rows (m);
	w = m->n_words;
	i = col_idx;
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
	{
		OF_EXIT_FUNCTION
		return 0;
	}
	if (j != i)
	{
		// swap column i an j the two matrices
		t = m->row[i];
		m->row[i] = m->row[j];
		m->row[j] = t;
		//  swap the partial sum
		tmp_buffer = check_values[i];
		check_values[i] = check_values[j];
		check_values[j] = tmp_buffer;
	}
	OF_EXIT_FUNCTION
	return 1;
}



#endif //ML_DECODING
#endif //OF_USE_LINEAR_BINARY_CODES_UTILS
#endif //OF_USE_DECODER
