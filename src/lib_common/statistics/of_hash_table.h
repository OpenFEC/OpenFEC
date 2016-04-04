/* $Id: of_hash_table.h 72 2012-04-13 13:27:26Z detchart $ */
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

#ifndef OF_HASH_TABLE_H
#define OF_HASH_TABLE_H

#ifdef OF_DEBUG

#include "of_linked_list.h"

/**
 * \struct of_hash_table
 * \brief This structure represents a hash table.
 */
typedef struct of_hash_table {
	UINT32			containers;
	UINT32			(*h_f)(const void* key);
	void			(*delete_f)(void *data);
	size_t			size;
	of_linked_list_t*	table;
} of_hash_table_t;

/**
 * @fn of_status_t 	of_hash_table_init(of_hash_table_t *hash,UINT32 containers,UINT32 (*h_f)(const void *key),void (*delete_f)(void *data))
 * @brief			init a hash table
 * @param hash		pointer to the hash table
 * @param containers number of containers
 * @param h_f		pointer to a hash function (this function generate a hash with a pointer to data)
 * @param delete_f  pointer to a delete function (this function will free memory data)
 * @return			Error status.
 */
of_status_t 	of_hash_table_init( 	of_hash_table_t *hash,
					UINT32 containers,
					UINT32 (*h_f)(const void *key),
					void (*delete_f)(void *data));

/**
 * @fn void 		of_hash_table_destroy(	of_hash_table_t *hash)
 * @brief			destroy and free memory of a hash table
 * @param hash		pointer to the hash table
 * @return			void
 */
void 		of_hash_table_destroy(	of_hash_table_t *hash);


/**
 * @fn of_status_t 	of_hash_table_insert(	of_hash_table_t *hash,
 const void *data)
 * @brief			insert an element into a hash table
 * @param hash		pointer to the hash table
 * @param data		pointer to data
 * @return			Error status.
 */
of_status_t 	of_hash_table_insert(	of_hash_table_t *hash,
					const void *data);

/**
 * @fn of_status_t 	of_hash_table_remove(	of_hash_table_t *hash,
 const void *data)
 * @brief			remove an element into a hash table
 * @param hash		pointer to the hash table
 * @param data		pointer to pointer to data
 * @return			Error status.
 */
of_status_t	of_hash_table_remove(	of_hash_table_t *hash,
					void **data);

/**
 * @fn of_status_t 	of_hash_table_search(	of_hash_table_t *hash,
 const void *data)
 * @brief			search an element 
 * @param hash		pointer to the hash table
 * @param data		pointer to pointer to data
 * @return			Error status.
 */
of_status_t	of_hash_table_search(	of_hash_table_t	*hash,
					void **data);

#define of_hash_table_size(hash)	((hash)->size)

#endif /* OF_DEBUG */

#endif /* OF_HASH_TABLE_H */
