#include <stdlib.h>

#include "domain_state.h"
#include "config.h"



caml_domain_state* Caml_state;
mlvalue* next_alloc; 

void caml_init_domain() {

  // On alloue la mémoire pour Caml_state
  Caml_state = malloc(sizeof(caml_domain_state));

  // On alloue la mémoire pour le tas et la pile
  Caml_state->stack = malloc(Stack_size);
  Caml_state->heap = malloc(Heap_size);

  // On initialise la taille du tas
  Caml_state->heap_size = Heap_size;

  // On initialise le pointeur next_alloc
  next_alloc = Caml_state->heap; // on initialisation le pointeur next_alloc au début du tas.

  Caml_state->accu = 0;
  Caml_state->sp = 0;
  Caml_state->env = Make_empty_env();
}

//On free tout ce qui a été alloué dans caml_init_domain qui correspond a la mémoire de notre vm 
//Question 2.3 grace a cette fonction nous n'avons plus de fuite mémoire donc plus besoin de valgrind
//depuis l'utilisation des macro nous avons des fuites mais celle-ci sont causé par make_closure qui utilise encore caml_alloc
void caml_free_domain() {
  free(Caml_state->stack); 
  free(Caml_state->heap);
  free(Caml_state);
}