#ifndef HILBERT_H
#define HILBERT_T

# include <stdio.h>
# include <stdlib.h>
# include <time.h>
void d2xy ( int n, int d, int *x, int *y );
int i4_power ( int i, int j );
void rot ( int n, int *x, int *y, int rx, int ry );
void timestamp ( );
int xy2d ( int n, int x, int y );


/******************************************************************************/

void d2xy ( int m, int d, int *x, int *y )

/******************************************************************************/
/*
  Purpose:

    D2XY converts a 1D Hilbert coordinate to a 2D Cartesian coordinate.

  Modified:

    05 December 2015

  Parameters:

    Input, int M, the index of the Hilbert curve.
    The number of cells is N=2^M.
    0 < M.

    Input, int D, the Hilbert coordinate of the cell.
    0 <= D < N * N.

    Output, int *X, *Y, the Cartesian coordinates of the cell.
    0 <= *X, *Y < N.
*/
{
  int n;
  int rx;
  int ry;
  int s;
  int t = d;

  n = i4_power ( 2, m );

  *x = 0;
  *y = 0;
  for ( s = 1; s < n; s = s * 2 )
  {
    rx = 1 & ( t / 2 );
    ry = 1 & ( t ^ rx );
    rot ( s, x, y, rx, ry );
    *x = *x + s * rx;
    *y = *y + s * ry;
    t = t / 4;
  }
  return;
}
/******************************************************************************/

int i4_power ( int i, int j )

/******************************************************************************/
/*
  Purpose:

    I4_POWER returns the value of I^J.

  Licensing:

    This code is distributed under the GNU LGPL license.

  Modified:

    23 October 2007

  Author:

    John Burkardt

  Parameters:

    Input, int I, J, the base and the power.  J should be nonnegative.

    Output, int I4_POWER, the value of I^J.
*/
{
  int k;
  int value;

  if ( j < 0 )
  {
    if ( i == 1 )
    {
      value = 1;
    }
    else if ( i == 0 )
    {
      fprintf ( stderr, "\n" );
      fprintf ( stderr, "I4_POWER - Fatal error!\n" );
      fprintf ( stderr, "  I^J requested, with I = 0 and J negative.\n" );
      exit ( 1 );
    }
    else
    {
      value = 0;
    }
  }
  else if ( j == 0 )
  {
    if ( i == 0 )
    {
      fprintf ( stderr, "\n" );
      fprintf ( stderr, "I4_POWER - Fatal error!\n" );
      fprintf ( stderr, "  I^J requested, with I = 0 and J = 0.\n" );
      exit ( 1 );
    }
    else
    {
      value = 1;
    }
  }
  else if ( j == 1 )
  {
    value = i;
  }
  else
  {
    value = 1;
    for ( k = 1; k <= j; k++ )
    {
      value = value * i;
    }
  }
  return value;
}
/******************************************************************************/

void rot ( int n, int *x, int *y, int rx, int ry ) 

/******************************************************************************/
/*
  Purpose:

    ROT rotates and flips a quadrant appropriately.

  Modified:

    05 December 2015

  Parameters:

    Input, int N, the length of a side of the square.  N must be a power of 2.

    Input/output, int *X, *Y, the old and the new coordinates.

    Input, int RX, RY, ???
*/
{
  int t;

  if ( ry == 0 )
  {
/*
  Reflect.
*/
    if ( rx == 1 )
    {
      *x = n - 1 - *x;
      *y = n - 1 - *y;
    }
/*
  Flip.
*/
     t = *x;
    *x = *y;
    *y =  t;
  }
  return;
}
/******************************************************************************/

void timestamp ( )

/******************************************************************************/
/*
  Purpose:

    TIMESTAMP prints the current YMDHMS date as a time stamp.

  Example:

    17 June 2014 09:45:54 AM

  Licensing:

    This code is distributed under the GNU LGPL license. 

  Modified:

    17 June 2014

  Author:

    John Burkardt

  Parameters:

    None
*/
{
# define TIME_SIZE 40

  static char time_buffer[TIME_SIZE];
  const struct tm *tm;
  time_t now;

  now = time ( NULL );
  tm = localtime ( &now );

  strftime ( time_buffer, TIME_SIZE, "%d %B %Y %I:%M:%S %p", tm );

  printf ( "%s\n", time_buffer );

  return;
# undef TIME_SIZE
}
/******************************************************************************/

int xy2d ( int m, int x, int y )

/******************************************************************************/
/*
  Purpose:

    XY2D converts a 2D Cartesian coordinate to a 1D Hilbert coordinate.

  Discussion:

    It is assumed that a square has been divided into an NxN array of cells,
    where N is a power of 2.

    Cell (0,0) is in the lower left corner, and (N-1,N-1) in the upper 
    right corner.

  Modified:

    05 December 2015

  Parameters:

    Input, int M, the index of the Hilbert curve.
    The number of cells is N=2^M.
    0 < M.

    Input, int X, Y, the Cartesian coordinates of a cell.
    0 <= X, Y < N.

    Output, int XY2D, the Hilbert coordinate of the cell.
    0 <= D < N * N.
*/
{
  int d = 0;
  int n;
  int rx;
  int ry;
  int s;

  n = i4_power ( 2, m );

  for ( s = n / 2; s > 0; s = s / 2 )
  {
    rx = ( x & s ) > 0;
    ry = ( y & s ) > 0;
    d = d + s * s * ( ( 3 * rx ) ^ ry );
    rot ( s, &x, &y, rx, ry );
  }
  return d;
}

#endif
