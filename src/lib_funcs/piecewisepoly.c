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

/** \file piecewisepoly.c
 * Provides routines for using piecewise polynomials
*/

#include <stdlib.h>
#include <float.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "stringmanip.h"
#include "array.h"
#include "polynomials.h"
#include "piecewisepoly.h"

#define ZEROTHRESH  2e0 * DBL_EPSILON

struct PwCouple
{
    struct PiecewisePoly * a;
    struct PiecewisePoly * b;
    double coeff[2];
};

double pw_eval(double x, void * args){
    struct PiecewisePoly * pw = args;
    double out = piecewise_poly_eval(pw,x);
    return out;
}

double pw_eval_prod(double x, void * args)
{
    struct PwCouple * c = args;
    double out = piecewise_poly_eval(c->a,x) * 
                 piecewise_poly_eval(c->b,x);

    //printf("x = %G out1=%G ,out2=%G\n",x, piecewise_poly_eval(c->a,x), out);
    return out;
}

double pw_eval_ope(double x, void * args){
    
    struct OrthPolyExpansion * ope = args;
    double out = orth_poly_expansion_eval(ope,x);
    //printf("(%G,%G)\n",x,out);
    return out;
}

double pw_eval_sum(double x, void * args)
{
    struct PwCouple * c = args;
    double out = 0.0;
    if (c->b == NULL){
        assert (c->a != NULL);
        return  c->coeff[0] * piecewise_poly_eval(c->a,x);
    }
    else if (c->a == NULL){
        assert (c->b != NULL);
        return  c->coeff[1] * piecewise_poly_eval(c->b,x);
    }
    else{
        return out = c->coeff[0] * piecewise_poly_eval(c->a,x) +
                 c->coeff[1] * piecewise_poly_eval(c->b,x);
     }
}

double pw_eval_neighbor(double x, void * args){
    
    struct PwCouple * c = args;
    double split = piecewise_poly_ub(c->a);
    double out;
    if (x <= split){
        out = piecewise_poly_eval(c->a,x);
    }
    else{
        out = piecewise_poly_eval(c->b,x);
    }
    return out;
}

void solveLin(double * x, double * y, double * coeff)
{
    double den = x[0]-x[1];
    coeff[0] = (y[0] - y[1]) / den; // slope
    coeff[1] = (x[0]*y[1] - x[1]*y[0]) /den; // offset;
}

void solveQuad(double * x, double * y, double * coeff){
    
    // high power to lower power
    double den = pow(x[0],2)*(x[1] - x[2]) - 
                 pow(x[1],2)*(x[0] - x[2]) +
                 pow(x[2],2)*(x[0] - x[1]);

    coeff[0] = (y[0] * (x[1] - x[2]) - y[1] * (x[0] - x[2]) + y[2] * (x[0] - x[1]))/den;
    coeff[1] = (pow(x[0],2) * (y[1] - y[2]) - pow(x[1],2) * (y[0] - y[2]) + pow(x[2],2) * (y[0] - y[1]))/den;
    coeff[2] = (pow(x[0],2) * (x[1]*y[2] - x[2]*y[1]) - pow(x[1],2) * (x[0]*y[2] - x[2]*y[0]) + pow(x[2],2) * (x[0]*y[1] - x[1]*y[0]))/den;
}

double pw_eval_lin_func(double x, void * args)
{   
    double * coeff = args;
    double out = coeff[0]*x + coeff[1];
    return out;
}

double pw_eval_quad_func(double x, void * args)
{   
    double * coeff = args;
    double out = coeff[0] * pow(x,2) + coeff[1] * x + coeff[2];
    return out;
}

/********************************************************//**
*   Allocate memory for a piecewise polynomial
*
*   \return p - polynomial
*************************************************************/
struct PiecewisePoly *
piecewise_poly_alloc()
{
    struct PiecewisePoly * p;
    if ( NULL == (p = malloc(sizeof(struct PiecewisePoly)))){
        fprintf(stderr,"failed to allocate memory for piecewise polynomial.\n");
        exit(1);
    }
    
    p->nbranches = 0;
    p->leaf = 1;
    p->ope = NULL;
    p->branches = NULL;
    return p;
}

/********************************************************//**
*   Allocate memory for a piecewise polynomial array
*   
*   \param size [in] - size of array
*
*   \return p - pw poly array filled with nulls
*************************************************************/
struct PiecewisePoly **
piecewise_poly_array_alloc(size_t size)
{
    struct PiecewisePoly ** p;
    if ( NULL == (p = malloc(size*sizeof(struct PiecewisePoly)))){
        fprintf(stderr,"failed to allocate memory for \
                        piecewise polynomial.\n");
        exit(1);
    }
    size_t ii; 
    for (ii = 0; ii < size; ii++){
        p[ii] = NULL;
    }
    return p;
}

/********************************************************//**
*   Copy a piecewise polynomial
*   
*   \param a [in] - pw polynomial to copy
*
*   \return pw - pw polynomial
*************************************************************/
struct PiecewisePoly *
piecewise_poly_copy(struct PiecewisePoly * a)
{
    if ( a != NULL ){
        struct PiecewisePoly * p = piecewise_poly_alloc();
        if (a->leaf == 1){
            p->ope = orth_poly_expansion_copy(a->ope);
        }
        else{
            p->leaf = 0;
            p->nbranches = a->nbranches;
            p->ope = NULL;
            p->branches = piecewise_poly_array_alloc(p->nbranches);
            size_t ii;
            for (ii = 0; ii < p->nbranches; ii++){
                p->branches[ii] = piecewise_poly_copy(a->branches[ii]);
            }
        }
        return p;
    }
    return NULL;
}

/********************************************************//**
*   Free memory allocated for piecewise polynomial
*
*   \param poly [inout] - polynomial to free
*************************************************************/
void
piecewise_poly_free(struct PiecewisePoly * poly){
    
    if (poly != NULL)
    {   
        if (poly->leaf == 1){
            orth_poly_expansion_free(poly->ope);
            poly->ope = NULL;
        }
        else{
            size_t ii;
            for (ii = 0; ii < poly->nbranches; ii++){
                piecewise_poly_free(poly->branches[ii]);
                poly->branches[ii] = NULL;
            }
            free(poly->branches);
            poly->branches = NULL;
        }
        free(poly);
        poly = NULL;
    }
}

/********************************************************//**
*   Construct a piecewise constant function
*
*   \param value [in] - value of the function
*   \param ptype [in] - type of polynomial
*   \param lb [in] - lower bound
*   \param ub [in] - upper bound
*
*   \return p - piecewise polynomial of one interval
*************************************************************/
struct PiecewisePoly *
piecewise_poly_constant(double value, enum poly_type ptype, double lb, double ub)
{
    
    struct PiecewisePoly * p = piecewise_poly_alloc();
    p->ope = orth_poly_expansion_constant(value, ptype, lb, ub);
    return p;
}

/********************************************************//**
*   Construct a piecewise linear function
*
*   \param slope [in] - value of the slope function
*   \param offset [in] - offset
*   \param ptype [in] - type of polynomial
*   \param lb [in] - lower bound
*   \param ub [in] - upper bound
*
*   \return p - piecewise polynomial of one interval
*************************************************************/
struct PiecewisePoly *
piecewise_poly_linear(double slope, double offset, enum poly_type ptype, double lb,  double ub)
{
    
    struct PiecewisePoly * p = piecewise_poly_alloc();
    p->ope = orth_poly_expansion_linear(slope, offset, ptype, lb, ub);
    return p;
}

