/* ============================================================================

   Copyright (C) 2017, 2018, 2019  Konrad Bernloehr
   
   This file is part of sim_telarray (also known as sim_hessarray)
   and also part of the IACT/atmo package for CORSIKA.

   The software in this file is free software: you can redistribute it 
   and/or modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program. If not, see <http://www.gnu.org/licenses/>.

============================================================================ */

/* =============================================================== */
/** @file rpolator.h 
 *  @short Memory structure and interfaces for rpolator interpolation code.
 *
 *  @author  Konrad Bernloehr 
 *  @date    2017 ... 2019
 */
/* =============================================================== */

#ifndef RPOLATOR_H__LOADED
#define RPOLATOR_H__LOADED 1

#define WITH_RPOLATOR 1

#ifdef __cplusplus
extern "C" {
#endif

/** Cubic spline interpolation (natural cubic splines = scheme 3, clampled cubic splines = scheme 4) */
#include <stdio.h>
struct cubic_params
{
   double a, b, c, d;  /**< r=xp-x[i], yp = a + b*r + c*r^2 + d*r^3 = ((d*r + c) * r + b) * r + a; */
};
typedef struct cubic_params CsplinePar;

/** Structure describing an interpolation table, interpolation scheme and selected options. */

struct rpol_table
{
   int ndim;          /**< 1 or 2 dimension(s), for the independent variable(s) that is. */
   size_t nx, ny;     /**< No. of entries in x and (optional) y points. */
   double *x;         /**< Supporting points in x */
   double *y;         /**< Supporting points in y (if ndim=2 and ny>1) */
   double *z;         /**< Table values (size nx for 1-dim or nx*ny for 2-dim) */
   double *zxmax;     /**< Optional max. of z values along each y line (at each x value) */
   double *zxmin;     /**< Optional min. of z values along each y line (at each x value) */
   double aux;        /**< This auxiliary value may be useful to the application but is not used internally. */
   double xmin, xmax; /**< Range covered in x */
   double dx, dxi;    /**< Step size in x and inverse of it, for equidistant only. */
   double xrise;      /**< +1 if x values are in ascending order; -1 for descending */
   double ymin, ymax; /**< Range covered in y (if ndim=2) */
   double dy, dyi;    /**< Step size in y and inverse of it, for equidistant only (2-D). */
   double yrise;      /**< +1 if y values are in ascending order; -1 for descending */
   double zmin, zmax; /**< Range covered in result values */
   char *fname;       /**< Name of the file from which the table was loaded (includes all options). */
   char *options;     /**< Options used in option parameter or NULL pointer. */
   int equidistant;   /**< Equidistant support points make life much easier (bit 0: x, bit 1: y). */
   int scheme;        /**< Requested interpolation scheme (0: nearest, 1: linear, 
                           2: quadratic, 3: natural cubic spline, 4: clampled cubic spline). */
   int clipping;      /**< 0: Extrapolate with edge value, 1: zero outside range. */
   int remapped;      /**< If user code remaps x, y, and/or z values w.r.t. the table input,
                           it should mark this here to avoid repeating it.
                           (Make sure to call rpol_check_equi_range() after you remap!) */
   int zxreq;         /**< Flag activated when options indicate that a set of zxmax values should be provided. */
   int logs, xlog, ylog, zlog; /**< Log applied to any x/y axis, to x axis, y axis, z axis? */
   CsplinePar *csp;   /**< Cubic spline parameters (scheme 3 and 4 only), need one-time initialisation. */
   int use_count;     /**< Indicates how often a table is in use, but not safe enough to make it a smart pointer. */
};
typedef struct rpol_table RpolTable;

/* rpolator.c */
int read_table(const char *fname, int maxrow, double *col1, double *col2);
int read_table2(const char *fname, int maxrow, double *col1, double *col2);
int read_table3(const char *fname, int maxrow, double *col1, double *col2, double *col3);
int read_table4(const char *fname, int maxrow, double *col1, double *col2, double *col3, double *col4);
int read_table5(const char *fname, int maxrow, double *col1, double *col2, double *col3, double *col4, double *col5);

int read_table_v(const char *fname, FILE *fptr, size_t *nrow, size_t ncol, double ***col, const size_t *selcol);

struct rpol_table *read_rpol1d_table(const char *fn);
struct rpol_table *read_rpol2d_table(const char *fn, const char *ymarker);
struct rpol_table *read_rpol3d_table(const char *fn);
struct rpol_table *read_rpol_table(const char *fname, int nd, const char *ymarker, const char *options);

struct rpol_table *simple_rpol1d_table(const char *label, double *x, double *y, int n, int clip);

int rpol_is_verbose(void);
int rpol_set_verbose(int lvl);

void rpol_info(struct rpol_table *rpt);
void rpol_info_lvl (struct rpol_table *rpt, int lvl);
void rpol_free(struct rpol_table *rpt, int removing);
void rpol_check_equi_range(struct rpol_table *rpt);

double rpol(double *x, double *y, int n, double xp);
double rpol_nearest(double *x, double *y, int n, double xp, int eq, int clip);
double rpol_linear(double *x, double *y, int n, double xp, int eq, int clip);
double rpol_2nd_order(double *x, double *y, int n, double xp, int eq, int clip);
CsplinePar* set_1d_cubic_params (double *x, double *y, int n, int clamped);
double rpol_cspline ( double *x, double *y, const CsplinePar *csp, int n, double xp, int eq, int clip );
double rpol_2d_linear(double *x, double *y, double *z, int nx, int ny, double xp, double yp, int eq, int clip);

double rpolate_1d(struct rpol_table *rpt, double x, int scheme);
double rpolate_1d_lin(struct rpol_table *rpt, double x);
double rpolate_2d(struct rpol_table *rpt, double x, double y, int scheme);
double rpolate(struct rpol_table *rpt, double x, double y, int scheme);

#ifdef __cplusplus
}
#endif

#endif
