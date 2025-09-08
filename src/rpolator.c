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
 *  In contrast to the older low-level table reading and rpol()
 *  1-D linear interpolation code, the function here allow
 *  flexible switching between 1-D and two different formats of
 *  2-D tables, with support for different interpolation schemes
 *  (default being linear).
 *
 *  @author  Konrad Bernloehr 
 *  @date    2017, ..., 2023
 */
/* =============================================================== */

#include "initial.h"
// #include <stdlib.h>
// #include <stdio.h>
// #include <string.h>
// #include <strings.h>

#include <math.h>
#include <ctype.h>

#include "rpolator.h"
#include "straux.h"
#include "fileopen.h"

#ifdef __GLIBC__
# ifndef _POSIX_C_SOURCE
/* Workaround for gcc in c99 language standard mode (too strict
   but glibc providing the functions we need here). */
char *strdup(const char *s);
int strcasecmp(const char *s1, const char *s2);
int strncasecmp(const char *s1, const char *s2, size_t n);
# endif
#endif

/* Without initial.h, the ANSI_C definition is never set and including straux.h useless. */
// int getword (const char *s, int *spos, char *word, int maxlen, char blank,
//              char endchar);

static void strip_comments(char *line);

static void strip_comments(char *line)
{
   int i, l;
   char *s;

   /* Strip all comments and trailing white space */
   if ( (s=strchr(line,'#')) != NULL )
      *s = '\0';
   if ( (s=strchr(line,'%')) != NULL )
      *s = '\0';
   l = strlen(line);
   for (i=l-1; i>=0; i--)
   {
      if ( line[i] == '\n' || line[i] == '\t' || line[i] == ' ' )
	 line[i] = '\0';
      else
	 break;
   }
}

/* ----------------------------- read_table -------------------------- */
/**
 *  @short Low-level reading of 2-column data tables up to given number of data rows.
 *
 *  @param fname Name of file to be opened.
 *  @param maxrow Maximum number of (non-empty, non-comment) rows of data to read.
 *  @param col1 Array where values of column 1 are to be copied to.
 *  @param col2 Array where values of column 2 are to be copied to.
 *
 *  @return Number of data rows read (usable values in col1 and col2)
 *     or -1 (error).
 */

int read_table (const char *fname, int maxrow, double *col1, double *col2)
{
   FILE *f;
   double x, y;
   char line[1024];
   int iline = 0, n=0, rc = 0;
   
   if ( (f = fileopen(fname,"r")) == NULL )
   {
      perror(fname);
      return -1;
   }
   
   while ( fgets(line,sizeof(line)-1,f) != NULL )
   {
      iline++;
      strip_comments(line);

      if ( line[0] == '\0' )
      	 continue;
      
      if ( (rc=sscanf(line,"%lf %lf",&x,&y)) != 2 )
      {
      	 fprintf(stderr,"Error in line %d of file %s (expcting 2 values, found %d).\n",
            iline,fname,rc);
	 fileclose(f);
	 return -1;
      }
      if ( n >= maxrow )
      {
      	 fprintf(stderr,"Too many entries in file %s (max=%d)\n",fname,maxrow);
	 fileclose(f);
	 return -1;
      }
      col1[n] = x;
      col2[n] = y;
      n++;
   }
   
   fflush(stdout);
   fprintf(stderr,"Table with %d rows has been read from file %s\n",n,fname);
   fileclose(f);

   return n;
}

/* read_table2 is just the same as read_table */

int read_table2 (const char *fname, int maxrow, double *col1, double *col2)
{
   return read_table(fname,maxrow,col1,col2);
}

/* ----------------------------- read_table3 -------------------------- */
/** read_table3() and so on have more columns than read_table but
    are still only suitable for 1-D interpolation. */

int read_table3 (const char *fname, int maxrow, 
   double *col1, double *col2, double *col3);

int read_table3 (const char *fname, int maxrow, 
   double *col1, double *col2, double *col3)
{
   FILE *f;
   double x, y, z;
   char line[1024];
   int iline = 0, n=0, rc = 0;

   if ( (f = fileopen(fname,"r")) == NULL )
   {
      perror(fname);
      return -1;
   }

   while ( fgets(line,sizeof(line)-1,f) != NULL )
   {
      iline++;
      strip_comments(line);

      if ( line[0] == '\0' )
      	 continue;
      
      if ( (rc = sscanf(line,"%lf %lf %lf",&x,&y,&z)) != 3 )
      {
      	 fprintf(stderr,"Error in line %d of file %s (expecting 3 values, found %d)\n",
            iline,fname,rc);
	 fileclose(f);
	 return -1;
      }
      if ( n >= maxrow )
      {
      	 fprintf(stderr,"Too many entries in file %s (max=%d)\n",fname,maxrow);
	 fileclose(f);
	 return -1;
      }
      col1[n] = x;
      col2[n] = y;
      col3[n] = z;
      n++;
   }

   fflush(stdout);
   fprintf(stderr,"Table with %d rows has been read from file %s\n",n,fname);
   fileclose(f);

   return n;
}

/* ----------------------------- read_table4 -------------------------- */

int read_table4 (const char *fname, int maxrow, 
   double *col1, double *col2, double *col3, double *col4);

int read_table4 (const char *fname, int maxrow, 
   double *col1, double *col2, double *col3, double *col4)
{
   FILE *f;
   double x, y, z, u;
   char line[1024];
   int iline = 0, n=0;

   if ( (f = fileopen(fname,"r")) == NULL )
   {
      perror(fname);
      return -1;
   }

   while ( fgets(line,sizeof(line)-1,f) != NULL )
   {
      iline++;
      strip_comments(line);

      if ( line[0] == '\0' )
      	 continue;
      
      if ( sscanf(line,"%lf %lf %lf %lf",&x,&y,&z,&u) != 4 )
      {
      	 fprintf(stderr,"Error in line %d of file %s\n",iline,fname);
	 fileclose(f);
	 return -1;
      }
      if ( n >= maxrow )
      {
      	 fprintf(stderr,"Too many entries in file %s (max=%d)\n",fname,maxrow);
	 fileclose(f);
	 return -1;
      }
      col1[n] = x;
      col2[n] = y;
      col3[n] = z;
      col4[n] = u;
      n++;
   }

   fflush(stdout);
   fprintf(stderr,"Table with %d rows has been read from file %s\n",n,fname);
   fileclose(f);

   return n;
}

/* ----------------------------- read_table5 -------------------------- */

int read_table5 (const char *fname, int maxrow, 
   double *col1, double *col2, double *col3, double *col4, double *col5);

int read_table5 (const char *fname, int maxrow, 
   double *col1, double *col2, double *col3, double *col4, double *col5)
{
   FILE *f;
   double x, y, z, u, v;
   char line[1024];
   int iline = 0, n=0;

   if ( (f = fileopen(fname,"r")) == NULL )
   {
      perror(fname);
      return -1;
   }

   while ( fgets(line,sizeof(line)-1,f) != NULL )
   {
      iline++;
      strip_comments(line);

      if ( line[0] == '\0' )
      	 continue;
      
      if ( sscanf(line,"%lf %lf %lf %lf %lf",&x,&y,&z,&u,&v) != 5 )
      {
      	 fprintf(stderr,"Error in line %d of file %s\n",iline,fname);
	 fileclose(f);
	 return -1;
      }
      if ( n >= maxrow )
      {
      	 fprintf(stderr,"Too many entries in file %s (max=%d)\n",fname,maxrow);
	 fileclose(f);
	 return -1;
      }
      col1[n] = x;
      col2[n] = y;
      col3[n] = z;
      col4[n] = u;
      col5[n] = v;
      n++;
   }

   fflush(stdout);
   fprintf(stderr,"Table with %d rows has been read from file %s\n",n,fname);
   fileclose(f);

   return n;
}

/* ----------------------------- read_table_v -------------------------- */
/**
 *  Read tables any length (up to some ridiculous maximum) with the requested columns
 *  either in natural order or picking columns as requested.
 *  All data areas are dynamically allocated (and must be fresh beforehand).
 *
 *  On memory allocation errors a -1 error code is returned but already allocated parts
 *  are not released and the file may still be opened.
 *
 *  @param fname Name or URL of file to read.
 *  @paran fptr  File pointer if file already open or NULL if fname has to be opened.
 *  @param nrow  Pointer to number of rows with valid data (pass address of a size_t variable).
 *               Input value used to guide initial allocation, not fixing actual rows to read.
 *  @param ncol  Number of columns of data requested to be read.
 *  @param col   Pointer to where pointers to column-wise data get allocated.
 *               (Need to pass address of a double ** variable.)
 *  @param selcol NULL for natural column order or pointer to ncol (natural, >=1) column numbers in selected order.
 *               Example: { 1, 7, 5 } will place data from the first column under (*col)[0],
 *               data from the seventh column under (*col)[1], and data from the fifth column under (*col)[2].
 *
 *  @return 0 (OK), -1 (memory error), -2 (invalid parameter),
 *              -3 (asking for too many columns), -4 (too many rows of data, stopped reading).
 */

int read_table_v (const char *fname, FILE *fptr, size_t *nrow, size_t ncol, double ***col, const size_t *selcol);

int read_table_v (const char *fname, FILE *fptr, size_t *nrow, size_t ncol, double ***col, const size_t *selcol)
{
   FILE *f = NULL;
   char line[10240], word[100];
   size_t astep = 50, arow = 0;
   size_t iline = 0, icol, maxcol = ncol, maxrow = 100000;
   size_t kline = 0;
   size_t arow_ini = 0; /* First need to check pointers before setting this */
   double *rval = NULL;
   size_t *rcol = NULL;
   int rc = 0;
   int ipos = 0;
   char *s = NULL;

   if ( fname == NULL || nrow == NULL || ncol < 1 || col == NULL || *col != NULL )
   {
      fflush(stdout);
      fprintf(stderr,"read_table_v(%s,...): invalid parameter(s)\n", fname);
      if ( nrow != NULL )
         *nrow = 0;
      return -2;
   }

   /* Don't trust in large numbers and ignore zero or negative numbers */
   arow_ini = (*nrow > 2000) ? 2000 : ( (*nrow <= 0) ? astep : *nrow );
   if ( arow_ini <= 10 )
      astep = 10;
   /* Reset any number to be returned. */
   *nrow = 0;

   /* Other than natural order of columns requested ? */
   if ( selcol != NULL )
   {
      for ( icol=0; icol<ncol; icol++ )
      {
         if ( selcol[icol] == 0 )
         {
            fprintf(stderr,"Column numbers must start at 1.\n");
            return -3;
         }
         if ( selcol[icol] > maxcol )
            maxcol = selcol[icol];
      }
   }
   if ( maxcol > 500 )
   {
      fflush(stdout);
      fprintf(stderr,"read_table_v(%s,...): Non-plausible columns requested\n", fname);
      return -2;
   }

   /* Allocate pointers to hold the column data later-on */
   if ( (*col = (double **) calloc(ncol,sizeof(double *))) == NULL )
      return -1;
   for ( icol=0; icol<ncol; icol++ )
   {
      if ( ((*col)[icol] = (double *) calloc(arow_ini,sizeof(double))) == NULL )
            return -1;
   }
   arow = arow_ini;

   if ( (rval = (double *) calloc(maxcol,sizeof(double))) == NULL )
      return -1;
   if ( (rcol = (size_t *) calloc(maxcol,sizeof(size_t))) == NULL )
      return -1;
   if ( selcol != NULL )
   {
      for ( icol=0; icol<ncol; icol++ )
      {
         rcol[selcol[icol]-1] = icol+1; /* Use offset of 1 to distinguish from unused columns */
      }
   }
   else
   {
      for ( icol=0; icol<ncol; icol++ )
      {
         rcol[icol] = icol+1; /* All in natural order */
      }
   }

   if ( fptr != NULL )
      f = fptr;
   else if ( (f = fileopen(fname,"r")) == NULL )
   {
      perror(fname);
      return -1;
   }
   
   rc = 0;

   while ( fgets(line,sizeof(line)-1,f) != NULL )
   {
      size_t nc = 0;
      int csvhead = 0;
      iline++;
      strip_comments(line);

      if ( line[0] == '\0' )
      	 continue;

      s = line;
      while ( *s == ' ' || *s == '\t' )
         s++;

      for ( ipos=0, nc=0; nc<maxcol && s[ipos] != '\0' && 
            getword(s,&ipos,word,sizeof(word)-1,' ','#') > 0; nc++ )
      {
         if ( rcol[nc] == 0 )
            continue; /* Unused column, rval value never filled. */
         if ( sscanf(word, "%lf", &rval[nc]) == 0 )
         {
            rval[nc] = 0.;
            /* In [E]CSV files, the first line after comments and empty lines
               would contain the column names rather than the first data */
            fflush(stdout);
            if ( kline == 0 && rcol[nc] == 1 )
            {
               kline++;
               csvhead = 1;
               fprintf(stderr,"File %s line %zu may be CSV column header line.\n", fname, iline);
            }
            else
            {
               fflush(stdout);
               fprintf(stderr,"File %s line %zu column %zu: Missing or invalid data.\n", fname, iline, nc+1);
            }
            break;
         }
      }
      if ( nc == 0 || csvhead ) /* No data in line: continue quietly */
         continue;
      else if ( nc < maxcol ) /* Some data but not enough: complain and continue without the data */
      {
         fflush(stdout);
         fprintf(stderr,"File %s line %zu: expected %zu columns but got only %zu.\n",
            fname, iline, maxcol, nc);
         continue;
      }
      kline++;

      /* Space for more rows needed? */
      if ( (*nrow) >= arow )
      {
         if ( astep*8 < arow )
            astep *= 2;  /* Less frequent re-allocations if we already have many rows */
         for ( icol=0; icol<ncol; icol++ )
         {
            double *p = (double *) realloc((*col)[icol],(arow+astep)*sizeof(double));
            if ( p == NULL ) /* If re-allocation fails for any column, keep what we got so far and stop */
            {
               rc = -1; /* failure indicator */
               break;
            }
            (*col)[icol] = p; /* replace column pointer with re-allocated pointer */
         }
         arow += astep;
      }
      if ( rc != 0 ) /* Cannot continue after allocation error (but keep data read so far). */
         break;

      /* Copy from temporary vector to final location */
      for ( icol=0; icol<ncol; icol++ )
      {
         if ( selcol == NULL )
            (*col)[icol][*nrow] = rval[icol]; /* Natural column order */
         else
            (*col)[icol][*nrow] = rval[selcol[icol]-1]; /* Selected column order */
      }
      /* Ready for next row */
      (*nrow)++;

      /* Despite dynamic allocation there is a point where we stop reading more data */
      if ( (*nrow) >= maxrow )
      {
         rc = -4;
         fflush(stdout);
         fprintf(stderr,"File %s has too many rows. Ignoring the rest.\n", fname);
         break;
      }
   }

   /* Close input file if it was opened by this function */
   if ( fptr == NULL && f != NULL && f != stdin )
      fileclose(f);

   /* Free local temporary data. */
   free(rval);
   free(rcol);
   
   /* We could re-allocate the column data if more rows were allocated
      than actually used but that would typically save only order 10%. Ignored here. */
   /* (Freeing column data when no longer needed is up to the calling function). */

   return rc;
}

