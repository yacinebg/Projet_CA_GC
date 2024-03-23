#include <stdlib.h>
#include "mlvalues.h"
#include "domain_state.h"

#include "gc.h"

void run_gc(){
    // mlvalue accu = Caml_state ->accu;
    mlvalue* stack = Caml_state -> stack;
    size_t sp = Caml_state-> sp;
}