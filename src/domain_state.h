#ifndef _DOMAIN_STATE_H
#define _DOMAIN_STATE_H
#define INITIAL_HEAP_SIZE (32 * 1024) // 32 Ko

#include "mlvalues.h"


typedef struct _caml_domain_state {
  /* Stack */
  mlvalue* stack;
  mlvalue* heap;
  size_t heap_size;
  size_t sp;      // Pour stocker le pointeur de pile
  mlvalue env;    // Pour stocker l'environnement courant
  mlvalue accu;   // Pour stocker l'accumulateur
} caml_domain_state;

/* The global state */
extern caml_domain_state* Caml_state;

/* Initialisation function for |Caml_state| */
void caml_init_domain();

#endif
