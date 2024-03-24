# Projet_CA_GC
# Garbage Collector Mark & Compact pour Mini-ZAM

Ce projet implémente un Garbage Collector (GC) de type Mark & Compact pour Mini-ZAM, une machine virtuelle simplifiée inspirée de la ZINC Abstract Machine (ZAM) utilisée par OCaml. Ce GC vise à optimiser la gestion de la mémoire en minimisant la fragmentation, améliorant ainsi les performances et l'utilisation de la mémoire des programmes exécutés sur Mini-ZAM.

## Fonctionnalités

- **Marquage des Objets Accessibles** : Identifie et marque tous les objets accessibles à partir des racines pour prévenir leur collecte.
- **Compactage de la Mémoire** : Déplace les objets marqués dans une zone contiguë de la mémoire, réduisant la fragmentation.
- **Gestion Dynamique du Tas** : Ajuste la taille du tas en fonction des besoins d'allocation de mémoire des programmes. (a implémenter)

## Dépendances

Ce projet est écrit en C et nécessite un compilateur C compatible avec la norme C99 (par exemple, GCC ou Clang) et `make` pour la compilation.

## Installation

1. **Cloner le dépôt** : `git clone https://example.com/projet-ca-gc-mini-zam.git`
2. **Compiler le projet** :
   ```bash
   cd projet-ca-gc-mini-zam
   make

## Tests
Exécuter un test : ./minizam tests/exemple_test.txt -res

Exécuter tous les tests : make test