/********************************************************//**
*   Construct a piecewise quadratic function \f$ ax^2 + bx + c \f$
*
*   \param a [in] - coefficient of squared term
*   \param b [in] - coefficient of linear term
*   \param c [in] - constant term
*   \param ptype [in] - type of polynomial
*   \param lb [in] - lower bound
*   \param ub [in] - upper bound
*
*   \return p - piecewise polynomial of one interval
*************************************************************/
struct PiecewisePoly *
piecewise_poly_quadratic(double a, double b, double c, enum poly_type ptype,
        double lb, double ub)
{
    
    struct PiecewisePoly * p = piecewise_poly_alloc();
    double coeff[3];
    coeff[0] = a; coeff[1] = b; coeff[2] = c;

    p->ope = orth_poly_expansion_init(ptype, 3, lb, ub);
    orth_poly_expansion_approx(&pw_eval_quad_func, coeff, p->ope);
    orth_poly_expansion_round(&(p->ope));
    return p;
}


/********************************************************//**
*   Get the lower bound of the space on which a pw polynomial
*   is defined
*
*   \param a [in] - pw poly
*
*   \return pw lower bound
*************************************************************/
double piecewise_poly_lb(struct PiecewisePoly * a)
{
    if (a->leaf  == 1){
        return a->ope->lower_bound;
    }
    else{
        return piecewise_poly_lb(a->branches[0]);
    }
}

/********************************************************//**
*   Get the upper bound of the space on which a pw polynomial
*   is defined
*
*   \param a [in] - pw poly
*
*   \return pw upper bound
*************************************************************/
double piecewise_poly_ub(struct PiecewisePoly * a)
{

    if (a->leaf == 1){
        return a->ope->upper_bound;
    }
    else{
        return piecewise_poly_ub(a->branches[a->nbranches-1]);
    }

}

/********************************************************//**
*   Get number of pieces in a piecewise poly
*
*   \param a [in] - pw polynomial
*   \param N [inout] - number of pieces
*************************************************************/
void piecewise_poly_nregions(size_t * N, struct PiecewisePoly * a)
{
    if (a->leaf == 1){
        *N = *N + 1;
    }
    else{
        size_t ii;
        for (ii = 0; ii < a->nbranches; ii++){
            piecewise_poly_nregions(N,a->branches[ii]);
        }
    }
}


/********************************************************//**
*   Get boundary nodes between piecewise polynomials
*
*   \param a [in] - pw polynomial
*   \param N [inout] - number of nodes (including lower and upper)
*   \param nodes [inout] - allocated nodes (can be NULL)
*   \param onNum [inout] - node which I am filling
*   
*   \note
*       Call this function as follows to obtain full recursion and answer
*       
*       \code{.c}
*           // struct PiecewisePoly * a = ...; // have a be a piecewise poly
*           double * nodes = NULL;
*           size_t N;
*           piecewise_poly_boundaries(a,&N,&nodes,NULL);
*       \endcode
*************************************************************/
void 
piecewise_poly_boundaries(struct PiecewisePoly * a, size_t *N, 
            double ** nodes,
            size_t * onNum)
{
    if (*nodes == NULL){ // first allocate required number of nodes
        size_t nregions = 0;
        piecewise_poly_nregions(&nregions,a);
        *N = nregions + 1;
        *nodes = calloc_double(*N);
        if ((*N) == 2){
            (*nodes)[0] = a->ope->lower_bound;
            (*nodes)[1] = a->ope->upper_bound;
        }
        else{
            size_t start = 0;
            piecewise_poly_boundaries(a,N,nodes,&start);
        }
    }
    else{
        if ( (*onNum) == 0){
            (*nodes)[0] = piecewise_poly_lb(a);
            *onNum = 1;
            piecewise_poly_boundaries(a,N,nodes,onNum);
        }
        else{
            if (a->leaf == 1){
                (*nodes)[*onNum] = a->ope->upper_bound;
                *onNum = (*onNum) + 1;
            }
            else{
                size_t ii;
                for (ii = 0; ii < a->nbranches; ii++){
                    piecewise_poly_boundaries(a->branches[ii],N,nodes,onNum);
                }
            }
        }

    }
}

/********************************************************//**
*   Evaluate a piecewise polynomial
*
*   \param poly [in] - pw poly
*   \param x [in] - location at which to evaluate
*
*   \return out - pw value
*************************************************************/
double
piecewise_poly_eval(struct PiecewisePoly * poly, double x){
    
    double out = 0.1234567890;
    if (poly->leaf == 1){
        out = orth_poly_expansion_eval(poly->ope, x);
    }
    else{
        size_t ii;
        for (ii = 0; ii < poly->nbranches; ii++){
            if (x <= piecewise_poly_ub(poly->branches[ii])){
                out = piecewise_poly_eval(poly->branches[ii],x);
                break;
            }
        }
    }
    return out;
}

/********************************************************//**
*   Differentiate a piecewise polynomial
*   
*   \param p [in] - pw poly to differentiate (from the left)
*
*   \return pnew - polynomial
*************************************************************/
struct PiecewisePoly * 
piecewise_poly_deriv(struct PiecewisePoly * p)
{

    struct PiecewisePoly * pnew = NULL;
    if (p == NULL){
        return pnew;
    }
    else if (p->leaf == 1){
        pnew = piecewise_poly_alloc();
        pnew->ope = orth_poly_expansion_deriv(p->ope);
    }
    else{
        pnew = piecewise_poly_alloc();
        pnew->leaf = 0;
        pnew->nbranches = p->nbranches;
        pnew->branches = piecewise_poly_array_alloc(p->nbranches);
        size_t ii;
        for (ii = 0; ii < p->nbranches; ii++){
            pnew->branches[ii] = piecewise_poly_deriv(p->branches[ii]);
        }
    }
    return pnew;
}

/********************************************************//**
*   Integrate a piecewise polynomial
*
*   \param poly [in] - pw polynomial to integrate
*
*   \return out - Integral of approximation
*************************************************************/
double
piecewise_poly_integrate(struct PiecewisePoly * poly)
{
    double out = 0.0;
    if (poly->leaf == 1){
        out = orth_poly_expansion_integrate(poly->ope);
    }
    else{
        size_t ii;
        for (ii = 0; ii < poly->nbranches; ii++){
            out = out + piecewise_poly_integrate(poly->branches[ii]);
        }
    }
    return out;
}

double * piecewise_poly_rr(struct PiecewisePoly * p, size_t * nkeep)
{
    double * real_roots = NULL;   
    if ( p->leaf == 1){
        real_roots = orth_poly_expansion_real_roots(p->ope, nkeep);
    }
    else{
        real_roots = piecewise_poly_rr(p->branches[0],nkeep);
        size_t ii,n0;
        for (ii = 1; ii < p->nbranches; ii++){
            n0 = 0;
            double * roots2 = piecewise_poly_rr(p->branches[ii], &n0);
            if (n0 > 0){
                real_roots = realloc(real_roots,(*nkeep + n0)*sizeof(double));
                assert(real_roots != NULL);
                memmove(real_roots + *nkeep, roots2, n0*sizeof(double));
                *nkeep = *nkeep + n0;
                free(roots2); roots2 = NULL;
            }
        }
    }
    return real_roots;
}

