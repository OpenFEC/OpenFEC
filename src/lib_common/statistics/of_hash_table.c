/* $Id: of_hash_table.c 72 2012-04-13 13:27:26Z detchart $ */
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

#ifdef OF_DEBUG

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "../of_types.h"
#include "../of_mem.h"
#include "../of_debug.h"


of_status_t 	of_hash_table_init( 	of_hash_table_t *hash,
					UINT32 containers,
					UINT32 (*h_f)(const void *key),
					void (*delete_f)(void *data))
{
	if ((hash->table = (of_linked_list_t*)malloc(containers * sizeof(of_linked_list_t))) == NULL)
		goto error;
	hash->containers = containers;
	UINT32 i;
	for (i=0;i<containers;i++)
		of_linked_list_init(&hash->table[i],delete_f);
	hash->h_f = h_f;
	hash->delete_f = delete_f;
	hash->size = 0;
	return OF_STATUS_OK;
error:
	return OF_STATUS_ERROR;
}


void 		of_hash_table_destroy(	of_hash_table_t *hash)
{
	UINT32 i;
	for (i=0;i<hash->containers;i++)
		of_linked_list_destroy(&hash->table[i]);
	free(hash->table);
	memset(hash,0,sizeof(of_hash_table_t));
	return;
}


of_status_t 	of_hash_table_insert(	of_hash_table_t *hash,
					const void *data)
{
	if (of_hash_table_search(hash,(void**)&data) == OF_STATUS_OK)
		goto error;
	UINT32 container = hash->h_f(data) % hash->containers;
	if (of_linked_list_insert(&hash->table[container],NULL,data) == OF_STATUS_OK)
		hash->size++;
	return OF_STATUS_OK;
error:
	return OF_STATUS_ERROR;
}


of_status_t	of_hash_table_remove(	of_hash_table_t *hash,
					void **data)
{
	UINT32 container = hash->h_f(*data) % hash->containers;
	of_node_t *n,*prec=NULL;
	for (n = of_linked_list_head(&hash->table[container]); n != NULL;n = of_linked_list_next(n))
	{
		if (hash->h_f(*data) == hash->h_f(of_linked_list_data(n)))
		{
			if (of_linked_list_remove(&hash->table[container],prec,data) == OF_STATUS_OK)
			{
				hash->size--;
				return OF_STATUS_OK;
			}
			else
				goto error;
		}
		prec = n;
	}
error:
	return OF_STATUS_ERROR;
}


of_status_t	of_hash_table_search(	of_hash_table_t	*hash,
					void **data)
{
	UINT32 container = hash->h_f(*data) % hash->containers;
	of_node_t *n;
	for (n = of_linked_list_head(&hash->table[container]);n != NULL;n=of_linked_list_next(n))
	{
		if (hash->h_f(*data) == hash->h_f(of_linked_list_data(n)))
		{
			*data = of_linked_list_data(n);
			return OF_STATUS_OK;
		}
	}
error:
	return OF_STATUS_ERROR;
}

#endif /* OF_DEBUG */