/* Global setting if rpol functions should be verbose, initialized from RPOL_VERBOSE. */

static int rpol_verbosity = -1;

/** Registered interpolation tables allow re-use of tables without
   having to load them again. Available for simple (2-column) 1-D
   interpolation and for rectangular-grid 2-D interpolation
   with y grid values in header line, x grid values in first column. */

struct rpt_list
{
   struct rpol_table *rpt;
   struct rpt_list *next;
};
static struct rpt_list *rpt_list_base;

struct rpol_table *read_rpol_table (const char *fn, int nd, const char *ymarker, const char *options);

/** 
 *  @short Simplified version for loading a 1-d table with two columns. 
 *
 *  The text free format input file is expected to contain (at least) two
 *  columns for x and y values. Any further columns are ignored.
 *  Comment and empty lines are ignored as well.
 */
 
struct rpol_table *read_rpol1d_table (const char *fn);
struct rpol_table *read_rpol1d_table (const char *fn)
{
   return read_rpol_table(fn,1,NULL,NULL);
}

/** 
 *  @short Simplified version for loading a 2-d table 1+ny columns (ny=available y values in header line).
 *
 *  The text input file is expected to contain a header line where,
 *  following the given marker text, the y values to which the following values
 *  correspond are listed. The number ny of distinct, ascending-order values in
 *  that line defines the number of z expected in the following data lines.
 *  The x value is the first value in each data line. If a data line contains
 *  more than 1+ny values the extra values are ignored.
 *  Comment (except header line) and empty lines are ignored.
 */

struct rpol_table *read_rpol2d_table (const char *fn, const char *ymarker);
struct rpol_table *read_rpol2d_table (const char *fn, const char *ymarker)
{
   return read_rpol_table(fn,2,ymarker,NULL);
}

/** 
 *  @short Simplified version for loading a 2-d table with x/y/z in three columns (x/y intervals rectangular).
 *
 *  Functionally equivalent to read_rpol2d_table() but the y values
 *  are not given just once in a specially marked header line but
 *  repeated as the second value in each line.
 *  Instead of one header line plus nx data lines with 1+ny values the
 *  input to this function is expected to contain nx*ny lines with 
 *  three values each.
 *  While x and y values do not need to be equidistant, they are required
 *  to match a rectangular grid, with the same distinct, ascending-order
 *  y values appearing for each x value and the same distinct, ascending-order
 *  x values appearing for each y value, eather as x1/y1/z11, x2/y1/z21, ...
 *  or x1/y/z11, x1/y2/z12, ...
 */
struct rpol_table *read_rpol3d_table (const char *fn);
struct rpol_table *read_rpol3d_table (const char *fn)
{
   return read_rpol_table(fn,3,NULL,NULL);
}

/** Report what verbosity level is set for the rpolator code */

int rpol_is_verbose(void)
{
   if ( rpol_verbosity == -1 ) /* Not initialized so far */
   {
      if ( getenv("RPOL_VERBOSE") == NULL )
         rpol_verbosity = 0;
      else
         rpol_verbosity = atoi(getenv("RPOL_VERBOSE"));
      if ( rpol_verbosity < 0 )
         rpol_verbosity = 0;
   }
   return rpol_verbosity;
}

/** Set the verbosity level for the rpolator code, return old level */

int rpol_set_verbose(int lvl)
{
   int old = rpol_verbosity;
   rpol_verbosity = (lvl >= 0) ? lvl : 0;
   return old;
}

/**
 *  @short Show information about given interpolation table
 */
 
void rpol_info (struct rpol_table *rpt);

void rpol_info (struct rpol_table *rpt)
{
   size_t i;
   int verbose = rpol_verbosity;

   if ( rpt == (struct rpol_table *) NULL )
      return;

   if ( rpt->fname != NULL && verbose )
   {
      if ( rpt->options == NULL )
         printf("Interpolation table '%s' (read without options)\n", rpt->fname);
      else
         printf("Interpolation table '%s' (read with options '%s')\n", rpt->fname, rpt->options);
   }
   else if ( rpt->fname != NULL )
         printf("Interpolation table '%s'\n", rpt->fname);
   else
      printf("Interpolation table without known name\n");
   printf("  ndim = %d (%s)\n", rpt->ndim,
      rpt->ndim==1 ? "1-D" :
      rpt->ndim==2 ? "2-D with y values in extra line" :
      rpt->ndim==3 ? "2-D with explicit x/y/z" : "???" );
   printf("  nx = %zu [%g : %g]%s\n", rpt->nx, rpt->xmin, rpt->xmax,
      rpt->xlog?" (log units)":"");
   if ( rpt->ndim!=1 || rpt->ny != 0 )
      printf("  ny = %zu [%g : %g]%s\n", rpt->ny, rpt->ymin, rpt->ymax,
         rpt->ylog?" (log units)":"");
   if ( rpt->zmin != 0. || rpt->zmin != 0. )
      printf("  nz = %zu [%g : %g]%s\n", 
         (rpt->ndim==1) ? rpt->nx : rpt->nx*rpt->ny, rpt->zmin, rpt->zmax,
         rpt->zlog?" (log units)":"");
   printf("  scheme = %d (%s)\n", rpt->scheme,
      rpt->scheme==0 ? "nearest" :
      rpt->scheme==1 ? "linear" :
      rpt->scheme==2 ? "quadratic" :
      rpt->scheme==3 ? "natural cubic splines" :
      rpt->scheme==4 ? "clamped cubic splines" : "???" );
   if ( (rpt->ndim == 1 && rpt->scheme > 4) ||
        (rpt->ndim >= 2 && rpt->scheme != 1) )
   {
      printf("     (this scheme not implemented yet, falling back to linear)\n");
   }
   printf("  equidistant = %d (%s)\n", rpt->equidistant,
      rpt->equidistant==0 ? "no" : 
      rpt->equidistant==1 ? "x" :
      rpt->equidistant==2 ? "y" :
      rpt->equidistant==3 ? "x/y" : "???" );
   printf("  clipping = %d\n", rpt->clipping);
   if ( rpt->remapped )
      printf("  user code applied remapping %d\n", rpt->remapped);
   if ( rpt->x == NULL )
      printf("  x values are missing!\n");
   else if ( rpol_verbosity >= 1 )
   {
      if ( rpt->xlog )
         printf("  log(x) values:");
      else
         printf("  x values:");
      for ( i=0; i<10 && i<rpt->nx; i++ )
         printf(" %g", rpt->x[i]);
      if ( i<rpt->nx )
         printf(" ...");
      printf("\n");
   }
   if ( rpt->y == NULL && rpt->ndim >= 2 )
      printf("  y values are missing!\n");
   else if ( rpt->ndim >= 2 && rpol_verbosity >= 1 )
   {
      if ( rpt->ylog )
         printf("  log(y) values:");
      else
         printf("  y values:");
      for ( i=0; i<10 && i<rpt->ny; i++ )
         printf(" %g", rpt->y[i]);
      if ( i<rpt->ny )
         printf(" ...");
      printf("\n");
   }
   if ( rpt->z == NULL )
      printf("  z values are missing!\n");
   else if ( rpol_verbosity >= 2 )
   {
      if ( rpt->zlog )
         printf("  log(z) values:");
      else
         printf("  z values:");
      if ( rpt->ndim==1 )
      {
         for ( i=0; i<10 && i<rpt->nx; i++ )
            printf(" %g", rpt->z[i]);
         if ( i<rpt->nx )
            printf(" ...");
      }
      else
      {
         for ( i=0; i<40 && i < rpt->nx*rpt->ny; i++ )
         {
            if ( i%rpt->ny == 0 && i != 0 )
               printf("\n           ");
            printf(" %g", rpt->z[i]);
         }
         if ( i< rpt->nx*rpt->ny )
            printf(" ...");
      }
      printf("\n");
   }
   if ( rpt->zxreq && rpt->ndim >= 2 )
   {
      if ( rpt->zxmax == NULL )
         printf("  x projection max cover envelope requested but not yet filled\n");
      else if ( rpol_verbosity >= 1 )
      {
         printf("  x projection max cover envelope:");
         for ( i=0; i<rpt->nx; i++ )
            printf(" %g", rpt->zxmax[i]);
         printf("\n");
      }
      else
         printf("  x projection max cover envelope is available\n");
      if ( rpt->zxmin == NULL )
         printf("  x projection min cover envelope requested but not yet filled\n");
      else if ( rpol_verbosity >= 1 )
      {
         printf("  x projection min cover envelope:");
         for ( i=0; i<rpt->nx; i++ )
            printf(" %g", rpt->zxmin[i]);
         printf("\n");
      }
      else
         printf("  x projection min cover envelope is available\n");
   }
   if ( rpt->aux != 0 )
      printf("  aux value: %g\n", rpt->aux);
}

/** Report table info at given temporary verbosity level */

void rpol_info_lvl (struct rpol_table *rpt, int lvl)
{
   int old = rpol_set_verbose(lvl);
   rpol_info(rpt);
   (void) rpol_set_verbose(old);
}

/**
 *  @short Free a previously allocated interpolation table data structure.
 *
 *  This is dangerous to used if the pointer is used in more than one place!
 *  Keep in mind that each time you ask to load the same table again
 *  you will just get a copy of the same pointer again. If that could be
 *  the case you better don't force deleting the structure itself.
 *
 */

void rpol_free (struct rpol_table *rpt, int removing);