/********************************************************//**
*   Obtain the real roots of a pw polynomial (only gives 1 of repeating roots)
*
*   \param p [in] - piecewise polynomial
*   \param nkeep [inout] - returns how many real roots there are 
*
*   \return real_roots - real roots of the pw polynomial
*
*   \note
*       Each root may be repeated twice
*************************************************************/
double *
piecewise_poly_real_roots(struct PiecewisePoly * p, size_t * nkeep)
{
    *nkeep = 0;    
    //printf("lb = %G\n",piecewise_poly_lb(p));
    double * roots = piecewise_poly_rr(p,nkeep);
    
    /*
    size_t ii, jj;
    size_t * keep = calloc_size_t(*nkeep);
    for (ii = 0; ii < *nkeep; ii++){
        for (jj = ii+1; jj < *nkeep; jj++){
            if (fabs(roots[ii] - roots[jj]) <= 1e-10){
                keep[jj] = 1;
            }
        }
    }
    size_t counter = 0;
    for (ii = 0; ii < *nkeep; ii++){
        if (keep[ii] == 0){
            counter++;
        }
    }
    double * real_roots = calloc_double(counter);
    for (ii = 0; ii < *nkeep; ii++){
        if (keep[ii] == 0){
            real_roots[ii] = roots[ii];
        }
    }
    free(keep); keep = NULL;
    free(roots); roots = NULL;
    */
    return roots;
}

/********************************************************//**
*   Obtain the maximum of a pw polynomial
*
*   \param p [in] - pw polynomial
*   \param x [inout] - location of maximum value
*
*   \return  val - maximum value
*   
*   \note
*       if constant function then just returns the left most point
*************************************************************/
double piecewise_poly_max(struct PiecewisePoly * p, double * x)
{
    double locfinal, valfinal;
    if ( p->leaf == 1){
        return orth_poly_expansion_max(p->ope, x);
    }
    else{
        size_t ii = 0;
        double loc2, val2;
        valfinal = piecewise_poly_max(p->branches[0],&locfinal);
        for (ii = 1; ii < p->nbranches;ii++){
            val2 = piecewise_poly_max(p->branches[ii],&loc2);
            if (val2 > valfinal){
                valfinal = val2;
                locfinal = loc2;
            }
        }
    }
    *x = locfinal;
    return valfinal;
}

/********************************************************//**
*   Obtain the minimum of a pw polynomial
*
*   \param p [in] - pw polynomial
*   \param x [inout] - location of minimum value
*
*   \return val - minimum value
*   
*   \note
*       if constant function then just returns the left most point
*************************************************************/
double piecewise_poly_min(struct PiecewisePoly * p, double * x)
{
    double locfinal, valfinal;
    if ( p->leaf == 1){
        return orth_poly_expansion_min(p->ope, x);
    }
    else{
        size_t ii = 0;
        double loc2, val2;
        valfinal = piecewise_poly_min(p->branches[0],&locfinal);
        for (ii = 1; ii < p->nbranches;ii++){
            val2 = piecewise_poly_min(p->branches[ii],&loc2);
            if (val2 < valfinal){
                valfinal = val2;
                locfinal = loc2;
            }
        }
    }
    *x = locfinal;
    return valfinal;
}

/********************************************************//**
*   Obtain the absolute maximum of a pw polynomial
*
*   \param p [in] - pw polynomial
*   \param x [inout] - location of absolute maximum
*
*   \return val - absolute maximum
*   
*   \note
*       if constant function then just returns the left most point
*************************************************************/
double piecewise_poly_absmax(struct PiecewisePoly * p, double * x)
{
    
    //printf("here!\n");
    double locfinal, valfinal;
    if ( p->leaf == 1){
        //double lb = piecewise_poly_lb(p);
        //double ub = piecewise_poly_ub(p);
        //printf("in leaf (%G,%G)\n",lb,ub);
        //if ((ub - lb) < 1000.0*DBL_EPSILON){
        //    return 0.0;
        //}
        //print_orth_poly_expansion(p->ope,3,NULL);
        double maxval = orth_poly_expansion_absmax(p->ope, x);
        //printf("max is %G \n",maxval);
        return maxval;
    }
    else{
        double loc2, val2;
        size_t ii = 0;
        valfinal = piecewise_poly_absmax(p->branches[0],&locfinal);
        //printf("nbranches = %zu\n",p->nbranches);
        for (ii = 1; ii < p->nbranches;ii++){
            val2 = piecewise_poly_absmax(p->branches[ii],&loc2);
            if (val2 > valfinal){
                valfinal = val2;
                locfinal = loc2;
            }
        }
    }
    *x = locfinal;
    return valfinal;
}

/********************************************************//**
*   Compute the norm of piecewise polynomial
*
*   \param p [in] - pw polynomial of which to obtain norm
*
*   \return out - norm of function
*
*   \note
*        Computes int_a^b f(x)^2 dx
*************************************************************/
double piecewise_poly_norm(struct PiecewisePoly * p){
    
    double out = piecewise_poly_inner(p,p);
    return sqrt(out);
}

/********************************************************//**
*   Multiply piecewise polynomial by -1
*
*   \param p [inout] - pw polynomial to multiply by -1
*************************************************************/
void 
piecewise_poly_flip_sign(struct PiecewisePoly * p)
{   
    if (p->leaf == 1){
        orth_poly_expansion_flip_sign(p->ope);
    }
    else{
        size_t ii;
        for (ii = 0; ii < p->nbranches; ii++){
            piecewise_poly_flip_sign(p->branches[ii]);
        }
    }
}

/////////////////////////////////////////////////////////////////////////

/********************************************************//**
*   Reapproximate a piecewise poly on a finer grid.
*
*   \param a [in] - pw polynomial to reapproximate
*   \param N [in] - number of nodes (including lb,ub)
*   \param nodes [in] - nodes at which to approximate (includes lb,ub);
*
*   \note
*       Each of the new pieces must be fully encompassed by an old piece
*       NOTE USES LEGENDRE POLYNOMIALS ON EACH LEAF
*************************************************************/
struct PiecewisePoly *
piecewise_poly_finer_grid(struct PiecewisePoly * a, size_t N, double * nodes)
{
    
    struct PiecewisePoly * p = NULL;
    if (N == 2){
        assert( a->leaf == 1);
        p = piecewise_poly_copy(a);
    }
    else{
        struct OpeAdaptOpts aopts;
        aopts.start_num = 8;
        aopts.coeffs_check = 2;
        aopts.tol = 1e-14;

        p->leaf = 0;
        p->nbranches = N-1;
        p->branches = piecewise_poly_array_alloc(N-1);
        size_t ii;
        for (ii = 0; ii < N-1; ii++){
            p->branches[ii] = piecewise_poly_alloc();
            p->branches[ii]->leaf = 1;
            p->branches[ii]->ope = 
                orth_poly_expansion_approx_adapt(pw_eval,&a,
                        LEGENDRE, nodes[ii],nodes[ii+1], &aopts);
        }
    }
    return p;
}

