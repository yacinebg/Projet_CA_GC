#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "domain_state.h"
#include "gc.h"
#include "config.h"

#define MLVALUE_NULL ((mlvalue)(-1)) // Valeur NULL pour les mlvalues, pas une adresse valide

typedef struct {
    mlvalue* old_address;
    mlvalue* new_address;
} address_mapping;

address_mapping* address_map;
size_t address_map_size; 

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


mlvalue new_address_of(mlvalue* old_address) {
    // Calculez la nouvelle adresse en fonction de la compaction effectuée
    for (size_t i = 0; i < address_map_size; i++) {
        if (address_map[i].old_address == old_address) {
            return *address_map[i].new_address;
        }
    }
    return MLVALUE_NULL; // Retourne MLVALUE_NULL si l'ancienne adresse n'est pas trouvée.
}

//mettre à jours les pointeurs
void update_all_pointers() {
    mlvalue* current = Caml_state->heap;

    // Vérifier que le pointeur courant ne dépasse jamais next_alloc
    while (current < next_alloc) {
        // Récupération de l'en-tête à l'emplacement actuel
        header_t header = *current;
        if (Is_block(header) && Color_hd(header) == BLACK) {
            // Taille de l'objet pointé par current
            size_t size = Size_hd(header);

            // Itération sur chaque champ de l'objet
            for (size_t i = 0; i < size; i++) {
                mlvalue* field_address = &Field(current, i);

                // Vérification que le champ est bien un bloc avant de continuer
                if (Is_block(*field_address)) {
                    // Récupération de l'adresse actuelle du champ
                    mlvalue field = *field_address;
                    mlvalue* field_ptr = Ptr_val(field);

                    // Assurer que field_ptr est dans les limites du tas
                    if (field_ptr >= Caml_state->heap && field_ptr < next_alloc) {
                        mlvalue new_address = new_address_of(field_ptr);
                        if (new_address != (mlvalue)NULL) {
                            *field_address = new_address;
                        }
                    }
                }
            }
            // Avancer au bloc suivant
            current += size + 1;
        } else {
            // Avancer d'un mot mémoire si ce n'est pas un bloc
            current += 1;
        }
    }
}



void compact() {
    mlvalue* from = Caml_state->heap;
    mlvalue* to = Caml_state->heap;
    
    
    // Initialisez le mappage d'adresses
    size_t map_capacity = Caml_state->heap_size;
    address_map = malloc(map_capacity * sizeof(address_mapping) * 10);
    size_t map_index = 0;


    // Premier parcours pour calculer les nouvelles adresses
    while (from < next_alloc) {
        header_t header = *((header_t*)from);
        if (Color_hd(header) == BLACK) {
            size_t size = Size_hd(header);

            // Assurez-vous que le mappage d'adresses a suffisamment de capacité
            if (map_index >= map_capacity) {
                // Redimensionnez address_map si nécessaire
                map_capacity *= 2;
                address_map = realloc(address_map, map_capacity * sizeof(address_mapping));
            }

            // Mettez à jour address_map avec les adresses anciennes et nouvelles
            address_map[map_index].old_address = from;
            address_map[map_index].new_address = to;
            map_index++;

            from += size + 1;
            to += size + 1;
        } else {
            from += Size_hd(header) + 1;
        }
    }

    // Deuxième parcours pour mettre à jour les pointeurs
    update_all_pointers();

    // Troisième parcours pour déplacer les objets
    from = Caml_state->heap;
    to = Caml_state->heap;
    while (from < next_alloc) {
        header_t header = *((header_t*)from);
        if (Color_hd(header) == BLACK) {
            size_t size = Size_hd(header);
            if (from != to) {
                memcpy(to, from, (size + 1) * sizeof(mlvalue));
            }
            to += size + 1;
        }
        from += Size_hd(header) + 1;
    }
    next_alloc = to;

    // Libérez le mappage d'adresses à la fin de la compaction
    free(address_map);
    address_map = NULL;
}

void print_heap_state() {
    printf("État du tas:\n");
    mlvalue* current = Caml_state->heap; // Point de départ du tas

    while (current < next_alloc) {
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
void reset_colors() {
    mlvalue* current = Caml_state->heap;
    while (current < next_alloc) {
        header_t header = *current;
        if (Is_block(header)) { // S'assurer que c'est bien un bloc
            size_t size = Size_hd(header);
            color_t color = Color_hd(header);
            if (color == BLACK) { // Si l'objet est marqué noir
                *current = Make_header(size, WHITE, Tag_hd(header)); // Remettre à blanc
            }
            current += size + 1; // Passer à l'objet suivant
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
    
    //Etape 1 : marquage 
    for (size_t i = 0; i < sp; i++) {
        mark(stack[i]);
    }

    mark(env);
    mark(accu);
    

    //Etape 2 : Compacter 
    compact();
    printf("gc fini\n");
    reset_colors();
    //print_heap_state();
}