#ifndef _DOMAIN_STATE_H
#define _DOMAIN_STATE_H

#include "mlvalues.h"


typedef struct _caml_domain_state {
  /* Stack */
  mlvalue* stack;
  /* Heap */
  mlvalue* heap;
  size_t heap_size; //taille macimum du tas

  /* Pointers */
  size_t sp;      // Pour stocker le pointeur de pile
  mlvalue env;    // Pour stocker l'environnement courant
  mlvalue accu;   // Pour stocker l'accumulateur

} caml_domain_state;

/* The global state */
extern caml_domain_state* Caml_state;

/* Initialisation function for |Caml_state| */
void caml_init_domain();
/* Free function for |Caml_state| */
void caml_free_domain();

#endif
