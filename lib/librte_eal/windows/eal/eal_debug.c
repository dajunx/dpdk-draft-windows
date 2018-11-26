/* SPDX-License-Identifier: BSD-3-Clause
* Copyright(c) 2017-2018 Intel Corporation
*/

#include <rte_windows.h>
#include <DbgHelp.h>

#include <rte_log.h>
#include <rte_debug.h>

#define MAX_TRACE_STACK_FRAMES	1024

/* dump the stack of the calling core */
void rte_dump_stack(void)
{
	void *pCallingStack[MAX_TRACE_STACK_FRAMES];
	WORD  numFrames;
	DWORD dwError;
	HANDLE hProcess = GetCurrentProcess();

	SymInitialize(hProcess, NULL, TRUE);
	numFrames = RtlCaptureStackBackTrace(0, MAX_TRACE_STACK_FRAMES, pCallingStack, NULL);

	for (int i = 0; i < numFrames; i++) {
	    DWORD64 dwAddress = (DWORD64)(pCallingStack[i]);
	    DWORD   dwDisplacement;

	    char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
	    PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;

	    pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
	    pSymbol->MaxNameLen = MAX_SYM_NAME;

	    // Get the symbol information from the address
	    if (SymFromAddr(hProcess, dwAddress, NULL, pSymbol)) {
		// Get the line number from the same address
		IMAGEHLP_LINE64 line;

		SymSetOptions(SYMOPT_LOAD_LINES);
		line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

		if (SymGetLineFromAddr64(hProcess, dwAddress, &dwDisplacement, &line))
		    printf("Currently at %s in %s: line: %lu: address: 0x%0X\n", pSymbol->Name, line.FileName, line.LineNumber, pSymbol->Address);
		else
		    goto error;
	    }
	    else
		goto error;

	    continue;
error:
	    dwError = GetLastError();
	    printf("SymFromAddr()/SymGetLineFromAddr64() failed: Error: %d\n", dwError);
	}

	return;
}

/* not implemented in this environment */
void rte_dump_registers(void)
{
	return;
}

/* call abort(), it will generate a coredump if enabled */
void __rte_panic(const char *funcname, const char *format, ...)
{
	va_list ap;

	rte_log(RTE_LOG_CRIT, RTE_LOGTYPE_EAL, "PANIC in %s():\n", funcname);
	va_start(ap, format);
	rte_vlog(RTE_LOG_CRIT, RTE_LOGTYPE_EAL, format, ap);
	va_end(ap);
	rte_dump_stack();
	rte_dump_registers();
	abort();
}

/*
 * Like rte_panic this terminates the application. However, no traceback is
 * provided and no core-dump is generated.
 */
void
rte_exit(int exit_code, const char *format, ...)
{
	va_list ap;

	if (exit_code != 0)
		RTE_LOG(CRIT, EAL, "Error - exiting with code: %d\n  Cause: ", exit_code);

	va_start(ap, format);
	rte_vlog(RTE_LOG_CRIT, RTE_LOGTYPE_EAL, format, ap);
	va_end(ap);

#ifndef RTE_EAL_ALWAYS_PANIC_ON_ERROR
	exit(exit_code);
#else
	rte_dump_stack();
	rte_dump_registers();
	abort();
#endif
}
