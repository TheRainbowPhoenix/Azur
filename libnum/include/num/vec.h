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

template<typename T, int N> requires(N > 0)
struct vec
{
    T coords[N];

    inline constexpr int size() const { return N; }

    inline constexpr T &operator[](int i) {
        return ((T *)this)[i];
    }
    inline constexpr T const &operator[](int i) const {
        return ((T *)this)[i];
    }
};

template<typename T>
struct vec<T,2>
{
    T x, y;

    vec(): x {0}, y {0} {}
    vec(T _x, T _y): x {_x}, y {_y} {}

    inline constexpr int size() const { return 2; }

    inline constexpr T &operator[](int i) {
        return ((T *)this)[i];
    }
    inline constexpr T const &operator[](int i) const {
        return ((T *)this)[i];
    }
};
using vec2 = vec<num,2>;

template<typename T>
struct vec<T,3>
{
    T x, y, z;

    vec(): x {0}, y {0}, z {0} {}
    vec(T _x, T _y, T _z): x {_x}, y {_y}, z {_z} {}
    vec(T scalar): x {scalar}, y {scalar}, z {scalar} {}
    vec(vec<T,2> v): x {v.x}, y {v.y}, z {0} {}
    vec(vec<T,2> v, T _z): x {v.x}, y {v.y}, z {_z} {}

    inline constexpr int size() const { return 3; }

    inline constexpr T &operator[](int i) {
        return ((T *)this)[i];
    }
    inline constexpr T const &operator[](int i) const {
        return ((T *)this)[i];
    }

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
struct vec<T,4>
{
    T x, y, z, w;

    vec(): x {0}, y {0}, z {0} {}
    vec(T _x, T _y, T _z, T _w): x {_x}, y {_y}, z {_z}, w {_w} {}
    vec(T scalar): x {scalar}, y {scalar}, z {scalar}, w {scalar} {}
    vec(vec<T,2> v): x {v.x}, y {v.y}, z {0}, w {0} {}
    vec(vec<T,2> v, T _z, T _w): x {v.x}, y {v.y}, z {_z}, w {_w} {}
    vec(vec<T,3> v): x {v.x}, y {v.y}, z {v.z}, w {0} {}
    vec(vec<T,3> v, T _w): x {v.x}, y {v.y}, z {v.z}, w {_w} {}

    inline constexpr int size() const { return 4; }

    inline constexpr T &operator[](int i) {
        return ((T *)this)[i];
    }
    inline constexpr T const &operator[](int i) const {
        return ((T *)this)[i];
    }

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
inline constexpr vec<T,N> &operator*=(vec<T,N> &lhs, vec<T,N> const &rhs) {
    T r = T(0);
    for(int i = 0; i < N; i++)
        r += lhs[i] * rhs[i];
    return r;
}
template<typename T, int N>
inline constexpr vec<T,N> &operator/=(vec<T,N> &lhs, T const &rhs) {
    for(int i = 0; i < N; i++)
        lhs[i] /= rhs;
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
