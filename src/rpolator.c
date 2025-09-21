/* ============================================================================

   Copyright (C) 2017, 2018, 2019, 2020, 2022, 2023  Konrad Bernloehr

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
/** @file rpolator.c 
 *  @short Reading of configuration data tables and interpolation.
 *
 *  @author  Konrad Bernloehr 
 *  @date    2017, ..., 2023
 */
/* =============================================================== */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "rpolator.h"

static void interp(double x, double *v, int n, int *ipl, double *rpl);

/* --------------------------- interp ------------------------------- */
/** 
 *  @short Linear interpolation with binary search algorithm.
 *
 *  Linear interpolation between data point in sorted (i.e. monotonic
 *  ascending [no descending yet]) order. This function determines between
 *  which two data points the requested coordinate is and where between
 *  them. If the given coordinate is outside the covered range, the
 *  value for the corresponding edge is returned.
 *
 *  A binary search algorithm is used for fast interpolation.
 *
 *  @param  x Input: the requested coordinate
 *  @param  v Input: tabulated coordinates at data points
 *  @param  n Input: number of data points
 *  @param  ipl Output: the number of the data point following the requested
 *          coordinate in the given sorting (1 <= ipl <= n-1)
 *  @param  rpl Output: the fraction (x-v[ipl-1])/(v[ipl]-v[ipl-1])
 *          with 0 <= rpl <= 1
*/      

static void interp ( double x, double *v, int n, int *ipl, double *rpl )
{
   int i, l, m, j, lm;

   if ( v[0] < v[n-1] )
   {
      if (x <= v[0])
      {
         *ipl = 1;
         *rpl = 0.;
         return;
      }
      else if (x >= v[n-1])
      {
         *ipl = n-1;
         *rpl = 1.;
         return;
      }
      lm = 0;
   }
   else
   {
      if (x >= v[0])
      {
         *ipl = 1;
         *rpl = 0.;
         return;
      }
      else if (x <= v[n-1])
      {
         *ipl = n-1;
         *rpl = 1.;
         return;
      }
      lm = 1;
   }

   l = (n+1)/2-1;
   m = (n+1)/2;
   for (i=1; i<=30; i++ )
   {
      j = l;
      if (j < 1) j=1;
      if (j > n-1) j=n-1;
      if (x >= v[j+lm-1] && x <= v[j-lm])
      {
         *ipl = j;
         if ( v[j] != v[j-1] )
            *rpl = (x-v[j-1])/(v[j]-v[j-1]);
         else
            *rpl = 0.5;
         return;
      }
      m = (m+1)/2;
      if (x > v[j-1])
         l = l + (1-2*lm)*m;
      else
         l = l - (1-2*lm)*m;
   }
   fprintf(stderr,"Interpolation error.\n");
}

/**
 *  Set up cubic spline parameters for n-1 intervals resulting from n data points.
 *  The resulting cubic spline can either be a 'natural' one (second derivative is zero
 *  at the edges) or a clamped one (first derivative is fixed, currently to zero, at the edges).
 *
 *  @param x        Input: Coordinates for data table
 *  @param y        Input: Corresponding values for data table
 *  @param n        Input: Number of data points
 *  @param clamped  Input: 0 (natural cubic spline) or 1 (clamped cubic spline)
 *
 *  @return Allocated array of four parameters each defining the third order polynomial in each interval.
 *          In case of invalid input parameters a NULL pointer is returned.
 */

