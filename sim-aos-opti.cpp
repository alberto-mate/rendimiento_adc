/* Librerias */
#include <iostream>
#include <math.h>
#include <fstream>
#include <random>
#include <vector>
#include <iomanip>
#include <opencv2/opencv.hpp>
using namespace std;

/* CONSTANTES */
const double GRAVITY_CONST = 6.674 * 1E-11; // Constante gravedad universal
const double M = 1E21;                      // Media (distribución normal)
const double SDM = 1E15;                    // Desviación (distribución normal)

/* ESTRUCTURAS */
/* Estructura objeto */
struct object {
    double pos_x;
    double pos_y;
    double pos_z;
    double speed_x;
    double speed_y;
    double speed_z;
    double mass;
    int id;
};

/* Estructura vector_elem */
struct vector_elem {
    double x;
    double y;
    double z;
};

/* DECLARACIÓN PREVIA DE FUNCIONES */
void vector_gravitational_force(object object_1, object object_2, double *forces);
void calc_gravitational(int num_objects, int index_1, vector<object> &objects, double *forces);
void vector_acceleration(object object_1, double *forces, vector_elem *acceleration);
void vector_speed(object *object_1, vector_elem *acceleration, double time_step);
void vector_position(object *object_1, double time_step);
void check_border(object *object_1, double size_enclosure);
bool check_collision(object object_1, object object_2);

