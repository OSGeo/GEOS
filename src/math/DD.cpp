/**********************************************************************
 *
 * GEOS - Geometry Engine Open Source
 * http://geos.osgeo.org
 *
 * Copyright (C) 2020 Crunchy Data
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU Lesser General Public Licence as published
 * by the Free Software Foundation.
 * See the COPYING file for more information.
 *
 **********************************************************************/

#include <cmath>

#include <geos/profiler.h>
#include <geos/math/DD.h>

namespace geos {
namespace math { // geos.util


/* private */
// DD
// DD::parse(std::string &str)
// {
//     DD d(0.0, 0.0);
//     return d;
// }

/* private */
int
DD::magnitude(double x)
{
    double xAbs = std::fabs(x);
    double xLog10 = std::log(xAbs) / std::log(10);
    int xMag = (int) std::floor(xLog10);
    /**
     * Since log computation is inexact, there may be an off-by-one error
     * in the computed magnitude.
     * Following tests that magnitude is correct, and adjusts it if not
     */
    double xApprox = std::pow(10, xMag);
    if (xApprox * 10 <= xAbs)
      xMag += 1;

    return xMag;
}

/* public */
bool DD::isNaN() const
{
    return std::isnan(hi);
}
/* public */
bool DD::isNegative() const
{
    return hi < 0.0 || (hi == 0.0 && lo < 0.0);
}
/* public */
bool DD::isPositive() const
{
    return hi > 0.0 || (hi == 0.0 && lo > 0.0);
}
/* public */
bool DD::isZero() const
{
    return hi == 0.0 && lo == 0.0;
}

/* public */
double DD::doubleValue() const
{
    return hi + lo;
}

/* public */
int DD::intValue() const
{
    return (int) hi;
}

/* public */
void DD::selfAdd(const DD &y)
{
    return selfAdd(y.hi, y.lo);
}

/* public */
void DD::selfAdd(double yhi, double ylo)
{
    double H, h, T, t, S, s, e, f;
    S = hi + yhi;
    T = lo + ylo;
    e = S - hi;
    f = T - lo;
    s = S-e;
    t = T-f;
    s = (yhi-e)+(hi-s);
    t = (ylo-f)+(lo-t);
    e = s+T; H = S+e; h = e+(S-H); e = t+h;

    double zhi = H + e;
    double zlo = e + (H - zhi);
    hi = zhi;
    lo = zlo;
    return;
}

/* public */
void DD::selfAdd(double y)
{
    double H, h, S, s, e, f;
    S = hi + y;
    e = S - hi;
    s = S - e;
    s = (y - e) + (hi - s);
    f = s + lo;
    H = S + f;
    h = f + (S - H);
    hi = H + h;
    lo = h + (H - hi);
    return;
}

/* public */
DD operator+(const DD &lhs, const DD &rhs)
{
    DD rv(lhs.hi, lhs.lo);
    rv.selfAdd(rhs);
    return rv;
}

/* public */
DD operator+(const DD &lhs, double rhs)
{
    DD rv(lhs.hi, lhs.lo);
    rv.selfAdd(rhs);
    return rv;
}

/* public */
void DD::selfSubtract(const DD &d)
{
    return selfAdd(-1*d.hi, -1*d.lo);
}

/* public */
void DD::selfSubtract(double p_hi, double p_lo)
{
    return selfAdd(-1*p_hi, -1*p_lo);
}

/* public */
void DD::selfSubtract(double y)
{
    return selfAdd(-1*y, 0.0);
}

/* public */
DD operator-(const DD &lhs, const DD &rhs)
{
    DD rv(lhs.hi, lhs.lo);
    rv.selfSubtract(rhs);
    return rv;
}

/* public */
DD operator-(const DD &lhs, double rhs)
{
    DD rv(lhs.hi, lhs.lo);
    rv.selfSubtract(rhs);
    return rv;
}

/* public */
void DD::selfMultiply(double yhi, double ylo)
{
    double hx, tx, hy, ty, C, c;
    C = SPLIT * hi; hx = C-hi; c = SPLIT * yhi;
    hx = C-hx; tx = hi-hx; hy = c-yhi;
    C = hi*yhi; hy = c-hy; ty = yhi-hy;
    c = ((((hx*hy-C)+hx*ty)+tx*hy)+tx*ty)+(hi*ylo+lo*yhi);
    double zhi = C+c; hx = C-zhi;
    double zlo = c+hx;
    hi = zhi;
    lo = zlo;
    return;
}

/* public */
void DD::selfMultiply(DD const &d)
{
    return selfMultiply(d.hi, d.lo);
}

/* public */
void DD::selfMultiply(double y)
{
    return selfMultiply(y, 0.0);
}

/* public */
DD operator*(const DD &lhs, const DD &rhs)
{
    DD rv(lhs.hi, lhs.lo);
    rv.selfMultiply(rhs);
    return rv;
}

/* public */
DD operator*(const DD &lhs, double rhs)
{
    DD rv(lhs.hi, lhs.lo);
    rv.selfMultiply(rhs);
    return rv;
}


/* public */
void DD::selfDivide(double yhi, double ylo)
{
    double hc, tc, hy, ty, C, c, U, u;
    C = hi/yhi; c = SPLIT*C; hc =c-C;
    u = SPLIT*yhi; hc = c-hc;
    tc = C-hc; hy = u-yhi; U = C * yhi;
    hy = u-hy; ty = yhi-hy;
    u = (((hc*hy-U)+hc*ty)+tc*hy)+tc*ty;
    c = ((((hi-U)-u)+lo)-C*ylo)/yhi;
    u = C+c;
    hi = u;
    lo = (C-u)+c;
    return;
}

/* public */
void DD::selfDivide(const DD &d)
{
    return selfDivide(d.hi, d.lo);
}

/* public */
void DD::selfDivide(double y)
{
    return selfDivide(y, 0.0);
}

/* public */
DD operator/(const DD &lhs, const DD &rhs)
{
    DD rv(lhs.hi, lhs.lo);
    rv.selfDivide(rhs);
    return rv;
}

/* public */
DD operator/(const DD &lhs, double rhs)
{
    DD rv(lhs.hi, lhs.lo);
    rv.selfDivide(rhs);
    return rv;
}

DD DD::negate() const
{
    DD rv(hi, lo);
    if (isNaN())
    {
        return rv;
    }
    rv.hi = -hi;
    rv.lo = -lo;
    return rv;
}

DD DD::reciprocal() const
{
    double  hc, tc, hy, ty, C, c, U, u;
    C = 1.0/hi;
    c = SPLIT*C;
    hc = c-C;
    u = SPLIT*hi;
    hc = c-hc; tc = C-hc; hy = u-hi; U = C*hi; hy = u-hy; ty = hi-hy;
    u = (((hc*hy-U)+hc*ty)+tc*hy)+tc*ty;
    c = ((((1.0-U)-u))-C*lo)/hi;
    double zhi = C+c;
    double zlo = (C-zhi)+c;
    DD rv(zhi, zlo);
    return rv;
}

DD DD::floor() const
{
    DD rv(hi, lo);
    if (isNaN()) return rv;
    double fhi = std::floor(hi);
    double flo = 0.0;
    // Hi is already integral.  Floor the low word
    if (fhi == hi) {
      flo = std::floor(lo);
    }
      // do we need to renormalize here?
    rv.hi = fhi;
    rv.lo = flo;
    return rv;
}

DD DD::ceil() const
{
    DD rv(hi, lo);
    if (isNaN()) return rv;
    double fhi = std::ceil(hi);
    double flo = 0.0;
    // Hi is already integral.  Ceil the low word
    if (fhi == hi) {
      flo = std::ceil(lo);
      // do we need to renormalize here?
    }
    rv.hi = fhi;
    rv.lo = flo;
    return rv;
}

int DD::signum() const
{
    if (hi > 0) return 1;
    if (hi < 0) return -1;
    if (lo > 0) return 1;
    if (lo < 0) return -1;
    return 0;
}

DD DD::rint() const
{
    DD rv(hi, lo);
    if (isNaN()) return rv;
    return (rv + 0.5).floor();
}

DD DD::trunc() const
{
    DD rv(hi, lo);
    if (isNaN()) return rv;
    if (isPositive())
        return rv.floor();
    return rv.ceil();
}

DD DD::abs() const
{
    DD rv(hi, lo);
    if (isNaN()) return rv;
    if (isNegative())
        return rv.negate();

    return rv;
}

DD DD::sqr() const
{
    DD rv(hi, lo);
    return rv * rv;
}

void DD::selfSqr()
{
    DD rhs(hi, lo);
    selfMultiply(rhs);
    return;
}




}
}
