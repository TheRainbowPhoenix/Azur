//---------------------------------------------------------------------------//
//  ,"  /\  ",    Azur: A game engine for CASIO fx-CG and PC                 //
// |  _/__\_  |   Designed by Lephe' and the Planète Casio community.        //
//  "._`\/'_."    License: MIT <https://opensource.org/licenses/MIT>         //
//---------------------------------------------------------------------------//
// num.vec: Vector arithmetic
//
// This header provides basic vector arithmetic. It is type-generic and works
// on any numerical type, including integers, floating-point types, and this
// library's fixed-point types. Not all operations are available for all types
// however.
//
// We slightly abuse the underlying object representation by expecting the
// vector type vec<T,N> to be isomorphic to T[N] so we can perform accesses to
// members with a pointer cast and array access. This access doesn't break
// strict aliasing rules since the members' type is correct.
//---

#pragma once

#include <num/num.h>

namespace libnum {

/* _vec_mixin: CRTP support type with common methods
   This structure type is inherited by all vec<T,N> types. It assumes that the
   vector type is isomorphic to T[N] and implements all the methods that can be
   derived from that. */
template<typename Vector, typename T, int N> requires(N > 0)
struct _vec_mixin
{
    /* Number of components in the vector. */
    inline constexpr int size() const {
        return N;
    }

    /* Access to the i-th component as an lvalue. */
    inline constexpr T &operator[](int i) {
        return ((T *)this)[i];
    }
    inline constexpr T const &operator[](int i) const {
        return ((T *)this)[i];
    }

    /* Dot product with another vector of the same size. */
    constexpr T dot(Vector const &other) const {
        T r(0);
        for(int i = 0; i < N; ++i)
            r += (*this)[i] * other[i];
        return r;
    }

    /* Vector length, squared. */
    constexpr T length2() const {
        T r(0);
        for(int i = 0; i < N; ++i)
            r += (*this)[i] * (*this)[i];
        return r;
    }

    /* Vector length. */
    // TODO: _vec_mixin<T,N>::length() only works for num (uses .sqrt() method)
    constexpr T length() const {
        return length2().sqrt();
    }

    /* Normalized */
    constexpr Vector normalize() const {
        return *static_cast<const Vector *>(this) / length();
    }
};

/* Vector type implementations. The generic type vec<T,N> uses an array and no
   specific facilities. The specialized types vec<T,2> through vec<T,4> provide
   access to members as .x, .y, .z, .w in addition to operator[]. They also
   provide swizzling-style constructors and methods. */

template<typename T, int N> requires(N > 0)
struct vec: _vec_mixin<vec<T,N>, T, N>
{
    T coords[N];
};

template<typename T>
struct vec<T,2>: _vec_mixin<vec<T,2>, T, 2>
{
    T x, y;

    vec(): x {0}, y {0} {}
    vec(T _x, T _y): x {_x}, y {_y} {}
};
using vec2 = vec<num,2>;

template<typename T>
struct vec<T,3>: _vec_mixin<vec<T,3>, T, 3>
{
    T x, y, z;

    vec(): x {0}, y {0}, z {0} {}
    vec(T _x, T _y, T _z): x {_x}, y {_y}, z {_z} {}
    vec(T scalar): x {scalar}, y {scalar}, z {scalar} {}
    vec(vec<T,2> v): x {v.x}, y {v.y}, z {0} {}
    vec(vec<T,2> v, T _z): x {v.x}, y {v.y}, z {_z} {}

    inline constexpr vec<T,2> xy() const {
        return vec<T,2>(x, y);
    }
    inline constexpr vec<T,2> xz() const {
        return vec<T,2>(x, z);
    }
    inline constexpr vec<T,2> yz() const {
        return vec<T,2>(y, z);
    }
};
using vec3 = vec<num,3>;

template<typename T>
struct vec<T,4>: _vec_mixin<vec<T,4>, T, 4>
{
    T x, y, z, w;

    vec(): x {0}, y {0}, z {0} {}
    vec(T _x, T _y, T _z, T _w): x {_x}, y {_y}, z {_z}, w {_w} {}
    vec(T scalar): x {scalar}, y {scalar}, z {scalar}, w {scalar} {}
    vec(vec<T,2> v): x {v.x}, y {v.y}, z {0}, w {0} {}
    vec(vec<T,2> v, T _z, T _w): x {v.x}, y {v.y}, z {_z}, w {_w} {}
    vec(vec<T,3> v): x {v.x}, y {v.y}, z {v.z}, w {0} {}
    vec(vec<T,3> v, T _w): x {v.x}, y {v.y}, z {v.z}, w {_w} {}

