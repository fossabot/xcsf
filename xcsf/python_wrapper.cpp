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
 */

#include <string>
#include <vector>
#include <boost/python.hpp>
#include <boost/python/numpy.hpp>

namespace p = boost::python;
namespace np = boost::python::numpy;

extern "C" {   
#include <stdbool.h>
#include "data_structures.h"
#include "config.h"
#include "random.h"
#include "input.h"
#include "cl_set.h"
}

extern "C" void xcsf_fit1(XCSF *, INPUT *, _Bool);
extern "C" void xcsf_fit2(XCSF *, INPUT *, INPUT *, _Bool);
extern "C" void xcsf_predict(XCSF *, double *, double *, int);
extern "C" void xcsf_print_pop(XCSF *, _Bool, _Bool);
extern "C" void xcsf_print_match_set(XCSF *, double *, _Bool, _Bool);

/* XCSF class */
struct XCS
{        
	XCSF xcs;
	INPUT train_data;
	INPUT test_data;

	XCS(int num_x_vars, int num_y_vars) : 
		XCS(num_x_vars, num_y_vars, "default.ini") {}

	XCS(int num_x_vars, int num_y_vars, const char *filename) {
		constants_init(&xcs, filename);
		xcs.num_x_vars = num_x_vars;
		xcs.num_y_vars = num_y_vars;
		xcs.pop_num = 0;
		xcs.pop_num_sum = 0;
		xcs.time = 0;
		train_data.rows = 0;
		train_data.x_cols = 0;
		train_data.y_cols = 0;
		train_data.x = NULL;
		train_data.y = NULL;
		test_data.rows = 0;
		test_data.x_cols = 0;
		test_data.y_cols = 0;
		test_data.x = NULL;
		test_data.y = NULL;
	}

	void fit(np::ndarray &train_X, np::ndarray &train_Y, _Bool shuffle) {
		// check inputs are correctly sized
		if(train_X.shape(0) != train_Y.shape(0)) {
			printf("error: training X and Y rows are not equal\n");
			return;
		}  
		// load training data
		train_data.rows = train_X.shape(0);
		train_data.x_cols = train_X.shape(1);
		train_data.y_cols = train_Y.shape(1);
		train_data.x = reinterpret_cast<double*>(train_X.get_data());
		train_data.y = reinterpret_cast<double*>(train_Y.get_data());
		// first execution
		if(xcs.pop_num == 0) {
			pop_init(&xcs);
		}       
		// execute
		xcsf_fit1(&xcs, &train_data, shuffle);
	}

	void fit(np::ndarray &train_X, np::ndarray &train_Y, 
			np::ndarray &test_X, np::ndarray &test_Y, _Bool shuffle) {
		// check inputs are correctly sized
		if(train_X.shape(0) != train_Y.shape(0)) {
			printf("error: training X and Y rows are not equal\n");
			return;
		}
		if(test_X.shape(0) != test_Y.shape(0)) {
			printf("error: testing X and Y rows are not equal\n");
			return;
		}
		if(train_X.shape(1) != test_X.shape(1)) {
			printf("error: number of training and testing X cols are not equal\n");
			return;
		}
		if(train_Y.shape(1) != test_Y.shape(1)) {
			printf("error: number of training and testing Y cols are not equal\n");
			return;
		}
		// load training data
		train_data.rows = train_X.shape(0);
		train_data.x_cols = train_X.shape(1);
		train_data.y_cols = train_Y.shape(1);
		train_data.x = reinterpret_cast<double*>(train_X.get_data());
		train_data.y = reinterpret_cast<double*>(train_Y.get_data());
		// load testing data
		test_data.rows = test_X.shape(0);
		test_data.x_cols = test_X.shape(1);
		test_data.y_cols = test_Y.shape(1);
		test_data.x = reinterpret_cast<double*>(test_X.get_data());
		test_data.y = reinterpret_cast<double*>(test_Y.get_data());
		// first execution
		if(xcs.pop_num == 0) {
			pop_init(&xcs);
		}       
		// execute
		xcsf_fit2(&xcs, &train_data, &test_data, shuffle);
	}

