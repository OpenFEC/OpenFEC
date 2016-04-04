/* $Id: of_mem.h 72 2012-04-13 13:27:26Z detchart $ */
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

/*
 * This module implements memory management operations.
 * In debug mode, it also performs memory usage statistics.
 */

#ifndef OF_MEM_H
#define OF_MEM_H

#include "statistics/of_statistics.h"

#ifdef OF_DEBUG /* { */



/**
 * @fn			UINT32		of_hash(const void *key)
 * @brief		get the hash of data parameter
 * @param data		(IN) Pointer to  data.
 * @return		hash value.
 */
UINT32		of_hash(const void *data);

/**
 * @fn			inline void*	of_malloc (size_t size, of_memory_usage_stats_t*)
 * @brief		do a malloc
 * @param size		(IN) size of wanted allocated area.
 * @param stats		(IN/OUT) Pointer to memory statistics
 * @return		allocated pointer or NULL if error.
 */
inline void*	of_malloc (size_t size, of_memory_usage_stats_t* stats);

/**
 * @fn			inline void*	of_calloc (size_t nmemb,size_t size, of_memory_usage_stats_t*)
 * @brief		do a calloc
 * @param nmemb		(IN) number of elements
 * @param size		(IN) size of wanted allocated area.
 * @param stats		(IN/OUT) Pointer to memory statistics
 * @return		allocated pointer or NULL if error.
 */
inline void*	of_calloc (size_t nmemb, size_t size, of_memory_usage_stats_t* stats);

/**
 * @fn			inline void*	of_realloc (void* prt, size_t size, of_memory_usage_stats_t* stats)
 * @brief		realloc memory adress
 * @param prt		(IN) pointer to realloc
 * @param size		(IN) size of wanted allocated area.
 * @param stats		(IN/OUT) Pointer to memory statistics
 * @return		allocated pointer or NULL if error.
 */
inline void*	of_realloc (void* prt, size_t size, of_memory_usage_stats_t* stats);

/**
 * @fn			inline void 	of_free (void* ptr, of_memory_usage_stats_t* stats)
 * @brief		free amemory adress
 * @param prt		(IN) pointer to free
 * @param stats		(IN/OUT) Pointer to memory statistics
 * @return		void
 */
inline void 	of_free (void* ptr, of_memory_usage_stats_t* stats);

#else  /* } OF_DEBUG { */

/**
 * \struct of_memory_usage_stats_t
 * \brief Fake memory usage statistics, for a given codec instance.
 */


/**
 * @fn			inline void*	of_malloc (size_t size)
 * @brief		do a malloc
 * @param size		(IN) size of wanted allocated area.
 * @return		allocated pointer or NULL if error.
 */
inline void*	of_malloc (size_t size);

/**
 * @fn			inline void*	of_calloc (size_t nmemb,size_t size)
 * @brief		do a calloc
 * @param nmemb		(IN) number of elements
 * @param size		(IN) size of wanted allocated area.
 * @return		allocated pointer or NULL if error.
 */
inline void*	of_calloc (size_t nmemb, size_t size);

/**
 * @fn			inline void*	of_realloc (void* ptr, size_t size)
 * @brief		realloc memory adress
 * @param ptr		(IN) pointer to realloc
 * @param size		(IN) size of wanted allocated area.
 * @return		allocated pointer or NULL if error.
 */
inline void*	of_realloc (void* ptr, size_t size);

/**
 * @fn			inline void 	of_free (void* ptr)
 * @brief		free amemory adress
 * @param ptr		(IN) pointer to free
 * @return		void
 */
inline void	of_free (void* ptr);

#endif /* } OF_DEBUG */


#endif  //OF_MEM_H
