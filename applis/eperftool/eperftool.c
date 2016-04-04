/* $Id: eperftool.c 2 2011-03-02 11:01:37Z detchart $ */
/*
 * OpenFEC.org AL-FEC Library.
 * (c) Copyright 2009-2011 INRIA - All rights reserved
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


/* AL-FEC extended performance evaluation tool */

#include "eperftool.h"


print_params()
{
	OF_TRACE_LVL(1, ("eperftool params:\n"))
	OF_TRACE_LVL(1, ("\tcodec_id:\t\t%i\n", codec_id))
	OF_TRACE_LVL(1, ("\ttot_nb_source_symbols:\t%i\n", tot_nb_source_symbols))
	OF_TRACE_LVL(1, ("\ttot_nb_repair_symbols:\t%i\n", tot_nb_repair_symbols))
	OF_TRACE_LVL(1, ("\tcode_rate:\t\t%i\n", code_rate))
	OF_TRACE_LVL(1, ("\tfec_ratio:\t\t%f\n", fec_ratio))
	OF_TRACE_LVL(1, ("\tsrc_pkt_ratio:\t\t%i\n", src_pkt_ratio))
	OF_TRACE_LVL(1, ("\tuse_src_pkt_ratio:\t%s\n", use_src_pkt_ratio ? "true" : "false"))
	OF_TRACE_LVL(1, ("\tsrc_pkt_nb:\t\t%i\n", src_pkt_nb))
	OF_TRACE_LVL(1, ("\tobject_size:\t\t%i bytes\n", object_size))
	OF_TRACE_LVL(1, ("\tsymbol_size:\t\t%i bytes\n", symbol_size))
	OF_TRACE_LVL(1, ("\tsuggested_seed:\t\t%i\n", suggested_seed))
	OF_TRACE_LVL(1, ("\tloss_model:\t\t%i\n", loss_model))
	OF_TRACE_LVL(1, ("\tldpc_N1:\t\t%i\n", ldpc_N1))
	OF_TRACE_LVL(1, ("\tp_success_when_losses:\t%f\n", p_success_when_losses))
	OF_TRACE_LVL(1, ("\tp_loss_when_ok:\t\t%f\n", p_loss_when_ok))
	OF_TRACE_LVL(1, ("\tp_loss:\t\t\t%f\n", p_loss))
	OF_TRACE_LVL(1, ("\tnb_loss:\t\t%i\n", nb_loss))
	return 0;
}


int
main   (int	argc,
	char	**argv )
{
	of_status_t	ret;

#ifdef WIN32
	QueryPerformanceFrequency(&freq);	/* finish time variable init */
#endif
	print_preamble(argv[0]);
	ret = parse_command_line(argc, argv);
	if (ret != OF_STATUS_OK) {
		OF_PRINT_ERROR(("ERROR, parse_command_line() failed\n"))
		goto error;
	}
	ret = finish_init_command_line_params();
	if (ret != OF_STATUS_OK) {
		OF_PRINT_ERROR(("ERROR, finish_init_command_line_params() failed\n"))
		goto error;
	}
	ret = init_sender();
	if (ret != OF_STATUS_OK) {
		OF_PRINT_ERROR(("ERROR, init_sender() failed\n"))
		goto error;
	}
	ret = init_receiver();
	if (ret != OF_STATUS_OK) {
		OF_PRINT_ERROR(("ERROR, init_receiver() failed\n"))
		goto error;
	}
	ret = init_tx_simulator();	/* must be done after init_sender */
	if (ret != OF_STATUS_OK) {
		OF_PRINT_ERROR(("ERROR, init_tx_simulator() failed\n"))
		goto error;
	}
#if 0
#ifdef OF_DEBUG
	/* Warning, this function is not compatible with a normal use of eperftool...
	 * Said differently, do not expect to be able to decode afterwards with the
	 * standard receive_and_decode() function. Reason is that get_next_symbol_received
	 * is not re-entrant. */
	print_params();
	//print_rx_stats();
#endif /* OF_DEBUG */
#endif
	ret = encode();
	if (ret != OF_STATUS_OK) {
		OF_PRINT_ERROR(("ERROR, encode() failed\n"))
		goto error;
	}
	ret = receive_and_decode();
	switch (ret)
	{
	case OF_STATUS_ERROR:
	case OF_STATUS_FATAL_ERROR:
		close_tx_simulator();
		OF_PRINT_ERROR(("ERROR, receive_and_decode() failed\n"))
		goto error;
	case OF_STATUS_FAILURE:
		OF_PRINT(("eperf_tool: decoding failure\n")) /* do not change message, automatic tests depend on it */
		OF_PRINT(("decoding_status=1\n"))
		break;
	case OF_STATUS_OK:
		OF_PRINT(("eperf_tool: decoding ok\n")) /* do not change message, automatic tests depend on it */
		OF_PRINT(("decoding_status=0\n"))
		break;
	}
	ret = close_tx_simulator();
	if (ret != OF_STATUS_OK) {
		OF_PRINT_ERROR(("ERROR, close_tx_simulator() failed\n"))
		goto error;
	}
	return 0;

error:
	OF_PRINT(("decoding_status=2\n"))
	return -1;
}


void
print_preamble (char *command_line)
{
	char		*version;	/* pointer to version string */
	char		*copyrights;	/* pointer to copyrights string */

	OF_PRINT((command_line))
	OF_PRINT(("eperf_tool: an extended AL-FEC performance evaluation tool\n"))
	/* NB: since session pointer is null, we only get a generic string for the
	 * OpenFEC.org project, not specific to the codec itself */
	of_more_about((of_session_t*)NULL, &version, &copyrights);
	OF_PRINT(("%s\n", version))
}