void rpol_free (struct rpol_table *rpt, int removing)
{
   struct rpt_list *rptl, *rptl_n;

   if ( rpt == (struct rpol_table *) NULL )
      return;

   /* Do nothing but decrement use counter if it seems to be in multiple use */
   if ( rpt->use_count > 1 )
   {
      rpt->use_count--;
      return;
   }
   else if ( rpt->use_count == 0 )
   {
      fprintf(stderr,"Trying to free a rpol table that should already be cleared.\n");
      return;
   }

   /* Free any allocated parts in the struct */
   if ( rpt->x != NULL )
   {
      free(rpt->x);
      rpt->x = NULL;
   }
   if ( rpt->y != NULL && rpt->y != rpt->z )
   {
      free(rpt->y);
      rpt->y = NULL;
   }
   else
      rpt->y = NULL;
   if ( rpt->z != NULL )
   {
      free(rpt->z);
      rpt->z = NULL;
   }
   if ( rpt->zxmax != NULL )
   {
      free(rpt->zxmax);
      rpt->zxmax = NULL;
   }
   if ( rpt->zxmin != NULL )
   {
      free(rpt->zxmin);
      rpt->zxmin = NULL;
   }
   if ( rpt->fname != NULL )
   {
      free(rpt->fname);
      rpt->fname = NULL;
   }
   if ( rpt->options != NULL )
   {
      free(rpt->options);
      rpt->options = NULL;
   }
   if ( rpt->csp != NULL )
   {
      free(rpt->csp);
      rpt->csp = NULL;
   }

   /* We should also remove it from the global linked
      list of interpolation tables, if it is there. */
   if ( rpt_list_base != NULL )
   {
      /* The to-be-deleted one may be the very first in the linked list */
      if ( rpt_list_base->rpt == rpt )
      {
         rptl = rpt_list_base;
         rptl_n = rptl->next;
         /* Move the second in place of the first */
         rpt_list_base = rptl_n;
         free(rptl);
         /* If there is only the dummy trailing pointer left,
            clear the rest of the linked list. */
         if ( rpt_list_base != NULL && rpt_list_base->rpt == NULL && rpt_list_base->next == NULL )
         {
            free(rpt_list_base);
            rpt_list_base = NULL;
         }
      }
      /* Or it could be anywhere after the first one */
      for ( rptl=rpt_list_base; rptl!=NULL && rptl->rpt != NULL && 
            rptl->next != NULL; rptl=rptl->next )
      {
         if ( rptl->next->rpt == rpt )
         {
            rptl_n = rptl->next;
            if ( rptl->next->next != NULL )
            {
               /* Move the second next in place of the next one. */
               rptl->next = rptl->next->next;
            }
            else
            {
               /* Actually, we should not come along here, even if the searched-for
                  pointer was the last one. It should still have a 'next' pointer
                  to an unused one. But better make sure we don't run off the end. */
               rptl->next = (struct rpt_list *) calloc(1,sizeof(struct rpt_list));
            }
            free(rptl_n);
         }
      }
   }
   
   if ( rpt->use_count > 0 )
      rpt->use_count--;

   if ( removing )
   {
      /* And finally free the table structure itself. */
      /* But only if you are really sure about it. */
      free(rpt);
   }
}

void rpol_check_equi_range(struct rpol_table *rpt);

void rpol_check_equi_range(struct rpol_table *rpt)
{
   size_t i, j;
   int xrise = 0, yrise = 0;
   size_t nz = 0;

   if ( rpt == NULL )
      return;
   rpt->equidistant = 0;

   if ( rpt->x == NULL || rpt->z == NULL ||
        (rpt->ndim >= 2 && rpt->y == NULL ) )
      return;
   if ( rpt->nx < 1 || (rpt->ndim >= 2 && rpt->ny < 1 ) )
      return;

   /* The number of z values in this table */
   nz = rpt->nx;
   if ( rpt->ndim > 1 )
      nz *= rpt->ny;

   /* Auto-revert 1-D tables with descending x values. */
   if ( rpt->ndim == 1 && rpt->nx > 1 && rpt->x[rpt->nx-1] < rpt->x[0] )
   {
      /* Reverse the order of the table */
      double xt, zt;
      for ( i=0; i<rpt->nx/2; i++) /* Leave out the middle one of an odd number of entries */
      {
         xt = rpt->x[i];
         zt = rpt->z[i];
         rpt->x[i] = rpt->x[rpt->nx-1-i];
         rpt->z[i] = rpt->z[rpt->nx-1-i];
         rpt->x[rpt->nx-1-i] = xt;
         rpt->z[rpt->nx-1-i] = zt;
      }
      if ( rpol_verbosity > 0 )
         fprintf(stderr, "Interpolation table '%s' reversed to ascending order.\n", rpt->fname);
   }

   /* Assume x (and y) are in ascending order; for 2-D that might be not the case */
   rpt->xrise = rpt->yrise = 1;

   /* Check if steps in x are equidistant */
   rpt->xmin = rpt->x[0];
   if ( rpt->nx > 1 )
   {
      rpt->dx = (rpt->x[rpt->nx-1] - rpt->x[0]) / (double) (rpt->nx-1);
      if ( rpt->dx != 0. )
         rpt->dxi = 1./rpt->dx;
      else
         rpt->dxi = 0.;
      rpt->xmax = rpt->x[rpt->nx-1];
   }
   else
   {
      rpt->dx = 0.;
      rpt->dxi = 0.;
      rpt->xmax = rpt->xmin;
   }
   rpt->equidistant = 1;
   for ( i=0; i<rpt->nx; i++ )
   {
      double xeq = rpt->xmin + i*rpt->dx;
      if ( fabs((xeq-rpt->x[i])*rpt->dxi) > 1e-10 )
      {
         rpt->equidistant &= 0x2; /* Turn off bit 1 */
         break;
      }
   }
   /* X in ascending order, descending order, or out of order ? */
   if ( rpt->nx >= 2 )
   {
      xrise = (rpt->x[1] >= rpt->x[0]) ? 1 : -1;
      for ( i=2; i<rpt->nx; i++ )
      {
         if ( (rpt->x[i] - rpt->x[i-1])*xrise < 0. )
         {
            fprintf(stderr,
               "X supporting positions of table '%s' are not in monotonic order.\n",
               rpt->fname);
            xrise *= 2;
            rpt->scheme = -3; /* Not suitable for interpolation */
            break;
         }
      }
      if ( xrise == -1 && rpol_verbosity > 0 )
         fprintf(stderr,
            "X supporting positions of table '%s' are in decreasing order.\n",
            rpt->fname);
      rpt->xrise = xrise;
   }
   if ( rpt->ndim == 2 || rpt->ndim == 3 )
   {
      /* Check if steps in y are equidistant */
      rpt->ymin = rpt->y[0];
      if ( rpt->ny > 1 )
      {
         rpt->dy = (rpt->y[rpt->ny-1] - rpt->y[0]) / (double) (rpt->ny-1);
         if ( rpt->dy != 0. )
            rpt->dyi = 1./rpt->dy;
         else
            rpt->dyi = 0.;
         rpt->ymax = rpt->y[rpt->ny-1];
      }
      else
      {
         rpt->dy = 0.;
         rpt->dyi = 0.;
         rpt->ymax = rpt->ymin;
      }
      rpt->equidistant |= 2;
      for ( i=0; i<rpt->ny; i++ )
      {
         double yeq = rpt->ymin + i*rpt->dy;
         if ( fabs((yeq-rpt->y[i])*rpt->dyi) > 1e-10 )
         {
            rpt->equidistant &= 0x1; /* Turn off bit 2 */
            break;
         }
      }
      /* Y in ascending order, descending order, or out of order ? */
      if ( rpt->ny >= 2 )
      {
         yrise = (rpt->y[1] >= rpt->y[0]) ? 1 : -1;
         for ( i=2; i<rpt->ny; i++ )
         {
            if ( (rpt->y[i] - rpt->y[i-1])*yrise < 0. )
            {
               fprintf(stderr,
                  "Y supporting positions of table '%s' are not in monotonic order.\n",
                  rpt->fname);
               yrise *= 2;
               rpt->scheme = -3; /* Not suitable for interpolation */
               break;
            }
         }
         if ( yrise == -1 && rpol_verbosity > 0 )
            fprintf(stderr,
               "Y supporting positions of table '%s' are in decreasing order.\n",
               rpt->fname);
         rpt->yrise = yrise;
      }
   }

   rpt->zmin = rpt->zmax = rpt->z[0];
   for ( i=0; i<nz; i++ )
   {
      if ( rpt->z[i] > rpt->zmax )
         rpt->zmax = rpt->z[i];
      else if ( rpt->z[i] < rpt->zmin )
         rpt->zmin = rpt->z[i];
   }
   
   /* For 2-D tables, the upper envelope in projection onto the x axis
      may have been requested. And has to be redone after any remapping. */
   if ( rpt->ndim > 1 && rpt->zxreq )
   {
      if ( rpt->zxmax == NULL ) /* Allocate if doing it the first time */
         rpt->zxmax = (double *) calloc(rpt->nx,sizeof(double));
      if ( rpt->zxmax != NULL ) /* Could have failed */
      {
         for ( i=0; i<rpt->nx; i++ )
         {
            rpt->zxmax[i] = rpt->z[i*rpt->ny];
            for (j=1; j<rpt->ny; j++ )
            {
               if ( rpt->z[i*rpt->ny+j] > rpt->zxmax[i] )
                  rpt->zxmax[i] = rpt->z[i*rpt->ny+j];
            }
         }
      }
      if ( rpt->zxmin == NULL ) /* Allocate if doing it the first time */
         rpt->zxmin = (double *) calloc(rpt->nx,sizeof(double));
      if ( rpt->zxmin != NULL ) /* Could have failed */
      {
         for ( i=0; i<rpt->nx; i++ )
         {
            rpt->zxmin[i] = rpt->z[i*rpt->ny];
            for (j=1; j<rpt->ny; j++ )
            {
               if ( rpt->z[i*rpt->ny+j] < rpt->zxmin[i] )
                  rpt->zxmin[i] = rpt->z[i*rpt->ny+j];
            }
         }
      }
   }
}

/**
 *  @short General function for loading interpolation table combines 1-D and 2-D grid case. 
 *
 *  @param fname   Text input file name
 *  @param nd      dimension/format parameter with the following possible values:
 *                 1: 1-D (2-column) input expected, 2: 2-D (1+ny columns) input with marker
 *                 indicating special line for y values, 3: 2-D (3 columns) with repeated
 *                 x and y values (matching rectangular grid), 0: format entirely defined
 *                 in the data file, requiring the first line to start as '#@RPOL@',
 *                 -1,-2,-3: If the first line starts as '#@RPOL@', this line defines the
 *                 format and otherwise it is falling back to format 1, 2, or 3, respectively.
 *  @param ymarker The marker indicating the special header line listing the y values
 *                 with nd=2 only (otherwise ignored).
 */

struct rpol_table *read_rpol_table (const char *fname, int nd, const char *ymarker, const char *options)
{
   struct rpol_table *rpt = NULL;
   struct rpt_list *rptl = NULL;
   char ymarker_i[100], fnplus[8192], fnclean[4097];
   char line[10240];
   char *fn_options = NULL;
   char options_eff[8192];
   size_t i, j;
   size_t expect_rows = 0, expect_cols = 0;
   int clip_default = 0;  /* No clipping */
   int scheme_default = 1; /* Linear interpolation */
   size_t xcol=1, ycol=2, zcol=3;
   int xlog=0, ylog=0, zlog=0;
   int rescan_first_line = 0;
   int verbose = 0, zxreq = 0;
   double aux = 0.;
   double xscale = 0., yscale = 0., zscale = 0.; /* means immediate rescaling not used */

   /* If never been here before, we need to allocate a starting point first. */
   if ( rpt_list_base == NULL )
   {
      if ( (rpt_list_base = (struct rpt_list *) calloc(1,sizeof(struct rpt_list))) == NULL )
      {
         fprintf(stderr, "rpt_list allocation problem\n");
         return NULL;
      }
   }
   
   if ( rpol_verbosity == -1 )
   {
      if ( getenv("RPOL_VERBOSE") == NULL )
         rpol_verbosity = 0;
      else
         rpol_verbosity = atoi(getenv("RPOL_VERBOSE"));
      if ( rpol_verbosity < 0 )
         rpol_verbosity = 0;
   }
   verbose = rpol_verbosity;

   /* The table is normally registered just under its name but marker and options would be included. */
   if ( strlen(fname) >= 4095 )
   {
      fprintf(stderr,"File name would be truncated: %s\n", fname);
      fnplus[4095] = fnclean[4096] = '\0';
      return NULL; /* Better to give up at this point */
   }
   strncpy(fnplus,fname,4095);   /* For given file name plus any API-passed options */
   strncpy(fnclean,fname,4095);  /* For given file name cleans of any options given along with it */
   if ( ymarker != NULL && *ymarker != '\0' )
   {
      strcat(fnplus,";ymarker=");
      strncat(fnplus,ymarker,2000);
   }
   options_eff[0] = '\0';
   if ( options != NULL && *options != '\0' )
   {
      if ( strlen(options) >= 2000 )
      {
         fprintf(stderr,"Options for %s would be truncated.\n",fname);
         return NULL;
      }
      strcat(fnplus,";options=");
      strncat(fnplus,options,2000);
      strcpy(options_eff,options);
   }
   
   /* Are there recognized rpolator options attached to the file name? */
   if ( (fn_options = strstr(fnclean,"#rpol:")) != NULL )
   {
      *fn_options = '\0'; /* Options registered with the file name but not used when opening the file. */
      fn_options += strlen("#rpol:"); /* Advance to the actual start of options as attached. */
      if ( verbose > 3 )
         printf("Rpolator options %s attached to file name %s.\n", fn_options, fnclean);
      if ( options_eff[0] != '\0' )
         strcat(options_eff,",");
      if ( strlen(options_eff) + strlen(fn_options) >= sizeof(options_eff) )
      {
         fprintf(stderr,"Effective options for %s would be truncated.\n",fname);
         return NULL;
      }
      strcat(options_eff,fn_options);
   }

   /* Go through the list of loaded tables and look for a match. */
   /* The number of tables is expected to be small enough for sequential search. */
   /* If it gets too large we could use a hash sum for accelerated search, but not at this stage. */
   for ( rptl=rpt_list_base; rptl!=NULL && rptl->rpt != NULL && 
            rptl->next != NULL; rptl=rptl->next )
   {
      if ( strcmp(fnplus,rptl->rpt->fname) == 0 && 
            (nd < 0 || nd == rptl->rpt->ndim) )
         break;
   }