CsplinePar* set_1d_cubic_params (double *x, double *y, int n, int clamped)
{
   int i, j;
   if ( n < 4 )
   {
      fprintf(stderr,"Not enough data points for cubic spline.\n");
      return NULL;
   }
   for ( i=0; i<n-1; i++ )
   {
      if ( x[i+1] <= x[i] )
      {
         fprintf(stderr,"Supporting points not in strictly ascending order.\n");
         return NULL;
      }
   }
   
   double deriv_left = 0., deriv_right = 0.;
   struct cubic_params *cpv = (struct cubic_params *) calloc(n,sizeof(struct cubic_params));

   /* Based on pseudocode in Wikipedia https://en.wikipedia.org/wiki/Spline_(mathematics) */
   /* Note that 'n' as used here is the number of support points while in that
      pseudocode it is lower by one (meant as the number of intervals resulting). */

   double a[n];
   for ( i=0; i<n; i++ )
      a[i] = y[i];
   double b[n-1], c[n], d[n-1], M[n-1], h[n-1];
   for (i=0; i<n-1; i++)
      h[i] = x[i+1] - x[i];
   double A[n];
   A[0] = A[n-2] = A[n-1] = 0.;
   for (i=1; i<n-1; i++)
      A[i] = 3.*(a[i+1]-a[i])/h[i] - 3.*(a[i]-a[i-1])/h[i-1];
   double l[n], z[n];
   if ( clamped ) /* Clampled instead of natural cubic spline */
   {
      A[0] = 3.*(a[1]-a[0])/h[0] - 3.*deriv_left;
      A[n-1] = 3.*deriv_right - 3.*(a[n-1]-a[n-2])/h[n-2];
      l[0] = 2.*h[0];
      M[0] = 0.5;
      z[0] = A[0]/l[0];
   }
   else /* Natural cubic spline, the more typical case */
   {
      l[0] = 1.;
      M[0] = 0.;
      z[0] = 0.;
   }
   for ( i=1; i<n-1; i++ )
   {
      l[i] = 2.*(x[i+1]-x[i-1]) - h[i-1]*M[i-1];
      M[i] = h[i] / l[i];
      z[i] = (A[i]-h[i-1]*z[i-1])  / l[i];
   }
   if ( clamped ) /* Clampled instead of natural cubic spline */
   {
      l[n-1] = h[n-2]*(2.-M[n-2]);
      z[n-1] = (A[n-1]-h[n-2]*z[n-2])/l[n-1];
      c[n-1] = z[n-1];
   }
   else /* Natural cubic spline */
   {
      l[n-1] = 1.;
      z[n-1] = 0.;
      c[n-1] = 0.;
   }
   for ( j=n-2; j>=0; j-- )
   {
      c[j] = z[j] - M[j]*c[j+1];
      b[j] = (a[j+1]-a[j])/h[j] - h[j]*(c[j+1]+2.*c[j])/3.;
      d[j] = (c[j+1]-c[j])/(3.*h[j]);
   }
   
   /* Copy the derived parameters to our common c-spline struct. */
   
   for ( i=0; i<n-1; i++ )
   {
      cpv[i].d = d[i];
      cpv[i].c = c[i];
      cpv[i].b = b[i];
      cpv[i].a = a[i];
   }

   return cpv;
}

static double csx (double r, const CsplinePar* cp);

static double csx (double r, const CsplinePar* cp)
{
   return ((cp->d*r + cp->c)*r + cp->b)*r + cp->a;
}

/* ----------------------------- rpol_cspline ------------------------------- */

/** Cubic spline interpolation in 1-D with clipping option outside range */
/**
 *  @short Cubic spline interpolation in 1-D with with either direct
 *         access for equidistant table or with binary search algorithm.
 *
 *  Quadratic interpolation between data point in sorted (i.e. monotonic
 *  ascending or descending) order. The resulting interpolated value
 *  is returned as a return value. If the table is known to be provided
 *  at equidistant supporting points, direct access is preferred.
 *  Otherwise a binary search algorithm is used to find the proper interval.
 *  Because of the overhead of calculating the cubic spline parameters,
 *  those have to be initialized before the interpolation can be used.
 *  Initialisation has to be for either natural or clampled cubic splines.
 *
 *  This function calls interp() to find out where to interpolate.
 *  
 *  @param   x    Input: Coordinates for data table
 *  @param   y    Input: Corresponding values for data table
 *  @param   csp  Input: Cubic spline parameters (a,b,c,d) for each of n-1 intervals
 *  @param   n    Input: Number of data points
 *  @param   xp   Input: Coordinate of requested value
 *  @param   eq   Input: If non-zero: table is at equidistant points.
 *  @param   clip Input: Zero: no clipping; extrapolate with left/right edge value outside range.
 *                       Non-zero: clip at edges; return 0. outside supported range.
 *
 *  @return  Interpolated value
 *
*/

double rpol_cspline ( double *x, double *y, const CsplinePar *csp, int n, double xp, int eq, int clip )
{
   int ipl = 1;
   double rpl = 0.;
   double r = 0.;

   if ( n < 4 || csp == NULL ) /* Invalid: not enough points for cubic splines. */
      return 0.; /* Setup should have downgraded the scheme beforehand. */

   if ( x[1] <= x[0] ) /* Invalid: supporting points in decreasing order */
      return 0.;

   if ( xp < x[0] ) /* Below first supporting point */
   {
      if ( clip )
         return 0.;
      else
         return y[0];
   }
   else if ( xp > x[n-1] ) /* Above last supporting point */
   {
      if ( clip )
         return 0.;
      else
         return y[n-1];
   }

   if ( eq ) /* Equidistant spacing in x */
   {
      double dxi = 1./(x[1] - x[0]);
      ipl = (int) ((xp-x[0])*dxi) + 1;
      if ( ipl > n-1 ) /* Could happen with rounding errors */
         ipl = n-1;
      // rpl = (xp-x[ipl-1])*dxi;
   }
   else
      interp ( xp, x, n, &ipl, &rpl );

   /* Keep in mind that we should have 1 <= ipl <= n-1 */

   if ( ipl > n-1 ) /* Can happen on upper boundary, perhaps rounding errors */
      return y[n-1];
   else if ( ipl < 1 ) /* In case of rounding errors slipping through */
      return y[0];

   r = xp - x[ipl-1];

   return csx(r, csp+ipl-1); /* Evaluate cubic spline for appropriate interval */
}
