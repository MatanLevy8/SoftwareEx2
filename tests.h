#ifndef TESTS_H
#define TESTS_H

#include <stdio.h>
#include "sp_image_proc_util.h"
#include "main_aux.h"
#include <stdlib.h>

void testSetInput(settings*);

void printItems(keyValue* items);

void printMatrix(double ** matrix, int maxNFeautres);

void printFeature(char* string, double* array);

#endif
