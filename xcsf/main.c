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
 * The main XCSF module.  
 *
 * 1) Initialises the environment: constants, random number generator, problem
 * function, and performance output writing.
 *
 * 2) Executes the experiments: iteratively retrieving a problem instance,
 * generating a match set, calculating a system prediction, providing
 * reinforcement and running the genetic algorithm.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <errno.h>
#include "data_structures.h"
#include "config.h"
#include "random.h"
#include "cl.h"
#include "cl_set.h"
#include "ga.h"
#include "input.h"
#include "perf.h"

void xcsf_fit1(XCSF *xcsf, INPUT *train_data, _Bool shuffle);
void xcsf_fit2(XCSF *xcsf, INPUT *train_data, INPUT *test_data, _Bool shuffle);
void xcsf_predict(XCSF *xcsf, double *input, double *output, int rows);
double xcsf_learn_trial(XCSF *xcsf, double *pred, double *x, double *y);
double xcsf_test_trial(XCSF *xcsf, double *pred, double *x, double *y);

int main(int argc, char **argv)
{    
	if(argc < 2 || argc > 3) {
		printf("Usage: xcsf inputfile [config.ini]\n");
		exit(EXIT_FAILURE);
	} 

	random_init();

	// initialise XCSF
	XCSF *xcsf = malloc(sizeof(XCSF));
    // read parameters from configuration file
	if(argc > 2) {
		constants_init(xcsf, argv[2]);
	}    
	else {
		constants_init(xcsf, "default.ini");
	}

	// read csv input data
	INPUT *train_data = malloc(sizeof(INPUT));
	INPUT *test_data = malloc(sizeof(INPUT));
	input_read_csv(argv[1], train_data, test_data);

	xcsf->num_x_vars = train_data->x_cols;
	xcsf->num_y_vars = train_data->y_cols;

	// initialise population
	pop_init(xcsf);
	// run an experiment
	xcsf_fit2(xcsf, train_data, test_data, true);

	// clean up
	set_kill(xcsf, &xcsf->pset);
	constants_free(xcsf);        
	free(xcsf);
	input_free(train_data);
	input_free(test_data);
	free(train_data);
	free(test_data);

	return EXIT_SUCCESS;
}

void xcsf_fit1(XCSF *xcsf, INPUT *train_data, _Bool shuffle)
{  
#ifdef GNUPLOT
	gplot_init(xcsf);
#endif
 
	// performance tracking
	double err[xcsf->PERF_AVG_TRIALS];
	// stores current system prediction
	double *pred = malloc(sizeof(double)*xcsf->num_y_vars);
	// current sample
	int row = 0;
	// each trial in an experiment
	for(int cnt = 0; cnt < xcsf->MAX_TRIALS; cnt++) {
		// select next training sample
		if(shuffle) {
			row = irand(0, train_data->rows);
		}
		else {
			row = (cnt % train_data->rows + train_data->rows) % train_data->rows;
		}
		double *x = &train_data->x[row * train_data->x_cols];
		double *y = &train_data->y[row * train_data->y_cols];
		// execute a training step and return the error
		err[cnt % xcsf->PERF_AVG_TRIALS] = xcsf_learn_trial(xcsf, pred, x, y);
		// display performance
		if(cnt % xcsf->PERF_AVG_TRIALS == 0 && cnt > 0) {
			disp_perf1(xcsf, err, cnt);
		}
	}

	// clean up
	free(pred);

#ifdef GNUPLOT
	gplot_free(xcsf);
#endif
}

