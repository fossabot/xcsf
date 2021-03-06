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

_Bool cond_rectangle_crossover(XCSF *xcsf, CL *c1, CL *c2);
_Bool cond_rectangle_general(XCSF *xcsf, CL *c1, CL *c2);
_Bool cond_rectangle_match(XCSF *xcsf, CL *c, double *x);
_Bool cond_rectangle_match_state(XCSF *xcsf, CL *c);
_Bool cond_rectangle_mutate(XCSF *xcsf, CL *c);
void cond_rectangle_copy(XCSF *xcsf, CL *to, CL *from);
void cond_rectangle_cover(XCSF *xcsf, CL *c, double *x);
void cond_rectangle_free(XCSF *xcsf, CL *c);
void cond_rectangle_init(XCSF *xcsf, CL *c);
void cond_rectangle_print(XCSF *xcsf, CL *c);
void cond_rectangle_rand(XCSF *xcsf, CL *c);
double cond_rectangle_mu(XCSF *xcsf, CL *c, int m);

static struct CondVtbl const cond_rectangle_vtbl = {
	&cond_rectangle_crossover,
	&cond_rectangle_general,
	&cond_rectangle_match,
	&cond_rectangle_match_state,
	&cond_rectangle_mutate,
	&cond_rectangle_mu,
	&cond_rectangle_copy,
	&cond_rectangle_cover,
	&cond_rectangle_free,
	&cond_rectangle_init,
	&cond_rectangle_print,
	&cond_rectangle_rand
};      