//////////////////////////////////////
/********************************************************//**
*   Compute the product of two piecewise polynomials
*
*   \param a [in] - first pw polynomial
*   \param b [in] - second pw polynomial
*
*   \return c - pw polynomial
*
*   \note 
*        Computes c(x) = a(x)b(x) where c is same form as a
*************************************************************/
struct PiecewisePoly *
piecewise_poly_prod(struct PiecewisePoly * a,
                    struct PiecewisePoly * b)
{
    struct PwPolyAdaptOpts aopts;
    aopts.ptype = LEGENDRE;
    aopts.maxorder = 7;
    aopts.coeff_check = 2;
    aopts.epsilon = 1e-7;
    aopts.minsize = 1e-3;
    aopts.nregions = 5;
    aopts.pts = NULL;

    double lb = piecewise_poly_lb(a);
    double ub = piecewise_poly_ub(a);
    struct PwCouple pwc;
    pwc.a = a;
    pwc.b = b;
    
    //printf("in prod \n");
    struct PiecewisePoly *c = 
            piecewise_poly_approx1_adapt(pw_eval_prod, &pwc, lb, ub, &aopts);
    //printf("out of prod \n");
    return c;
}


/********************************************************//**
*   Inner product between two pw polynomials 
*
*   \param a [in] - first pw polynomial
*   \param b [in] - second pw polynomai
*
*   \return out - inner product
*
*   Notes: 
*          Computes int_{lb}^ub  a(x)b(x) dx 
*************************************************************/
double 
piecewise_poly_inner(struct PiecewisePoly * a, struct PiecewisePoly * b)
{
    //printf("there!\n");
    struct PiecewisePoly * c = piecewise_poly_prod(a,b);
    /*
    printf("===============================================\n");
    print_piecewise_poly(c,3,NULL);
    printf("===============================================\n");
    */
    //printf("got prod\n");
    double out = piecewise_poly_integrate(c);
    piecewise_poly_free(c);
    return out;
}



/********************************************************//**
*   Multiply by scalar and add two PwPolynomials
*
*   \param a [in] - scaling factor for first pw poly
*   \param x [in] - first pw poly
*   \param b [in] - scaling factor for second pw poly
*   \param y [in] - second pw poly
*
*   \return p - pw poly
*
*   \note 
*       Computes z=ax+by, where x and y are pw polys
*       Requires both polynomials to have the same upper 
*       and lower bounds
*   
*************************************************************/
struct PiecewisePoly *
piecewise_poly_daxpby(double a, struct PiecewisePoly * x,
                       double b, struct PiecewisePoly * y)
{
    struct PwPolyAdaptOpts aopts;
    aopts.ptype = LEGENDRE;
    aopts.maxorder = 7;
    aopts.coeff_check = 2;
    aopts.epsilon = 1e-8;
    aopts.minsize = 1e-3;
    aopts.nregions = 5;
    aopts.pts = NULL;

    double lb = piecewise_poly_lb(x);
    double ub = piecewise_poly_ub(x);
    struct PwCouple pwc;
    pwc.a = x;
    pwc.b = y;
    pwc.coeff[0] = a;
    pwc.coeff[1] = b;

    struct PiecewisePoly *c = 
            piecewise_poly_approx1_adapt(pw_eval_sum, &pwc, lb, ub, &aopts);
    return c;
}


/********************************************************//**
*   Compute the sum of two piecewise polynomials with
*   matching hierarchy
*   
*   \param a [in] - weight of first pw polynomial
*   \param x [in] - first pw polynomial
*   \param b [in] - weight of second pw polynomial
*   \param y [in] - second pw polynomial
*
*   \return c - pw polynomial
*
*   \note 
*        Computes c = a*x + b*x where c is same form as a
*************************************************************/
struct PiecewisePoly *
piecewise_poly_matched_daxpby(double a, struct PiecewisePoly * x,
                              double b,struct PiecewisePoly * y)
{

    struct PiecewisePoly * c = piecewise_poly_alloc();
    if (y == NULL){
        assert (x != NULL);
        if ( x->leaf == 1){
            c->leaf = 1;
            c->ope = orth_poly_expansion_daxpby(a,x->ope,b,NULL);
        }
        else{
            c->leaf = 0;
            c->nbranches = x->nbranches;
            c->branches = piecewise_poly_array_alloc(c->nbranches);
            size_t ii;
            for (ii = 0; ii < c->nbranches; ii++){
                c->branches[ii] = 
                    piecewise_poly_matched_daxpby(a,x->branches[ii],b,NULL);
            }
        }
    }
    else if ( x == NULL){
        assert ( y != NULL );
        if ( y->leaf == 1){
            c->leaf = 1;
            c->ope = orth_poly_expansion_daxpby(b,y->ope,a,NULL);
        }
        else{
            c->leaf = 0;
            c->nbranches = y->nbranches;
            c->branches = piecewise_poly_array_alloc(c->nbranches);
            size_t ii;
            for (ii = 0; ii < c->nbranches; ii++){
                c->branches[ii] = 
                    piecewise_poly_matched_daxpby(b,y->branches[ii],a,NULL);
            }
        }
    }
    else{
        //printf("in here!\n");
        if ( x->leaf == 1 ){
            assert ( y->leaf == 1);
            c->leaf = 1;
            c->ope = orth_poly_expansion_daxpby(a,x->ope,b,y->ope);
        }
        else{
            assert (x->nbranches == y->nbranches);
            c->leaf = 0;
            c->nbranches = y->nbranches;
            c->branches = piecewise_poly_array_alloc(c->nbranches);
            size_t ii;
            for (ii = 0; ii < c->nbranches; ii++){
                c->branches[ii] = 
                    piecewise_poly_matched_daxpby(a,x->branches[ii],
                                                  b,y->branches[ii]);
            }
        }
    }
    return c;
}

/********************************************************//**
*   Compute the product of two piecewise polynomials with
*   matching hierarchy
*
*   \param a [in] - first pw polynomial
*   \param b [in] - second pw polynomial
*
*   \return c - pw polynomial
*
*   \note 
*        Computes c(x) = a(x)b(x) where c is same form as a
*************************************************************/
struct PiecewisePoly *
piecewise_poly_matched_prod(struct PiecewisePoly * a,struct PiecewisePoly * b)
{
    struct PiecewisePoly * c = piecewise_poly_alloc();

    if ( a->leaf == 1){
        assert ( b->leaf == 1);
        c->leaf = 1;
        c->ope = orth_poly_expansion_prod(a->ope,b->ope);
    }
    else{
        assert ( a->nbranches == b->nbranches );
        c->leaf = 0;
        c->nbranches = a->nbranches;
        c->branches = piecewise_poly_array_alloc(c->nbranches);
        size_t ii;
        for (ii = 0; ii < c->nbranches; ii++){
            c->branches[ii] = 
                piecewise_poly_matched_prod(a->branches[ii],b->branches[ii]); 
        }
    }
    return c;
}



