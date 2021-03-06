/*
 * Copyright (C) 2015--2019 Richard Preen <rpreen@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
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
 * The normalised least mean square classifier computed prediction module.
 *
 * Creates a weight vector representing a polynomial function to compute the
 * expected value given a problem instance and adapts the weights using the
 * least mean square update (also known as the modified Delta rule, or
 * Widrow-Hoff update.)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include "data_structures.h"
#include "random.h"
#include "cl.h"
#include "pred_nlms.h"

typedef struct PRED_NLMS {
	int weights_length;
	double **weights;
	double *pre;
} PRED_NLMS;

void pred_nlms_init(XCSF *xcsf, CL *c)
{
	PRED_NLMS *pred = malloc(sizeof(PRED_NLMS));
	c->pred = pred;

	if(xcsf->PRED_TYPE == 1) {
		// offset(1) + n linear + n quadratic + n*(n-1)/2 mixed terms
		pred->weights_length = 1 + 2 * xcsf->num_x_vars + 
			xcsf->num_x_vars * (xcsf->num_x_vars - 1) / 2;
	}
	else {
		pred->weights_length = xcsf->num_x_vars+1;
	}

	pred->weights = malloc(sizeof(double*) * xcsf->num_y_vars);
	for(int var = 0; var < xcsf->num_y_vars; var++) {
		pred->weights[var] = malloc(sizeof(double)*pred->weights_length);
	}
	for(int var = 0; var < xcsf->num_y_vars; var++) {
		pred->weights[var][0] = xcsf->XCSF_X0;
		for(int i = 1; i < pred->weights_length; i++) {
			pred->weights[var][i] = 0.0;
		}
	}

	pred->pre = malloc(sizeof(double) * xcsf->num_y_vars);
}

void pred_nlms_copy(XCSF *xcsf, CL *to, CL *from)
{
	PRED_NLMS *to_pred = to->pred;
	PRED_NLMS *from_pred = from->pred;
	for(int var = 0; var < xcsf->num_y_vars; var++) {
		memcpy(to_pred->weights[var], from_pred->weights[var], 
				sizeof(double)*from_pred->weights_length);
	}
	memcpy(to_pred->pre, from_pred->pre, sizeof(double) * xcsf->num_y_vars);
}

void pred_nlms_free(XCSF *xcsf, CL *c)
{
	PRED_NLMS *pred = c->pred;
	for(int var = 0; var < xcsf->num_y_vars; var++) {
		free(pred->weights[var]);
	}
	free(pred->weights);
	free(pred->pre);
	free(pred);
}

void pred_nlms_update(XCSF *xcsf, CL *c, double *y, double *x)
{
	PRED_NLMS *pred = c->pred;

	double norm = xcsf->XCSF_X0 * xcsf->XCSF_X0;
	for(int i = 0; i < xcsf->num_x_vars; i++) {
		norm += x[i] * x[i];
	}      

	// pre has been updated for the current state during set_pred()
	for(int var = 0; var < xcsf->num_y_vars; var++) {
		double error = y[var] - pred->pre[var]; // pred_nlms_compute(c, x);
		double correction = (xcsf->XCSF_ETA * error) / norm;
		// update first coefficient
		pred->weights[var][0] += xcsf->XCSF_X0 * correction;
		int index = 1;
		// update linear coefficients
		for(int i = 0; i < xcsf->num_x_vars; i++) {
			pred->weights[var][index++] += correction * x[i];
		}

		if(xcsf->PRED_TYPE == 1) {
			// update quadratic coefficients
			for(int i = 0; i < xcsf->num_x_vars; i++) {
				for(int j = i; j < xcsf->num_x_vars; j++) {
					pred->weights[var][index++] += correction * x[i] * x[j];
				}
			}
		}
	}
}

double *pred_nlms_compute(XCSF *xcsf, CL *c, double *x)
{
	PRED_NLMS *pred = c->pred;
	for(int var = 0; var < xcsf->num_y_vars; var++) {
		// first coefficient is offset
		double pre = xcsf->XCSF_X0 * pred->weights[var][0];
		int index = 1;
		// multiply linear coefficients with the prediction input
		for(int i = 0; i < xcsf->num_x_vars; i++) {
			pre += pred->weights[var][index++] * x[i];
		}

		if(xcsf->PRED_TYPE == 1) {
			// multiply quadratic coefficients with prediction input
			for(int i = 0; i < xcsf->num_x_vars; i++) {
				for(int j = i; j < xcsf->num_x_vars; j++) {
					pre += pred->weights[var][index++] * x[i] * x[j];
				}
			}
		}

		pred->pre[var] = pre;
	}
	return pred->pre;
} 

double pred_nlms_pre(XCSF *xcsf, CL *c, int p)
{
	(void)xcsf;
	PRED_NLMS *pred = c->pred;
	return pred->pre[p];
}

void pred_nlms_print(XCSF *xcsf, CL *c)
{
	PRED_NLMS *pred = c->pred;
	printf("weights: ");
	for(int var = 0; var < xcsf->num_y_vars; var++) {
		for(int i = 0; i < pred->weights_length; i++) {
			printf("%f, ", pred->weights[var][i]);
		}
		printf("\n");
	}
}