   /* We should not run off the end of the list but always have an unused pointer at the end. */
   if ( rptl == NULL )
   {
      fprintf(stderr, "rpt_list problem\n");
      return NULL;
   }

   /* If we loaded this interpolation table before, no need to load it again. */
   if ( rptl->rpt != NULL )
   {
      if ( verbose >= 2 )
      {
         printf("Interpolation table '%s' already loaded.\n", rptl->rpt->fname);
      }
      rptl->rpt->use_count++;
      return rptl->rpt;
   }

   ymarker_i[0] = '\0';

   FILE *f = fileopen(fnclean,"r"); /* Use the cleaned file name (without any '#rpol:...') */

   if ( f == NULL )
   {
      perror(fnclean);
      return NULL;
   }

   if ( nd <= 0 ) /* Auto-detect table type from header line */
   {
      if ( fgets(line,sizeof(line)-1,f) == NULL )
      {
         fprintf(stderr,"%s: Empty file cannot be used as a table.\n", fnclean);
         return NULL;
      }
      
      /* Default dimension number unless we see a "#@RPOL@" header line */
      if ( nd < 0 )
         nd = -nd;

      if ( strncmp(line,"#@RPOL@",7) == 0 ) /* Must be in first line in file! */
      {
         char *s = line+7;
         /* If the next character is not white-space or end-of-line then
            we seem to have a (somehow delimited) marker text indicating
            the start of a line with all supporting y values (for nd=2). */
         if ( *s != ' ' && *s != '\t' && *s != '\n' && *s != '\0' )
         {
            char *e;
            char del = *s; /* Delimiter for optional 2-D y marker */
            s++;
            if ( del == '(' ) /* Allow matching parenthesis as delimiters */
               del = ')';
            if ( del == '[' ) /* Allow matching brackets as delimiters */
               del = ']';
            if ( del == '{' ) /* Allow matching braces as delimiters */
               del = '}';
            /* Search for matchin end delimiter before end of line. */
            for ( e=s; *e != del && *e != '\n' && *e != '\0'; e++ )
               ;
            if ( *e == del ) /* Found matching delimiter */
            {
               *e = '\0';
               strncpy(ymarker_i,s,sizeof(ymarker_i)-1);
               ymarker_i[sizeof(ymarker_i)-1] = '\0';
               *e = del;
               s = e+1;
            }
            else if ( !isdigit(*e) )
            {
               fprintf(stderr,
                  "No matching delimiters in '#@RPOL#' header line of file '%s'.\n",
                  fnclean);
            }
         }

         while ( *s == ' ' || *s == '\t' ) /* Skip over whitespace */
            s++;

         if ( isdigit(*s) ) /* Dimension number following directly, overrides default */
            nd = atoi(s);

         /* Options indicating dimension, clipping, interpolation scheme, or unusal row or column numbers. */
         /* if ( (s = strcasestr(s,"OPTIONS:")) != NULL ) */ /* not portable */
         if ( (s = strstr(s,"OPTIONS:")) != NULL )
         {
            char word[100];
            int spos=0;
            s += 8;
            while ( *s == ' ' || *s == '\t' ) /* Skip over whitespace */
               s++;
            while ( getword(s,&spos,word,sizeof(word)-1,',','\n') > 0 )
            {
               if ( rpol_verbosity >= 4 )
                  printf("Processing table header option %s\n", word);
               if ( strncasecmp(word,"CLIP=",5) == 0 ) /* Clipping outside supported range */
               {
                  if ( strcasecmp(word+5,"ON") == 0 || strcasecmp(word+5,"YES") == 0 )
                     clip_default = 1;
                  else if ( strcasecmp(word+5,"OFF") == 0 || strcasecmp(word+5,"NO") == 0 )
                     clip_default = 0;
                  else
                     clip_default = atoi(word+5);
               }
               else if ( strcasecmp(word,"CLIP") == 0 )
               {
                  clip_default = 1;
               }
               else if ( strcasecmp(word,"NOCLIP") == 0 )
               {
                  clip_default = 0;
               }
               else if ( strcasecmp(word,"ZXMAX") == 0 || strcasecmp(word,"ZXMIN") == 0 )
               {
                  zxreq = 1;
               }
               else if ( strcasecmp(word,"VERBOSE") == 0 )
               {
                  if ( verbose <= 0 )
                     verbose = 1;
               }
               else if ( strncasecmp(word,"VERBOSE=",8) == 0 )
               {
                  verbose = atoi(word+8);
               }
               else if ( strncasecmp(word,"SCHEME=",7) == 0 ) /* Interpolation scheme */
               {
                  scheme_default = atoi(word+7);
               }
               else if ( strncasecmp(word,"ROWS=",5) == 0 ) /* Minimum number of rows to expect */
               {
                  expect_rows = atoi(word+5);
               }
               else if ( strncasecmp(word,"COLS=",5) == 0 ) /* Minimum number of columns to expect */
               {
                  expect_cols = atoi(word+5);
               }
               else if ( strncasecmp(word,"COLUMNS=",8) == 0 )
               {
                  expect_cols = atoi(word+8);
               }
               else if ( strncasecmp(word,"AUX=",4) == 0 )
               {
                  aux = atof(word+4);
               }
               else if ( strncasecmp(word,"XCOL=",5) == 0 )
               {
                  xcol = atoi(word+5);
               }
               else if ( strncasecmp(word,"XSCALE=",7) == 0 )
               {
                  if ( strcasecmp(word+7,"deg2rad") == 0 )
                     xscale = M_PI/180.;
                  else if ( strcasecmp(word+7,"rad2deg") == 0 )
                     xscale = 180./M_PI;
                  else
                     xscale = atof(word+7);
               }
               else if ( strncasecmp(word,"XLOG",4) == 0 )
               {
                  xlog = 1;
               }
               else if ( strncasecmp(word,"YCOL=",5) == 0 )
               {
                  ycol = atoi(word+5);
               }
               else if ( strncasecmp(word,"YSCALE=",7) == 0 )
               {
                  if ( strcasecmp(word+7,"deg2rad") == 0 )
                     yscale = M_PI/180.;
                  else if ( strcasecmp(word+7,"rad2deg") == 0 )
                     yscale = 180./M_PI;
                  else
                     yscale = atof(word+7);
               }
               else if ( strncasecmp(word,"YLOG",4) == 0 )
               {
                  ylog = 1;
               }
               else if ( strncasecmp(word,"ZCOL=",5) == 0 )
               {
                  zcol = atoi(word+5);
                  if ( nd == 1 )
                  {
                     fprintf(stderr,"Using invalid ZCOL parameter in header for 1-D table '%s'.\n", fname);
                     ycol = zcol;
                  }
               }
               else if ( strncasecmp(word,"ZSCALE=",7) == 0 )
               {
                  if ( strcasecmp(word+7,"deg2rad") == 0 )
                     zscale = M_PI/180.;
                  else if ( strcasecmp(word+7,"rad2deg") == 0 )
                     zscale = 180./M_PI;
                  else
                     zscale = atof(word+7);
               }
               else if ( strncasecmp(word,"ZLOG",4) == 0 )
               {
                  zlog = 1;
               }
               else
               {
                  fprintf(stderr,
                     "Unknown option '%s' in '#@RPOL#' header line of file '%s' is ignored.\n",
                     word, fnclean);
               }
            }
         } /* "OPTIONS:" */
      }
      else
         rescan_first_line = 1; /* Was not an '#@RPOL#' header line but something else */
      
      if ( nd <= 0 || nd > 3 )
      {
         fprintf(stderr,"No valid interpolation table header in '%s'.\n", fnclean);
         return NULL;
      }
   }

   if ( nd != 1 && nd != 2 && nd != 3 )
   {
      fprintf(stderr,"Invalid dimension/format parameter nd=%d for file '%s'.\n", nd, fnclean);
      return NULL;
   }

   /* Same procedure as above for options passed to the function, 
      overriding corresponding options in the file. */
   if ( options_eff[0] != '\0' )
   {
      char word[100];
      int spos=0;
      const char *s = options_eff;
      while ( *s == ' ' || *s == '\t' ) /* Skip over whitespace */
         s++;
      while ( getword(s,&spos,word,sizeof(word)-1,',','\n') > 0 )
      {
         if ( rpol_verbosity >= 4 || verbose > 0 )
            printf("Processing parameter option %s\n", word);
         if ( strncasecmp(word,"CLIP=",5) == 0 ) /* Clipping outside supported range */
         {
            if ( strcasecmp(word+5,"ON") == 0 || strcasecmp(word+5,"YES") == 0 )
               clip_default = 1;
            else if ( strcasecmp(word+5,"OFF") == 0 || strcasecmp(word+5,"NO") == 0 )
               clip_default = 0;
            else
               clip_default = atoi(word+5);
         }
         else if ( strcasecmp(word,"CLIP") == 0 )
         {
            clip_default = 1;
         }
         else if ( strcasecmp(word,"NOCLIP") == 0 )
         {
            clip_default = 0;
         }
         else if ( strcasecmp(word,"ZXMAX") == 0 || strcasecmp(word,"ZXMIN") == 0 )
         {
            zxreq = 1;
         }
         else if ( strcasecmp(word,"VERBOSE") == 0 )
         {
            if ( verbose <= 0 )
               verbose = 1;
         }
         else if ( strncasecmp(word,"VERBOSE=",8) == 0 )
         {
            verbose = atoi(word+8);
         }
         else if ( strncasecmp(word,"SCHEME=",7) == 0 ) /* Interpolation scheme */
         {
            scheme_default = atoi(word+7);
         }
         else if ( strncasecmp(word,"ROWS=",5) == 0 ) /* Minimum number of rows to expect, saves re-allocation */
         {
            expect_rows = atoi(word+5);
         }
         else if ( strncasecmp(word,"COLS=",5) == 0 ) /* Minimum number of columns to expect */
         {
            expect_cols = atoi(word+5);
         }
         else if ( strncasecmp(word,"COLUMNS=",8) == 0 ) /* Same as 'COLS=' */
         {
            expect_cols = atoi(word+8);
         }
         else if ( strncasecmp(word,"AUX=",4) == 0 ) /* Just stored as is, for user to decide what it is */
         {
            aux = atof(word+4);
         }
         else if ( strncasecmp(word,"XCOL=",5) == 0 ) /* Which input column corresponds to x values */
         {
            xcol = atoi(word+5);
         }
         else if ( strncasecmp(word,"XSCALE=",7) == 0 ) /* Possible x scaling factors */
         {
            if ( strcasecmp(word+7,"deg2rad") == 0 )
               xscale = M_PI/180.;
            else if ( strcasecmp(word+7,"rad2deg") == 0 )
               xscale = 180./M_PI;
            else
               xscale = atof(word+7);
         }
         else if ( strncasecmp(word,"XLOG",4) == 0 ) /* X internally treated through its log(x) value */
         {
            xlog = 1;
         }
         else if ( strncasecmp(word,"YCOL=",5) == 0 )
         {
            ycol = atoi(word+5);
         }
         else if ( strncasecmp(word,"YSCALE=",7) == 0 )
         {
            if ( strcasecmp(word+7,"deg2rad") == 0 )
               yscale = M_PI/180.;
            else if ( strcasecmp(word+7,"rad2deg") == 0 )
               yscale = 180./M_PI;
            else
               yscale = atof(word+7);
         }
         else if ( strncasecmp(word,"YLOG",4) == 0 )
         {
            ylog = 1;
         }
         else if ( strncasecmp(word,"ZCOL=",5) == 0 )
         {
            zcol = atoi(word+5);
            if ( nd == 1 )
            {
               if ( rpol_verbosity > 0 || ycol != 2 )
                  fprintf(stderr,"Using invalid ZCOL parameter in options for 1-D table '%s'.\n", fname);
               ycol = zcol; /* Probably meant the column of dependent values, unless trying to set both */
            }
         }
         else if ( strncasecmp(word,"ZSCALE=",7) == 0 )
         {
            if ( strcasecmp(word+7,"deg2rad") == 0 )
               zscale = M_PI/180.;
            else if ( strcasecmp(word+7,"rad2deg") == 0 )
               zscale = 180./M_PI;
            else
               zscale = atof(word+7);
         }
         else if ( strncasecmp(word,"ZLOG",4) == 0 )
         {
            zlog = 1;
         }
         else
         {
            fprintf(stderr,
               "Unknown option '%s' in option parameter for file '%s' is ignored.\n",
               word, fnclean);
         }
      }
   }
   
   /* Since we have to load a new table, allocate it. */
   rpt = (struct rpol_table *) calloc(1,sizeof(struct rpol_table));
   if ( rpt == NULL )
   {
      fprintf(stderr, "rpol_table allocation problem\n");
      return NULL;
   }
   
   rpt->aux = aux;
   if ( aux != 0. && (rpol_verbosity > 3 || verbose > 0) )
      printf("Auxiliary value = %g\n", aux);
   rpt->zxreq = zxreq;

