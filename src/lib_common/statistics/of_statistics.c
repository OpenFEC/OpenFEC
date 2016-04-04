/* $Id: of_statistics.c 72 2012-04-13 13:27:26Z detchart $ */
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

#include "of_statistics.h"


#ifdef OF_DEBUG
void of_print_memory_statistics(of_memory_usage_stats_t* mem) {
	if (mem != NULL)
	{
		if (mem->hash != NULL)
		{
			OF_TRACE_LVL (0, ("Memory stats:\n\tmaximum_mem=%i, current_mem=%i\n\tmalloc=%u, calloc=%u, realloc=%u, free=%u (*alloc-free=%i missed blocks)\n",
				mem->maximum_mem, mem->current_mem,
				mem->nb_malloc, mem->nb_calloc,
				mem->nb_realloc, mem->nb_free,
				mem->nb_malloc + mem->nb_calloc - mem->nb_free))
		}
	}
}

#endif
