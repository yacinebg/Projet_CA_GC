#include <stdlib.h>
#include <stdio.h>


#include "domain_state.h"
#include "gc.h"
#include "config.h"

void mark(mlvalue v){
    if(Is_block(v) && Color(v) == WHITE){
        header_t* header = (header_t*)(Ptr_val(v) - 1);
        *header = Make_header(Size(v), GRAY, Tag(v));

        
        for(size_t i = 0 ; i <Size(v); i++){
            mark(Field(v,i));
        }

        *header = Make_header(Size(v), BLACK, Tag(v));
    }
}


void print_heap_state() {
    printf("État du tas:\n");
    mlvalue* current = Caml_state->heap; // Point de départ du tas
    mlvalue* heap_end = current + (Caml_state->heap_size / sizeof(mlvalue)); // Fin du tas

    while (current < heap_end) {
        header_t header = *current; // Récupérez l'en-tête à l'adresse actuelle
        // Vérifiez si l'adresse contient un bloc (et non un entier)
        if (Is_block(header)) {
            color_t color = Color_hd(header); // Obtenez la couleur du bloc
            size_t size = Size_hd(header); // Obtenez la taille du bloc
            tag_t tag = Tag_hd(header); // Obtenez le tag du bloc

            // Affichez les informations du bloc
            printf("Bloc à l'adresse %p: Taille: %lu, Tag: %u, Couleur: %s\n",
                   (void*)current, size, tag,
                   color == WHITE ? "Blanc" :
                   color == GRAY ? "Gris" :
                   color == BLACK ? "Noir" : "Inconnu");

            // Déplacez le pointeur actuel à la fin du bloc pour passer au suivant
            current += size + 1;
        } else {
            // S'il s'agit d'un entier, passez simplement au mot suivant
            current++;
        }
    }
}

void run_gc(){
    // Accédez aux racines
    mlvalue accu = Caml_state->accu; 
    mlvalue* stack = Caml_state->stack;
    size_t sp = Caml_state->sp; 
    mlvalue env = Caml_state->env;

    for (size_t i = 0; i < sp; i++) {
        mark(stack[i]);
    }

    mark(env);
    mark(accu);
    print_heap_state();
}