/********************************************************//**
*   Convert two piecewise polynomials to ones with matching
*   splits / hierarchy
*
*   \param ain [in] - first pw polynomial
*   \param aout [inout] - new matched pw polynomial 1 (unallocated)
*   \param bin [in] - second pw polynomial
*   \param bout [inout] - new matched pw polynomial 2 (unallocated)
*
*   \note
*       New lower bound is highest lower bound, and new upper bound is lowest upper bound
*************************************************************/
void
piecewise_poly_match(struct PiecewisePoly * ain, struct PiecewisePoly ** aout,
                     struct PiecewisePoly * bin, struct PiecewisePoly ** bout)
{
    double * nodesa = NULL;
    double * nodesb = NULL;
    size_t Na, Nb;
    piecewise_poly_boundaries(ain, &Na, &nodesa,NULL);
    piecewise_poly_boundaries(bin, &Nb, &nodesb,NULL);
    
    
    double lb = nodesa[0] < nodesb[0] ? nodesa[0] : nodesb[0];
    double ub = nodesa[Na-1] > nodesb[Nb-1] ? nodesa[Na-1] : nodesb[Nb-1];
    
    //printf("Na = %zu, Nb = %zu\n",Na,Nb);
    //printf("lower bound = %3.5f\n",lb);
    //printf("upper bound = %3.5f\n",ub);

    double * nodes = calloc_double(Na + Nb);
    nodes[0] = lb;

    size_t inda = 1;
    while (nodesa[inda] < lb){
        inda++;
    }
    size_t indb = 1;
    while (nodesb[indb] < lb){
        indb++;
    }

    size_t cind = 1;
    while (nodes[cind-1] < ub){
        if (nodesa[inda] <= nodesb[indb]){
            nodes[cind] = nodesa[inda];
        }
        else if (nodesb[indb] < nodesa[inda]){
            nodes[cind] = nodesb[indb];
        }

        if (fabs(nodesb[indb] - nodesa[inda]) < DBL_EPSILON){
            inda++;
            indb++;
        }
        else if (nodesb[indb] < nodesa[inda]){
            indb++;
        }
        else if (nodesa[inda] < nodesb[indb]){
            inda++;
        }
        cind++;
    }

    double * newnodes = realloc(nodes, cind * sizeof(double));
    if (newnodes == NULL){
        fprintf(stderr,"Error (re)allocating memory in piecewise_poly_match");
        exit(1);
    }
    else{
        nodes = newnodes;
    }

    
    //printf("Number of matched nodes are %zu \n", cind);
    //dprint(cind,nodes);

    *aout = piecewise_poly_finer_grid(ain, cind, nodes);
    *bout = piecewise_poly_finer_grid(bin, cind, nodes);
    free(nodesa); nodesa = NULL;
    free(nodesb); nodesb = NULL;
    free(nodes); nodes = NULL;
}



/********************************************************//**
*   Remove left-most piece of pw Poly
*
*   \param a [inout] - pw polynomial to trim
*
*   \return poly - left most orthogonal polynomial expansion
*
*   \note
*       If *a* doesn't have a split, the orthogonal expansion is extracted
*       and a is turned to NULL
*************************************************************/
/*
struct OrthPolyExpansion * 
piecewise_poly_trim_left(struct PiecewisePoly ** a)
{   
    struct OrthPolyExpansion * poly = NULL;
    if (a == NULL){
        return poly;
    }
    else if (*a == NULL){
        return poly;
    }
    else if ((*a)->ope != NULL){
        poly = orth_poly_expansion_copy((*a)->ope);
        piecewise_poly_free(*a);
        *a = NULL;
    }
    else if ( (*a)->down[0]->ope != NULL)  // remove the left 
    {
        //printf("removing left lb=%G ub=%G \n", piecewise_poly_lb((*a)->left), piecewise_poly_ub((*a)->left));
        //printf("new lb should be %G \n", piecewise_poly_lb((*a)->right));
        poly = orth_poly_expansion_copy((*a)->down[0]->ope);
        //piecewise_poly_free( (*a)->left);
        
        (*a)->nbranches -= 1;
        size_t ii;
        for (ii = 0; ii < (*a)->nbranches; ii++){
            piecewise_poly_free((*a)->down[ii]);
            (*a)->down[ii] = piecewise_poly_copy((*a)->down[ii+1]);
        }
        piecewise_poly_free((*a)->down[(*a)->nbranches]);
        //(*a)->ope = (*a)->right->ope;
        //if ( (*a)->ope == NULL){
        //    (*a)->split = (*a)->right->split;
        //   // printf("new split is %G\n", (*a)->split);
        //}
        //(*a)->left = (*a)->right->left;
        //(*a)->right = (*a)->right->right;
       // printf("new lb = %G \n", piecewise_poly_lb(*a));
    }
    else {
        poly = piecewise_poly_trim_left( &((*a)->down[0]));
    }
    return poly;
}
*/


/********************************************************//**
*   Check if discontinuity exists between two neighboring
*   piecewise polynomials (upper bound of left == lower bound of right)
*  
*   \param left [in] - left pw polynomial   
*   \param right [in] - right pw polynomial   
*   \param numcheck [in] - number of derivatives to check (if 0 then check only values)
*   \param tol [in] - tolerance defining how big a jump defins discontinuity
*
*   \return 0 if no discontinuity, 1 if discontinuity
*************************************************************/
int piecewise_poly_check_discontinuity(struct PiecewisePoly * left, 
                                       struct PiecewisePoly * right, 
                                       int numcheck, double tol)
{
    if (numcheck == -1){
        return 0;
    }

    double ubl = piecewise_poly_ub(left);
    double lbr = piecewise_poly_lb(right);
    assert(fabs(ubl-lbr) < DBL_EPSILON*100);

    double val1, val2;
    val1 = piecewise_poly_eval(left, ubl);
    val2 = piecewise_poly_eval(right, lbr);

    double diff = fabs(val1-val2);
    if (fabs(val1) >= 1.0){
        diff /= fabs(val1);
    }
    
    int out;
    if ( diff < tol ){
        struct PiecewisePoly * dleft = piecewise_poly_deriv(left);
        struct PiecewisePoly * dright = piecewise_poly_deriv(right);
        out = piecewise_poly_check_discontinuity(dleft, dright, numcheck-1,tol);
        piecewise_poly_free(dleft); dleft = NULL;
        piecewise_poly_free(dright); dright = NULL;
    }
    else{
        out = 1;
    }
    return out;
}

