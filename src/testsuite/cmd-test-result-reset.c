/* Copyright (c) 2002-2009 Dovecot Sieve authors, see the included COPYING file
 */

#include "sieve-common.h"
#include "sieve-script.h"
#include "sieve-commands.h"
#include "sieve-validator.h"
#include "sieve-generator.h"
#include "sieve-interpreter.h"
#include "sieve-code.h"
#include "sieve-binary.h"
#include "sieve-dump.h"
#include "sieve.h"

#include "testsuite-common.h"
#include "testsuite-result.h"
#include "testsuite-smtp.h"

/*
 * Test_result_execute command
 *
 * Syntax:   
 *   test_result_execute
 */

static bool cmd_test_result_reset_generate
	(const struct sieve_codegen_env *cgenv, struct sieve_command_context *ctx);

const struct sieve_command cmd_test_result_reset = { 
	"test_result_reset", 
	SCT_COMMAND, 
	0, 0, FALSE, FALSE,
	NULL, NULL, NULL,
	cmd_test_result_reset_generate, 
	NULL 
};

/* 
 * Operation 
 */

static int cmd_test_result_reset_operation_execute
	(const struct sieve_operation *op, 
		const struct sieve_runtime_env *renv, sieve_size_t *address);

const struct sieve_operation test_result_reset_operation = { 
	"TEST_RESULT_RESET",
	&testsuite_extension, 
	TESTSUITE_OPERATION_TEST_RESULT_RESET,
	NULL, 
	cmd_test_result_reset_operation_execute 
};

/* 
 * Code generation 
 */

static bool cmd_test_result_reset_generate
(const struct sieve_codegen_env *cgenv, 
	struct sieve_command_context *tst ATTR_UNUSED)
{
	sieve_operation_emit_code(cgenv->sbin, &test_result_reset_operation);

	return TRUE;
}

/*
 * Intepretation
 */

static int cmd_test_result_reset_operation_execute
(const struct sieve_operation *op ATTR_UNUSED,
	const struct sieve_runtime_env *renv,
	sieve_size_t *address ATTR_UNUSED)
{
	testsuite_result_reset(renv);
	testsuite_smtp_reset();

	return SIEVE_EXEC_OK;
}



