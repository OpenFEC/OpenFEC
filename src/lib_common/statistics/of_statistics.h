/* $Id: of_statistics.h 72 2012-04-13 13:27:26Z detchart $ */
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
 * This module implements statistics about memoryoperations.
 */
#ifndef STATISTICS_H
#define STATISTICS_H


#include "of_linked_list.h"
#include "of_hash_table.h"
#include "of_queue.h"
#include "of_symbols_stats.h"

#ifdef OF_DEBUG

/**
 * \struct of_memory_block_t
 * \brief This structure represents a memory block which contains an allocated pointer and its size
 */
typedef struct of_memory_block
{
	void	*ptr;
	size_t	size;
} of_memory_block_t;


/**
 * \struct of_memory_usage_stats_t
 * \brief Memory usage statistics, for a given codec instance.
 */
typedef struct of_memory_usage_stats
{
	UINT32 		nb_malloc;
	UINT32 		nb_calloc;
	UINT32 		nb_realloc;
	UINT32 		nb_free;
	UINT32		current_mem;	/* current amount of memory used by a codec instance */
	UINT32		maximum_mem;	/* maximum amount of memory used by a codec instance */
	of_hash_table_t	*hash;
} of_memory_usage_stats_t;

#else

typedef struct of_memory_usage_stats
{} of_memory_usage_stats_t;

void of_print_memory_statistics(of_memory_usage_stats_t*);
#endif // OF_DEBUG

#endif //STATISTICS_H