	np::ndarray predict(np::ndarray &T) {
		// inputs to predict
		double *input = reinterpret_cast<double*>(T.get_data());
		int rows = T.shape(0);
		// predicted outputs
		double *output = (double *) malloc(sizeof(double) * rows * xcs.num_y_vars);
		xcsf_predict(&xcs, input, output, rows);
		// return numpy array
		np::ndarray result = np::from_data(output, np::dtype::get_builtin<double>(),
				p::make_tuple(rows, xcs.num_y_vars), 
				p::make_tuple(sizeof(double), sizeof(double)), p::object());
		return result;
	}

	void print_pop(_Bool print_cond, _Bool print_pred) {
		xcsf_print_pop(&xcs, print_cond, print_pred);
	}

	void print_match_set(np::ndarray &X, _Bool print_cond, _Bool print_pred) {
		double *input = reinterpret_cast<double*>(X.get_data());
		xcsf_print_match_set(&xcs, input, print_cond, print_pred);
	}

	/* GETTERS */
	_Bool get_pop_init() { return xcs.POP_INIT; }
	double get_theta_mna() { return xcs.THETA_MNA; }
	int get_max_trials() { return xcs.MAX_TRIALS; }
	int get_perf_avg_trials() { return xcs.PERF_AVG_TRIALS; }
	int get_pop_size() { return xcs.POP_SIZE; }
	double get_alpha() { return xcs.ALPHA; }
	double get_beta() { return xcs.BETA; }
	double get_delta() { return xcs.DELTA; }
	double get_eps_0() { return xcs.EPS_0; }
	double get_err_reduc() { return xcs.ERR_REDUC; }
	double get_fit_reduc() { return xcs.FIT_REDUC; }
	double get_init_error() { return xcs.INIT_ERROR; }
	double get_init_fitness() { return xcs.INIT_FITNESS; }
	double get_nu() { return xcs.NU; }
	double get_theta_del() { return xcs.THETA_DEL; }
	int get_cond_type() { return xcs.COND_TYPE; }
	int get_pred_type() { return xcs.PRED_TYPE; }
	double get_p_crossover() { return xcs.P_CROSSOVER; }
	double get_p_mutation() { return xcs.P_MUTATION; }
	double get_theta_ga() { return xcs.THETA_GA; }
	int get_theta_offspring() { return xcs.THETA_OFFSPRING; }
	double get_mueps_0() { return xcs.muEPS_0; }
	int get_num_sam() { return xcs.NUM_SAM; }
	double get_max_con() { return xcs.MAX_CON; }
	double get_min_con() { return xcs.MIN_CON; }
	double get_s_mutation() { return xcs.S_MUTATION; }
	int get_num_hidden_neurons() { return xcs.NUM_HIDDEN_NEURONS; }
	int get_hidden_neuron_activation() { return xcs.HIDDEN_NEURON_ACTIVATION; }
	int get_dgp_num_nodes() { return xcs.DGP_NUM_NODES; }
	int get_gp_num_cons() { return xcs.GP_NUM_CONS; }
	int get_gp_init_depth() { return xcs.GP_INIT_DEPTH; }
	double get_xcsf_eta() { return xcs.XCSF_ETA; }
	double get_xcsf_x0() { return xcs.XCSF_X0; }
	double get_rls_scale_factor() { return xcs.RLS_SCALE_FACTOR; }
	double get_rls_lambda() { return xcs.RLS_LAMBDA; }
	double get_theta_sub() { return xcs.THETA_SUB; }
	_Bool get_ga_subsumption() { return xcs.GA_SUBSUMPTION; }
	_Bool get_set_subsumption() { return xcs.SET_SUBSUMPTION; }
	int get_pop_num() { return xcs.pop_num; }
	int get_pop_num_sum() { return xcs.pop_num_sum; }
	int get_time() { return xcs.time; }
	double get_num_x_vars() { return xcs.num_x_vars; }
	double get_num_y_vars() { return xcs.num_y_vars; }                      

