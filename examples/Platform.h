#pragma once

#ifdef SYS_WINDOWS
	#include <synchapi.h>

	#define sleep(x) Sleep(x*1000)
#else
	#include <stdlib.h>
#endif
