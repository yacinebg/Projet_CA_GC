#include <stdlib.h>

#include "domain_state.h"
#include "config.h"



caml_domain_state* Caml_state;
mlvalue* next_alloc; 

void caml_init_domain() {

  Caml_state = malloc(sizeof(caml_domain_state));

  Caml_state->stack = malloc(Stack_size);

  Caml_state->heap = malloc(INITIAL_HEAP_SIZE);

  Caml_state->heap_size = INITIAL_HEAP_SIZE;

  next_alloc = Caml_state->heap; // on initialisation le pointeur next_alloc au début du tas.

  Caml_state->accu = 0;
  Caml_state->sp = 0;
  Caml_state->env = Make_empty_env();
}
