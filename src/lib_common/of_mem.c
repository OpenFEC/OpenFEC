/* $Id: of_mem.c 72 2012-04-13 13:27:26Z detchart $ */
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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "of_types.h"
#include "of_mem.h"
#include "of_debug.h"


#ifndef OF_DEBUG

inline void* of_malloc (size_t	size)
{
	return malloc (size);
}


inline void* of_calloc (size_t	nmemb,
		 size_t	size)
{
	return calloc (nmemb, size);
}


inline void* of_realloc (void* ptr,
		  size_t size)
{
	return realloc (ptr, size);

}


inline void  of_free (void* ptr)
{
	if (ptr)
	{
		free (ptr);

	}
}


#else  /* } { */


// return as key, pointer to allocated block
UINT32 of_hash(const void	*data)
{
	UINT32		lowbits;
	/* cast the pointer (a 64 bit integer on LP64 systems) to uintptr_t first, and then
	 * truncate it to keep only the lowest 32 bits. Works the same on both 32 bit and 64
	 * bit systems */
	lowbits = 0xFFFFFFFF & (uintptr_t)(((of_memory_block_t*)data)->ptr);
	return lowbits;
}


inline void* of_malloc (size_t				size,
			of_memory_usage_stats_t*	stats)
{
	void	*p;

	p = malloc (size);
	if ((p != NULL) && (stats != NULL))
	{
		of_memory_block_t	*b;
		b	= (of_memory_block_t *)calloc(1, sizeof(of_memory_block_t));
		b->size	= size;
		b->ptr	= p;
		of_hash_table_insert(stats->hash, (const void *)b);
		stats->current_mem += size;
		if (stats->current_mem > stats->maximum_mem)
		{
			stats->maximum_mem = stats->current_mem;
		}
		stats->nb_malloc++;
	}
	return p;
}


inline void* of_calloc (size_t				nmemb,
			size_t				size,
			of_memory_usage_stats_t*	stats)
{
	void	*p;
	p = calloc (nmemb, size);
	if ((p != NULL) && (stats != NULL))
	{
		of_memory_block_t	*b;
		b	= (of_memory_block_t *)calloc(1, sizeof(of_memory_block_t));
		b->size	= size*nmemb;
		b->ptr	= p;
		of_hash_table_insert(stats->hash, (const void *)b);
		stats->current_mem += (size*nmemb);
		if (stats->current_mem > stats->maximum_mem)
		{
			stats->maximum_mem = stats->current_mem;
		}
		stats->nb_calloc++;
	}
	return p;
}


inline void* of_realloc (void*				ptr,
			 size_t				size,
			 of_memory_usage_stats_t*	stats)
{
	void	*p;
	if (stats != NULL)
	{
		of_memory_block_t	b;
		of_memory_block_t	*ptr_b;

		b.ptr	= ptr;
		b.size	= 0;
		ptr_b = &b;
		if (of_hash_table_search(stats->hash, (void**)&ptr_b) == OF_STATUS_OK)
		{
			stats->current_mem -= ptr_b->size;
			of_hash_table_remove(stats->hash, (void**)&ptr_b);
		}
		else
		{
			OF_PRINT_ERROR(("(memory stats): not found in list of allocated buffers\n"))
		}
	}
	p =  realloc (ptr, size);
	if (stats != NULL)
	{
		of_memory_block_t	b;

		b.size	= size;
		b.ptr	= p;
		of_hash_table_insert(stats->hash, (const void *)&b);
		stats->current_mem += size;
		if (stats->current_mem > stats->maximum_mem)
		{
			stats->maximum_mem = stats->current_mem;
		}
		stats->nb_realloc++;
	}
	return p;
}


inline void  of_free   (void*				ptr,
			of_memory_usage_stats_t*	stats)
{
	if (ptr == NULL)
	{
		return;
	}
	if (stats != NULL)
	{
		of_memory_block_t	b;
		of_memory_block_t	*ptr_b;

		b.size	= 0;
		b.ptr	= ptr;
		ptr_b = &b;
		if (of_hash_table_search(stats->hash, (void**)&ptr_b) == OF_STATUS_OK)
		{
			stats->current_mem -= ptr_b->size;
			if (of_hash_table_remove(stats->hash, (void**)&ptr_b) != OF_STATUS_OK)
				OF_PRINT(("not removed..\n"))
		}
		else
		{
			OF_PRINT_ERROR(("(memory stats): not found in list of allocated buffers\n"))
		}
		stats->nb_free++;
	}
	free (ptr);
}

#endif /* } */
