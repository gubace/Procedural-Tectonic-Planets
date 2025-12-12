https://hal.science/hal-02136820/file/2019-Procedural-Tectonic-Planets.pdf

https://www.youtube.com/watch?v=GJQVl6Xld0w

# Projet c++ Cmake OpenGL

Pour lancer le projet :

mkdir build && cd build && cmake..

make -j

./Projet3D

Ensuite la sumulation se controle avec les touches du clavier.

'h' pour help 

ou cf la documentation pour la liste des touches.

Dans main.cpp tout en haut les variables globales:
int nbPlates = 10;
int nbiter_resample = 15;
int spherepoints = 2048 * 24;

Controlent le nombre de plaques techtoniques.
Le nombre d'iterations avant le remeshing.
Le nombre de samplePoins sur la planete.

