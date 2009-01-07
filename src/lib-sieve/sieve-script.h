/* Copyright (c) 2002-2009 Dovecot Sieve authors, see the included COPYING file
 */

#ifndef __SIEVE_SCRIPT_H
#define __SIEVE_SCRIPT_H

#include "sieve-common.h"

/*
 * Sieve script object
 */

struct sieve_script;

struct sieve_script *sieve_script_create
	(const char *path, const char *name, 
		struct sieve_error_handler *ehandler, bool *exists_r);

struct sieve_script *sieve_script_create_in_directory
	(const char *dirpath, const char *name,
    	struct sieve_error_handler *ehandler, bool *exists_r);

void sieve_script_ref(struct sieve_script *script);
void sieve_script_unref(struct sieve_script **script);

/*
 * Filename filter
 */
 
bool sieve_script_file_has_extension(const char *filename);

/*
 * Accessors
 */
 
const char *sieve_script_name(const struct sieve_script *script);
const char *sieve_script_filename(const struct sieve_script *script);
const char *sieve_script_path(const struct sieve_script *script);
const char *sieve_script_binpath(const struct sieve_script *script);
const char *sieve_script_dirpath(const struct sieve_script *script);

/* 
 * Stream management 
 */

struct istream *sieve_script_open(struct sieve_script *script, bool *deleted_r);
void sieve_script_close(struct sieve_script *script);

uoff_t sieve_script_get_size(const struct sieve_script *script);

/*
 * Comparison
 */
 
int sieve_script_cmp
	(const struct sieve_script *script1, const struct sieve_script *script2);
unsigned int sieve_script_hash(const struct sieve_script *script);
bool sieve_script_older(const struct sieve_script *script, time_t time);

static inline bool sieve_script_equals
	(const struct sieve_script *script1, const struct sieve_script *script2)
{
	return ( sieve_script_cmp(script1, script2) == 0 );
}

#endif /* __SIEVE_SCRIPT_H */
