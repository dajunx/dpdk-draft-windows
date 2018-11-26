/* SPDX-License-Identifier: BSD-3-Clause
* Copyright(c) 2017-2018 Intel Corporation
*/

#include <stdlib.h>

int setenv(const char *name, const char *value, int overwrite)
{
	char * curenv;
	size_t len;

	// Does the environment variable already exist?
	errno_t err = _dupenv_s(&curenv, &len, name);

	// Free the allocated memory - it is okay to call free(NULL)
	free(curenv);

	if (err || overwrite)
	{
		char newval[128];
		sprintf_s(newval, sizeof(newval), "%s=%s", name, value);
		return _putenv(newval);
	}

	return 0;
}