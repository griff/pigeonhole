/* Copyright (c) 2002-2010 Dovecot Sieve authors, see the included COPYING file
 */

#include <stdio.h>
#include <string.h>

#include "lib.h"
#include "str.h"
#include "mempool.h"
#include "ostream.h"

#include "sieve-common.h"
#include "sieve-extensions.h"
#include "sieve-commands.h"
#include "sieve-code.h"
#include "sieve-actions.h"
#include "sieve-generator.h"
#include "sieve-binary.h"
#include "sieve-result.h"
#include "sieve-comparators.h"

#include "sieve-dump.h"

/* 
 * Code dumper extension
 */

struct sieve_code_dumper_extension_reg {
	const struct sieve_code_dumper_extension *cdmpext;
	const struct sieve_extension *ext;
	void *context;
};

struct sieve_code_dumper {
	pool_t pool;
					
	/* Dump status */
	sieve_size_t pc;          /* Program counter */
	
	const struct sieve_operation *operation;
	sieve_size_t mark_address;
	unsigned int mark_line;
	unsigned int mark_last_line;
	unsigned int indent;
	
	/* Dump environment */
	struct sieve_dumptime_env *dumpenv; 

	struct sieve_binary_debug_reader *dreader;
	
	ARRAY_DEFINE(extensions, struct sieve_code_dumper_extension_reg);
};

struct sieve_code_dumper *sieve_code_dumper_create
(struct sieve_dumptime_env *denv) 
{
	pool_t pool;
	struct sieve_code_dumper *dumper;
	
	pool = pool_alloconly_create("sieve_code_dumper", 4096);	
	dumper = p_new(pool, struct sieve_code_dumper, 1);
	dumper->pool = pool;
	dumper->dumpenv = denv;
	dumper->pc = 0;
	
	/* Setup storage for extension contexts */		
	p_array_init(&dumper->extensions, pool, 
		sieve_extensions_get_count(denv->svinst));

	return dumper;
}

void sieve_code_dumper_free(struct sieve_code_dumper **dumper) 
{	
	sieve_binary_debug_reader_deinit(&(*dumper)->dreader);

	pool_unref(&((*dumper)->pool));
	*dumper = NULL;
}

pool_t sieve_code_dumper_pool(struct sieve_code_dumper *dumper)
{
	return dumper->pool;
}

/* EXtension support */

void sieve_dump_extension_register
(struct sieve_code_dumper *dumper, const struct sieve_extension *ext,
	const struct sieve_code_dumper_extension *cdmpext, void *context)
{
	struct sieve_code_dumper_extension_reg *reg;

	if ( ext->id < 0 ) return;

	reg = array_idx_modifiable(&dumper->extensions, (unsigned int) ext->id);
	reg->cdmpext = cdmpext;
	reg->ext = ext;
	reg->context = context;
}

void sieve_dump_extension_set_context
(struct sieve_code_dumper *dumper, const struct sieve_extension *ext, 
	void *context)
{
	struct sieve_code_dumper_extension_reg *reg;

	if ( ext->id < 0 ) return;

	reg = array_idx_modifiable(&dumper->extensions, (unsigned int) ext->id);
	reg->context = context;
}

void *sieve_dump_extension_get_context
(struct sieve_code_dumper *dumper, const struct sieve_extension *ext) 
{
	const struct sieve_code_dumper_extension_reg *reg;

	if  ( ext->id < 0 || ext->id >= (int) array_count(&dumper->extensions) )
		return NULL;
	
	reg = array_idx(&dumper->extensions, (unsigned int) ext->id);		

	return reg->context;
}

/* Dump functions */

void sieve_code_dumpf
(const struct sieve_dumptime_env *denv, const char *fmt, ...)
{
	struct sieve_code_dumper *cdumper = denv->cdumper;	
	unsigned tab = cdumper->indent;
	 
	string_t *outbuf = t_str_new(128);
	va_list args;
	
	va_start(args, fmt);	
	str_printfa(outbuf, "%08llx: ", (unsigned long long) cdumper->mark_address);
	
	if ( cdumper->mark_line > 0 && (cdumper->indent == 0 ||
		cdumper->mark_line != cdumper->mark_last_line) ) {
		str_printfa(outbuf, "%4u: ", cdumper->mark_line);
		cdumper->mark_last_line = cdumper->mark_line;
	} else {
		str_append(outbuf, "      ");
	}

	while ( tab > 0 )	{
		str_append(outbuf, "  ");
		tab--;
	}
	
	str_vprintfa(outbuf, fmt, args);
	str_append_c(outbuf, '\n');
	va_end(args);
	
	o_stream_send(denv->stream, str_data(outbuf), str_len(outbuf));
}

static inline void sieve_code_line_mark
(const struct sieve_dumptime_env *denv, sieve_size_t location)
{
	if ( denv->cdumper->dreader != NULL )  {
		denv->cdumper->mark_line = sieve_binary_debug_read_line
			(denv->cdumper->dreader, location); 
	}
}

void sieve_code_mark(const struct sieve_dumptime_env *denv)
{
	denv->cdumper->mark_address = denv->cdumper->pc;
	sieve_code_line_mark(denv, denv->cdumper->pc);
}

