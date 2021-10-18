/* Librerias */
#include <iostream>
#include <math.h>
#include <string>
#include <fstream>
#include <random>
#include <chrono>
#include <iomanip>
#include <vector>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

/* CONSTANTES */
const double GRAVITY_CONST = 6.674 * pow(10, -11); // Constante gravedad universal
const double M = pow(10, 21);                      // Media (distribución normal)
const double SDM = pow(10, 15);                    // Desviación (distribución normal)

/* ESTRUCTURA OBJETO*/
struct object
{
    double pos_x;
    double pos_y;
    double pos_z;
    double speed_x;
    double speed_y;
    double speed_z;
    double mass;
};

struct vector_elem
{
    double x;
    double y;
    double z;
};
/* Declaración previa de las funciones */
void vector_gravitational_force(object object_1, object object_2, double* forces);
void calc_gravitational(int num_objects, int index_1, object* objects, double* forces);
void vector_acceleration(object object_1, double* forces, vector_elem* acceleration);
void vector_speed(object *object_1, vector_elem *acceleration, float time_step);
void vector_position(object *object_1, float time_step);
void check_border(object *object_1, float size_enclosure);
bool check_collision(object object_1, object object_2);

/* MAIN */
int main(int argc, char const *argv[])
{

    /* Comprobación inicial variables */
    if (argc != 6)
    {
        cerr << "Número de argumentos incorrecto\n";
        return -1;
    }

    /* Comprobación de valores iniciales de variables */
    if ((atoi(argv[1]) <= 0 || atoi(argv[2]) <= 0 || atoi(argv[3]) <= 0 || atof(argv[4]) <= 0.0 || atof(argv[5]) <= 0.0) ||
        (atof(argv[1]) != atoi(argv[1]) || atof(argv[2]) != atoi(argv[2]) || atof(argv[3]) != atoi(argv[3])))
    {
        cerr << "Datos erróneos de los argumentos\n";
        return -2;
    }

    /* Almacenamiento de los argumentos en sus respectivas variables */
    int num_objects = atoi(argv[1]);      // Número de objetos a simular (>0 entero)
    int num_iterations = atoi(argv[2]);   // Número de iteraciones a simular (>0 entero)
    int random_seed = atoi(argv[3]);      // Semilla para distribuciones aleatorias
    float size_enclosure = atof(argv[4]); // Tamaño del recinto (>0 real)
    float time_step = atof(argv[5]);      // Incremento de tiempo en cada iteración (>0 real)

    /* Impresión por pantalla de las variables */
    cout << "num_objects: " << num_objects << "\n";
    cout << "num_iterations: " << num_iterations << "\n";
    cout << "random_seed: " << random_seed << "\n";
    cout << "size_enclosure: " << size_enclosure << "\n";
    cout << "time_step: " << time_step << "\n";

    /* Coordenadas aleatorias */
    mt19937_64 gen(random_seed);
    uniform_real_distribution<double> position_dist(0.0, nextafter(size_enclosure, numeric_limits<double>::max()));
    normal_distribution<double> mass_dist(M, SDM);

    /* AOS - Array of Structs */
    object objects[num_objects];

    /* Fichero de configuracion inicial */
    ofstream file_init;
    file_init.open("init_config.txt");
    file_init << fixed << setprecision(3) << size_enclosure << " " << time_step << " " << num_objects << endl;

    /* Creación de objetos */
    for (int i = 0; i < num_objects; i++)
    {
        // Igual meter variables para ver si mejora rendimiento
        objects[i].pos_x = position_dist(gen);
        objects[i].pos_y = position_dist(gen);
        objects[i].pos_z = position_dist(gen);
        objects[i].speed_x = 0;
        objects[i].speed_y = 0;
        objects[i].speed_z = 0;
        objects[i].mass = mass_dist(gen);

        // Ponemos la precisión a 3 decimales. Imprimimos el objeto
        file_init << fixed << setprecision(3) << objects[i].pos_x << " " << objects[i].pos_y << " " << objects[i].pos_z << " " << objects[i].speed_x << " " << objects[i].speed_y << " " << objects[i].speed_z << " " << objects[i].mass << endl;
    }

    file_init.close(); // Cerramos el fichero

    /* Comprobar que no hay colisiones antes de las iteraciones */

    /* Iteraciones */
    for (int iteration = 0; iteration < num_iterations; iteration++)
    {
        /* Bucle obtener nueva posición de los objetos en la iteración. Comprueba también que no se haya pasado de los límites.*/
        for (int i = 0; i < num_objects; i++){
            if (objects[i].mass!=NULL){
                double forces[3] = {0,0,0};
                calc_gravitational(num_objects, i, objects, forces);

                vector_elem *acceleration;
                vector_acceleration(objects[i], forces, acceleration);
                
                /* Calculamos la velocidad del objeto para obtener la posición */
                vector_speed(&objects[i], acceleration, time_step);

                // Vector posiciones
                vector_position(&objects[i], time_step);

                // Comprobar bordes
                check_border(&objects[i], size_enclosure);
            }
        } 
        //cout<<"Nuevas posiciones calculadas \n";

        /* Bucle anidado para comprobar colisiones entre objetos */
        int num_objects_after_delete = num_objects;
        for (int i = 0; i < num_objects; i++){
            for (int j = 0; j < num_objects-i-1; j++){ 
                // Se resta i y 1 para evitar comprobaciones dobles
                // Comprobar colisiones
                if (i!=j && objects[i].mass!=NULL && objects[j].mass!=NULL){
                    if (check_collision(objects[i], objects[j])){
                        objects[i].mass += objects[j].mass;
                        objects[i].speed_x += objects[j].speed_x;
                        objects[i].speed_y += objects[j].speed_y;
                        objects[i].speed_z += objects[j].speed_z;
                        num_objects_after_delete -= 1;
                        objects[i].mass = NULL;
                        cout<<num_objects_after_delete<<"\n";
                    }
                }
            }
        }
        
        // Cambiar array objects
        int counter = 0;
        while (counter<num_objects_after_delete){
            if (objects[counter].mass==NULL){
                for (int i = 0; i<num_objects-counter-1; i++){
                    objects[i+counter]=objects[i+counter+1];
                    // num_objects = 4; counter = 0 i<3
                    // i=0 objects[0]=objects[1]
                    // i=1 objects[1]=objects[2]
                    // i=2 objects[2]=objects[3]

                    // num_objects = 4; counter = 1 i<2
                    // i=0 objects[1]=objects[2]
                    // i=1 objects[2]=objects[3]

                    // num_objects = 4; counter = 2 i<1
                    // i=0 objects[2]=objects[3]

                    // num_objects = 4; counter = 3 i<0
                }

                num_objects -= 1;
            }else{
                counter++;
            }
        }
        cout<<"Fin iteración: "<<iteration<<" Num objetos:"<<num_objects<<"\n";
    }

}

