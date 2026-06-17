#ifndef _XDG_VEC3FA_H
#define _XDG_VEC3FA_H

#include <array>
#include <assert.h>
#include <iostream>
#include <math.h>
#include <immintrin.h>
#include <xmmintrin.h>
#include <limits>
#include <vector>

#ifndef NDEBUG
#define __forceinline inline
#else
#define __forceinline inline __attribute__((always_inline))
#endif

#include <fmt/format.h>
#include "xdg/constants.h"

namespace xdg {
struct Vec3fa {
  union{
// TODO :remove this union and AVX2 support?
#ifdef __AVX2__
    __m256 v;
#endif
    struct{ float x,y,z;  size_t a;};
  };

  __forceinline Vec3fa(std::vector<float> const& vec) : x(vec[0]), y(vec[1]), z(vec[2]), a(0) {
    if (vec.size() != 3)
      throw std::runtime_error("Vec3fa constructor from std::vector<float> requires a vector of size 3");
  }

  __forceinline Vec3fa(std::array<float, 3> const& arr) : x(arr[0]), y(arr[1]), z(arr[2]), a(0) { }

  __forceinline Vec3fa () {}
#ifdef __AVX2__
  __forceinline Vec3fa ( const __m256 v_ ) : v(v_) { }
#endif
  __forceinline Vec3fa            ( const Vec3fa& other ) { x = other.x; y = other.y; z = other.z; a = other.a; }
  __forceinline Vec3fa& operator =( const Vec3fa& other ) { x = other.x; y = other.y; z = other.z; a = other.a; return *this;}

  __forceinline Vec3fa( const float pa ) { x = pa; y = pa; z = pa; a = pa;}
  __forceinline Vec3fa( const float pa[3]) { x = pa[0]; y = pa[1]; z = pa[2]; }
  __forceinline Vec3fa( const float px, const float py, const float pz) { x = px; y = py; z = pz; a = pz;}
  __forceinline Vec3fa( const float px, const float py, const float pz, const int pa) { x = px; y = py; z = pz; a = pa; }


  __forceinline const float& operator[](const size_t index) const { assert(index < 3); return (&x)[index]; }
  __forceinline       float& operator[](const size_t index)       { assert(index < 3); return (&x)[index]; }

  __forceinline float length_sqr () const { return x*x + y*y + z*z; }
  __forceinline float length () const { return sqrt(length_sqr()); }

  __forceinline Vec3fa normalize() {
    float len = length();
    len = len < min_rcp_input ? min_rcp_input : len;
    x /= len; y /= len; z/= len;
    return *this;
  }

  __forceinline bool approx_eq (const Vec3fa& other, const float tol = 1e-6) const {
    return fabs(x - other.x) < tol && fabs(y - other.y) < tol && fabs(z - other.z) < tol;
  }

    __forceinline Vec3fa operator +=(const Vec3fa& v) { x = x + v.x; y = y +v.y; z = z + v.z; return *this; }

    __forceinline Vec3fa operator -=(const Vec3fa& v) { x = x - v.x; y = y -v.y; z = z - v.z; return *this; }

    __forceinline Vec3fa operator *=(const Vec3fa& v) { x = x * v.x; y = y *v.y; z = z * v.z; return *this; }

    __forceinline Vec3fa operator /=(const Vec3fa& v) { x = x / v.x; y = y /v.y; z = z / v.z; return *this; }

    __forceinline float dot(const Vec3fa& v) const
    {
      return x * v.x + y * v.y + z * v.z;
    }

