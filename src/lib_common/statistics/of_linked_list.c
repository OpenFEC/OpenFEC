/* $Id: of_linked_list.c 72 2012-04-13 13:27:26Z detchart $ */
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
#include <string.h>
#include "../of_types.h"
#include "../of_mem.h"
#include "../of_debug.h"


of_status_t of_linked_list_init(of_linked_list_t *l,void (*delete_f)(void *data))
{
	if (l == NULL)
		goto error;
	l->head = NULL;l->tail = NULL;l->size=0;
	l->delete_f = delete_f;
	return OF_STATUS_OK;
error:
	return OF_STATUS_ERROR;
}

of_status_t of_linked_list_destroy(of_linked_list_t *l)
{
	void *data=NULL;
	if (l == NULL)
		goto error;
	while (of_linked_list_size(l) > 0)
	{
		if ( of_linked_list_remove(l,NULL,(void **) &data) == OF_STATUS_OK)
		{
			if ( l->delete_f != NULL)
				l->delete_f(data);
		}
	}
	memset(l,0,sizeof(of_linked_list_t));
	return OF_STATUS_OK;
error:
	return OF_STATUS_ERROR;
}

of_status_t of_linked_list_insert(of_linked_list_t *l,of_node_t *n,const void *data)
{
	of_node_t 	*new_node;
	if ((new_node = (of_node_t*)malloc(sizeof(of_node_t))) == NULL)
		goto error;
	new_node->data = (void*)data;
	if (n == NULL)
	{
		if( of_linked_list_size(l) == 0)
			l->tail = new_node;
		new_node->next = l->head;
		new_node->prev = NULL;
		l->head = new_node;
	}
	else
	{
		if (n->next == NULL)
			l->tail = new_node;
		else
			n->next->prev=new_node;
		new_node->next = n->next;
		
		new_node->prev = n;
		n->next = new_node;
		
	}
	l->size++;
	return OF_STATUS_OK;
error:
	return OF_STATUS_ERROR;
}

of_status_t of_linked_list_remove(of_linked_list_t *l,of_node_t *n,void **data)
{
	of_node_t	*old_node;
	if (of_linked_list_size(l) == 0)
		goto error;
	// remove first element
	if (n == NULL)
	{
		*data = l->head->data;
		old_node = l->head;
		l->head = l->head->next;
		if (l->head != NULL)
			l->head->prev=NULL;
		if( of_linked_list_size(l) == 1)
			l->tail = NULL;
	}
	else
	{
		if (n->next == NULL)
			goto error;
		*data = n->next->data;
		old_node = n->next;
		n->next = n->next->next;
		if (n->next == NULL)
			l->tail = n;
		else
			n->next->prev=n;
	}
	free (old_node);
	l->size--;
	return OF_STATUS_OK;
error:
	return OF_STATUS_ERROR;
}

