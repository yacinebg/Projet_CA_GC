#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "domain_state.h"
#include "interp.h"
#include "config.h"
#include "mlvalues.h"
#include "instruct.h"
#include "alloc.h"
#include "primitives.h"

#include "gc.c"

/* Helpers to manipulate the stack. Note that |sp| always point to the
   first empty element in the stack; hence the prefix -- in POP, but
   postfix ++ in PUSH. */
#define POP_STACK() stack[--sp]
#define PUSH_STACK(x) stack[sp++] = x



mlvalue caml_interprete(code_t* prog) {

  mlvalue* stack = Caml_state->stack;
  mlvalue accu = Val_long(0);
  mlvalue env = Make_empty_env();

  register unsigned int sp = 0;
  register unsigned int pc = 0;
  unsigned int extra_args = 0;
  unsigned int trap_sp = 0;

  while(1) {

#ifdef DEBUG
      printf("pc=%d  accu=%s  sp=%d extra_args=%d trap_sp=%d stack=[",
             pc, val_to_str(accu), sp, extra_args, trap_sp);
      if (sp > 0) {
        printf("%s", val_to_str(stack[sp-1]));
      }
      for (int i = sp-2; i >= 0; i--) {
        printf(";%s", val_to_str(stack[i]));
      }
      printf("]  env=%s\n", val_to_str(env));
      print_instr(prog, pc);
#endif
    // on va sauvegarder l'etat de la pile, accu et de l'env à chaque instruction
    switch (prog[pc++]) {
    case CONST:
      accu = Val_long(prog[pc++]);
      Caml_state->accu = accu; 
      break;

    case PRIM:
      switch (prog[pc++]) {
      case ADD:   accu = ml_add(accu, POP_STACK()); break;
      case SUB:   accu = ml_sub(accu, POP_STACK()); break;
      case DIV:   accu = ml_div(accu, POP_STACK()); break;
      case MUL:   accu = ml_mul(accu, POP_STACK()); break;
      case OR:    accu = ml_or(accu, POP_STACK()); break;
      case AND:   accu = ml_and(accu, POP_STACK()); break;
      case NOT:   accu = ml_not(accu); break;
      case NE:    accu = ml_ne(accu, POP_STACK()); break;
      case EQ:    accu = ml_eq(accu, POP_STACK()); break;
      case LT:    accu = ml_lt(accu, POP_STACK()); break;
      case LE:    accu = ml_le(accu, POP_STACK()); break;
      case GT:    accu = ml_gt(accu, POP_STACK()); break;
      case GE:    accu = ml_ge(accu, POP_STACK()); break;
      case PRINT: accu = ml_print(accu); break;
      }
      Caml_state->accu = accu;
      // La pile est modifiée seulement pour certaines opérations, donc nous mettons à jour `sp` ici.
      Caml_state->sp = sp;
      break;

    case BRANCH:
      pc = prog[pc];
      break;

    case BRANCHIFNOT:
      if (Long_val(accu) == 0) {
        pc = prog[pc];
      } else {
        pc++;
      }
      break;

    case PUSH:
      PUSH_STACK(accu);
      Caml_state->sp = sp;
      break;

    case POP:
      POP_STACK();
      Caml_state->sp = sp;
      break;

    case ACC:
      accu = stack[sp-prog[pc++]-1];
      Caml_state->accu = accu;
      break;

    case ENVACC:
      accu = Field(env, prog[pc++]);
      Caml_state->accu = accu;
      break;

    case APPLY: {
      uint64_t n = prog[pc++];
      // Incrémentez d'abord sp pour faire de la place pour env, pc et extra_args.
      sp += 3;

      // Décaler les arguments n vers le haut dans la pile pour faire de la place.
      for (uint64_t i = n; i > 0; i--) {
          stack[sp - i] = stack[sp - i - 3];
      }

      // Insérez env, pc actuel, et extra_args dans les espaces libérés.
      stack[sp - n - 3] = env;              // L'environnement actuel
      stack[sp - n - 2] = Val_long(pc);     // Le pointeur de code actuel (pc sera modifié juste après)
      stack[sp - n - 1] = Val_long(extra_args); // Les arguments supplémentaires actuels

      // Mettez à jour pc, env, et extra_args pour l'appel de la fonction.
      pc = Addr_closure(accu);        // Pointeur de code de la fermeture dans accu
      env = Env_closure(accu);        // Environnement de la fermeture
      extra_args = n-1; 
      break;
    }

    case APPTERM: {
      uint64_t n = prog[pc++];
      uint64_t m = prog[pc++];

      // Pas besoin de tampon temporaire. Déplacez directement les `n` arguments.
      for (uint64_t i = 0; i < n; i++) {
        stack[sp - m + i] = stack[sp - n + i];
      }

      // Ajuster sp pour refléter le retrait des `m-n` éléments et la reposition des arguments.
      sp -= (m - n);

      // Mettez à jour pc, env, et extra_args pour l'appel de la fonction appelée.
      pc = Addr_closure(accu);
      env = Env_closure(accu);
      extra_args = n - 1; // Préparez extra_args pour la nouvelle fonction.
      break;
    }


    case RETURN: {
      uint64_t n = prog[pc++];
      for (uint64_t i = 0; i < n; i++) {
        POP_STACK();
      }
      if (extra_args == 0) {
        extra_args = Long_val(POP_STACK());
        pc  = Long_val(POP_STACK());
        env = POP_STACK();
      } else {
        extra_args--;
        pc = Addr_closure(accu);
        env = Env_closure(accu);
      }
      Caml_state->sp = sp;
      Caml_state->env = env;
      break;
    }

    case RESTART: {
      unsigned int n = Size(env);
      for (unsigned int i = n-1; i > 0; i--) {
        PUSH_STACK(Field(env,i));
      }
      env = Field(env,0);
      Caml_state->env = env;
      extra_args += n-1;
      break;
    }

    case GRAB: {
      uint64_t n = prog[pc++];
      if (extra_args >= n) {
        extra_args -= n;
      } else {
        mlvalue closure_env = Make_env(extra_args + 2);
        Field(closure_env,0) = env;
        for (unsigned int i = 0; i <= extra_args; i++) {
          Field(closure_env,i+1) = POP_STACK();
        }
        accu = make_closure(pc-3,closure_env);
        extra_args = Long_val(POP_STACK());
        pc  = Long_val(POP_STACK());
        env = POP_STACK();
        Caml_state->sp = sp;
      }
      Caml_state->accu = accu;
      Caml_state->env = env;
      break;
    }

    case CLOSURE: {
      uint64_t addr = prog[pc++];
      uint64_t n = prog[pc++];
      if (n > 0) {
        PUSH_STACK(accu);
      }
      mlvalue closure_env = Make_env(n+1);
      Field(closure_env,0) = Val_long(addr);
      for (uint64_t i = 0; i < n; i++) {
        Field(closure_env,i+1) = POP_STACK();
      }
      accu = make_closure(addr,closure_env);
      Caml_state->accu = accu;
      Caml_state->sp = sp;
      break;
    }

    case CLOSUREREC: {
      uint64_t addr = prog[pc++];
      uint64_t n = prog[pc++];
      if (n > 0) {
        PUSH_STACK(accu);
      }
      mlvalue closure_env = Make_env(n+1);
      Field(closure_env,0) = Val_long(addr);
      for (uint64_t i = 0; i < n; i++) {
        Field(closure_env,i+1) = POP_STACK();
      }
      accu = make_closure(addr,closure_env);
      PUSH_STACK(accu);
      Caml_state->accu = accu;
      Caml_state->sp = sp;
      break;
    }

    case OFFSETCLOSURE: {
      accu = make_closure(Long_val(Field(env,0)), env);
      Caml_state->accu = accu;
      break;
    }

    case MAKEBLOCK: {
      uint64_t n = prog[pc++];
      mlvalue blk = make_block(n,BLOCK_T);
      if (n > 0) {
        Field(blk,0) = accu;
        for (unsigned int i = 1; i < n; i++) {
          Field(blk, i) = POP_STACK();
        }
      }
      accu = blk;
      Caml_state->accu = accu;
      Caml_state->sp = sp;
      break;
    }

    case GETFIELD: {
      uint64_t n = prog[pc++];
      accu = Field(accu, n);
      Caml_state->accu = accu;
      break;
    }

    case VECTLENGTH: {
      accu = Val_long(Size(accu));
      Caml_state->accu = accu;
      break;
    }

    case GETVECTITEM: {
      uint64_t n = Long_val(POP_STACK());
      accu = Field(accu, n);
      Caml_state->accu = accu;
      Caml_state->sp = sp;

      break;
    }

    case SETFIELD: {
      uint64_t n = prog[pc++];
      Field(accu, n) = POP_STACK();
      Caml_state->accu = accu;
      Caml_state->sp = sp;
      break;
    }

    case SETVECTITEM: {
      uint64_t n = Long_val(POP_STACK());
      mlvalue v = POP_STACK();
      Field(accu, n) = v;
      accu = Unit;
      Caml_state->accu = accu;
      Caml_state->sp = sp;
      break;
    }

    case ASSIGN: {
      uint64_t n = prog[pc++];
      stack[sp-n-1] = accu;
      accu = Unit;
      Caml_state->accu = accu;
      break;
    }

    case PUSHTRAP: {
      uint64_t addr = prog[pc++];
      PUSH_STACK(Val_long(extra_args));
      PUSH_STACK(env);
      PUSH_STACK(Val_long(trap_sp));
      PUSH_STACK(Val_long(addr));
      trap_sp = sp;
      Caml_state->sp = sp;
      break;
    }

    case POPTRAP: {
      POP_STACK(); // popping pc
      trap_sp = Long_val(POP_STACK());
      POP_STACK(); // popping env
      POP_STACK(); // popping extra_args
      Caml_state->sp = sp;
      break;
    }

    case RAISE: {
      if (trap_sp == 0) {
        fprintf(stderr, "Uncaught exception: %s\n", val_to_str(accu));
        exit(EXIT_FAILURE);
      } else {
        sp = trap_sp;
        pc = Long_val(POP_STACK());
        trap_sp = Long_val(POP_STACK());
        env = POP_STACK();
        extra_args = Long_val(POP_STACK());
        
        Caml_state->sp = sp;
        Caml_state->env = env;
        break;
      }
    }

    case STOP:
      return accu;

    default:
      fprintf(stderr, "Unkown bytecode: %lu at offset %d\n", prog[pc-1], pc-1);
      exit(EXIT_FAILURE);
    }
  }

}