    __forceinline Vec3fa cross(const Vec3fa& v) const
    {
      return Vec3fa(y * v.z - z * v.y,
                    z * v.x - x * v.z,
                    x * v.y - y * v.x);
    }
};

__forceinline Vec3fa operator +( const Vec3fa& b, const Vec3fa& c ) { return Vec3fa(b.x+c.x, b.y+c.y, b.z+c.z, b.a+c.a); }
__forceinline Vec3fa operator -( const Vec3fa& b, const Vec3fa& c ) { return Vec3fa(b.x-c.x, b.y-c.y, b.z-c.z, b.a-c.a); }
__forceinline Vec3fa operator *( const Vec3fa& b, const Vec3fa& c ) { return Vec3fa(b.x*c.x, b.y*c.y, b.z*c.z, b.a*c.a); }
__forceinline Vec3fa operator *( const float& pa, const Vec3fa& c ) { return Vec3fa(pa) * c; }
__forceinline Vec3fa operator *( const Vec3fa& c, const float& pa ) { return Vec3fa(pa) * c; }
__forceinline Vec3fa operator /( const Vec3fa& b, const Vec3fa& c ) { return Vec3fa(b.x/c.x, b.y/c.y, b.z/c.z, b.a/c.a); }
__forceinline Vec3fa operator /( const float& pa, const Vec3fa& c ) { return Vec3fa(pa) / c; }
__forceinline Vec3fa operator /( const Vec3fa& c, const float& pa ) { return c / Vec3fa(pa); }

__forceinline bool operator ==( const Vec3fa& b, const Vec3fa& c)
{
  return b.x == c.x && b.y == c.y && b.z == c.z;
}

#if defined(__AVX2__)
__forceinline const Vec3fa min( const Vec3fa& a, const Vec3fa& b ) { return _mm256_min_ps(a.v, b.v); }
__forceinline const Vec3fa max( const Vec3fa& a, const Vec3fa& b ) { return _mm256_max_ps(a.v, b.v); }
#else
__forceinline const Vec3fa min( const Vec3fa& b, const Vec3fa& c ) { return Vec3fa(std::min(b.x,c.x),std::min(b.y,c.y),
									   std::min(b.z,c.z),std::min(b.a,c.a)); }
__forceinline const Vec3fa max( const Vec3fa& b, const Vec3fa& c ) { return Vec3fa(std::max(b.x,c.x),std::max(b.y,c.y),
									   std::max(b.z,c.z),std::max(b.a,c.a)); }
#endif

__forceinline float reduce_add( const Vec3fa &v ) { return v.x + v.y + v.z; }


__forceinline float reduce_mul( const Vec3fa& v ) { return v.x * v.y * v.z; }

__forceinline float reduce_min( const Vec3fa& v ) { return std::min(std::min(v.x, v.y), v.z); }

__forceinline float reduce_max( const Vec3fa& v ) { return std::max(std::max(v.x, v.y), v.z); }

__forceinline float halfArea(Vec3fa v) { return v.x*(v.y+v.z)+(v.y*v.z); }

__forceinline Vec3fa zero_fix( const Vec3fa& a )
  {
    return Vec3fa(fabs(a.x) < min_rcp_input ? float(min_rcp_input) : a.x,
                  fabs(a.y) < min_rcp_input ? float(min_rcp_input) : a.y,
                  fabs(a.z) < min_rcp_input ? float(min_rcp_input) : a.z);
  }

__forceinline const Vec3fa rcp(const Vec3fa& v )
{
  return Vec3fa(1.0f/v.x, 1.0f/v.y, 1.0f/v.z);
}

__forceinline const Vec3fa rcp_safe(const Vec3fa& a) { return rcp(zero_fix(a)); }

__forceinline Vec3fa operator +( const Vec3fa &a ) { return Vec3fa(+a.x, +a.y, +a.z); }

__forceinline Vec3fa operator -( const Vec3fa &a ) { return Vec3fa(-a.x, -a.y, -a.z); }

__forceinline float dot( const Vec3fa& a, const Vec3fa& b ) { return a.dot(b); }

__forceinline Vec3fa cross( const Vec3fa& a, const Vec3fa& b ) { return a.cross(b); }

__forceinline std::ostream& operator <<(std::ostream &os, Vec3fa  const& v) {
  return os << '[' << v[0] << ' ' << v[1] << ' ' << v[2] << ' ' << v.a << ']';
}



} // end namespace xdg

#endif