   if ( ymarker == NULL && *ymarker_i != '\0' )
      ymarker = ymarker_i;
   else if ( ymarker != NULL && *ymarker == '\0' && *ymarker_i != '\0' )
      ymarker = ymarker_i;

   /* If we expected a header line but there was none, and it was not
      a comment line either, the table reading code must see that again. */
   /* That includes a y coordinates line with a marker starting like a comment line. */
   /* The current solution is ugly and subject to change ... */
   if ( rescan_first_line && (line[0] != '#' || 
         (nd==2 && ymarker[0]=='#' && strncmp(line,ymarker,strlen(ymarker)) == 0)) )
   {
      if ( rpol_verbosity > 0 || verbose > 0 )
         fprintf(stderr, "Re-opening file '%s' to avoid losing first line of data in the absence of comments.\n", fname);
      if ( f == stdin )
         fprintf(stderr,"Not re-opening standard input. May have lost first line of data.\n");
      else
      {
         /* We cannot un-'fgets' the first line; re-open file instead. */
         fileclose(f);
         f = fileopen(fnclean,"r");
         if ( f == NULL )
         {
            perror(fname);
            rpol_free(rpt,1);
            return NULL;
         }
      }
   }

   if ( nd == 1 )
   {
      double **xy = NULL; /* Pointer to (to-be-allocated) data column pointers. */
      size_t xycol[2] = { xcol, ycol }; /* Default: 1, 2 */
      size_t nrows = expect_rows;
      int rc = 0;

      if ( xcol == 1 && ycol == 2 )
         rc = read_table_v (fname, f, &nrows, 2, &xy, NULL);
      else
         rc = read_table_v (fname, f, &nrows, 2, &xy, xycol);

      if ( (rc < 0 && nrows == 0) || xy == NULL )
      {
         fileclose(f);
         rpol_free(rpt,1);
         return NULL;
      }
      
      if ( verbose >= 3 )
      {
         printf("\nRaw data for 1-D table:\n");
         for (i=0; i<nrows; i++ )
            printf("x/y[%ju] = %g %g\n", i, xy[0][i], xy[1][i]);
         printf("\n");
      }

      rpt->x = xy[0];
      rpt->z = rpt->y = xy[1];
      free(xy);
      xy = NULL;

      rpt->ndim = 1;
      rpt->nx = nrows;
      rpt->ny = 0;
      rpt->zxreq = 0; /* Not applicable for 1-D even if requested */
   }
   else if ( nd == 2 )
   {
      char word[32];
      int iline = 0;
      int ipos = 0;
      int rc = 0;
      char *s, *t;
      double **xz = NULL; /* Pointer to (to-be-allocated) data column pointers. */
      double *y = NULL;
      size_t nrows = 0;
      size_t ny = 0;
      size_t nym = 0;
      if ( ymarker != NULL && *ymarker != '\0' )
         nym = strlen(ymarker);

      line[0] = '\0';
      while ( fgets(line,sizeof(line)-1,f) != NULL )
      {
         iline++;
         if ( nym > 0 )
         {
            /* With a defined marker, everything else is ignored */
            if ( strncmp(line,ymarker,nym) == 0 )
               break;
         }
         else
         {
            /* Without marker, all empty and comment lines are ignored */
            strip_comments(line);
            if ( line[0] != '\0' )
               break;
         }
         line[0] = '\0';
      }

      if ( line[0] == '\0' )
      {
         fprintf(stderr,"No suitable header line in table %s.\n", fname);
         rpol_free(rpt,1);
         fileclose(f);
      	 return NULL;
      }

      /* There may be an auxiliary value after the marker but before y values. */
      s = line + nym;
      while ( *s == ' ' || *s == '\t' )
         s++;
      if ( (t = strrchr(s,'=')) != NULL ) /* Separator is last '=' */
      {
         *t = '\0';
         rpt->aux = atof(s);
         if ( rpol_verbosity > 3 || verbose > 0 )
            printf("Auxiliary value set to %g ('%s')\n", rpt->aux, s);
         s = t+1;
         while ( *s == ' ' || *s == '\t' )
            s++;
      }
      
      if ( rpol_verbosity > 3 || verbose > 0 )
         printf("Y supporting positions after header markup '%s' of length %zu: %s\n",
            ymarker==NULL?"":ymarker, nym, s);

      if ( expect_cols < 1 )
         expect_cols = 2;
      if ( expect_cols > 500 )
         expect_cols = 500;
      /* Pre-allocate a buffer for up to 500 y values (or matching expected data columns) */
      if ( (y = (double *) calloc(expect_cols,sizeof(double))) == NULL )
      {
         fprintf(stderr,"rpol_table allocation problem\n");
         rpol_free(rpt,1);
         fileclose(f);
      	 return NULL;
      }
      for ( ipos=0; s[ipos] != '\0' && 
            getword(s,&ipos,word,30,' ','#') > 0 && ny < expect_cols;  )
      {
         if ( sscanf(word, "%lf", y+ny) == 1 )
            ny++;
         else
            break;

         /* If we are going to run out of space for y values, then expand. */
         if ( ny+1 > expect_cols && s[ipos] != '\0' && s[ipos] != '\n' )
         {
            double *yn = (double *) realloc(y,expect_cols*2*sizeof(double));
            if ( yn != NULL )
            {
               y = yn;
               expect_cols *= 2;
            }
         }
      }
      if ( ny == expect_cols ) /* Just as expected? */
      {
         rpt->y = y; /* No need to reallocate. Just copy pointer. */
         y = NULL;   /* Reset old pointer. */
      }
      /* Try to re-allocate to actually needed size */
      else if ( ny == 0 || 
          (rpt->y = (double *) realloc(y,ny*sizeof(double))) == NULL )
      {
         /* Re-allocation failed due to lack of memory (errno == ENOMEM). 
            Old one still around. */
         free(y);
         y = NULL;
         fprintf(stderr,"rpol_table allocation problem\n");
         rpol_free(rpt,1);
         fileclose(f);
      	 return NULL;
      }
      else /* Successfully re-allocated. Reset old pointer. */
         y = NULL;

      rpt->ny = ny;

      /* The rest of the table should be all lines with one x value
         followed by ny z values each. No fancy mapping used here. */
      rc = read_table_v (fname, f, &nrows, ny+1, &xz, NULL);

      if ( (rc < 0 && nrows == 0) || xz == NULL )
      {
         if ( xz != NULL )
            free(xz);
         fileclose(f);
         rpol_free(rpt,1);
         return NULL;
      }
      
      if ( verbose >= 3 )
      {
         printf("\nRaw data for 2-D table with y values in header line:\n");
         printf("y=");
         for ( j=0; j<ny; j++ )
            printf(" %g", rpt->y[j]);
         printf("\n");

         for (i=0; i<nrows; i++ )
         {
            printf("x/z...[%ju] = %g", i, xz[0][i]);
            for ( j=0; j<ny; j++ )
               printf(" %g", xz[j+1][i]);
            printf("\n");
         }
         printf("\n");
      }

      rpt->x = xz[0];
      rpt->nx = nrows;
      if ( (rpt->z = (double*) calloc(rpt->nx*rpt->ny,sizeof(double))) == NULL )
      {
         for ( i=0; i<ny; i++ )
            if ( xz[i+1] != NULL )
               free(xz[i+1]);
         free(xz);
         fileclose(f);
         rpol_free(rpt,1);
         return NULL;
      }

      for ( i=0; i<rpt->nx; i++ )
         for ( j=0; j<rpt->ny; j++ )
            rpt->z[i*rpt->ny+j] = xz[j+1][i];

      fflush(stdout);
      fprintf(stderr,"Table for 2-D interpolation with %ju rows and %ju columns has been read from file %s\n", 
         rpt->nx, rpt->ny, fname);
   }
   else if ( nd == 3 )
   {
      double **xyz = NULL; /* Pointer to (to-be-allocated) data column pointers. */
      size_t xyzcol[3] = { xcol, ycol, zcol }; /* Default: 1, 2, 3 */
      size_t nrows = -expect_rows;
      size_t nx, ny;
      int rc = 0;
      int xfirst = -1;

      if ( xcol == 1 && ycol == 2 && zcol == 3 )
         rc = read_table_v (fname, f, &nrows, 3, &xyz, NULL);
      else
         rc = read_table_v (fname, f, &nrows, 3, &xyz, xyzcol);

      fileclose(f);

      if ( (rc < 0 && nrows == 0) || xyz == NULL )
      {
         rpol_free(rpt,1);
         if ( xyz != NULL )
            free(xyz);
         return NULL;
      }
      if ( xyz[0] == NULL || xyz[1] == NULL || xyz[2] == NULL )
      {
         if ( xyz[0] != NULL )
            free(xyz[0]);
         if ( xyz[1] != NULL )
            free(xyz[1]);
         if ( xyz[2] != NULL )
            free(xyz[2]);
         free(xyz);
         rpol_free(rpt,1);
         return NULL;
      }
      
      if ( verbose >= 3 )
      {
         printf("\nRaw data for 2-D table with explicit x/y/z values:\n");
         for (i=0; i<nrows; i++ )
            printf("x/y/z[%ju] = %g %g %g\n", i, xyz[0][i], xyz[1][i], xyz[2][i]);
         printf("\n");
      }

      if ( nrows <= 1 )
      {
         rpt->x = xyz[0];
         rpt->y = xyz[1];
         rpt->z = xyz[2];
         rpt->nx = rpt->ny = 1;
      }
      else if ( xyz[0][0] == xyz[0][1] && xyz[1][0] != xyz[1][1] )
      {
         xfirst = 0; /* y values change first, x stays the same */
      }
      else if ( xyz[0][0] != xyz[0][1] && xyz[1][0] == xyz[1][1] )
      {
         xfirst = 1; /* x values change first, y stays the same */
      }
      else
         rc = 4; /* Both changing is not what we want */

      nx = ny = 1;
      if ( xfirst )
      {
         /* Sequence of x values until y changes */
         for ( i=1; i<nrows && rc == 0; i++ )
         {
            if ( xyz[1][i-1] != xyz[1][i] )
            {
               nx = i;
               ny = nrows/nx;
               break;
            }
         }
         /* Check that x and y values repeat as expected for rectangular grid */
         for ( i=0; i<nx && rc==0; i++ )
            for ( j=0; j<ny && rc==0; j++ )
            {
               if ( xyz[0][j*nx+i] != xyz[0][i] )
                  rc = 4;
               else if ( xyz[1][j*nx+i] != xyz[1][j*nx] )
                  rc = 4;
            }
      }
      else if ( xfirst == 0 )
      {
         /* Sequence of y values until x changes */
         for ( i=1; i<nrows && rc == 0; i++ )
         {
            if ( xyz[0][i-1] != xyz[0][i] )
            {
               ny = i;
               nx = nrows/ny;
               break;
            }
         }
         /* Check that x and y values repeat as expected for rectangular grid */
         for ( i=0; i<nx && rc==0; i++ )
            for ( j=0; j<ny && rc==0; j++ )
            {
               if ( xyz[0][i*ny+j] != xyz[0][i*ny] )
                  rc = 4;
               else if ( xyz[1][i*ny+j] != xyz[1][j] )
                  rc = 4;
            }
      }
      if ( nx*ny != nrows )
         rc = 4;
      
      if ( rc != 0 )
      {
         fprintf(stderr,"Invalid order of entries in %s (nx=%ju, ny=%ju, nrows=%ju, xfirst=%d).\n", 
            fnplus, nx, ny, nrows, xfirst);
         free(xyz);
         rpol_free(rpt,1);
         return NULL;
      }
      
      if ( (rpt->x = (double *) calloc(nx,sizeof(double))) == NULL || 
           (rpt->y = (double *) calloc(ny,sizeof(double))) == NULL ||
           (rpt->z = (double *) calloc(nx*ny,sizeof(double))) == NULL )
      {
         free(xyz[0]);
         free(xyz[1]);
         free(xyz[2]);
         free(xyz);
         rpol_free(rpt,1);
         return NULL;
      }
      if ( xfirst == 1 )
      {
         for ( i=0; i<nx; i++ )
            rpt->x[i] = xyz[0][i];
         for ( j=0; j<ny; j++ )
            rpt->y[j] = xyz[1][j*nx];
         for ( i=0; i<nx && rc==0; i++ )
            for ( j=0; j<ny && rc==0; j++ )
               rpt->z[i*ny+j] = xyz[2][j*nx+i];
      }
      else if ( xfirst == 0 )
      {
         for ( i=0; i<nx; i++ )
            rpt->x[i] = xyz[0][i*ny];
         for ( j=0; j<ny; j++ )
            rpt->y[j] = xyz[1][j];
         for ( i=0; i<nx && rc==0; i++ )
            for ( j=0; j<ny && rc==0; j++ )
               rpt->z[i*ny+j] = xyz[2][i*ny+j];
      }
      
      if ( xfirst != -1 )
      {
         free(xyz[0]);
         free(xyz[1]);
         free(xyz[2]);
      }
      free(xyz);
      
      rpt->nx = nx;
      rpt->ny = ny;
   }

