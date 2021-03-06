// Copyright (c) 2014-2015, Massachusetts Institute of Technology
//
// This file is part of the Compressed Continuous Computation (C3) toolbox
// Author: Alex A. Gorodetsky 
// Contact: goroda@mit.edu

// All rights reserved.

// Redistribution and use in source and binary forms, with or without modification, 
// are permitted provided that the following conditions are met:

// 1. Redistributions of source code must retain the above copyright notice, 
//    this list of conditions and the following disclaimer.

// 2. Redistributions in binary form must reproduce the above copyright notice, 
//    this list of conditions and the following disclaimer in the documentation 
//    and/or other materials provided with the distribution.

// 3. Neither the name of the copyright holder nor the names of its contributors 
//    may be used to endorse or promote products derived from this software 
//    without specific prior written permission.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE 
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//Code

/** \file piecewisepoly.h
 * Provides header files and structure definitions for functions in piecewisepoly.c
 */

#ifndef PIECEWISEPOLY_H
#define PIECEWISEPOLY_H

#include <stdlib.h>

struct PwPolyOpts;
struct PwPolyOpts;
struct PwPolyOpts * pw_poly_opts_alloc(enum poly_type, double,double);
void pw_poly_opts_free(struct PwPolyOpts *);
void pw_poly_opts_free_deep(struct PwPolyOpts **);

void pw_poly_opts_set_lb(struct PwPolyOpts *, double);
double pw_poly_opts_get_lb(const struct PwPolyOpts *);
void pw_poly_opts_set_ub(struct PwPolyOpts *, double);
double pw_poly_opts_get_ub(const struct PwPolyOpts *);
void pw_poly_opts_set_ptype(struct PwPolyOpts *, enum poly_type);
enum poly_type pw_poly_opts_get_ptype(const struct PwPolyOpts *);

void pw_poly_opts_set_minsize(struct PwPolyOpts *,double);
void pw_poly_opts_set_maxorder(struct PwPolyOpts *, size_t);
void pw_poly_opts_set_maxnum(struct PwPolyOpts *, size_t);
void pw_poly_opts_set_coeffs_check(struct PwPolyOpts *, size_t);
void pw_poly_opts_set_tol(struct PwPolyOpts *, double);
void pw_poly_opts_set_nregions(struct PwPolyOpts *, size_t);
void pw_poly_opts_set_pts(struct PwPolyOpts *, size_t, double *);


/** \struct PiecewisePoly
 * \brief Tree structure to represent piecewise polynomials
 * \var PiecewisePoly::leaf
 * 1 if leaf 0 otherwise
 * \var PiecewisePoly::nbranches
 * number of branches extending from current root. may be unspecified if leaf=1
 * \var PiecewisePoly::branches
 * branches from root. If it is a leaf then branches=NULL
 * \var PiecewisePoly::ope
 * Polynomial if leaf, NULL otherwise
*/
struct PiecewisePoly
{   
    int leaf;
    size_t nbranches;
    struct PiecewisePoly ** branches;
    struct OrthPolyExpansion * ope; 
};

//allocation and deallocation
struct PiecewisePoly * piecewise_poly_alloc();
struct PiecewisePoly ** piecewise_poly_array_alloc(size_t);
struct PiecewisePoly * piecewise_poly_copy(const struct PiecewisePoly *);
void piecewise_poly_free(struct PiecewisePoly *);
void piecewise_poly_array_free(struct PiecewisePoly **, size_t);

// some getters and setters
enum poly_type piecewise_poly_get_ptype(const struct PiecewisePoly * p);

// some initializers
struct PiecewisePoly *
piecewise_poly_genorder(size_t, struct PwPolyOpts *);
struct PiecewisePoly * 
piecewise_poly_constant(double, struct PwPolyOpts *);
struct PiecewisePoly * 
piecewise_poly_linear(double, double, struct PwPolyOpts *);
struct PiecewisePoly * 
piecewise_poly_quadratic(double,double,double, struct PwPolyOpts *);
void piecewise_poly_split(struct PiecewisePoly *, double);
void piecewise_poly_splitn(struct PiecewisePoly *, size_t, const double *);