void sieve_code_mark_specific
(const struct sieve_dumptime_env *denv, sieve_size_t location)
{
	denv->cdumper->mark_address = location;
	sieve_code_line_mark(denv, location);
}

void sieve_code_descend(const struct sieve_dumptime_env *denv)
{
	denv->cdumper->indent++;
}

void sieve_code_ascend(const struct sieve_dumptime_env *denv)
{
	if ( denv->cdumper->indent > 0 )
		denv->cdumper->indent--;
}

/* Operations and operands */

bool sieve_code_dumper_print_optional_operands
	(const struct sieve_dumptime_env *denv, sieve_size_t *address)
{
	int opt_code = -1;
	
	if ( sieve_operand_optional_present(denv->sblock, address) ) {
		
		while ( opt_code != 0 ) {			
			if ( !sieve_operand_optional_read(denv->sblock, address, &opt_code) ) {
				return FALSE;
			}

			if ( opt_code == SIEVE_OPT_SIDE_EFFECT ) {
				if ( !sieve_opr_side_effect_dump(denv, address) )
					return FALSE;
			}
		}
	} 
	return TRUE;
}
 
/* Code Dump */

static bool sieve_code_dumper_print_operation
	(struct sieve_code_dumper *dumper) 
{	
	struct sieve_dumptime_env *denv = dumper->dumpenv;
	struct sieve_operation *oprtn = &denv->oprtn;
	sieve_size_t address;
	
	/* Mark start address of operation */
	dumper->indent = 0;
	address = dumper->mark_address = dumper->pc;

	sieve_code_line_mark(denv, address);

	/* Read operation */
	if ( sieve_operation_read(denv->sblock, &(dumper->pc), oprtn) ) {
		const struct sieve_operation_def *op = oprtn->def;

		if ( op->dump != NULL )
			return op->dump(denv, &(dumper->pc));
		else if ( op->mnemonic != NULL )
			sieve_code_dumpf(denv, "%s", op->mnemonic);
		else
			return FALSE;
			
		return TRUE;
	}		
	
	sieve_code_dumpf(denv, "Failed to read opcode.");
	return FALSE;
}

void sieve_code_dumper_run(struct sieve_code_dumper *dumper) 
{
	const struct sieve_dumptime_env *denv = dumper->dumpenv;
	struct sieve_binary *sbin = denv->sbin;
	struct sieve_binary_block *sblock = denv->sblock;
	unsigned int debug_block_id, ext_count;
	bool success = TRUE;

	dumper->pc = 0;

	/* Heading */
	o_stream_send_str(denv->stream, "Address   Line  Code\n");

	/* Load debug block */
	sieve_code_mark(denv);
	
	if ( sieve_binary_read_unsigned(sblock, &dumper->pc, &debug_block_id) ) {
		struct sieve_binary_block *debug_block =
			sieve_binary_block_get(sbin, debug_block_id);

		if ( debug_block == NULL ) {
			sieve_code_dumpf(denv, "Invalid id %d for debug block.", debug_block_id);
			return;
		} else {
			/* Initialize debug reader */
			dumper->dreader = sieve_binary_debug_reader_init(debug_block);

			/* Dump block id */
			sieve_code_dumpf(denv, "DEBUG BLOCK: %d", debug_block_id);
		}
	} else {
		sieve_code_dumpf(denv, "Binary code header is corrupt.");
		return;
	}

	/* Load and dump extensions listed in code */
	sieve_code_mark(denv);
	
	if ( sieve_binary_read_unsigned(sblock, &dumper->pc, &ext_count) ) {
		unsigned int i;
		
		sieve_code_dumpf(denv, "EXTENSIONS [%d]:", ext_count);
		sieve_code_descend(denv);
		
		for ( i = 0; i < ext_count; i++ ) {
			unsigned int code = 0;
			const struct sieve_extension *ext;
			
			T_BEGIN {
				sieve_code_mark(denv);
			
				if ( !sieve_binary_read_extension(sblock, &dumper->pc, &code, &ext) ) {
					success = FALSE;
					break;
				}
      	
				sieve_code_dumpf(denv, "%s", sieve_extension_name(ext));
      
				if ( ext->def != NULL && ext->def->code_dump != NULL ) {
					sieve_code_descend(denv);
					if ( !ext->def->code_dump(ext, denv, &dumper->pc) ) {
						success = FALSE;
						break;
					}
					sieve_code_ascend(denv);
				}
			} T_END;
		}
		
		sieve_code_ascend(denv);
	}	else
		success = FALSE;
		
	if ( !success ) {
		sieve_code_dumpf(denv, "Binary code header is corrupt.");
		return;
	}
	
	while ( dumper->pc < 
		sieve_binary_block_get_size(sblock) ) {

		T_BEGIN {
			success = sieve_code_dumper_print_operation(dumper);
		} T_END;

		if ( !success ) {
			sieve_code_dumpf(dumper->dumpenv, "Binary is corrupt.");
			return;
		}
	}
	
	/* Mark end of the binary */
	dumper->indent = 0;
	dumper->mark_address = sieve_binary_block_get_size(sblock);
	sieve_code_dumpf(dumper->dumpenv, "[End of code]");	
}