   /* Parameters describing table and how to use it. */
   rpt->fname = strdup(fnplus);
   if ( options != NULL && *options != '\0' )
      rpt->options = strdup(options);
   rpt->ndim = nd;
   rpt->scheme = scheme_default;
   rpt->clipping = clip_default;
   rpt->equidistant = 0; /* Need to find out later-on */
   rpt->use_count = 1;

   /* Asking for log units in x, y, and/or z is something that
      only affects interpolation but the x or y positions passed
      for interpolation are still as in the input file. */   
   if ( xlog && rpt->x != NULL )
   {
      if ( verbose >= 2 )
         printf("Using logarithm of x values.\n");
      for ( i=0; i<rpt->nx; i++ )
      {
         if ( rpt->x[i] > 0. )
            rpt->x[i] = log(rpt->x[i]);
         else
         {
            if ( verbose > 0 )
               printf("Cannot represent x value of %g with zlog option.\n", rpt->x[i]);
            rpt->x[i] = -99999.;
         }
      }
      rpt->xlog = rpt->logs = 1;
   }
   if ( ylog && rpt->ndim == 1 )
   {
      if ( verbose > 0 )
         printf("Assuming 'zlog' option for 'ylog' with 1-D table.\n");
      zlog = 1;
   }
   if ( ylog && rpt->ndim >= 2 && rpt->y != NULL )
   {
      if ( verbose >= 2 )
         printf("Using logarithm of y values.\n");
      for ( i=0; i<rpt->ny; i++ )
      {
         if ( rpt->y[i] > 0. )
            rpt->y[i] = log(rpt->y[i]);
         else
         {
            if ( verbose > 0 )
               printf("Cannot represent y value of %g with zlog option.\n", rpt->y[i]);
            rpt->y[i] = -99999.;
         }
      }
      rpt->ylog = rpt->logs = 1;
   }
   if ( zlog && rpt->z != NULL )
   {
      size_t nz = rpt->nx;
      if ( rpt->ndim > 1 )
         nz *= rpt->ny;
      if ( verbose >= 2 )
         printf("Using logarithm of z values.\n");
      for ( i=0; i<nz; i++ )
      {
         if ( rpt->z[i] > 0. )
            rpt->z[i] = log(rpt->z[i]);
         else
         {
            if ( verbose > 0 )
               printf("Cannot represent z value of %g with zlog option.\n", rpt->z[i]);
            rpt->z[i] = -99999.;
         }
      }
      rpt->zlog = 1;
   }

   /* x/y/z scaling is applied immediately and (unlike log scale)
      not reverted when doing the interpolation. So if the table
      had values, for example, in degrees and you tell to convert
      it to radians, you must provide coordinates that are in
      radians. */
   if ( xscale != 0. && rpt->x != NULL )
   {
      if ( verbose >= 2 )
         printf("Scaling x values by factor %g\n", xscale);
      for ( i=0; i<rpt->nx; i++ )
         rpt->x[i] *= xscale;
   }
   if ( yscale != 0. && rpt->ndim > 1 && rpt->y != NULL )
   {
      if ( verbose >= 2 )
         printf("Scaling y values by factor %g\n", yscale);
      for ( i=0; i<rpt->ny; i++ )
         rpt->y[i] *= yscale;
   }
   if ( zscale != 0. && rpt->z != NULL )
   {
      if ( verbose >= 2 )
         printf("Scaling z values by factor %g\n", zscale);
      size_t nz = rpt->nx;
      if ( rpt->ndim > 1 )
         nz *= rpt->ny;
      for ( i=0; i<nz; i++ )
         rpt->z[i] *= zscale;
   }

   /* Check if table has equidistant intervals and what the
      ranges are. If a later remapping of x, y, and/or z is
      applied, this check has to be redone. */
   rpol_check_equi_range(rpt);

   /* Now hook it to the end of the linked list of these tables. */
   rptl->rpt = rpt;
   rptl->next = (struct rpt_list *) calloc(1,sizeof(struct rpt_list));
   if ( rptl->next == NULL )
   {
      fprintf(stderr, "rpt_list allocation problem\n");
      return NULL;
   }

   if ( verbose >= 2 )
      rpol_info(rpt);
   else if ( verbose )
   {
      if ( rpt->ndim == 1 )
         printf("Table '%s' is 1-D with %zu supporting points",
            fname, rpt->nx);
      else
         printf("Table '%s' is 2-D with %zu * %zu supporting points",
            fname, rpt->nx, rpt->ny);
      if ( rpt->options != NULL )
         printf(" (with options '%s')", rpt->options);
      printf(".\n");
   }

   if ( verbose )
   {
      if ( (rpt->equidistant & 3) == 3 )
         printf("Table has equidistant supporting points in x and y.\n");
      else if ( (rpt->equidistant & 1) )
         printf("Table has equidistant supporting points in x.\n");
      else if ( (rpt->equidistant & 2) )
         printf("Table has equidistant supporting points in y.\n");
   }
   
   if ( rpt->ndim == 1 )
   {
      if ( rpt->scheme == 2 )
      {
         if ( rpt->nx < 3 )
            rpt->scheme = 1; /* Fall back to linear with too few support points */
      }
      else if ( rpt->scheme == 3 || rpt->scheme == 4 ) 
      {
         if ( rpt->nx < 3 )
            rpt->scheme = 1; /* Only linear possible */
         else if ( rpt->nx == 3 ) /* 2nd order should work */
            rpt->scheme = 2;
         else
         {
            rpt->csp = set_1d_cubic_params(rpt->x, rpt->z, rpt->nx, (rpt->scheme == 4));
            if ( rpt->csp == NULL )
               rpt->scheme = 1; /* Fall back to linear if setting up cubic spline fails */
         }
      }
   }

   return rpt;
}

/* ================================================================== */
/**
 *  A simplified way of setting up a 1-D rpol table for local use, without
 *  reading any files. The returned pointer is also not hooked into the
 *  global linked list, thus is supposed to be safe to be removed after use.
 */

struct rpol_table *simple_rpol1d_table(const char *label, double *x, double *y, int n, int clip)
{
   int i;
   struct rpol_table *rpt = NULL;
   if ( n < 2 || x == 0 || y == 0 )
   {
      fprintf(stderr, "Invalid attempt to book simple 1-D rpol table\n");
      return NULL;
   }
   
   rpt = (struct rpol_table *) calloc(1,sizeof(struct rpol_table));
   if ( rpt == NULL )
   {
      fprintf(stderr, "rpol_table allocation problem\n");
      return NULL;
   }

   rpt->ndim = 1;
   rpt->nx = (size_t) n;
   rpt->ny = 0;
   rpt->x = (double *) calloc(rpt->nx,sizeof(double));
   rpt->z = (double *) calloc(rpt->nx,sizeof(double));
   if ( rpt->x == NULL || rpt->y == NULL )
   {
      fprintf(stderr, "rpol_table allocation problem\n");
      return NULL;
   }
   for ( i=0; i<n; i++ )
   {
      rpt->x[i] = x[i];
      rpt->y[i] = y[i];
   }
   rpt->fname = strdup(label);
   rpt->scheme = 1; /* linear */
   rpt->clipping = clip;
   rpt->use_count = -1; /* Local use, OK to be removed */
   rpol_check_equi_range(rpt);

   return rpt;
}

/* ================================================================== */
/*
   Linear interpolation functions from an older program of K.B.
   A binary search algorithm is used for fast interpolation.
*/

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
 *	    coordinate in the given sorting (1 <= ipl <= n-1)
 *  @param  rpl Output: the fraction (x-v[ipl-1])/(v[ipl]-v[ipl-1])
 *	    with 0 <= rpl <= 1
*/      

static void interp ( double x, double *v, int n, int *ipl, double *rpl )
{
   int i, l, m, j, lm;

#ifdef DEBUG_TEST_ALL
   if ( v == NULL || n <= 2 )
   {
      fprintf(stderr,"Invalid parameters for interpolation.\n");
      *ipl = 1;
      *rpl = 0.;
      return;
   }
#endif

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

/* ==================== 1-D interpolation functions ========================= */

/* ----------------------------- rpol ------------------------------- */
/**
 *  @short Linear interpolation with binary search algorithm.
 *
 *  Linear interpolation between data point in sorted (i.e. monotonic
 *  ascending or descending) order. The resulting interpolated value
 *  is returned as a return value.
 *  This is the old-style function without any option for equidistant
 *  support points or clipping.
 *  Note that rpol(px,py,n,xp) is the same as rpol_linear(px,py,n,xp,0,0).
 *
 *  This function calls interp() to find out where to interpolate.
 *  
 *  @param   x  Input: Coordinates for data table
 *  @param   y  Input: Corresponding values for data table
 *  @param   n  Input: Number of data points
 *  @param   xp Input: Coordinate of requested value
 *
 *  @return  Interpolated value
 *
*/

double rpol ( double *x, double *y, int n, double xp )
{
   int ipl = 1;
   double rpl = 0.;

   interp ( xp, x, n, &ipl, &rpl );
   return y[ipl-1]*(1.-rpl) + y[ipl]*rpl;
}

/* ----------------------------- rpol_nearest ------------------------------- */

/**
 *  @short Nearest value (not actually interpolation) in 1-D with with either direct
 *         access for equidistant table or with binary search algorithm.
 *
 *  Take the nearest data point in sorted (i.e. monotonic
 *  ascending [no descending yet]) order. The selected value
 *  is returned as a return value. If the table is known to be provided
 *  at equidistant supporting points, direct access is preferred.
 *  Otherwise a binary search algorithm is used to find the proper interval.
 *
 *  This function calls interp() to find out where to interpolate.
 *  
 *  @param   x  Input: Coordinates for data table
 *  @param   y  Input: Corresponding values for data table
 *  @param   n  Input: Number of data points
 *  @param   xp Input: Coordinate of requested value
 *  @param   eq Input: If non-zero: table is at equidistant points.
 *  @param   clip Input: Zero: no clipping; extrapolate with left/right edge value outside range.
 *                       Non-zero: clip at edges; return 0. outside supported range.
 *
 *  @return  Nearest value
 *
*/

double rpol_nearest ( double *x, double *y, int n, double xp, int eq, int clip );

double rpol_nearest ( double *x, double *y, int n, double xp, int eq, int clip )
{
   int ipl = 1;
   double rpl = 0.;
   
#ifdef RPOL_DEBUG
   printf("   rpol_nearest(x, y, %d, %g, %d, %d)\n", n, xp, eq, clip);
#endif

   if ( n < 2 ) /* Invalid: zero or only one supporting point */
      return 0.;

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
      /* double dx = (x[n-1]-x[0]) / (n-1.); */
      double dxi = 1./(x[1] - x[0]);
      ipl = (int) ((xp-x[0])*dxi) + 1;
      if ( ipl > n-1 ) /* Could happen with rounding errors */
         ipl = n-1;
      rpl = (xp-x[ipl-1])*dxi;
   }
   else
      interp ( xp, x, n, &ipl, &rpl );

   if ( rpl < 0.5 )
      return y[ipl-1];
   else
      return y[ipl];
}

/* ----------------------------- rpol_linear ------------------------------- */

/**
 *  @short Linear interpolation in 1-D with with either direct
 *         access for equidistant table or with binary search algorithm.
 *
 *  Linear interpolation between data point in sorted (i.e. monotonic
 *  ascending or descending) order. The resulting interpolated value
 *  is returned as a return value. If the table is known to be provided
 *  at equidistant supporting points, direct access is preferred.
 *  Otherwise a binary search algorithm is used to find the proper interval.
 *
 *  This function calls interp() to find out where to interpolate.
 *  
 *  @param   x  Input: Coordinates for data table
 *  @param   y  Input: Corresponding values for data table
 *  @param   n  Input: Number of data points
 *  @param   xp Input: Coordinate of requested value
 *  @param   eq Input: If non-zero: table is at equidistant points.
 *  @param   clip Input: Zero: no clipping; extrapolate with left/right edge value outside range.
 *                       Non-zero: clip at edges; return 0. outside supported range.
 *
 *  @return  Interpolated value
 *
*/

double rpol_linear ( double *x, double *y, int n, double xp, int eq, int clip );

double rpol_linear ( double *x, double *y, int n, double xp, int eq, int clip )
{
   int ipl = 1;
   double rpl = 0.;
   
#ifdef RPOL_DEBUG
   printf("   rpol_linear(x, y, %d, %g, %d, %d)\n", n, xp, eq, clip);
#endif

   if ( n < 2 ) /* Invalid: zero or only one supporting point */
      return 0.;

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
      /* double dx = (x[n-1]-x[0]) / (n-1.); */
      double dxi = 1./(x[1] - x[0]);
      ipl = (int) ((xp-x[0])*dxi) + 1;
      if ( ipl > n-1 ) /* Could happen with rounding errors */
         ipl = n-1;
      rpl = (xp-x[ipl-1])*dxi;
   }
   else
      interp ( xp, x, n, &ipl, &rpl );

   return y[ipl-1]*(1.-rpl) + y[ipl]*rpl;
}

