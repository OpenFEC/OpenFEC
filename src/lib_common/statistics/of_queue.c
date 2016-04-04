/* $Id: of_queue.c 72 2012-04-13 13:27:26Z detchart $ */
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

#include "of_queue.h"
#include <stdlib.h>

of_status_t of_queue_init(of_queue_t *q)
{
	if (q == NULL)
		goto error;
	q->tail = q->head = NULL;q->size = 0;
	return OF_STATUS_OK;
error:
	return OF_STATUS_ERROR;
}

of_status_t of_queue_push(of_queue_t *q, void* data)
{
	of_queue_node_t *n = (of_queue_node_t*)calloc(1, sizeof(of_queue_node_t));
	n->data = data;
	if (q != NULL)
	{
		if ( of_queue_size(q) == 0)
		{
			q->tail=n;
			q->head=n;
		}
		else
		{
			n->next=q->head;
			q->head->prev=n;
			q->head=n;
		}
	}
	q->size++;
	return OF_STATUS_OK;
error:
	return OF_STATUS_ERROR;
}

void* of_queue_pop(of_queue_t *q)
{
	void *data=NULL;
	of_queue_node_t *n;
	if (q != NULL )
	{
		if ( of_queue_size(q) == 0 )
			goto finished;
		n = q->tail;
		data = n->data;
		if ( of_queue_size(q) == 1)
		{
			q->head = NULL;
			q->tail = NULL;
			free(n);
		}
		else
		{
			n=n->prev;
			free(n->next);
			n->next=NULL;
			q->tail = n;
		}
	}
	q->size--;
finished:
	return data;
}