/* MAIN */
int main(int argc, char const *argv[]) {
    /* Comprobación número inicial argumentos */
    if (argc != 6){
        cerr << "Número de argumentos incorrecto\n";
        // Print argc
        cerr << "Uso: ./sim-aos-opti <num_objects> <num_iterations> <random_seed> <size_enclosure> <time_step>\n";
        return -1;
    }

    /* Comprobación de valores iniciales de argumentos */
    if ((atoi(argv[1]) <= 0 || atoi(argv[2]) <= 0 || atoi(argv[3]) <= 0 || atof(argv[4]) <= 0.0 || atof(argv[5]) <= 0.0) || (atof(argv[1]) != atoi(argv[1]) || atof(argv[2]) != atoi(argv[2]) || atof(argv[3]) != atoi(argv[3]))) {
        cerr << "Datos erróneos de los argumentos\n";
        return -2;
    }

    /* Almacenamiento de los argumentos en sus respectivas variables */
    int num_objects = atoi(argv[1]);       // Número de objetos a simular (>0 entero)
    int num_iterations = atoi(argv[2]);    // Número de iteraciones a simular (>0 entero)
    int random_seed = atoi(argv[3]);       // Semilla para distribuciones aleatorias
    double size_enclosure = stod(argv[4]); // Tamaño del recinto (>0 real)
    double time_step = stod(argv[5]);      // Incremento de tiempo en cada iteración (>0 real)

    /* Coordenadas y masas pseudoaleatorias */
    mt19937_64 gen(random_seed);
    uniform_real_distribution<> position_dist(0.0, size_enclosure);
    float scale_factor = size_enclosure/1000.0;
    cout << "Scale factor: " << scale_factor << endl;
    normal_distribution<> mass_dist{M, SDM};

    /* Definición de colores*/
    std::unordered_map<int, cv::Scalar> color_map;
    color_map[0] = cv::Scalar(0, 0, 255);    // Blue for point with ID 1
    color_map[1] = cv::Scalar(0, 255, 0);    // Green for point with ID 2
    color_map[2] = cv::Scalar(255, 0, 0);    // Red for point with ID 3
    color_map[3] = cv::Scalar(0, 128, 255);  // Yellow for point with ID 4
    color_map[4] = cv::Scalar(255, 0, 255);  // Magenta for point with ID 5

    /* AOS - Array of Structs */
    vector<object> objects(num_objects);

    /* Fichero de configuracion inicial */
    ofstream file_init;
    file_init.open("init_config.txt");
    file_init << fixed << setprecision(3) << size_enclosure << " " << time_step << " " << num_objects << endl;

    /* Creación de objetos */
    for (int i = 0; i < num_objects; i++) {
        objects[i].pos_x = position_dist(gen); // Posicion x
        objects[i].pos_y = position_dist(gen); // Posicion y
        objects[i].pos_z = position_dist(gen); // Posicion z
        objects[i].mass = mass_dist(gen); // Masa
        objects[i].id = i; // Identificador

        // Ponemos la precisión a 3 decimales. Imprimimos el objeto
        file_init << fixed << setprecision(3) << objects[i].pos_x << " " << objects[i].pos_y << " " << objects[i].pos_z << " " << objects[i].speed_x << " " << objects[i].speed_y << " " << objects[i].speed_z << " " << objects[i].mass << endl;
    }

    file_init.close(); // Cerramos el fichero "init_config.txt"

    /* Bucle anidado para comprobar colisiones entre objetos previas a las iteraciones */
    for (long unsigned int i = 0; i < objects.size(); i++) {
        for (long unsigned int j = i + 1; j < objects.size(); j++) {   
            // Comprobar colisiones
            // Colision entre objetos diferentes que no hayan sido eliminados con anterioridad
            if (check_collision(objects[i], objects[j])) {   
                // Actualización de la masa y velocidades del primer objeto que colisiona generando uno nuevo
                objects[i].mass += objects[j].mass;
                objects[i].speed_x += objects[j].speed_x;
                objects[i].speed_y += objects[j].speed_y;
                objects[i].speed_z += objects[j].speed_z;

                // Eliminamos el objeto del vector
                objects.erase(objects.begin() + j);
                j--;
            }
        }
    }
    cv::namedWindow("Object Positions", cv::WINDOW_NORMAL);
    cv::Mat canvas(size_enclosure/scale_factor, size_enclosure/scale_factor, CV_8UC3, cv::Scalar(255, 255, 255));  // Create a black canvas of size 'size_enclosure'


    /* Iteraciones */
    for (int iteration = 0; iteration < num_iterations; iteration++) {
        
        /* Bucle para obtener nuevas propiedades de los objetos en la iteración (fuerzas, aceleración y velocidad) */
        for (int i = 0; i < num_objects; i++) {
            // Solo entrarán en el condicional objetos que no se han eliminado
            // Cálculo de la fuerza gravitatoria
            double forces[3] = {0.0, 0.0, 0.0};
            calc_gravitational(num_objects, i, objects, forces);
            // cout << "Forces " << i << " ax: " << forces[0] << " ay: " << forces[1] << " az: " << forces[2] << "\n";
            // Cálculo del vector aceleración
            vector_elem *acceleration = (vector_elem *)malloc(sizeof(vector_elem));
            vector_acceleration(objects[i], forces, acceleration);
            //  Cálculo del vector velocidad
            vector_speed(&objects[i], acceleration, time_step);
        }

        /* Bucle para calcular posiciones y comprobar bordes */
        for (int i = 0; i < num_objects; i++) {
            // Cálculo del vector posiciones
            vector_position(&objects[i], time_step);
            //  Comprobar bordes
            check_border(&objects[i], size_enclosure);
        }

        /* Bucle anidado para comprobar colisiones entre objetos */
        for (long unsigned int i = 0; i < objects.size(); i++) {
            for (long unsigned int j = i + 1; j < objects.size(); j++) {   
                // Comprobar colisiones
                // Colision entre objetos diferentes que no hayan sido eliminados con anterioridad
                if (check_collision(objects[i], objects[j])) {   
                    // Actualización de la masa y velocidades del primer objeto que colisiona generando uno nuevo
                    objects[i].mass += objects[j].mass;
                    objects[i].speed_x += objects[j].speed_x;
                    objects[i].speed_y += objects[j].speed_y;
                    objects[i].speed_z += objects[j].speed_z;

                    // Eliminamos el objeto del vector
                    objects.erase(objects.begin() + j);
                    j--;
                }
            }
        }

        /* Limpia el canvas*/
        canvas.setTo(cv::Scalar(255, 255, 255));
        /* Obtiene el maximo y minimo de la masa de todos los objetos*/
        float max_mass = 0;
        float min_mass = 1E25;
        for (int i = 0; i < num_objects; i++) {
            if (objects[i].mass > max_mass) max_mass = objects[i].mass;
            if (objects[i].mass < min_mass) min_mass = objects[i].mass;
        }

        /* Bucle para pintar los objetos en el canvas */
        for (int i = 0; i < num_objects; i++) {
            // Calculate the position of the object on the canvas
            int x = static_cast<int>(objects[i].pos_x/scale_factor);
            int y = static_cast<int>(objects[i].pos_y/scale_factor);

            /* En funcion el max_mass y min_mass normaliza los valores para el radio*/
            int radius = static_cast<int>(((objects[i].mass - min_mass) / (max_mass - min_mass))*8 + 5);

            // Draw a circle representing the object on the canvas
            int color_index = objects[i].id % color_map.size();
            cv::circle(canvas, cv::Point(x, y), radius, color_map[color_index], -1);
        }
        
        // Show the canvas in the OpenCV window
        cv::imshow("Object Positions", canvas);
        // Wait for a key press and store the pressed key
        int key = cv::waitKey(10);
        // Check if the pressed key is 'Esc' (ASCII code 27)
        if (key == 27) {
            std::cout << "Esc key pressed. Exiting..." << std::endl;
            return 0;
        }
        /*
        // Save the frame as an image
        // Create file name with leading zeros
        std::ostringstream oss;
        oss << std::setw(4) << std::setfill('0') << iteration;
        std::string filename = "gif/frames/frame_" + oss.str() + ".png";
        cv::imwrite(filename, canvas);
        */

        // Actualizamos el número de objetos en el vector
        num_objects = objects.size();
        //cout << "Fin iteración: " << iteration << " Num objetos:" << num_objects << "\n";
    }

    /* Escribimos en el archivo "final_config.txt" los parámetros finales */
    ofstream file_final;
    file_final.open("final_config.txt");
    file_final << fixed << setprecision(3) << size_enclosure << " " << time_step << " " << num_objects << endl;

    for (int i = 0; i < num_objects; i++) {
        file_final << fixed << setprecision(3) << objects[i].pos_x << " " << objects[i].pos_y << " " << objects[i].pos_z << " " << objects[i].speed_x << " " << objects[i].speed_y << " " << objects[i].speed_z << " " << objects[i].mass << endl;
    }

    file_init.close(); // Cerramos el fichero "final_config.txt"
    cv::destroyWindow("Object Positions");
}

