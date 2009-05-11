/*
 * Copyright 2009 Ossi Saresoja
 *
 * This file is part of Kunquat.
 *
 * Kunquat is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Kunquat is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Kunquat.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <math.h>
#include <stdarg.h>
#include <string.h>

#define PI 3.14159265358979323846
#define MAX(x,y) ((x)>(y) ? (x) : (y))
#define MIN(x,y) ((x)<(y) ? (x) : (y))

double sinc(double x)
{
  return x == 0.0 ? 1.0 : sin(x)/x;
}

double powi(double x, int n)
{
  double y = 1.0;

  while(n != 0)
  {
    if(n & 1)
      y *= x;
    n >>= 1;
    x *= x;
  }
  return y;
}

int binom(int n, int k)
{
  return (k==0 || k==n) ? 1 : binom(n-1,k-1) + binom(n-1,k);
}

double poly(double x, int n, ...)
{
  va_list k;
  int i;
  double y;

  va_start(k, n);

  y = va_arg(k, double);
  for (i=0;i<n;++i)
    y = y*x + va_arg(k, double);
  va_end(k);
  return y;
}

#define DPROD(histbuff, sourcebuff, coeffs, n, i, acc, j, k, oper) { \
for((j)=0,(k)=(i);(k)<(n);++(j),++(k))                               \
  (acc) oper (histbuff)[(k)]*(coeffs)[(j)];                          \
                                                                     \
for((k)-=(n);(j)<(n);++(j),++(k))                                    \
  (acc) oper (sourcebuff)[(k)]*(coeffs)[(j)];                        \
}

#define BUFFER(histbuff, sourcebuff, n, amount) {                                \
if((amount)<(n)){                                                                \
  memmove(&(histbuff)[0], &(histbuff)[(amount)], ((n)-(amount))*sizeof(double)); \
  memcpy(&(histbuff)[(n)-(amount)], &(sourcebuff)[0], (amount)*sizeof(double));  \
}                                                                                \
 else                                                                            \
  memcpy(&(histbuff)[0], &(sourcebuff)[(amount)-(n)], (n)*sizeof(double));       \
}

void simple_lowpass_fir_create(int n, double f, double coeffs[])
{
  int i;
  for(i=0;i<=n;++i)
    coeffs[i] = 2*f*sinc(PI*f*(2*i-n));
}

#define C1 1.41421356237309504880 //sqrt(2)
#define C2 2.61312592975275305571 //sqrt(4+2*sqrt(2)) or equivalenty sqrt(2+sqrt(2))+sqrt(2-sqrt(2))
#define C3 2.23606797749978969641 //sqrt(5)

void bilinear_butterworth_lowpass_iir__create(int n, double f, double coeffsa[], double coeffsb[])
{
  int i;
  double a0=1.0,fna0;

  f = 2*tan(PI*f);

  switch(n)
  {
  case 1:
    coeffsa[0] = poly(f, 1, 1.0, -2.0);
    a0         = poly(f, 1, 1.0,  2.0);
    break;
  case 2:
    coeffsa[0] = poly(f, 2, 1.0, -2*C1,  4.0);
    coeffsa[1] = poly(f, 2, 2.0,   0.0, -8.0);
    a0         = poly(f, 2, 1.0,  2*C1,  4.0);
    break;
  case 3:
    coeffsa[0] = poly(f, 3, 1.0, -4.0,  8.0,  -8.0);
    coeffsa[1] = poly(f, 3, 3.0, -4.0, -8.0,  24.0);
    coeffsa[2] = poly(f, 3, 3.0,  4.0, -8.0, -24.0);
    a0         = poly(f, 3, 1.0,  4.0,  8.0,   8.0);
    break;
  case 4:
    coeffsa[0] = poly(f, 4, 1.0, -2*C2,   8.0+4*C1,  -8*C2,  16.0);
    coeffsa[1] = poly(f, 4, 4.0, -4*C2,        0.0,  16*C2, -64.0);
    coeffsa[2] = poly(f, 4, 6.0,   0.0, -16.0-8*C1,    0.0,  96.0);
    coeffsa[3] = poly(f, 4, 4.0,  4*C2,        0.0, -16*C2, -64.0);
    a0         = poly(f, 4, 1.0,  2*C2,   8.0+4*C1,   8*C2,  16.0);
    break;
  case 5:
    coeffsa[0] = poly(f, 5,  1.0, -2.0-2*C3,  12.0+4*C3, -24.0 -8*C3,  16.0+16*C3,  -32.0);
    coeffsa[1] = poly(f, 5,  5.0, -6.0-6*C3,  12.0+4*C3,  24.0 +8*C3, -48.0-48*C3,  160.0);
    coeffsa[2] = poly(f, 5, 10.0, -4.0-4*C3, -24.0-8*C3,  48.0+16*C3,  32.0+32*C3, -320.0);
    coeffsa[3] = poly(f, 5, 10.0,  4.0+4*C3, -24.0-8*C3, -48.0-16*C3,  32.0+32*C3,  320.0);
    coeffsa[4] = poly(f, 5,  5.0,  6.0+6*C3,  12.0+4*C3, -24.0 -8*C3, -48.0-48*C3, -160.0);
    a0         = poly(f, 5,  1.0,  2.0+2*C3,  12.0+4*C3,  24.0 +8*C3,  16.0+16*C3,   32.0);
  }

  for(i=0;i<n;++i)
    coeffsa[i] /= a0;

  fna0 = powi(f,n)/a0;

  for(i=0;i<=n;++i)
    coeffsb[i] = binom(n,i)*fna0;
}

void fir_filter(int n, double coeffs[], frame_t histbuff[], int amount, frame_t inbuff[], frame_t outbuff[])
{
  int i,j,k;
  double temp;

  for(i=0;i<amount;++i)
  {
    temp = inbuff[i]*coeffs[n];

    DPROD(histbuff, inbuff, coeffs, n, i, temp, j, k, +=);

    outbuff[i] = temp;
  }

  BUFFER(histbuff, inbuff, n, amount);
}

void iir_filter_df1(int na, double coeffsa[], frame_t histbuffa[], int nb, double coeffsb[], frame_t histbuffb[], int amount, frame_t inbuff[], frame_t outbuff[])
{
  int i,j,k;
  double temp;

  for(i=0;i<amount;++i)
  {
    temp = inbuff[i]*coeffsb[nb];

    DPROD(histbuffa, outbuff, coeffsa, na, i, temp, j, k, -=);

    DPROD(histbuffb,  inbuff, coeffsb, nb, i, temp, j, k, +=);

    outbuff[i] = temp;
  }

  BUFFER(histbuffa, outbuff, na, amount);

  BUFFER(histbuffb,  inbuff, nb, amount);
}


void iir_filter_df2(int na, double coeffsa[], int nb, double coeffsb[], frame_t histbuff[], int amount, frame_t inbuff[], frame_t outbuff[])
{
  int i,j,k;
  double temp;

  for(i=0;i<amount;++i)
  {
    temp = inbuff[i];

    DPROD(histbuff, inbuff, coeffsa, na, i, temp, j, k, -=);

    inbuff[i] = temp;

    temp *= coeffsb[nb];

    DPROD(histbuff, inbuff, coeffsb, nb, i, temp, j, k, +=);

    outbuff[i] = temp;
  }

  BUFFER(histbuff, inbuff, MAX(na,nb), amount);
}


void iir_filter_pure(int n, double coeffs[], frame_t histbuff[], int amount, frame_t inbuff[], frame_t outbuff[])
{
  int i,j,k;
  double temp;

  for(i=0;i<amount;++i)
  {
    temp = inbuff[i];

    DPROD(histbuff, outbuff, coeffs, n, i, temp, j, k, -=);

    outbuff[i] = temp;
  }

  BUFFER(histbuff, outbuff, n, amount);
}
