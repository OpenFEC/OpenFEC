/* $Id: of_galois_field_code.h 2 2011-03-02 11:01:37Z detchart $ */
/*
 * OpenFEC.org AL-FEC Library.
 * (c) Copyright 2009 - 2011 INRIA - All rights reserved
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

#ifndef GALOIS_FIELD_CODE_H
#define GALOIS_FIELD_CODE_H
#include <stdint.h>
#include <strings.h>

#include "../../../lib_common/of_debug.h"
#include "../../../lib_common/of_mem.h"
#include "../../../lib_common/of_openfec_api.h"

#ifdef OF_USE_REED_SOLOMON_2_M_CODEC

#define FEC_MAGIC	0xFECC0DEC


/*
 * Primitive polynomials - see Lin & Costello, Appendix A,
 * and  Lee & Messerschmitt, p. 453.
 */
static const char *of_rs_allPp[] =      /* GF_BITS	polynomial		*/
{
	NULL,				/*  0	no code			*/
	NULL,				/*  1	no code			*/
	"111",				/*  2	1+x+x^2			*/
	"1101",				/*  3	1+x+x^3			*/
	"11001",			/*  4	1+x+x^4			*/
	"101001",			/*  5	1+x^2+x^5		*/
	"1100001",			/*  6	1+x+x^6			*/
	"10010001",			/*  7	1 + x^3 + x^7		*/
	"101110001",			/*  8	1+x^2+x^3+x^4+x^8	*/
	"1000100001",			/*  9	1+x^4+x^9		*/
	"10010000001",			/* 10	1+x^3+x^10		*/
	"101000000001",			/* 11	1+x^2+x^11		*/
	"1100101000001",		/* 12	1+x+x^4+x^6+x^12	*/
	"11011000000001",		/* 13	1+x+x^3+x^4+x^13	*/
	"110000100010001",		/* 14	1+x+x^6+x^10+x^14	*/
	"1100000000000001",		/* 15	1+x+x^15		*/
	"11010000000010001"		/* 16	1+x+x^3+x^12+x^16	*/
};

/**
 * Galois-Field-Code stable codec specific control block structure.
 */
typedef struct of_galois_field_code_cb
{
/*****************************************************************************
*                              of_cb_t                                       *
******************************************************************************/
	of_codec_id_t		codec_id;		/* must begin with fec_codec_id          */
	of_codec_type_t		codec_type;		/* must be 2nd item                      */
	UINT32			nb_source_symbols;	/** k parameter (AKA code dimension).    */
	UINT32			nb_repair_symbols;	/** r = n - k parameter.                 */
	UINT32			encoding_symbol_length;	/** symbol length.                   */
/*****************************************************************************/

	/** Memory usage statistics, for this codec instance. */
	of_memory_usage_stats_t	*stats;

	UINT16			nb_bits; 	/* between 2..16 */
	UINT16			field_size; 	/* 2^nb_bits */
	gf*			of_rs_gf_exp;	/* index->poly form conversion table	*/
	int*			of_rs_gf_log;	/* Poly->index form conversion table	*/
	gf*			of_rs_inverse;	/* inverse of field elem.		*/
	gf**			of_gf_mul_table; /* table of 2 numbers multiplication */

#ifdef OF_USE_ENCODER
	/**
	 * Encoding matrix (G).
	 * The encoding matrix is computed starting with a Vandermonde matrix,
	 * and then transforming it into a systematic matrix.
	 */
	gf			*enc_matrix ;
#endif
#ifdef OF_USE_DECODER
	/**
	 * Decoding matrix (k*k).
	 */
	gf			*dec_matrix;
#endif
	UINT32			magic;
} of_galois_field_code_cb_t;


/**
 * just a helper to init all we need to use GF
 */
of_status_t	of_rs_2m_init(of_galois_field_code_cb_t* ofcb);
/**
 * and the helper to release memory
 */
void		of_rs_2m_release(of_galois_field_code_cb_t* ofcb);

/**
 * even if only decoder is defined, we need an encoding matrix.
 */
of_status_t	of_rs_2m_build_encoding_matrix(of_galois_field_code_cb_t* ofcb);

#ifdef OF_USE_DECODER
of_status_t	of_rs_2m_build_decoding_matrix(of_galois_field_code_cb_t* ofcb,int* index);
of_status_t	of_rs_2m_decode(of_galois_field_code_cb_t* ofcb,gf *pkt[], int index[], int sz);
#endif

#ifdef OF_USE_ENCODER
of_status_t	of_rs_2m_encode(of_galois_field_code_cb_t* ofcb,gf *_src[], gf *_fec, int index, int sz);
#endif

#endif //OF_USE_GALOIS_FIELD_CODES_UTILS

#endif //GALOIS_FIELD_CODE_H