/*
struct PiecewisePoly *
piecewise_poly_merge_left(struct PiecewisePoly ** p, struct PwPolyAdaptOpts * aopts)
{
    struct PiecewisePoly * pnew = NULL;
    //printf("in here\n");
    if (p == NULL){
        return pnew;
    }
    else if (*p == NULL){
        return pnew;
    }
    else if ( (*p)->ope != NULL){
        pnew = piecewise_poly_copy(*p);
        return pnew;
    }
    
    pnew = piecewise_poly_alloc();
    pnew->left = piecewise_poly_alloc();
    //printf("p == NULL = %d\n", p ==NULL);
    pnew->left->ope = piecewise_poly_trim_left(p);
    int disc;
    if ( *p == NULL ){
        pnew->ope = orth_poly_expansion_copy(pnew->left->ope);
        piecewise_poly_free(pnew->left);
        disc = -1;
    }
    else{
        disc = piecewise_poly_check_discontinuity(pnew->left, *p, 2, 1e-1);
    }
    while (disc == 0){
        //printf("discontinuity does not exist at %G \n", piecewise_poly_ub(pnew->left));
        struct OrthPolyExpansion * rightcut = piecewise_poly_trim_left(p);

        //printf("p == NULL 2 = %d\n", *p ==NULL);
        //printf("rightcut == NULL = %d\n", rightcut == NULL);

        struct PiecewisePoly temp;
        temp.ope = rightcut;

        struct PwCouple c;
        c.a = pnew->left;
        c.b = &temp;
            
        struct PiecewisePoly * newleft = piecewise_poly_alloc();
        if (aopts == NULL){
            newleft->ope = orth_poly_expansion_approx_adapt(pw_eval_neighbor,&c,
               rightcut->p->ptype, piecewise_poly_lb(c.a), piecewise_poly_ub(c.b), NULL);

        }
        else{
            struct OpeAdaptOpts adopts;
            adopts.start_num = aopts->maxorder;
            adopts.coeffs_check = aopts->coeff_check;
            adopts.tol = aopts->epsilon;

            newleft->ope = orth_poly_expansion_approx_adapt(pw_eval_neighbor,&c,
                rightcut->p->ptype, piecewise_poly_lb(c.a), piecewise_poly_ub(c.b), &adopts);
        }
        orth_poly_expansion_free(rightcut);
        rightcut = NULL;
        
        piecewise_poly_free(pnew); pnew = NULL;
        if ( *p == NULL ){
            pnew = piecewise_poly_copy(newleft);
            disc = -1;
        }
        else{
            pnew = piecewise_poly_alloc();
            pnew->left = piecewise_poly_copy(newleft);
            disc = piecewise_poly_check_discontinuity(pnew->left, *p, 2, 1e-6);;
        }
        piecewise_poly_free(newleft); newleft = NULL;
    }
    //printf("p==NULL? %d\n",*p==NULL);
    if (disc == 1){
        pnew->split = piecewise_poly_ub(pnew->left);
        pnew->right = piecewise_poly_merge_left(p,aopts);
        pnew->ope = NULL;
    }

    return pnew;
}
*/

// factorial for x = 0...8
static const size_t factorial [] = {1, 1, 2, 6, 24, 120, 720, 5040, 40320};
double eval_coeff(size_t l, double * stencil, size_t nstencil)
{
    assert ( (nstencil-1) <= 8); //only ones for which I have a factorial
    
    // m = nstencil-1
    double out = (double) factorial[nstencil-1];
    size_t ii; 
    for (ii = 0; ii < nstencil; ii++){
        if (ii != l){
            out /= (stencil[l]-stencil[ii]);
        }
    }
    return out;
}

double eval_jump(double x, double * stencil, double * vals, size_t nstencil)
{
    double den = 0.0;
    double out = 0.0;
    double temp;
    size_t ii;
    for (ii = 0; ii < nstencil; ii++){
        temp = eval_coeff(ii,stencil,nstencil);
        if (stencil[ii] > x){
            den += temp;
        }
        out += temp * vals[ii];
    }
    assert (fabs(den) >= DBL_EPSILON);
    out = out / den;
    return out;
}

size_t get_stencil(double x, size_t nstencil, double * total, size_t ntotal)
{
    assert (nstencil <= ntotal);
    assert (x > total[0]);
    assert (x < total[ntotal-1]);
    size_t start;

    size_t ii = 0;
    while ( total[ii] < x){
        ii++;
    }

    if (ii == 1){
        start = 0;
    }
    else if (ii == (ntotal-1)){
        start = ntotal-nstencil;
    }
    else{
        size_t f = ii-1;
        size_t b = ii;
        // now total[ii] is the first element greater than x in total
        double difff, diffb;
        size_t ninc = 2;
        while (ninc < nstencil){
            if (f == 0){
                b++;
            }
            else if (b == ntotal-1){
                f--;
            }
            else{
                difff = total[b+1] - x;
                diffb = x - total[f-1];
                if (difff < diffb){
                    b++;
                }
                else{
                    f--;
                }
            }
            ninc ++;
        }
        start = f;
    }
    return start;
}

/********************************************************//**
*   Evaluate a MinModed jump function based on polynomial annihilation on
*   function values obtained at a sorted set (low to high) of input
*   
*   \param x [in] - location to check
*   \param total [in] - set of sorted points
*   \param evals [in] - function evaluations
*   \param ntotal [in] - number of points
*   \param minm [in] - minimum polynomial order to annihilate
*   \param maxm [in] - maximum polynomial order to annihilate
*
*   \return jump - jump value;
*************************************************************/
double minmod_eval(double x, double * total, double * evals, size_t ntotal, 
                    size_t minm, size_t maxm)
{   
    // note m = nstencil-1
    size_t start = get_stencil(x,minm+1,total, ntotal);
    double jump = eval_jump(x, total+start, evals+start, minm+1);
    int sign = 1;
    if (jump < 0.0){
        sign = -1;
    }
    size_t ii;
    double newjump;
    int newsign;
    for (ii = minm+1; ii <= maxm; ii++){
        newsign = 1;
        start = get_stencil(x,ii+1, total, ntotal);
        newjump = eval_jump(x, total+start, evals+start,ii+1);
        if (newjump < 0.0){
            newsign = -1;
        }
        if (newsign != sign){
            return 0.0;
        }
        else if (sign > 0){
            if (newjump < jump){
                jump = newjump;
            }
        }
        else { // sign < 0
            if (newjump > jump){
                jump = newjump;
            }
        }
    }
    return jump;
}

/********************************************************//**
*   Check whether discontinuity exists based on polynomial annihilation 
*   of function values obtained at a sorted set (low to high) of input
*   
*   \param x [in] - location to check
*   \param total [in] - set of sorted points
*   \param evals [in] - function evaluations
*   \param ntotal [in] - number of points
*   \param minm [in] - minimum polynomial order to annihilate
*   \param maxm [in] - maximum polynomial order to annihilate
*
*   \return disc = 1 if discontinuity exists 0 if it does not
*************************************************************/
int minmod_disc_exists(double x, double * total, double * evals, 
                        size_t ntotal,size_t minm, size_t maxm)
{
    
    double jump = minmod_eval(x,total,evals,ntotal,minm,maxm);
    double h = total[1]-total[0];
    double diff;
    size_t ii = 2;
    for (ii = 2; ii < ntotal; ii++){
        diff = total[ii]-total[0];
        if (diff < h){
            h = diff;
        }
    }
    
    double oom = floor(log10(h));
    //double oom = ceil(log10(h));
    //printf("oom=%G\n",oom);
    int disc = 1;
    if (fabs(jump) <= pow(10.0,oom)){
    //if (fabs(jump) <= pow(10.0,oom)){
        disc= 0;   
    }
    //printf("(x,jump,tol,disc) = (%G,%G,%G,%d)\n",
    //                    x,jump,pow(10.0,oom),disc);
    return disc;
}

