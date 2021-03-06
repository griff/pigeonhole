/* Copyright (c) 2002-2018 Pigeonhole authors, see the included COPYING file
 */

#ifndef __SIEVE_SMTP_H
#define __SIEVE_SMTP_H

#include "sieve-common.h"

bool sieve_smtp_available
	(const struct sieve_script_env *senv);

struct sieve_smtp_context;

struct sieve_smtp_context *sieve_smtp_start
	(const struct sieve_script_env *senv,
		const struct smtp_address *mail_from);
void sieve_smtp_add_rcpt
	(struct sieve_smtp_context *sctx,
		const struct smtp_address *rcpt_to);
struct ostream *sieve_smtp_send
	(struct sieve_smtp_context *sctx);

struct sieve_smtp_context *sieve_smtp_start_single
	(const struct sieve_script_env *senv,
		const struct smtp_address *rcpt_to,
		const struct smtp_address *mail_from,
		struct ostream **output_r);

void sieve_smtp_abort
	(struct sieve_smtp_context *sctx);
int sieve_smtp_finish
	(struct sieve_smtp_context *sctx, const char **error_r);

#endif /* __SIEVE_SMTP_H */
