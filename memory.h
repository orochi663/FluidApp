/*
 *  memory.h
 *  FluidApp
 */

#ifndef MEMORY_H
#define MEMORY_H

#include <setjmp.h>
#include "error.h"

//Provides a means to deal with the memory in the system.  That is, a simple
//way to handle malloc/free where multiple objects may need this one to live
//for a certain amount of time.
//
//Also, a generic way to free the data on a single thread (required for OpenGL)
//
//	All methods are thread-safe!

//Function pointer to deallocator.  Used to free up the memory once it's
//no longer needed.  Required as things remain in memory for a potentially
//longer amount of time.
typedef void(*x_dealloc)(void *o);

//Replacement allocator
void *x_malloc(int size, x_dealloc in_d) __attribute__ ((malloc));

//Replacment free function
void x_free(void *in_o);

//Retain (extension)
void x_retain(void *in_o);


//Initializes the memory model...
void x_init();

//try-catch-finally-throw mechanisms...
//	Raised errors automatically GCed!
#define x_try	sigjmp_buf __pvt_except;	\
				sigjmp_buf *__pvt_except_p = x_setupBuff(&__pvt_except); \
				int __exception; \
				if ((__exception = _setjmp(__pvt_except)) == 0) {
#define x_catch(o)	} else { error *o = (error*)__exception; o;
#define x_finally	x_free((error*)__exception);}	\
						x_setupBuff(__pvt_except_p); \
						if (__pvt_except_p) _setjmp(*__pvt_except_p);

//How to raise an exception
void x_raise(error *e);

//Used internally by exception system
sigjmp_buf *x_setupBuff(sigjmp_buf *in_newBuff);

//Since I use this function so often; it shall be dumped here
double x_time();

#endif
