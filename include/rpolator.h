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

/* rpolator.c */
CsplinePar* set_1d_cubic_params (double *x, double *y, int n, int clamped);
double rpol_cspline ( double *x, double *y, const CsplinePar *csp, int n, double xp, int eq, int clip );

#ifdef __cplusplus
}
#endif

#endif