	/* SETTERS */
	void set_pop_init(_Bool a) { xcs.POP_INIT = a; }
	void set_theta_mna(double a) { xcs.THETA_MNA = a; }
	void set_max_trials(int a) { xcs.MAX_TRIALS = a; }
	void set_perf_avg_trials(int a) { xcs.PERF_AVG_TRIALS = a; }
	void set_pop_size(int a) { xcs.POP_SIZE = a; }
	void set_alpha(double a) { xcs.ALPHA = a; }
	void set_beta(double a) { xcs.BETA = a; }
	void set_delta(double a) { xcs.DELTA = a; }
	void set_eps_0(double a) { xcs.EPS_0 = a; } 
	void set_err_reduc(double a) { xcs.ERR_REDUC = a; }
	void set_fit_reduc(double a) { xcs.FIT_REDUC = a; }
	void set_init_error(double a) { xcs.INIT_ERROR = a; }
	void set_init_fitness(double a) { xcs.INIT_FITNESS = a; }
	void set_nu(double a) { xcs.NU = a; }
	void set_theta_del(double a) { xcs.THETA_DEL = a; }
	void set_cond_type(int a) { xcs.COND_TYPE = a; }
	void set_pred_type(int a) { xcs.PRED_TYPE = a; }
	void set_p_crossover(double a) { xcs.P_CROSSOVER = a; }
	void set_p_mutation(double a) { xcs.P_MUTATION = a; }
	void set_theta_ga(double a) { xcs.THETA_GA = a; }
	void set_theta_offspring(int a) { xcs.THETA_OFFSPRING = a; }
	void set_mueps_0(double a) { xcs.muEPS_0 = a; }
	void set_num_sam(int a) { xcs.NUM_SAM = a; }
	void set_max_con(double a) { xcs.MAX_CON = a; }
	void set_min_con(double a) { xcs.MIN_CON = a; }
	void set_s_mutation(double a) { xcs.S_MUTATION = a; }
	void set_num_hidden_neurons(int a) { xcs.NUM_HIDDEN_NEURONS = a; }
	void set_hidden_neuron_activation(int a) { xcs.HIDDEN_NEURON_ACTIVATION = a; }
	void set_dgp_num_nodes(int a) { xcs.DGP_NUM_NODES = a; }
	void set_gp_num_cons(int a) { xcs.GP_NUM_CONS = a; }
	void set_gp_init_depth(int a) { xcs.GP_INIT_DEPTH = a; }
	void set_xcsf_eta(double a) { xcs.XCSF_ETA = a; }
	void set_xcsf_x0(double a) { xcs.XCSF_X0 = a; }
	void set_rls_scale_factor(double a) { xcs.RLS_SCALE_FACTOR = a; }
	void set_rls_lambda(double a) { xcs.RLS_LAMBDA = a; }
	void set_theta_sub(double a) { xcs.THETA_SUB = a; }
	void set_ga_subsumption(_Bool a) { xcs.GA_SUBSUMPTION = a; }
	void set_set_subsumption(_Bool a) { xcs.SET_SUBSUMPTION = a; }
};