void xcsf_fit2(XCSF *xcsf, INPUT *train_data, INPUT *test_data, _Bool shuffle)
{   
#ifdef GNUPLOT
	gplot_init(xcsf);
#endif
 
	// performance tracking
	double err[xcsf->PERF_AVG_TRIALS];
	double terr[xcsf->PERF_AVG_TRIALS];
	// stores current system prediction
	double *pred = malloc(sizeof(double)*xcsf->num_y_vars);
	// current sample
	int row = 0;
	// each trial in an experiment
	for(int cnt = 0; cnt < xcsf->MAX_TRIALS; cnt++) {
 		// select next training sample
		if(shuffle) {
			row = irand(0, train_data->rows);
		}
		else {
			row = (cnt % train_data->rows + train_data->rows) % train_data->rows;
		}     	
		double *x = &train_data->x[row * train_data->x_cols];
		double *y = &train_data->y[row * train_data->y_cols];
		err[cnt % xcsf->PERF_AVG_TRIALS] = xcsf_learn_trial(xcsf, pred, x, y);
		// select next testing sample
		row = irand(0, test_data->rows);
		if(shuffle) {
			row = irand(0, test_data->rows);
		}
		else {
			row = (cnt % test_data->rows + test_data->rows) % test_data->rows;
		}     	
		x = &test_data->x[row * test_data->x_cols];
		y = &test_data->y[row * test_data->y_cols];
		// calculate the system error
		terr[cnt % xcsf->PERF_AVG_TRIALS] = xcsf_test_trial(xcsf, pred, x, y);
		// display performance
		if(cnt % xcsf->PERF_AVG_TRIALS == 0 && cnt > 0) {
			disp_perf2(xcsf, err, terr, cnt);
		}
	}

	// clean up
	free(pred);

#ifdef GNUPLOT
	gplot_free(xcsf);
#endif
}
 
double xcsf_learn_trial(XCSF *xcsf, double *pred, double *x, double *y)
{
	// create match set
	NODE *mset = NULL, *kset = NULL;
	int msize = 0, mnum = 0;
	set_match(xcsf, &mset, &msize, &mnum, x, &kset);
	// calculate system prediction
	set_pred(xcsf, &mset, msize, x, pred);
	// provide reinforcement to the set
	set_update(xcsf, &mset, &msize, &mnum, x, y, &kset);
	// run the genetic algorithm
	ga(xcsf, &mset, msize, mnum, &kset);
	// increment learning time
	xcsf->time += 1;
	// clean up
	set_kill(xcsf, &kset); // kills deleted classifiers
	set_free(xcsf, &mset); // frees the match set list
	// return the system error
	double error = 0.0;
	for(int i = 0; i < xcsf->num_y_vars; i++) {
		error += (y[i]-pred[i])*(y[i]-pred[i]);
	}
	error /= xcsf->num_y_vars; // MSE
	return error;
}
 
double xcsf_test_trial(XCSF *xcsf, double *pred, double *x, double *y)
{
	// create match set
	NODE *mset = NULL, *kset = NULL;
	int msize = 0, mnum = 0;
	set_match(xcsf, &mset, &msize, &mnum, x, &kset);
	// calculate system prediction
	set_pred(xcsf, &mset, msize, x, pred);
	// clean up
	set_kill(xcsf, &kset); // kills deleted classifiers
	set_free(xcsf, &mset); // frees the match set list  
	// return the system error
	double error = 0.0;
	for(int i = 0; i < xcsf->num_y_vars; i++) {
		error += (y[i]-pred[i])*(y[i]-pred[i]);
	}
	error /= xcsf->num_y_vars; // MSE
	return error;      
}

void xcsf_predict(XCSF *xcsf, double *input, double *output, int rows)
{   
	for(int row = 0; row < rows; row++) {
		// create match set
		NODE *mset = NULL, *kset = NULL;
		int msize = 0, mnum = 0;
		set_match(xcsf, &mset, &msize, &mnum, &input[row*xcsf->num_x_vars], &kset);
		// calculate system prediction
		set_pred(xcsf, &mset, msize, &input[row*xcsf->num_x_vars], &output[row*xcsf->num_y_vars]);
		// clean up
		set_kill(xcsf, &kset); // kills deleted classifiers
		set_free(xcsf, &mset); // frees the match set list      
	}
}

void xcsf_print_pop(XCSF *xcsf, _Bool print_cond, _Bool print_pred)
{
    set_print(xcsf, xcsf->pset, print_cond, print_pred);
}

void xcsf_print_match_set(XCSF *xcsf, double *input, _Bool print_cond, _Bool print_pred)
{
	// create match set
	NODE *mset = NULL, *kset = NULL;
	int msize = 0, mnum = 0;
	set_match(xcsf, &mset, &msize, &mnum, input, &kset);
    set_print(xcsf, mset, print_cond, print_pred);
}
