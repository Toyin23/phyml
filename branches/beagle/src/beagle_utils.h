/*
 * author: Imran Fanaswala
 */

#ifndef BEAGLE_UTILS_H
#define BEAGLE_UTILS_H

#include "assert.h"
#include "libhmsbeagle-1/libhmsbeagle/beagle.h"
#include "utilities.h"
#define UNINITIALIZED -42

int  create_beagle_instance(t_tree* tree, int quiet);
int  finalize_beagle_instance(t_tree* tree);
void update_beagle_partials(t_tree* tree, t_edge* b, t_node* d, bool scale);
double* int_to_double(const int* src, int num_elems);
double* short_to_double(const short* src, int num_elems);
double* float_to_double(const phydbl *src, int num_elems);



#endif // BEAGLE_UTILS_H