BOOST_PYTHON_MODULE(xcsf)
{
	np::initialize();
	random_init();

	void (XCS::*fit1)(np::ndarray&, np::ndarray&, _Bool) = &XCS::fit;
	void (XCS::*fit2)(np::ndarray&, np::ndarray&, np::ndarray&, np::ndarray&, _Bool) = &XCS::fit;

	p::class_<XCS>("XCS", p::init<int, int>())
		.def(p::init<int, int, const char *>())
		.def("fit", fit1)
		.def("fit", fit2)
		.def("predict", &XCS::predict)
		.add_property("POP_INIT", &XCS::get_pop_init, &XCS::set_pop_init)
		.add_property("THETA_MNA", &XCS::get_theta_mna, &XCS::set_theta_mna)
		.add_property("MAX_TRIALS", &XCS::get_max_trials, &XCS::set_max_trials)
		.add_property("PERF_AVG_TRIALS", &XCS::get_perf_avg_trials, &XCS::set_perf_avg_trials)
		.add_property("POP_SIZE", &XCS::get_pop_size, &XCS::set_pop_size)
		.add_property("ALPHA", &XCS::get_alpha, &XCS::set_alpha)
		.add_property("BETA", &XCS::get_beta, &XCS::set_beta)
		.add_property("DELTA", &XCS::get_delta, &XCS::set_delta)
		.add_property("EPS_0", &XCS::get_eps_0, &XCS::set_eps_0)
		.add_property("ERR_REDUC", &XCS::get_err_reduc, &XCS::set_err_reduc)
		.add_property("FIT_REDUC", &XCS::get_fit_reduc, &XCS::set_fit_reduc)
		.add_property("INIT_ERROR", &XCS::get_init_error, &XCS::set_init_error)
		.add_property("INIT_FITNESS", &XCS::get_init_fitness, &XCS::set_init_fitness)
		.add_property("NU", &XCS::get_nu, &XCS::set_nu)
		.add_property("THETA_DEL", &XCS::get_theta_del, &XCS::set_theta_del)
		.add_property("COND_TYPE", &XCS::get_cond_type, &XCS::set_cond_type)
		.add_property("PRED_TYPE", &XCS::get_pred_type, &XCS::set_pred_type)
		.add_property("P_CROSSOVER", &XCS::get_p_crossover, &XCS::set_p_crossover)
		.add_property("P_MUTATION", &XCS::get_p_mutation, &XCS::set_p_mutation)
		.add_property("THETA_GA", &XCS::get_theta_ga, &XCS::set_theta_ga)
		.add_property("THETA_OFFSPRING", &XCS::get_theta_offspring, &XCS::set_theta_offspring)
		.add_property("muEPS_0", &XCS::get_mueps_0, &XCS::set_mueps_0)
		.add_property("NUM_SAM", &XCS::get_num_sam, &XCS::set_num_sam)
		.add_property("MAX_CON", &XCS::get_max_con, &XCS::set_max_con)
		.add_property("MIN_CON", &XCS::get_min_con, &XCS::set_min_con)
		.add_property("S_MUTATION", &XCS::get_s_mutation, &XCS::set_s_mutation)
		.add_property("NUM_HIDDEN_NEURONS", &XCS::get_num_hidden_neurons, &XCS::set_num_hidden_neurons)
		.add_property("HIDDEN_NEURON_ACTIVATION", &XCS::get_hidden_neuron_activation, &XCS::set_hidden_neuron_activation)
		.add_property("DGP_NUM_NODES", &XCS::get_dgp_num_nodes, &XCS::set_dgp_num_nodes)
		.add_property("GP_NUM_CONS", &XCS::get_gp_num_cons, &XCS::set_gp_num_cons)
		.add_property("GP_INIT_DEPTH", &XCS::get_gp_init_depth, &XCS::set_gp_init_depth)
		.add_property("XCSF_ETA", &XCS::get_xcsf_eta, &XCS::set_xcsf_eta)
		.add_property("XCSF_X0", &XCS::get_xcsf_x0, &XCS::set_xcsf_x0)
		.add_property("RLS_SCALE_FACTOR", &XCS::get_rls_scale_factor, &XCS::set_rls_scale_factor)
		.add_property("RLS_LAMBDA", &XCS::get_rls_lambda, &XCS::set_rls_lambda)
		.add_property("THETA_SUB", &XCS::get_theta_sub, &XCS::set_theta_sub)
		.add_property("GA_SUBSUMPTION", &XCS::get_ga_subsumption, &XCS::set_ga_subsumption)
		.add_property("SET_SUBSUMPTION", &XCS::get_set_subsumption, &XCS::set_set_subsumption)
		.def("pop_num", &XCS::get_pop_num)
		.def("pop_num_sum", &XCS::get_pop_num_sum)
		.def("time", &XCS::get_time)
		.def("num_x_vars", &XCS::get_num_x_vars)
		.def("num_y_vars", &XCS::get_num_y_vars)
		.def("print_pop", &XCS::print_pop)
		.def("print_match_set", &XCS::print_match_set)
		;
}
