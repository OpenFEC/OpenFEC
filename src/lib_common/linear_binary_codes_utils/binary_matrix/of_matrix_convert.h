/* $Id: of_matrix_convert.h 2 2011-03-02 11:01:37Z detchart $ */

/* Copyright (c) 1996 by Radford M. Neal
 *
 * Permission is granted for anyone to copy, use, modify, or distribute this
 * program and accompanying programs and documents for any purpose, provided
 * this copyright notice is retained and prominently displayed, along with
 * a note saying that the original programs are available from Radford Neal's
 * web page, and note is made of any changes made to the programs.  The
 * programs and documents are distributed without any warranty, express or
 * implied.  As the programs were written for research purposes only, they have
 * not been tested to the degree that would be advisable in any important
 * application.  All use of these programs is entirely at the user's own risk.
 */

#ifndef OF_MATRIX_CONVERT
#define OF_MATRIX_CONVERT

#include "of_matrix_sparse.h"
#include "of_matrix_dense.h"

#ifdef OF_USE_LINEAR_BINARY_CODES_UTILS
/**
 * @fn			void of_mod2sparse_to_dense (of_mod2sparse * m, of_mod2dense * r)
 * @brief		convert a sparse matrix to a dense matrix
 * @param m		(IN) sparse matrix
 * @param r		(IN) dense matrix
 * @return		void
 */
void of_mod2sparse_to_dense (of_mod2sparse * m, of_mod2dense * r);

/**
 * @fn			void of_mod2dense_to_sparse (of_mod2dense *m, of_mod2sparse *r,of_memory_usage_stats_t *stats)
 * @brief		convert a dense matrix to a sparse matrix
 * @param m		(IN) dense matrix
 * @param r		(IN) sparse matrix
 * @param stats		(IN/OUT) memory statistics (can be NULL)
 * @return		void
 */
void of_mod2dense_to_sparse (of_mod2dense *m, of_mod2sparse *r,of_memory_usage_stats_t *stats);

#endif //OF_USE_LINEAR_BINARY_CODES_UTILS

#endif /* OF_MATRIX_CONVERT */