/********************************************************//**
*   Jump detector, locates jumps (discontinuities) in a one dimensional function
*   
*   \param f [in] - one dimensional function
*   \param args [in] - function arguments
*   \param lb [in] - lower bound of domain
*   \param ub [in] - upper bound of domain
*   \param nsplit [in] - number of subdomains to split if disc possibly exists
*   \param tol [in] - closest distance between two function evaluations, defines the
*   resolution
*   \param edges [inout] - array of edges (location of jumps)
*   \param nEdge [inout] - number of edges 
*
*************************************************************/
void locate_jumps(double (*f)(double, void *), void * args,
                  double lb, double ub, size_t nsplit, double tol,
                  double ** edges, size_t * nEdge)
{
    size_t minm = 2;
    size_t maxm = 5; // > nsplit ? nsplit-1 : 8 ;
    //size_t maxm = 5 >= nsplit ? nsplit-1 : 8 ;

    if ((ub-lb) < tol){
        //printf("add edge between (%G,%G)\n",lb,ub);
        double out = (ub+lb)/2.0;

        double * new_edge = realloc(*edges, (*nEdge+1)*sizeof(double));
        assert (new_edge != NULL);
        *edges = new_edge;
        (*edges)[*nEdge] = out;
        (*nEdge) = (*nEdge)+1;

    }
    else{
        //printf("refine from %G - %G\n",lb,ub);
        double * pts = linspace(lb,ub,nsplit+1);
        double * vals = calloc_double(nsplit+1);
        size_t ii;
        for (ii = 0; ii < nsplit+1; ii++){
            vals[ii] = f(pts[ii],args);    
        }
        double x;
        int disc;
        //double jump;
        for (ii = 0; ii < nsplit; ii++){
            x = (pts[ii] + pts[ii+1])/2.0;
            disc = minmod_disc_exists(x,pts,vals,nsplit+1,minm,maxm);
            //jump = minmod_eval(x,pts,vals,nsplit+1,minm,maxm);
            //printf("checking disc at %G = (%G,%d)\n",x,jump,disc);
            //printf("disc exists at %G\n",x);
            if (disc == 1){ // discontinuity potentially exists so refine
                locate_jumps(f,args,pts[ii],pts[ii+1],nsplit,tol,edges,nEdge);
            }
        }
        free(pts); pts = NULL;
        free(vals); vals = NULL;
    }
}

/********************************************************//**
*   Create Approximation by polynomial annihilation-based splitting
*   
*   \param f [in] - function to approximate
*   \param args [in] - function arguments
*   \param lb [in] - lower bound of input space
*   \param ub [in] - upper bound of input space
*   \param aoptsin [in] - approximation options
*
*   \return p - polynomial
*************************************************************/
/*
struct PiecewisePoly *
piecewise_poly_approx2(double (*f)(double, void *), void * args, double lb,
                        double ub, struct PwPolyAdaptOpts * aoptsin)
{
    struct PwPolyAdaptOpts * aopts = NULL;
    if (aoptsin != NULL){
        aopts = aoptsin;
    }
    else{
        aopts = malloc(sizeof(struct PwPolyAdaptOpts));
        if (aopts == NULL){
            fprintf(stderr, "cannot allocate space for pw adapt opts");
            exit(1);
        }
        aopts->ptype = LEGENDRE;
        aopts->maxorder = 30;
        aopts->minsize = 1e-13;
        aopts->coeff_check = 4;
        aopts->epsilon = 1e-10;
        aopts->other = NULL;
    }
    
    double * edges = NULL;
    size_t nEdge = 0;
    //printf("locate jumps!\n");
    locate_jumps(f,args,lb,ub,aopts->maxorder,aopts->minsize,&edges,&nEdge);

    //printf("number of edges are %zu\n",nEdge);
    //printf("Edges are\n");
    //size_t iii;
    //for (iii = 0; iii < nEdge;iii++){
    //    printf("(-buf, , buf) (%3.15G,%3.15G,%3.15G)\n",
    //        edges[iii]-aopts->minsize,edges[iii],edges[iii]+aopts->minsize);
    //}
    //dprint(nEdge,edges);
    //

    struct OpeAdaptOpts adopts;
    adopts.start_num = 6;
    adopts.coeffs_check = aopts->coeff_check;
    adopts.tol = aopts->epsilon;
    
    double * nodes = calloc_double(nEdge*2+2);
    nodes[0] = lb;
    size_t ii,jj = 1;
    for (ii = 0; ii < nEdge; ii++){
        nodes[jj] = edges[ii] - aopts->minsize;
        nodes[jj+1] = edges[ii] + aopts->minsize;
        jj += 2;
    }
    nodes[nEdge*2+1] = ub;
    
    struct PiecewisePoly * p = 
        piecewise_poly_approx_from_nodes(f, args, aopts->ptype,
                         nodes,nEdge*2+2, 2.0*aopts->minsize, &adopts);
    
    free(edges); edges = NULL;
    free(nodes); nodes = NULL;

    if (aoptsin == NULL){
        free(aopts);
        aopts = NULL;
    }
    return p;
}
*/

/********************************************************//**
*   Create Approximation by hierarchical splitting
*   
*   \param f [in] - function to approximate
*   \param args [in] - function arguments
*   \param lb [in] - lower bound of input space
*   \param ub [in] - upper bound of input space
*   \param aoptsin [in] - approximation options
*
*   \return p - polynomial
*************************************************************/
struct PiecewisePoly *
piecewise_poly_approx1(double (*f)(double, void *), void * args, double lb,
                        double ub, struct PwPolyAdaptOpts * aoptsin)
{
    struct PwPolyAdaptOpts * aopts = NULL;
    if (aoptsin != NULL){
        aopts = aoptsin;
    }
    else{
        aopts = malloc(sizeof(struct PwPolyAdaptOpts));
        if (aopts == NULL){
            fprintf(stderr, "cannot allocate space for pw adapt opts");
            exit(1);
        }
        aopts->ptype = LEGENDRE;
        aopts->maxorder = 7;
        //aopts->minsize = 1e5 * ZEROTHRESH;
        //aopts->coeff_check = 2;
        //aopts->epsilon = 1e-12;
        aopts->nregions = 5;
        aopts->pts = NULL;
        aopts->other = NULL;
    }

    size_t N = aopts->maxorder+1;
    struct PiecewisePoly * poly = piecewise_poly_alloc();
    if (aopts->nregions == 1){
        poly->leaf = 1;
        poly->ope = orth_poly_expansion_init(aopts->ptype, N, lb, ub);
        orth_poly_expansion_approx(f,args,poly->ope);
        orth_poly_expansion_round(&(poly->ope));
    }
    else{
        //printf("lb=%G,ub=%G,num=%zu\n",lb,ub,aopts->nregions);
        double * pts = linspace(lb,ub,aopts->nregions+1);
        //dprint(aopts->nregions+1,pts);
        //printf("\n");
        poly->leaf = 0;
        poly->nbranches = aopts->nregions;
        poly->branches = piecewise_poly_array_alloc(poly->nbranches);

        double clb,cub; 
        size_t ii;
        for (ii = 0; ii < poly->nbranches; ii++){
            if (aopts->pts == NULL){
                clb = pts[ii];
                cub = pts[ii+1];
            }
            else{
                clb = aopts->pts[ii];
                cub = aopts->pts[ii+1];
            }

            //printf("new upper = %G\n",cub);
            
            poly->branches[ii] = piecewise_poly_alloc();
            poly->branches[ii]->leaf = 1;
            poly->branches[ii]->ope = 
                orth_poly_expansion_init(aopts->ptype, N, clb, cub);
            orth_poly_expansion_approx(f,args,poly->branches[ii]->ope);
            orth_poly_expansion_round(&(poly->branches[ii]->ope));
        }
        free(pts); pts = NULL;
    }

    if (aoptsin == NULL){
        free(aopts);
        aopts = NULL;
    }

    return poly;
}