    inline constexpr vec<T,3> xyz() const {
        return vec<T,3>(x, y, z);
    }
};
using vec4 = vec<num,4>;

/* The following concept identifies vec types */

template<typename T>
struct is_vec_trait: std::false_type {};

template<typename T, int N>
struct is_vec_trait<vec<T,N>>: std::true_type {};

template<typename T>
concept is_vec = is_vec_trait<T>::value;

/* Arithmetic */

template<typename T, int N>
inline constexpr vec<T,N> &operator+=(vec<T,N> &lhs, vec<T,N> const &rhs) {
    for(int i = 0; i < N; i++)
        lhs[i] += rhs[i];
    return lhs;
}
template<typename T, int N>
inline constexpr vec<T,N> &operator-=(vec<T,N> &lhs, vec<T,N> const &rhs) {
    for(int i = 0; i < N; i++)
        lhs[i] -= rhs[i];
    return lhs;
}
template<typename T, int N>
inline constexpr vec<T,N> &operator*=(vec<T,N> &lhs, T const &rhs) {
    for(int i = 0; i < N; i++)
        lhs[i] *= rhs;
    return lhs;
}
template<typename T, int N>
inline constexpr vec<T,N> &operator*=(vec<T,N> &lhs, int rhs) {
    for(int i = 0; i < N; i++)
        lhs[i] *= rhs;
    return lhs;
}
template<typename T, int N>
inline constexpr vec<T,N> &operator/=(vec<T,N> &lhs, T const &rhs) {
    for(int i = 0; i < N; i++)
        lhs[i] /= rhs;
    return lhs;
}
template<typename T, int N>
inline constexpr vec<T,N> &operator/=(vec<T,N> &lhs, int rhs) {
    for(int i = 0; i < N; i++)
        lhs[i] /= rhs;
    return lhs;
}

template<typename T, int N>
inline constexpr vec<T,N> operator+(vec<T,N> const &lhs) {
    return lhs;
}
template<typename T, int N>
inline constexpr vec<T,N> operator-(vec<T,N> lhs) {
    for(int i = 0; i < N; i++)
        lhs[i] = -lhs[i];
    return lhs;
}
template<typename T, int N>
inline constexpr vec<T,N> operator+(vec<T,N> lhs, vec<T,N> const &rhs) {
    return lhs += rhs;
}
template<typename T, int N>
inline constexpr vec<T,N> operator-(vec<T,N> lhs, vec<T,N> const &rhs) {
    return lhs -= rhs;
}
template<typename T, int N>
inline constexpr vec<T,N> operator*(vec<T,N> lhs, T const &rhs) {
    return lhs *= rhs;
}
template<typename T, int N>
inline constexpr vec<T,N> operator*(T const &lhs, vec<T,N> rhs) {
    for(int i = 0; i < N; i++)
        rhs[i] *= lhs;
    return rhs;
}
template<typename T, int N>
inline constexpr vec<T,N> operator*(vec<T,N> lhs, int rhs) {
    return lhs *= rhs;
}
template<typename T, int N>
inline constexpr vec<T,N> operator*(int lhs, vec<T,N> rhs) {
    for(int i = 0; i < N; i++)
        rhs[i] *= lhs;
    return rhs;
}
template<typename T, int N>
inline constexpr vec<T,N> operator/(vec<T,N> lhs, T const &rhs) {
    return lhs /= rhs;
}
template<typename T, int N>
inline constexpr vec<T,N> operator/(vec<T,N> lhs, int rhs) {
    return lhs /= rhs;
}

/* Comparisons */

template<typename T, int N>
inline constexpr bool operator==(vec<T,N> const &lhs, vec<T,N> const &rhs) {
    for(int i = 0; i < N; i++) {
        if(lhs[i] != rhs[i])
            return false;
    }
    return true;
}
template<typename T, int N>
inline constexpr bool operator!=(vec<T,N> const &lhs, vec<T,N> const &rhs) {
    for(int i = 0; i < N; i++) {
        if(lhs[i] != rhs[i])
            return true;
    }
    return false;
}

} /* namespace libnum */
