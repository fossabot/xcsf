/*
 * Copyright (C) 2019 Richard Preen <rpreen@gmail.com>
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
 * The hyperrectangle classifier condition module.
 *
 * Provides functionality to create real-valued hyperrectangle conditions
 * whereby a classifier matches for a given problem instance if, and
 * only if, all of the current state variables fall within the area covered.
 * Includes operations for copying, mutating, printing, etc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include "data_structures.h"
#include "random.h"
#include "cl.h"
#include "cond_rectangle.h"

typedef struct COND_RECTANGLE {
	double *lower;
	double *upper;
	_Bool m;
	double *mu;
} COND_RECTANGLE;

void cond_rectangle_bounds(XCSF *xcsf, double *l, double *u);

void cond_rectangle_init(XCSF *xcsf, CL *c)
{
	COND_RECTANGLE *cond = malloc(sizeof(COND_RECTANGLE));
	cond->lower = malloc(sizeof(double) * xcsf->num_x_vars);
	cond->upper = malloc(sizeof(double) * xcsf->num_x_vars); 
	c->cond = cond;
	sam_init(xcsf, &cond->mu);
}

void cond_rectangle_free(XCSF *xcsf, CL *c)
{
	COND_RECTANGLE *cond = c->cond;
	free(cond->lower);
	free(cond->upper);
	sam_free(xcsf, cond->mu);
	free(c->cond);
}

double cond_rectangle_mu(XCSF *xcsf, CL *c, int m)
{
	(void)xcsf;
	COND_RECTANGLE *cond = c->cond;
	return cond->mu[m];
}

void cond_rectangle_copy(XCSF *xcsf, CL *to, CL *from)
{
	COND_RECTANGLE *to_cond = to->cond;
	COND_RECTANGLE *from_cond = from->cond;
	memcpy(to_cond->lower, from_cond->lower, sizeof(double)*xcsf->num_x_vars);
	memcpy(to_cond->upper, from_cond->upper, sizeof(double)*xcsf->num_x_vars);
	sam_copy(xcsf, to_cond->mu, from_cond->mu);
}                             

void cond_rectangle_rand(XCSF *xcsf, CL *c)
{
	COND_RECTANGLE *cond = c->cond;
	for(int i = 0; i < xcsf->num_x_vars; i++) {
		cond->lower[i] = ((xcsf->MAX_CON - xcsf->MIN_CON) * drand()) + xcsf->MIN_CON;
		cond->upper[i] = ((xcsf->MAX_CON - xcsf->MIN_CON) * drand()) + xcsf->MIN_CON;
		cond_rectangle_bounds(xcsf, &cond->lower[i], &cond->upper[i]);
	}
}

void cond_rectangle_bounds(XCSF *xcsf, double *l, double *u)
{
	if(*l < xcsf->MIN_CON) {
		*l = xcsf->MIN_CON;
	}
	else if(*l > xcsf->MAX_CON) {
		*l = xcsf->MAX_CON;
	}   
	if(*u < xcsf->MIN_CON) {
		*u = xcsf->MIN_CON;
	}
	else if(*u > xcsf->MAX_CON) {
		*u = xcsf->MAX_CON;
	}   
	if(*l > *u) {
		double tmp = *l;
		*l = *u;
		*u = tmp;
	}
}

void cond_rectangle_cover(XCSF *xcsf, CL *c, double *x)
{
	COND_RECTANGLE *cond = c->cond;
	for(int i = 0; i < xcsf->num_x_vars; i++) {
		cond->lower[i] = x[i] - ((xcsf->MAX_CON - xcsf->MIN_CON) * drand() * 0.5);
		cond->upper[i] = x[i] + ((xcsf->MAX_CON - xcsf->MIN_CON) * drand() * 0.5);
		cond_rectangle_bounds(xcsf, &cond->lower[i], &cond->upper[i]);
	}
}

_Bool cond_rectangle_match(XCSF *xcsf, CL *c, double *x)
{
	COND_RECTANGLE *cond = c->cond;
	for(int i = 0; i < xcsf->num_x_vars; i++) {
		if(cond->lower[i] > x[i] || cond->upper[i] < x[i]) {
			cond->m = false;
			return false;
		}
	}
	cond->m = true;
	return cond->m;
}

_Bool cond_rectangle_match_state(XCSF *xcsf, CL *c)
{
	(void)xcsf;
	COND_RECTANGLE *cond = c->cond;
	return cond->m;
}

_Bool cond_rectangle_crossover(XCSF *xcsf, CL *c1, CL *c2) 
{
	COND_RECTANGLE *cond1 = c1->cond;
	COND_RECTANGLE *cond2 = c2->cond;
	_Bool changed = false;
	// uniform crossover
	if(drand() < xcsf->P_CROSSOVER) {
		for(int i = 0; i < xcsf->num_x_vars; i++) {
			// lower interval
			if(drand() < 0.5) {
				double tmp = cond1->lower[i];
				cond1->lower[i] = cond2->lower[i];
				cond2->lower[i] = tmp;
				changed = true;
			}
			// upper interval
			if(drand() < 0.5) {
				double tmp = cond1->upper[i];
				cond1->upper[i] = cond2->upper[i];
				cond2->upper[i] = tmp;
				changed = true;
			}
			cond_rectangle_bounds(xcsf, &cond1->lower[i], &cond1->upper[i]);
			cond_rectangle_bounds(xcsf, &cond2->lower[i], &cond2->upper[i]);
		}
	}
	return changed;
}

_Bool cond_rectangle_mutate(XCSF *xcsf, CL *c)
{
	COND_RECTANGLE *cond = c->cond;
	_Bool changed = false;
	double step = xcsf->S_MUTATION;
	// adapt mutation rates
	if(xcsf->NUM_SAM > 0) {
		sam_adapt(xcsf, cond->mu);
		xcsf->P_MUTATION = cond->mu[0];
		if(xcsf->NUM_SAM > 1) {
			step = cond->mu[1];
		}
	}

	for(int i = 0; i < xcsf->num_x_vars; i++) {
		// lower interval
		if(drand() < xcsf->P_MUTATION) {
			cond->lower[i] += ((drand()*2.0)-1.0)*step;
			changed = true;
		}
		// upper interval
		if(drand() < xcsf->P_MUTATION) {
			cond->upper[i] += ((drand()*2.0)-1.0)*step;
			changed = true;
		}
		cond_rectangle_bounds(xcsf, &cond->lower[i], &cond->upper[i]);
	}
	return changed;
}

_Bool cond_rectangle_general(XCSF *xcsf, CL *c1, CL *c2)
{
	// returns whether cond1 is more general than cond2
	COND_RECTANGLE *cond1 = c1->cond;
	COND_RECTANGLE *cond2 = c2->cond;
	for(int i = 0; i < xcsf->num_x_vars; i++) {
		if(cond1->lower[i] > cond2->lower[i] 
				|| cond1->upper[i] < cond2->upper[i]) {
			return false;
		}
	}
	return true;
}  

void cond_rectangle_print(XCSF *xcsf, CL *c)
{
	COND_RECTANGLE *cond = c->cond;
	printf("rectangle:");
	for(int i = 0; i < xcsf->num_x_vars; i++) {
		printf(" (%5f, ", cond->lower[i]);
		printf("%5f)", cond->upper[i]);
	}
	printf("\n");
}
