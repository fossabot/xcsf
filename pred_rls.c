/*
 * Copyright (C) 2015 Richard Preen <rpreen@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 **************
 * Description: 
 **************
 * The recursive least square classifier computed prediction module.
 */

#ifdef RLS_PREDICTION
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include "random.h"
#include "cons.h"
#include "cl.h"

#define RLS_SCALE_FACTOR 1000.0
#define RLS_LAMBDA 1.0

void matrix_matrix_multiply(double *srca, double *srcb, double *dest, int n);
void matrix_vector_multiply(double *srcm, double *srcv, double *dest, int n);
void init_matrix(double *matrix);

double *tmp_input;
double *tmp_vec;
double *tmp_matrix1;
double *tmp_matrix2;

void pred_init(PRED *pred)
{
#ifdef QUADRATIC
	// offset(1) + n linear + n quadratic + n*(n-1)/2 mixed terms
	pred->weights_length = 1+2*state_length+state_length*(state_length-1)/2;
#else
	pred->weights_length = state_length+1;
#endif
	pred->weights = malloc(sizeof(double) * pred->weights_length);
	pred->weights[0] = XCSF_X0;
	for(int i = 1; i < pred->weights_length; i++)
		pred->weights[i] = 0.0;

	// initialise gain matrix
	pred->matrix = malloc(sizeof(double)*pow(pred->weights_length,2));
	init_matrix(pred->matrix);

	// initialise temporary arrays (only needs to be done once)
	if(tmp_vec == NULL) {
		tmp_vec = malloc(sizeof(double)*pred->weights_length);
		tmp_input = malloc(sizeof(double)*pred->weights_length);
		tmp_matrix1 = malloc(sizeof(double)*pow(pred->weights_length,2));
		tmp_matrix2 = malloc(sizeof(double)*pow(pred->weights_length,2));
	}
}
 	
void init_matrix(double *matrix)
{
	for(int row = 0; row < state_length; row++) {
		for(int col = 0; col < state_length; col++) {
			if(row != col)
				matrix[row*state_length+col] = 0.0;
			else
				matrix[row*state_length+col] = RLS_SCALE_FACTOR;
		}
	}
}
 
void pred_copy(PRED *to, PRED *from)
{
	memcpy(to->weights, from->weights, sizeof(double)*from->weights_length);
	memcpy(to->matrix, from->matrix, sizeof(double)*pow(from->weights_length,2));
}
 
void pred_free(PRED *pred)
{
	free(pred->weights);
	free(pred->matrix);
}
     
void pred_update(PRED *pred, double p, double *state)
{
	tmp_input[0] = XCSF_X0;
	int index = 1;
	// linear terms
	for(int i = 0; i < state_length; i++)
		tmp_input[index++] = state[i];
#ifdef QUADRATIC
	// quadratic terms
	for(int i = 0; i < state_length; i++)
		for(int j = 0; j < state_length; j++)
			tmp_input[index++] = pow(state[i],2);
#endif

	// determine gain vector = matrix * tmp_input
	matrix_vector_multiply(pred->matrix, tmp_input, tmp_vec, pred->weights_length);
	
	// divide gain vector by lambda + tmp_vec
	double divisor = RLS_LAMBDA;
	for(int i = 0; i < pred->weights_length; i++)
		divisor += tmp_input[i] * tmp_vec[i];
	for(int i = 0; i < pred->weights_length; i++)
		tmp_vec[i] /= divisor;

	// update weights using the error
	// pre has been updated for the current state during set_pred()
	double error = p - pred->pre; // pred_compute(c, state);
	for(int i = 0; i < pred->weights_length; i++)
		pred->weights[i] += error * tmp_vec[i];

	// update gain matrix
	for(int i = 0; i < pred->weights_length; i++) {
		for(int j = 0; j < pred->weights_length; j++) {
			double tmp = tmp_vec[i] * tmp_input[j];
			if(i == j)
				tmp_matrix1[i*state_length+j] = 1.0 - tmp;
			else
				tmp_matrix1[i*state_length+j] = -tmp;
		}
	}
	matrix_matrix_multiply(tmp_matrix1, pred->matrix, tmp_matrix2, pred->weights_length);

	// divide gain matrix entries by lambda
	for(int row = 0; row < pred->weights_length; row++) {
		for(int col = 0; col < pred->weights_length; col++) {
			pred->matrix[row*state_length+col] = tmp_matrix2[row*state_length+col] / RLS_LAMBDA;
		}
	}
}

double pred_compute(PRED *pred, double *state)
{
	// first coefficient is offset
	double pre = XCSF_X0 * pred->weights[0];
	int index = 1;
	// multiply linear coefficients with the prediction input
	for(int i = 0; i < state_length; i++)
		pre += pred->weights[index++] * state[i];
#ifdef QUADRATIC
	// multiply quadratic coefficients with prediction input
	for(int i = 0; i < state_length; i++) {
		for(int j = i; j < state_length; j++) {
			pre += pred->weights[index++] * state[i] * state[j];
		}
	}
#endif
	pred->pre = pre;
	return pre;
} 

void pred_print(PRED *pred)
{
	printf("weights: ");
	for(int i = 0; i < pred->weights_length; i++)
		printf("%f, ", pred->weights[i]);
	printf("\n");
}

void matrix_matrix_multiply(double *srca, double *srcb, double *dest, int n)
{
	for(int i = 0; i < n; i++) {
		for(int j = 0; j < n; j++) {
			dest[i*state_length+j] = srca[i*state_length] * srcb[j];
			for(int k = 0; k < n; k++) {
				dest[i*state_length+j] += srca[i*state_length+k] * srcb[k*state_length+j];
			}
		}
	}
}

void matrix_vector_multiply(double *srcm, double *srcv, double *dest, int n)
{
	for(int i = 0; i < n; i++) {
		dest[i] = srcm[i*state_length] * srcv[0];
		for(int j = 1; j < n; j++) {
			dest[i] += srcm[i*state_length+j] * srcv[j];
		}
	}
}
#endif