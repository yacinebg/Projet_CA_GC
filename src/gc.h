#ifndef _GC_H
#define _GC_H
#include "mlvalues.h"

extern void mark(mlvalue v);
extern void print_heap_state(void);
extern void run_gc(void);
#endif /* _GC_H */