/* ----------------------------- rpol_2nd_order ------------------------------- */

/** Second/third order interpolation in 1-D with clipping option outside range */
/**
 *  @short Second to third order interpolation in 1-D with with either direct
 *         access for equidistant table or with binary search algorithm.
 *         Instead of third order Lagrange interpolation it uses left- and
 *         right-sided 2nd order interpolation and a weighted mean between
 *         the two variants, rendering it effectively third order except for
 *         the intervals next to borders where it degenerates to 2nd order.
 *
 *  Higher-order interpolation between data point in sorted (i.e. monotonic
 *  ascending or descending) order. The resulting interpolated value
 *  is returned as a return value. If the table is known to be provided
 *  at equidistant supporting points, direct access is preferred.
 *  Otherwise a binary search algorithm is used to find the proper interval.
 *  Since there is no initialization phase, this is actually slower than
 *  the cubic spline interpolation.
 *
 *  This function calls interp() to find out where to interpolate.
 *  
 *  @param   x  Input: Coordinates for data table
 *  @param   y  Input: Corresponding values for data table
 *  @param   n  Input: Number of data points
 *  @param   xp Input: Coordinate of requested value
 *  @param   eq Input: If non-zero: table is at equidistant points.
 *  @param   clip Input: Zero: no clipping; extrapolate with left/right edge value outside range.
 *                       Non-zero: clip at edges; return 0. outside supported range.
 *
 *  @return  Interpolated value
 *
*/

double rpol_2nd_order ( double *x, double *y, int n, double xp, int eq, int clip );

double rpol_2nd_order ( double *x, double *y, int n, double xp, int eq, int clip )
{
   int ipl = 1;
   double rpl = 0;
#if 0
   double xds, fds = 0., fdl = 0., fdr = 0., fd2 = 0., xdl, xdr;
#endif

#ifdef RPOL_DEBUG
   printf("   rpol_2nd_order(x, y, %d, %g, %d, %d)\n", n, xp, eq, clip);
#endif

   if ( n < 3 ) /* Invalid: Less than three supporting points does not even give 2nd order */
      return rpol_linear(x,y,n,xp,eq,clip);

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
      rpl = (xp-x[ipl-1])*dxi;
   }
   else
      interp ( xp, x, n, &ipl, &rpl );
   
   /* Keep in mind that we should have 1 <= ipl <= n-1 */

   if ( ipl > n-1 ) /* Can happen on upper boundary, perhaps rounding errors */
      return y[n-1];
   else if ( ipl < 1 ) /* In case of rounding errors slipping through */
      return y[0];

#if 0
   /* Older interpolation code, behaving a bit odd and no longer used ... */
   fds = y[ipl] - y[ipl-1];
   xds = x[ipl] - x[ipl-1];
   if ( ipl < 2 ) /* First interval */
   {
      fdl = y[1] - y[0];
      xdl = 0.5*(x[1] + x[0]);
   }
   else /* We have another interval on the left side of this one */
   {
      fdl = (y[ipl] - y[ipl-2]) * xds / (x[ipl] - x[ipl-2]);
      xdl = 0.5*(x[ipl] + x[ipl-2]);
   }
   if ( ipl < n-1 ) /* We have another interval to the right side of this one */
   {
      fdr = (y[ipl+1] - y[ipl-1]) * xds / (x[ipl+1] - x[ipl-1]);
      xdr = 0.5*(x[ipl+1] + x[ipl-1]);
   }
   else /* Last interval */
   {
      fdr = y[ipl] - y[ipl-1];
      xdr = x[ipl] + x[ipl-1];
   }
   if ( xdr >= xdl )
      fd2 = (fdr - fdl) * xds / (xdr-xdl);
    else
       fd2 = 0.;

   return y[ipl-1] + (fds-0.5*fd2)*rpl + 0.5*fd2*rpl*rpl;
#endif

   double x0l, x1l, x2l, x0r, x1r, x2r;
   double y0l, y1l, y2l, y0r, y1r, y2r;

   if ( ipl < 2 ) /* First interval */
   {
      /* We make left-sided = right-sided */
      x0l = x[ipl-1];
      y0l = y[ipl-1];
      x1l = x[ipl];
      y1l = y[ipl];
      x2l = x[ipl+1];
      y2l = y[ipl+1];
   }
   else /* We have another interval on the left side of this one */
   {
      x0l = x[ipl-2];
      y0l = y[ipl-2];
      x1l = x[ipl-1];
      y1l = y[ipl-1];
      x2l = x[ipl];
      y2l = y[ipl];
   }
   if ( ipl < n-1 ) /* We have another interval to the right side of this one */
   {
      x0r = x[ipl-1];
      y0r = y[ipl-1];
      x1r = x[ipl];
      y1r = y[ipl];
      x2r = x[ipl+1];
      y2r = y[ipl+1];
   }
   else /* Last interval */
   {
      x0r = x[ipl-2];
      y0r = y[ipl-2];
      x1r = x[ipl-1];
      y1r = y[ipl-1];
      x2r = x[ipl];
      y2r = y[ipl];
   }
   
   /* Langrange's interpolation basis functions (left side) */
   double L0l = ((xp-x1l)*(xp-x2l)) / ((x0l-x1l)*(x0l-x2l));
   double L1l = ((xp-x0l)*(xp-x2l)) / ((x1l-x0l)*(x1l-x2l));
   double L2l = ((xp-x0l)*(xp-x1l)) / ((x2l-x0l)*(x2l-x1l));
   /* Langrange's interpolation basis functions (right side) */
   double L0r = ((xp-x1r)*(xp-x2r)) / ((x0r-x1r)*(x0r-x2r));
   double L1r = ((xp-x0r)*(xp-x2r)) / ((x1r-x0r)*(x1r-x2r));
   double L2r = ((xp-x0r)*(xp-x1r)) / ((x2r-x0r)*(x2r-x1r));
   
   /* Interpolation results for left and right side */
   double P2l = y0l*L0l + y1l*L1l + y2l*L2l;
   double P2r = y0r*L0r + y1r*L1r + y2r*L2r;
   
   /* Simply averaging them would be actually 2nd order ... */
   // return 0.5*(P2l+P2r);
   /* ... but since we know where we are in the interval we can do a weighted average,
      making this interpolation effectively a 3rd order interpolation, except in the 
      intervals next to the boundaries where it degenerates to 2nd order. */
   return (1.-rpl)*P2l + rpl*P2r;
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

double rpol_cspline ( double *x, double *y, const CsplinePar *csp, int n, double xp, int eq, int clip );

