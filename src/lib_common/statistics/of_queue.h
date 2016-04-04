/* $Id: of_queue.h 72 2012-04-13 13:27:26Z detchart $ */
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


#ifndef OF_QUEUE_H
#define OF_QUEUE_H

#include <stdio.h>
#include "../of_openfec_api.h"

/**
 * \struct of_queue_node_t
 * \brief This structure represents a queue node
 */
typedef struct of_queue_node
{
	struct of_queue_node *prev;
	struct of_queue_node *next;
	void *data;
} of_queue_node_t;

/**
 * \struct of_queue_t
 * \brief This structure represents a queue
 */
typedef struct of_queue
{
	UINT32		size;
	of_queue_node_t *head;
	of_queue_node_t *tail;
} of_queue_t;

/**
 * @fn of_status_t of_queue_init(of_queue_t *q)
 * @brief			init a queue
 * @param	q		pointer to the queue
 * @return			Error status.
 */
of_status_t of_queue_init(of_queue_t *q);

/**
 * @fn of_status_t of_queue_push(of_queue_t *q,void* data)
 * @brief			push an element into a queue
 * @param	q		pointer to the queue
 * @param data		pointer to data
 * @return			Error status.
 */
of_status_t of_queue_push(of_queue_t *q,void* data);

/**
 * @fn void* of_queue_pop(of_queue_t *q)
 * @brief			pop an element of a queue
 * @param	q		pointer to the queue
 * @return			pointer to element data.
 */
void* of_queue_pop(of_queue_t *q);

#define of_queue_size(q) 	((q)->size)

#endif //OF_QUEUE_H
