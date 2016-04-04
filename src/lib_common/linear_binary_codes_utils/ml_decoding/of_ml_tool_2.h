/* $Id: of_ml_tool_2.h 72 2012-04-13 13:27:26Z detchart $ */
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

#ifndef _OF_ML_TOOL_2_H
#define _OF_ML_TOOL_2_H

#include "../../of_openfec_profile.h"

#ifdef OF_USE_DECODER
#ifdef OF_USE_LINEAR_BINARY_CODES_UTILS
#ifdef ML_DECODING

#include "../../of_types.h"
#include "../../of_openfec_api.h" 	
#include "of_ml_decoding.h"

/**
 * This function create a copy of the linear system (H). If the ML decoding fails, H can be rolled back.
 *
 * @fn of_status_t		of_linear_binary_code_copy_simplified_linear_system (of_linear_binary_code_cb_t* ofcb)
 * @brief			creates a copy of H matrix and update all ML deconding variables
 * @param ofcb			(IN/OUT) Linear-Binary-Code control-block.
 * @return			OF_STATUS_OK if it's OK
 */
of_status_t of_linear_binary_code_copy_simplified_linear_system (of_linear_binary_code_cb_t* ofcb);

/**
 * This function creates the simplified linear system
 *
 * @fn of_status_t		of_linear_binary_code_create_simplified_linear_system (of_linear_binary_code_cb_t* ofcb)
 * @brief			creates the simplified linear system
 * @param ofcb			(IN/OUT) Linear-Binary-Code control-block.
 * @return			OF_STATUS_OK if it's OK
 */
of_status_t of_linear_binary_code_create_simplified_linear_system (of_linear_binary_code_cb_t* ofcb);

/**
 * This function apply a Gauss pivoting on dense system
 *
 * @fn of_status_t		of_linear_binary_code_apply_gauss_pivoting(of_linear_binary_code_cb_t* ofcb,void * encoding_symbols_tab[])
 * @brief			apply a Gauss pivoting
 * @param ofcb			(IN/OUT) Linear-Binary-Code control-block.
 * @param encoding_symbols_tab  (IN/OUT) encoding symbols table
 *
 * @return			OF_STATUS_OK if it's OK
 */
of_status_t of_linear_binary_code_apply_gauss_pivoting (of_linear_binary_code_cb_t* ofcb,
				     void * encoding_symbols_tab[]);

/**
 * This function inject a new symbol into the triangular system
 *
 * @fn of_status_t		of_linear_binary_code_inject_symbol_in_triangular_system(of_linear_binary_code_cb_t* ofcb,void * encoding_symbol_tab[],void * new_symbol,INT32 simplified_col)
 * @brief			Inject a new symbol into the triangular system
 * @param ofcb			(IN/OUT) Linear-Binary-Code control-block.
 * @param encoding_symbol_tab  (IN/OUT) encoding symbols table
 * @param new_symbol		(IN/OUT)  new_symbol
 * @param simplified_col	(IN) number of simplified column in the system
 *
 * @return			OF_STATUS_OK if it's OK
 */
of_status_t of_linear_binary_code_inject_symbol_in_triangular_system (of_linear_binary_code_cb_t* ofcb,
						   void * encoding_symbol_tab[],
						   void * new_symbol,
						   INT32 simplified_col);

/**
 * This function solve the triangular system
 *
 * @fn of_status_t		of_linear_binary_code_solve_triangular_system (of_linear_binary_code_cb_t* ofcb,void * encoding_symbol_tab[])
 * @brief			solve the triangular system
 * @param ofcb			(IN/OUT) Linear-Binary-Code control-block.
 * @param encoding_symbol_tab  (IN/OUT) encoding symbols table
 * @return			OF_STATUS_OK if it's OK
 */
of_status_t of_linear_binary_code_solve_triangular_system (of_linear_binary_code_cb_t* ofcb,
					void * encoding_symbol_tab[]);

/**
 * This function launch the ML decoding : first, it creates the triangular system, then, solve it
 *
 * @fn of_status_t		of_linear_binary_code_launch_ml_decoding (of_linear_binary_code_cb_t* ofcb,void * encoding_symbol_tab[])
 * @brief			launch the ML decoding
 * @param ofcb			(IN/OUT) Linear-Binary-Code control-block.
 * @param encoding_symbol_tab  (IN/OUT) encoding symbols table
 * @return			OF_STATUS_OK if it's OK
 */
of_status_t of_linear_binary_code_launch_ml_decoding (of_linear_binary_code_cb_t* ofcb,
				   void * encoding_symbol_tab[]);

/**
 * This function invert the dense system
 *
 * @fn INT32			of_linear_binary_code_invert_dense_system (of_linear_binary_code_cb_t* ofcb,of_mod2dense *m)
 * @brief			invert the dense system
 * @param ofcb			(IN/OUT) Linear-Binary-Code control-block.
 * @param m			(IN/OUT) matrix to find the invert
 * @return			1 if it's OK, 0 if error
 */
INT32 of_linear_binary_code_invert_dense_system (of_linear_binary_code_cb_t* ofcb,
				of_mod2dense *m	/* The matrix to find the inverse of (destroyed) */);

/**
 * This function triangularize the dense system with a canvas to be solved
 *
 * @fn INT32			of_linear_binary_code_triangularize_dense_system_with_canvas (of_linear_binary_code_cb_t* ofcb,of_mod2dense *m,void * encoding_symbol_tab[])
 * @brief			triangularize the dense system to be solved
 * @param ofcb			(IN/OUT) Linear-Binary-Code control-block.
 * @param m			(IN/OUT) matrix for dense system
 * @param encoding_symbol_tab   (IN/OUT) encoding symbols table
 * @return			1 if it's OK, 0 if error
 */
INT32 of_linear_binary_code_triangularize_dense_system_with_canvas (of_linear_binary_code_cb_t* ofcb,
						 of_mod2dense *m,
						 void * encoding_symbol_tab[]);

/**
 * This function do a forward substitution on encoding symbols
 *
 * @fn INT32			of_linear_binary_code_forward_substitution (of_linear_binary_code_cb_t* ofcb,void * encoding_symbol_tab[],of_mod2dense *m,void ** check_values)
 * @brief			do a forward substitution on encoding symbols
 * @param ofcb			(IN/OUT) Linear-Binary-Code control-block.
 * @param encoding_symbol_tab   (IN/OUT) encoding symbols table
 * @param m			(IN/OUT) matrix for dense system
 * @param check_values          (IN/OUT) constant  members
 * @return			1 if it's OK, 0 if error
 */
INT32 of_linear_binary_code_forward_substitution (of_linear_binary_code_cb_t* ofcb,
				void * encoding_symbol_tab[],
				of_mod2dense* m,
				void ** check_values);

#endif //ML_DECODING
#endif //OF_USE_LINEAR_BINARY_CODES_UTILS
#endif //OF_USE_DECODER

#endif