/********************************************************//**
*   Create Approximation by hierarchical splitting (adaptively)
*   
*   \param f [in] - function to approximate
*   \param args [in] - function arguments
*   \param lb [in] - lower bound of input space
*   \param ub [in] - upper bound of input space
*   \param aoptsin [in] - approximation options
*
*   \return p - polynomial
*************************************************************/
struct PiecewisePoly *
piecewise_poly_approx1_adapt(
                double (*f)(double, void *), void * args, double lb,
                double ub, struct PwPolyAdaptOpts * aoptsin)
{
    struct PwPolyAdaptOpts * aopts = NULL;
    if (aoptsin != NULL){
        aopts = aoptsin;
    }
    else{
        aopts = malloc(sizeof(struct PwPolyAdaptOpts));
        if (aopts == NULL){
            fprintf(stderr, "cannot allocate space for pw adapt opts");
            exit(1);
        }
        aopts->ptype = LEGENDRE;
        aopts->maxorder = 7;
        aopts->minsize = 1e-5;
        aopts->coeff_check = 2;
        aopts->epsilon = 1e-8;
        aopts->nregions = 5;
        aopts->pts = NULL;
        aopts->other = NULL;
    }
    
    struct PiecewisePoly * poly = piecewise_poly_approx1(f,args,lb,ub,aopts);
    size_t ii,jj;
    //printf("nbranches = %zu\n",poly->nbranches);
    for (ii = 0; ii < poly->nbranches; ii++){
        size_t npolys = poly->branches[ii]->ope->num_poly;
        double lbs = piecewise_poly_lb(poly->branches[ii]);
        double ubs = piecewise_poly_ub(poly->branches[ii]);
        //printf("checking branch (%G,%G)\n",lbs,ubs);
        int refine = 0;
        //printf("npolys = %zu\n",npolys);
        size_t ncheck = aopts->coeff_check < npolys ? 
                            aopts->coeff_check : npolys;
        double sum = 0.0;
        for (jj = 0; jj < npolys; jj++){
            sum += pow(poly->branches[ii]->ope->coeff[jj],2);
        }
        sum = 1.0;
        //sum = fmax(sum,1.0);
        for (jj = 0; jj < ncheck; jj++){
            double c =  poly->branches[ii]->ope->coeff[npolys-1-jj];
            //printf("coeff = %G\n,",c);
            if (fabs(c) > (aopts->epsilon * sum)){
                refine = 1;
                //printf("refine \n");
                break;
            }
        }
        //printf("(ubs-lbs)=%G, minsize=%G \n", ubs-lbs, aopts->minsize);
        if ( (ubs-lbs) < aopts->minsize ){
            refine = 0;
        }
        //*
        if (refine == 1){
            //printf("refining branch (%G,%G)\n",lbs,ubs);
            //printf("diff = %G, minsize = %G\n",ubs-lbs, aopts->minsize);
            //break;
            piecewise_poly_free(poly->branches[ii]);
            poly->branches[ii] = NULL;
            assert(aopts->pts == NULL);
            poly->branches[ii] = 
                piecewise_poly_approx1_adapt(f,args,lbs,ubs,aopts);//aopts);
                //piecewise_poly_approx1(f,args,lbs,ubs,aopts);

        }
        //*/
    }

    if (aoptsin == NULL){
        free(aopts);
        aopts = NULL;
    }

    return poly;
}

////////////////////////////////////////////////////////

/********************************************************//**
*   Serialize pw polynomial
*   
*   \param ser [in] - location to which to serialize
*   \param p [in] - polynomial
*   \param totSizeIn - if not null then only return total size of array without serialization! if NULL then serialiaze
*
*   \return ptr : pointer to end of serialization
*************************************************************/
unsigned char *
serialize_piecewise_poly(unsigned char * ser, 
        struct PiecewisePoly * p,
        size_t * totSizeIn)
{
    
    size_t totsize; 
    unsigned char * ptr = NULL;
    if (totSizeIn != NULL){
        if (p->leaf == 1){
            ptr = serialize_orth_poly_expansion(ser, p->ope, &totsize);
        }
        else{
            size_t tsize;
            size_t ii;
            ptr = serialize_piecewise_poly(ser, p->branches[0], &totsize);
            for (ii = 1; ii < p->nbranches; ii++){
                tsize = 0;
                ptr = serialize_piecewise_poly(ptr,p->branches[ii], &tsize);
                totsize += tsize;
            }
            totsize += sizeof(size_t); // for nbranches
        }
        *totSizeIn = totsize + sizeof(int); //sizeof(int) is for saying whether  it is a leaf or not
        return ptr;
    }
    else{
        int leaf;
        if (p->leaf == 1){
            leaf = 1;
            ptr = serialize_int(ser,leaf);
            ptr = serialize_orth_poly_expansion(ptr, p->ope, NULL);
        }
        else{
            leaf = 0;
            ptr = serialize_int(ser,leaf);
            ptr = serialize_size_t(ptr, p->nbranches);
            size_t ii;
            for (ii = 0; ii < p->nbranches; ii++){
                ptr = serialize_piecewise_poly(ptr,p->branches[ii], NULL);
            }
        }
    }
    return ptr;
}

/********************************************************//**
*   Deserialize pw polynomial
*
*   \param ser [in] - input string
*   \param poly [inout]: pw polynomial
*
*   \return ptr - ser + number of bytes of poly expansion
*************************************************************/
unsigned char * 
deserialize_piecewise_poly(unsigned char * ser, 
        struct PiecewisePoly ** poly)
{
    
    *poly = piecewise_poly_alloc();
    int leaf;
    unsigned char * ptr = deserialize_int(ser, &leaf);
    if (leaf == 1){
        (*poly)->leaf = 1;
        ptr = deserialize_orth_poly_expansion(ptr, &((*poly)->ope));
    }
    else{
        (*poly)->leaf = 0;
        //ptr = deserialize_int(ptr, &((*poly)->leaf));
        ptr = deserialize_size_t(ptr, &((*poly)->nbranches));
        (*poly)->branches = piecewise_poly_array_alloc((*poly)->nbranches);
        size_t ii;
        for (ii = 0; ii < (*poly)->nbranches; ii++){
            ptr = deserialize_piecewise_poly(ptr, &((*poly)->branches[ii]));
        }
    }
    
    return ptr;
}


void print_piecewise_poly(struct PiecewisePoly * pw, size_t prec, void *args)
{
    if (pw->ope != NULL){
        print_orth_poly_expansion(pw->ope,prec,args);
    }
    /*
    else{
        printf("%G\n",pw->split);
        printf("| \n");
        print_piecewise_poly(pw->left,prec,args);
    }
    */
}