double rpol_cspline ( double *x, double *y, const CsplinePar *csp, int n, double xp, int eq, int clip )
{
   int ipl = 1;
   double rpl = 0.;
   double r = 0.;

#ifdef RPOL_DEBUG
   printf("   rpol_cspline(x, csp, %d, %g, %d, %d)\n", n, xp, eq, clip);
#endif

   if ( n < 4 || csp == NULL ) /* Invalid: not enough points for cubic splines. */
      return rpol_linear(x,y,n,xp,eq,clip); /* Setup should have downgraded the scheme beforehand. */

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

/* ==================== 2-D interpolation functions ========================= */

/* The only implemented interpolation scheme for 2-D tables so far is linear */

/** Linear interpolation in 2-D */

double rpol_2d_linear (double *x, double *y, double *z, int nx, int ny, double xp, double yp, int eq, int clip);

double rpol_2d_linear (double *x, double *y, double *z, int nx, int ny, double xp, double yp, int eq, int clip)
{
   int ipl = 1, jpl = 1;
   double rpl = 0., spl = 0.;

#ifdef RPOL_DEBUG
   printf("   rpol_2d_linear(x, y, z, %d, %d, %g, %g, %d, %d)\n", nx, ny, xp, yp, eq, clip);
#endif
   
   if ( nx < 2 || ny < 2 ) /* Invalid: zero or only one supporting point in either x or y */
      return 0.;

   if ( x[1] <= x[0] || y[1] <= y[0] ) /* Invalid: supporting points in decreasing order */
      return 0.;

   if ( xp < x[0] ) /* Below first supporting point in x */
   {
      if ( clip )
         return 0.;
      ipl = 1;
      rpl = 0.;
#ifdef RPOL_DEBUG
      printf("      x below: ipl = %d, rpl=%g\n", ipl, rpl);
#endif
   }
   else if ( xp > x[nx-1] ) /* Above last supporting point in x */
   {
      if ( clip )
         return 0.;
      ipl = nx-1;
      rpl = 1.0;
#ifdef RPOL_DEBUG
      printf("      x above: ipl = %d, rpl=%g\n", ipl, rpl);
#endif
   }
   else if ( (eq&1) ) /* Equidistant spacing in x */
   {
      double dxi = 1./(x[1] - x[0]);
      ipl = (int) ((xp-x[0])*dxi) + 1;
      if ( ipl > nx-1 ) /* Could happen with rounding errors */
         ipl = nx-1;
      rpl = (xp-x[ipl-1])*dxi;
#ifdef RPOL_DEBUG
      printf("      equidistant x: ipl = %d, rpl=%g\n", ipl, rpl);
#endif
   }
   else
   {
      interp ( xp, x, nx, &ipl, &rpl );
#ifdef RPOL_DEBUG
      printf("      non-equidistant x: ipl = %d, rpl=%g\n", ipl, rpl);
#endif
   }

   if ( yp < y[0] ) /* Below first supporting point in y */
   {
      if ( clip )
         return 0.;
      jpl = 1;
      spl = 0.;
#ifdef RPOL_DEBUG
      printf("      y below: jpl = %d, spl=%g\n", jpl, spl);
#endif
   }
   else if ( yp > y[ny-1] ) /* Above last supporting point in y */
   {
      if ( clip )
         return 0.;
      jpl = ny-1;
      spl = 1.0;
#ifdef RPOL_DEBUG
      printf("      y above: jpl = %d, spl=%g\n", jpl, spl);
#endif
   }
   else if ( (eq&2) ) /* Equidistant spacing in y */
   {
      double dyi = 1./(y[1] - y[0]);
      jpl = (int) ((yp-y[0])*dyi) + 1;
      if ( jpl > ny-1 ) /* Could happen with rounding errors */
         jpl = ny-1;
      spl = (yp-y[jpl-1])*dyi;
#ifdef RPOL_DEBUG
      printf("      equidistant y: jpl = %d, spl=%g\n", jpl, spl);
#endif
   }
   else
   {
      interp ( yp, y, ny, &jpl, &spl );
#ifdef RPOL_DEBUG
      printf("      non-equidistant y: jpl = %d, spl=%g\n", jpl, spl);
#endif
   }

#ifdef RPOL_DEBUG
   printf("           z_i,j =   %g  %g\n"
          "                     %g  %g\n", 
            z[(ipl-1)*ny+(jpl-1)], z[ipl*ny+(jpl-1)],
            z[(ipl-1)*ny+jpl], z[ipl*ny+jpl]);
#endif

   return ( z[(ipl-1)*ny+(jpl-1)]*(1.-rpl) + z[ipl*ny+(jpl-1)]*rpl ) * (1.-spl)
        + ( z[(ipl-1)*ny+jpl]*(1.-rpl) + z[ipl*ny+jpl]*rpl ) * spl;
}

/* =============== High-level interpolation functions ================ */

/* ----------------------------- rpolate_1d ------------------------------- */
/**
 *  @short High-level interpolation function (user code only has to keep 
 *     a pointer to the allocated object) limited to 1-D table interpolation.
 *
 *  @param rpt Pointer to interpolation table structure, previously set up
 *      with read_rpol_table. Keep in mind that it gets only allocated once
 *      and, if you want to free it, you should not free it more than once.
 *      In the case of this function, it should represent a 1-D table.
 *  @param x The x coordinate (abscissa) value at which the 1-D table is to be interpolated.
 *  @param scheme Interpolation scheme: 0 ... 4 for 
 *      a specific user-defined scheme or -1 (or other values outside of [0:4] range)
 *      for the scheme determined when the table was read and allocated.
 *      For 2-D tables for which an upper envelope in x projection was requested
 *      a -1 scheme will interpolate in this upper envelope and -2 the lower envelope.
 */

double rpolate_1d(struct rpol_table *rpt, double x, int scheme);

double rpolate_1d(struct rpol_table *rpt, double x, int scheme)
{
   double z = 0.;

#ifdef RPOL_DEBUG
   printf("   rpolate_1d(%p, %g, %d)\n", rpt, x, scheme);
#endif
   
   if ( rpt == NULL )
      return 0.;

   if ( rpol_verbosity > 5 )
   {
      fprintf(stderr,"1-D interpolation in table '%s' at x=%g in scheme %d\n",
         (rpt==NULL)?"NULL":rpt->fname, x, scheme);
   }
   if ( rpt->xlog )
   {
      if ( x > 0. )
         x = log(x);
      else
         return 0.;
   }

   if ( scheme < 0 ) /* special cases */
   {
      /* Special case for requesting the zxmax upper envelope projection from a 2-D table */
      if ( scheme == -1 && rpt->ndim >= 2 && rpt->zxmax != NULL )
      {
         if ( rpol_verbosity > 3 )
            fprintf(stderr,"Maximum z (along y) value interpolation from 2-D table.\n");
         if ( rpt->zlog )
            return exp(rpol_linear(rpt->x, rpt->zxmax, rpt->nx, x, (rpt->equidistant & 0x01), rpt->clipping));
         else
            return rpol_linear(rpt->x, rpt->zxmax, rpt->nx, x, (rpt->equidistant & 0x01), rpt->clipping);
      }
      /* Special case for requesting the zxmin lower envelope projection from a 2-D table */
      else if ( scheme == -2 && rpt->ndim >= 2 && rpt->zxmin != NULL )
      {
         if ( rpol_verbosity > 3 )
            fprintf(stderr,"Minimum z (along y) value interpolation from 2-D table.\n");
         if ( rpt->zlog )
            return exp(rpol_linear(rpt->x, rpt->zxmin, rpt->nx, x, (rpt->equidistant & 0x01), rpt->clipping));
         else
            return rpol_linear(rpt->x, rpt->zxmin, rpt->nx, x, (rpt->equidistant & 0x01), rpt->clipping);
      }
      else
         fprintf(stderr,"Unexpected scheme %d interpolation in ndim=%d table %s (zxmin %s NULL, zxmax %s NULL).\n",
            scheme, rpt->ndim, rpt->fname, (rpt->zxmin == NULL)?"is":"is not", (rpt->zxmax == NULL)?"is":"is not");
   }

   if ( rpt->ndim != 1 )
   {
      fprintf(stderr,"Requested 1-D interpolation (scheme %d) from non-1-D table %s.\n", scheme, rpt->fname);
      return 0.;
   }

   if ( scheme < 0 || scheme > 4 )
   {
      if ( rpol_verbosity > 3 )
         fprintf(stderr, "Overriding requested interpolation scheme %d with table-specific scheme %d.\n",
            scheme, rpt->scheme);
      scheme = rpt->scheme;
   }

   switch ( scheme )
   {
      case 0: /* zero-order, nearest value */
         z = rpol_nearest(rpt->x, rpt->z, rpt->nx, x, (rpt->equidistant & 0x01), rpt->clipping);
         break;
      case 1: /* first order, linear interpolation (default) */
         z = rpol_linear(rpt->x, rpt->z, rpt->nx, x, (rpt->equidistant & 0x01), rpt->clipping);
         break;
      case 2: /* 2nd order (is not a 2nd order spline) */
         z = rpol_2nd_order(rpt->x, rpt->z, rpt->nx, x, (rpt->equidistant & 0x01), rpt->clipping);
         break;
      case 3: /* Natural cubic spline */
      case 4: /* Clamped cubic spline */
         z = rpol_cspline(rpt->x, rpt->z, rpt->csp, rpt->nx, x, (rpt->equidistant & 0x01), rpt->clipping);
         break;
      default:
         return 0.;
   }
   
   if ( rpt->zlog )
   {
      if ( rpol_verbosity > 3 )
         fprintf(stderr,
            "Converting 1-D interpolation result from internal log units (%g) to linear (%g).\n",
            z, exp(z));
      return exp(z);
   }
   else
      return z;
}

/* --------------------------- rpolate_1d_lin ----------------------------- */
/**
 *  @short High-level interpolation function (user code only has to keep 
 *     a pointer to the allocated object) limited to 1-D table interpolation,
 *     with linear interpolation scheme hard-wired (independent of any
 *     '#@RPOL@' header line).
 *     This is the most direct equivalence to the older rpol() function.
 *
 *  @param rpt Pointer to interpolation table structure, previously set up
 *      with read_rpol_table. Keep in mind that it gets only allocated once
 *      and, if you want to free it, you should not free it more than once.
 *      In the case of this function, it should represent a 1-D table.
 *  @param x The x coordinate (abscissa) value at which the 1-D table is to be interpolated.
 */

double rpolate_1d_lin(struct rpol_table *rpt, double x);

double rpolate_1d_lin(struct rpol_table *rpt, double x)
{
   return rpolate_1d(rpt,x,1);
}

/* ----------------------------- rpolate_2d ------------------------------- */
/**
 *  @short High-level interpolation function (user code only has to keep 
 *     a pointer to the allocated object) limited to 2-D table interpolation.
 *     Fall-back to 1-D only after issueing a warning.
 *
 *  @param rpt Pointer to interpolation table structure, previously set up
 *      with read_rpol_table. Keep in mind that it gets only allocated once
 *      and, if you want to free it, you should not free it more than once.
 *      In the case of this function, it should represent a 2-D table.
 *  @param x The x coordinate value at which the 2-D table is to be interpolated.
 *  @param y The y coordinate value at which the 2-D table is to be interpolated (ignored for 1-D).
 *  @param scheme Interpolation scheme: 0 ... 4 (not all implemented) for 
 *      a specific user-defined scheme or -1 (or other values outside of [0:4] range)
 *      for the scheme determined when the table was read and allocated.
 */


double rpolate_2d(struct rpol_table *rpt, double x, double y, int scheme)
{
   double z = 0.;

#ifdef RPOL_DEBUG
   printf("   rpolate_2d(%p, %g, %g, %d)\n", rpt, x, y, scheme);
#endif
   
   if ( rpt == NULL )
      return 0.;

   if ( rpt->ndim != 2 && rpt->ndim != 3 )
   {
      fprintf(stderr,"Requested 2-D interpolation from non-2-D table %s (fall-back to 1-D).\n", rpt->fname);
      return rpolate_1d(rpt,x,scheme);
   }
   if ( rpol_verbosity > 5 )
   {
      fprintf(stderr,"2-D interpolation in table '%s' at x=%g, y=%g in scheme %d\n",
         (rpt==NULL)?"NULL":rpt->fname, x, y, scheme);
   }
   if ( rpt->logs )
   {
      if ( rpt->xlog )
      {
         if ( x > 0. )
         {
            if ( rpol_verbosity > 3 )
               fprintf(stderr,"Applying log to x value for 2-D interpolation.\n");
            x = log(x);
         }
         else
            return 0.;
      }
      if ( rpt->ylog && rpt->ndim >= 2 )
      {
         if ( y > 0. )
         {
            if ( rpol_verbosity > 3 )
               fprintf(stderr,"Applying log to y value for 2-D interpolation.\n");
            y = log(y);
         }
         else
            return 0.;
      }
   }

   if ( scheme < 0 || scheme > 4 )
      scheme = rpt->scheme;

   switch ( scheme ) /* In 2-D tables it is always linear interpolation. */
   {
      case 0:
      case 1:
      case 2: 
      case 3:
      case 4:
         z = rpol_2d_linear(rpt->x, rpt->y, rpt->z, rpt->nx, rpt->ny, x, y, rpt->equidistant, rpt->clipping);
         break;
      default:
         if ( rpol_verbosity > 0 )
            fprintf(stderr,"Interpolation table '%s' is not usable.\n", rpt->fname);
         return 0.;
   }
   
   if ( rpt->zlog )
   {
      if ( rpol_verbosity > 3 )
         fprintf(stderr,
            "Converting 2-D interpolation result from internal log units (%g) to linear (%g).\n",
            z, exp(z));
      return exp(z);
   }
   else
   {
      return z;
   }
}

/* ------------------------------- rpolate -------------------------------- */
/**
 *  @short High-level interpolation function (user code only has to keep 
 *     a pointer to the allocated object) generic for 1-D and 2-D tables.
 *
 *  @param rpt Pointer to interpolation table structure, previously set up
 *      with read_rpol_table. Keep in mind that it gets only allocated once
 *      and, if you want to free it, you should not free it more than once.
 *      In the case of this function, it can represent either a 1-D or 2-D table.
 *  @param x The x coordinate value at which the 1-D or 2-D table is to be interpolated.
 *  @param y The y coordinate value at which a 2-D table is to be interpolated (ignored for 1-D).
 *      If non-zero for 1-D tables, a warning may be issued.
 *  @param scheme Interpolation scheme: 0 ... 4 (not all implemented) for 
 *      a specific user-defined scheme or -1 (or other values outside of [0:4] range)
 *      for the scheme determined when the table was read and allocated.
 */


double rpolate(struct rpol_table *rpt, double x, double y, int scheme)
{
#ifdef RPOL_DEBUG
   printf("   rpolate_2d(%p, %g, %g, %d)\n", rpt, x, y, scheme);
#endif
   
   if ( rpol_verbosity > 5 )
   {
      fprintf(stderr,"Interpolation in table '%s' at x=%g, y=%g in scheme %d\n",
         (rpt==NULL)?"NULL":rpt->fname, x, y, scheme);
   }
   if ( rpt == NULL )
      return 0.;
   if ( rpt->ndim == 1 && y == 0. )
      return rpolate_1d(rpt,x,scheme);
   else
      return rpolate_2d(rpt,x,y,scheme);
}

#ifdef TEST

/* ==================== Test code ========================= */

int main (int argc, char **argv)
{
   const char *fn = "-", *marker = NULL, *options = NULL;
   double x, y, z, z2, zn;
   int nd = -1;
   int clip = 0;
   char line[100];
   struct rpol_table *rpt = NULL;
   int verbose = 0;
   
   while ( argc > 1 && argv[1][0] == '-' && argv[1][1] != '\0' )
   {
      if ( strcmp(argv[1],"-c") == 0 )
      {
         clip = 1;
         argc--;
         argv++;
      }
      else if ( strcmp(argv[1],"-v") == 0 )
      {
         verbose = 1;
         argc--;
         argv++;
      }
      else if ( argc > 2 && strcmp(argv[1],"-n") == 0 )
      {
         nd = atoi(argv[2]);
         argc-=2;
         argv+=2;
      }
   }
   if ( argc > 1 )
      fn = argv[1];
   if ( argc > 2 )
      nd = atoi(argv[2]);
   if ( argc > 3 )
      marker = argv[3];
   if ( argc > 4 )
      options = argv[4];

   rpt = read_rpol_table(fn,nd,marker,options);
   /* Repeating that will be noted with high enough RPOL_VERBOSE setting: */
   if ( verbose )
   {
      printf("\nTrying to reload table twice now to see what happens:\n");
      rpt = read_rpol_table(fn,nd,marker,options);
      rpt = read_rpol_table(fn,nd,marker,options);
   }

   if ( rpt == NULL )
   {
      fprintf(stderr, "No interpolation table.\n");
      exit(1);
   }
   
   if ( clip )
      rpt->clipping = 1;

   if ( verbose )
   {
      printf("\nThat is what we got now:\n");
      rpol_info(rpt);
   }

   while ( fgets(line,sizeof(line)-1,stdin) != NULL )
   {
      if ( line[0] == '#' || line[0] == '\n' )
         continue;
      if ( rpt->ndim == 1 )
      {
         if ( sscanf(line,"%lf",&x) == 1 )
         {
            if ( verbose )
            {
               // z = rpol_lookup_1d(rpt,x);
               z = rpol_linear(rpt->x,rpt->z,rpt->nx,x,rpt->equidistant,clip);
               zn = rpol_nearest(rpt->x,rpt->z,rpt->nx,x,rpt->equidistant,clip);
               z2 = rpol_2nd_order(rpt->x,rpt->z,rpt->nx,x,rpt->equidistant,clip);
               // printf("x = %lf  ->  z = %g (%g)\n", x, z, z2);
               printf("%lf  %g  (%g %g %g)\n", x, z, zn, z, z2);
               z = rpolate_1d(rpt,x,1);
               zn = rpolate_1d(rpt,x,0);
               z2 = rpolate_1d(rpt,x,2);
               printf("%lf  %g  (%g %g %g)\n", x, rpolate (rpt,x,0.,-1), zn, z, z2);
            }
            else
               printf("%g  %g\n", x, rpolate (rpt,x,0.,-1));
         }
         else
            printf("(Expected 'x' value.)\n");
      }
      else if ( rpt->ndim >= 2 )
      {
         if ( sscanf(line,"%lf %lf",&x,&y) == 2 )
         {
            if ( verbose )
            {
               // z = rpol_lookup_2d(rpt,x,y);
               z = rpol_2d_linear(rpt->x, rpt->y, rpt->z, rpt->nx, rpt->ny, x, y, rpt->equidistant,clip);
               // printf("x = %lf, y = %lf  ->  z = %g\n", x, y, z);
               printf("%lf  %lf  %g\n", x, y, z);
               printf("%lf  %lf  %g\n", x, y, rpolate (rpt,x,y,-1));
            }
            else
               printf("%g  %g  %g\n", x, y, rpolate (rpt,x,y,-1));
         }
         else if ( sscanf(line,"%lf, %lf",&x,&y) == 2 )
            printf("%g  %g  %g\n", x, y, rpolate (rpt,x,y,-1));
         else
            printf("(Expected 'x y' values.)\n");
      }
      else
      {
         fprintf(stderr,"Invalid table:\n");
         rpol_info(rpt);
      }
   }
   
   return 0;
}

#endif
