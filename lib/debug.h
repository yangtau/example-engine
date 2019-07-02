#ifndef _BTREE_DEBUG_H
#define _BTREE_DEBUG_H

//#define DEBUG_CHECK_MEM

#ifdef DEBUG_CHECK_MEM

#include <Windows.h>
#include <assert.h>

#define MEM_CHECK assert(_CrtCheckMemory());

#else

#define MEM_CHECK

#endif  // DEBUG_CHECK_MEM

#endif  // _BTREE_DEBUG_H