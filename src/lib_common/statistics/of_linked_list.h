/* $Id: of_linked_list.h 72 2012-04-13 13:27:26Z detchart $ */
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


/**
 * This module implements a linked list
 */
#ifndef OF_LINKED_LIST_H
#define OF_LINKED_LIST_H

#include <stdlib.h>
#include "../of_openfec_api.h"

/**
 * \struct of_node_t
 * \brief This structure represents a node used in a linked list.
 */
typedef struct of_node {
	void* 		data;
	struct of_node	*next;
	struct of_node	*prev;
} of_node_t;

/**
 * \struct of_linked_list_t
 * \brief This structure represents a linked list with head and tail pointers
 */
typedef struct of_linked_list {
	UINT32 		size;
	void 		(*delete_f)(void* data);
	//int 		(*corresp_f)(const void *v1,const void *v2);
	of_node_t 	*head;
	of_node_t 	*tail;
} of_linked_list_t;

/**
 * @fn of_status_t	of_linked_list_init    (of_linked_list_t	*l,void (*delete_f)(void *data))
 * @brief			init a linked list
 * @param	l		pointer to the linked list
 * @param delete_f	pointer to the delete function : this function knows which type are data pointer and can free memory.
 * @return			Error status.
 */
of_status_t	of_linked_list_init    (of_linked_list_t	*l,
					void (*delete_f)(void *data));

/**
 * @fn of_status_t	of_linked_list_destroy (of_linked_list_t	*l)
 * @brief			destroy a linked list and all its elements
 * @param	l		pointer to the linked list
 * @return			Error status.
 */
of_status_t	of_linked_list_destroy (of_linked_list_t	*l);

/**
 * @fn of_status_t	of_linked_list_insert  (of_linked_list_t	*l,of_node_t		*n,const void		*data)
 * @brief			insert a node into a linked list
 * @param	l		pointer to the linked list
 * @param   n		pointer to the node you want to insert after
 * @param   data	pointer to data (into the new node)
 * @return			Error status.
 */
of_status_t	of_linked_list_insert  (of_linked_list_t	*l,
					of_node_t		*n,
					const void		*data);

/**
 * @fn of_status_t	of_linked_list_remove  (of_linked_list_t	*l,of_node_t		*n,void			**data)
 * @brief			remove the node after a node n (if n is not null) or in head.
 * @param	l		pointer to the linked list
 * @param   n		pointer to the node you want to remove after
 * @param   data	pointer to pointer to data contained in the removed node
 * @return			Error status.
 */
of_status_t	of_linked_list_remove  (of_linked_list_t	*l,
					of_node_t		*n,
					void			**data);

#define of_linked_list_size(l) 	((l)->size)
#define of_linked_list_head(l) 	((l)->head)
#define of_linked_list_tail(l) ((l)->tail)
#define of_linked_list_next(n) (n->next)
#define of_linked_list_data(n) (n->data)


#endif //OF_LINKED_LIST_H