//basic functions to extract information
int piecewise_poly_isflat(const struct PiecewisePoly *);
double piecewise_poly_lb(const struct PiecewisePoly *);
double piecewise_poly_ub(const struct PiecewisePoly *);
void piecewise_poly_nregions_base(size_t *,const struct PiecewisePoly *);
size_t piecewise_poly_nregions(const struct PiecewisePoly *);
void piecewise_poly_boundaries(const struct PiecewisePoly *,size_t *,double**,size_t *);

//operations using one piecewise poly
double piecewise_poly_eval(const struct PiecewisePoly *, double);
void piecewise_poly_scale(double, struct PiecewisePoly *);
struct PiecewisePoly * piecewise_poly_deriv(const struct PiecewisePoly *);
double piecewise_poly_integrate(const struct PiecewisePoly *);
double * piecewise_poly_real_roots(const struct PiecewisePoly *, size_t *);
double piecewise_poly_max(const struct PiecewisePoly *, double *);
double piecewise_poly_min(const struct PiecewisePoly *, double *);
double piecewise_poly_absmax(const struct PiecewisePoly *, double *,void*);
double piecewise_poly_norm(const struct PiecewisePoly *);
void piecewise_poly_flip_sign(struct PiecewisePoly *);


//operations to modfiy/generate modfied piecewise polynomials
void piecewise_poly_copy_leaves(const struct PiecewisePoly *,
                                struct PiecewisePoly ** , size_t *);
void piecewise_poly_ref_leaves(struct PiecewisePoly *,
                               struct PiecewisePoly ** , size_t *);
void piecewise_poly_flatten(struct PiecewisePoly *);
struct PiecewisePoly * 
piecewise_poly_finer_grid(const struct PiecewisePoly *, size_t, double *);

//operations using two piecewise polynomials
void piecewise_poly_match(struct PiecewisePoly *, struct PiecewisePoly **,
                     struct PiecewisePoly *, struct PiecewisePoly **);
struct PiecewisePoly *
piecewise_poly_prod(const struct PiecewisePoly *,const struct PiecewisePoly *);
double piecewise_poly_inner(const struct PiecewisePoly *,const struct PiecewisePoly *);
struct PiecewisePoly *
piecewise_poly_daxpby(double, const struct PiecewisePoly *,
                      double, const struct PiecewisePoly *);
struct PiecewisePoly *
piecewise_poly_matched_daxpby(double,const struct PiecewisePoly *,
                              double,const struct PiecewisePoly *);
struct PiecewisePoly *
piecewise_poly_matched_prod(const struct PiecewisePoly *,
                            const struct PiecewisePoly *);


// Approximation
struct PiecewisePoly *
piecewise_poly_approx1(struct PwPolyOpts *,struct Fwrap *);
  /* struct PwPolyOpts *); */
struct PiecewisePoly *
piecewise_poly_approx1_adapt(struct PwPolyOpts *, struct Fwrap *);
                /* double (*f)(double, void *), void *, */
                /* /\* double, double, *\/ */
                /* struct PwPolyOpts *); */


///////////////////////////////////////////////

struct OrthPolyExpansion * piecewise_poly_trim_left(struct PiecewisePoly **);

int piecewise_poly_check_discontinuity(struct PiecewisePoly *, 
                                       struct PiecewisePoly *, 
                                       int, double);

struct PiecewisePoly *
piecewise_poly_merge_left(struct PiecewisePoly **, struct PwPolyOpts *);


double minmod_eval(double, double *, double *, size_t,size_t, size_t);
int minmod_disc_exists(double, double *, double *, size_t,size_t, size_t);

void locate_jumps(double (*)(double, void *), void *,
                  double, double, size_t, double, double **, size_t *);

struct PiecewisePoly *
piecewise_poly_approx2(double (*)(double, void *), void *, double,
                        double, struct PwPolyOpts *);
// serialization and printing
unsigned char * 
serialize_piecewise_poly(unsigned char *, struct PiecewisePoly *, size_t *); 
unsigned char *
deserialize_piecewise_poly(unsigned char *, struct PiecewisePoly ** ); 


void print_piecewise_poly(struct PiecewisePoly * pw, size_t, void *);

void
piecewise_poly_savetxt(const struct PiecewisePoly *, FILE *,
                       size_t);
struct PiecewisePoly * piecewise_poly_loadtxt(FILE *);

#endif