/* FUNCIONES */
/* Distancia euclídea entre dos objetos */
double euclidean_norm(object object_1, object object_2) {
    return std::sqrt((object_1.pos_x - object_2.pos_x) * (object_1.pos_x - object_2.pos_x) + (object_1.pos_y - object_2.pos_y) * (object_1.pos_y - object_2.pos_y) + (object_1.pos_z - object_2.pos_z) * (object_1.pos_z - object_2.pos_z));
}

/* Fuerza gravitatoria entre dos objetos */
void vector_gravitational_force(object object_1, object object_2, double *forces) {
    double dist = euclidean_norm(object_1, object_2);
    double Fg = GRAVITY_CONST * object_1.mass * object_2.mass/ (dist*dist*dist);
    forces[0] += (Fg * (object_1.pos_x - object_2.pos_x));
    forces[1] += (Fg * (object_1.pos_y - object_2.pos_y));
    forces[2] += (Fg * (object_1.pos_z - object_2.pos_z));
}

/* Fuerza gravitatoria que ejerce un objeto */
void calc_gravitational(int num_objects, int i, std::vector<object> &objects, double *forces) {
    for (int j = 0; j < num_objects; j++) {
        if (j != i) {
            vector_gravitational_force(objects[j], objects[i], forces);
        }
    }
}

/* Vector aceleración */
void vector_acceleration(object object_1, double *forces, vector_elem *acceleration) {
    acceleration->x = forces[0] / object_1.mass;
    acceleration->y = forces[1] / object_1.mass;
    acceleration->z = forces[2] / object_1.mass;

}

/* Vector velocidad */
void vector_speed(object *object_1, vector_elem *acceleration, double time_step) {
    /* Cálculo del vector velocidad */
    object_1->speed_x += (acceleration->x * time_step);
    object_1->speed_y += (acceleration->y * time_step);
    object_1->speed_z += (acceleration->z * time_step);
}

/* Vector de posicion */
void vector_position(object *object_1, double time_step) {
    /* Cálculo del vector posición */
    object_1->pos_x += (object_1->speed_x * time_step);
    object_1->pos_y += (object_1->speed_y * time_step);
    object_1->pos_z += (object_1->speed_z * time_step);
}

/* Función para recolocar al objeto si traspasa los límites */
void check_border(object *object_1, double size_enclosure) {
    // Checks posición x
    if (object_1->pos_x <= 0) { 
        object_1->pos_x = 0;
        object_1->speed_x = -1 * (object_1->speed_x);
    } else if (object_1->pos_x >= size_enclosure) {
        object_1->pos_x = size_enclosure;
        object_1->speed_x = -1 * (object_1->speed_x);
    }

    // Checks posición y
    if (object_1->pos_y <= 0) {
        object_1->pos_y = 0;
        object_1->speed_y = -1 * (object_1->speed_y);
    } else if (object_1->pos_y >= size_enclosure) {
        object_1->pos_y = size_enclosure;
        object_1->speed_y = -1 * (object_1->speed_y);
    }

    // Checks posición z
    if (object_1->pos_z <= 0) {
        object_1->pos_z = 0;
        object_1->speed_z = -1 * (object_1->speed_z);
    } else if (object_1->pos_z >= size_enclosure) {
        object_1->pos_z = size_enclosure;
        object_1->speed_z = -1 * (object_1->speed_z);
    }
}

/* Comprobar colisión entre dos objetos (distancia euclídea entre objetos menor que 1) */
bool check_collision(object object_1, object object_2) {
    if (euclidean_norm(object_1, object_2) < 1) {
        return true;
    }
    return false;
}
