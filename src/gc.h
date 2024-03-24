#ifndef _GC_H
#define _GC_H
#include "mlvalues.h"

extern void mark(mlvalue v);
extern void print_heap_state(void);
extern void run_gc(void);
extern void compact();
extern void update_all_pointers();
extern mlvalue new_address_of(mlvalue* old_address);

#endif /* _GC_H */