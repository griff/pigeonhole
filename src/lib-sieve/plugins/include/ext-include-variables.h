/* Copyright (c) 2002-2009 Dovecot Sieve authors, see the included COPYING file
 */

#ifndef __EXT_INCLUDE_VARIABLES_H
#define __EXT_INCLUDE_VARIABLES_H

#include "sieve-common.h"

#include "sieve-ext-variables.h"

#include "ext-include-common.h"

/* 
 * Variable import-export
 */
 
struct sieve_variable *ext_include_variable_import_global
	(struct sieve_validator *valdtr, struct sieve_command *cmd, 
		const char *variable);

/*
 * Binary symbol table
 */

bool ext_include_variables_save
	(struct sieve_binary *sbin, struct sieve_variable_scope *global_vars);
bool ext_include_variables_load
	(const struct sieve_extension *this_ext, struct sieve_binary *sbin, 
		sieve_size_t *offset, unsigned int block,
		struct sieve_variable_scope **global_vars_r);
bool ext_include_variables_dump
	(struct sieve_dumptime_env *denv, struct sieve_variable_scope *global_vars);

/*
 * Validation
 */

void ext_include_variables_global_namespace_init
	(const struct sieve_extension *this_ext, struct sieve_validator *valdtr);
		
#endif /* __EXT_INCLUDE_VARIABLES_H */