/* FUNCIONES */
/* Distancia euclídea entre dos objetos */
double euclidean_norm(object object_1, object object_2)
{   
    return std::sqrt(pow(object_1.pos_x - object_2.pos_x, 2) + pow(object_1.pos_y - object_2.pos_y, 2) + pow(object_1.pos_z - object_2.pos_z, 2));
}

/* Fuerza gravitatoria entre dos objetos */
void vector_gravitational_force(object object_1, object object_2, double* forces)
{
    double force_module = (GRAVITY_CONST * object_1.mass * object_2.mass) / (pow(euclidean_norm(object_1, object_2), 3));
    forces[0] += force_module * (object_1.pos_x - object_2.pos_x);
    forces[1] += force_module * (object_1.pos_y - object_2.pos_y);
    forces[2] += force_module * (object_1.pos_z - object_2.pos_z);
}

/* Fuerza gravitatoria de 1 objeto */
void calc_gravitational(int num_objects, int index_1, object* objects, double* forces){
    for (int j=0; j<num_objects; j++){
        if (j!=index_1){
            vector_gravitational_force(objects[j], objects[index_1], forces);
        }
    }
}

void vector_acceleration(object object_1, double* forces, vector_elem* acceleration)
{
    acceleration->x= forces[0]/object_1.mass;
    acceleration->y= forces[1]/object_1.mass;
    acceleration->z= forces[2]/object_1.mass;
}

/* Vector velocidad */
void vector_speed(object *object_1, vector_elem *acceleration, float time_step)
{      
    /* Cálculo del vector velocidad */
    object_1->speed_x = object_1->speed_x + (acceleration->x * time_step);
    object_1->speed_y = object_1->speed_y + (acceleration->y * time_step);
    object_1->speed_z = object_1->speed_z + (acceleration->z * time_step);
}

/* Vector de posicion */
void vector_position(object *object_1, float time_step)
{
    /* Cálculo del vector posición */
    object_1->pos_x = object_1->pos_x + (object_1->speed_x * time_step);
    object_1->pos_y = object_1->pos_y + (object_1->speed_y * time_step);
    object_1->pos_z = object_1->pos_z + (object_1->speed_z * time_step);

}

/* Función para recolocar al objeto si traspasa los límites */
void check_border(object *object_1, float size_enclosure)
{
    /* Checks posición x */
    if (object_1->pos_x <= 0)
    {
        object_1->pos_x = 0;
        object_1->speed_x = -1 * object_1->speed_x;
    }
    else if (object_1->pos_x >= size_enclosure)
    {
        object_1->pos_x = size_enclosure;
        object_1->speed_x = -1 * (object_1->speed_x);
    }

    /* Checks posición y */
    if (object_1->pos_y <= 0)
    {
        object_1->pos_y = 0;
        object_1->speed_y = -1 * (object_1->speed_y);
    }
    else if (object_1->pos_y >= size_enclosure)
    {
        object_1->pos_y = size_enclosure;
        object_1->speed_y = -1 * (object_1->speed_y);
    }

    /* Checks posición z */
    if (object_1->pos_z <= 0)
    {
        object_1->pos_z = 0;
        object_1->speed_z = -1 * object_1->speed_z;
    }
    else if (object_1->pos_z >= size_enclosure)
    {
        object_1->pos_z = size_enclosure;
        object_1->speed_z =  -1 * object_1->speed_z;
    }
}

bool check_collision(object object_1, object object_2)
{
    /* Comprobar si colisionan (distancia euclídea entre 1 y 2 menor que 1) */
    if (euclidean_norm(object_1, object_2) < 1)
    {
        return true;
    }
    return false;
}