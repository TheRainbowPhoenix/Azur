//---------------------------------------------------------------------------//
//  ,"  /\  ",    Azur: A game engine for CASIO fx-CG and PC                 //
// |  _/__\_  |   Designed by Lephe' and the Planète Casio community.        //
//  "._`\/'_."    License: MIT <https://opensource.org/licenses/MIT>         //
//---------------------------------------------------------------------------//
// unit_sample.h: Utility for generating sample values for testing
//
// This file mainly provides the runOnSampleInputs() function which runs a
// boolean function f() on a series of generated inputs. It achieves this by
// reading a storage of pre-computed inputs for each of the arguments of f.
// This requires a storage to exist for each argument's type, which is provided
// by SampleBase and subclassed by Sample.
//
// The meat of the generation is the generateIntSample() function, which
// generates sparse sets of integers of varied size that omit needlessly
// redundant input while densely covering special cases.
//---

#pragma once

#include <vector>
#include <type_traits>
#include <functional>
#include <cassert>
#include <stdint.h>

#include <num/num.h>
using namespace num;

//---
// Integer sampling
//---

template<typename T, typename Uint>
void generateIntSample(std::vector<T> &v, std::function<T(Uint)> ctor,
    Uint start, Uint length, int count)
{
    /* When we get to a set of size 16, do all values. */
    if(count <= 16) {
        /* If we have an even-only range, force in some odd numbers too. */
        bool switch_odd = (length >= (Uint)(2*count));
        Uint step = length / count;
        while(count-- > 0) {
            v.push_back(ctor(start));
            start += switch_odd ? step ^ (count & 1) : step;
        }
        return;
    }

    /* Otherwise, fractally divide into 16 segments and assign a portion of the
       available points to each section. We divide the count of points in 16ths
       so that there is at least one point in each segment. */
    Uint sublength = length / 16;
    int subcount = count / 16;
    /* This array must add up to 16. */
    int props[16] = { 4, 1, 1, 0, 0, 2, 0, 0, 1, 0, 2, 0, 0, 0, 1, 4 };
    for(int i = 0; i < 16; i++) {
        if(props[i])
            generateIntSample(v, ctor, (Uint)(start + i * sublength),
                sublength, subcount * props[i]);
    }
}

//---
// Input sampling
//---

template<typename T>
struct SampleBase {
    static std::vector<T> v;
};

template<typename T>
std::vector<T> SampleBase<T>::v;

template<typename T>
struct Sample {};

template<typename T>
concept has_sample = requires { Sample<T>::get; };

template<>
struct Sample<num8>: SampleBase<num8>
{
    static std::vector<num8> const &get() {
        if(v.size() > 0)
            return v;
        for(int i = 0; i <= 0xff; i++) {
            num8 x;
            x.v = i;
            v.push_back(x);
        }
        return v;
    }
};

template<>
struct Sample<num16>: SampleBase<num16>
{
    static std::vector<num16> const &get() {
        if(v.size() > 0)
            return v;
        auto f = [](uint16_t i) { num16 x; x.v = i; return x; };
        generateIntSample<num16, uint16_t>(v, f, 0, 1 << 15, 512);
        generateIntSample<num16, uint16_t>(v, f, -(1 << 15), 1 << 15, 512);
        return v;
    }
};

template<>
struct Sample<num32>: SampleBase<num32>
{
    static std::vector<num32> const &get() {
        if(v.size() > 0)
            return v;
        auto f = [](uint32_t i) { num32 x; x.v = i; return x; };
        generateIntSample<num32, uint32_t>(v, f, 0, 1 << 15, 512);
        generateIntSample<num32, uint32_t>(v, f, -(1 << 15), 1 << 15, 512);
        generateIntSample<num32, uint32_t>(v, f, 0, 1ul << 31, 512);
        generateIntSample<num32, uint32_t>(v, f, 1ul << 31, 1ul << 31, 512);
        return v;
    }
};

template<>
struct Sample<int>: SampleBase<int>
{
    static std::vector<int> const &get() {
        if(v.size() > 0)
            return v;
        auto f = [](uint32_t i) { return i; };
        generateIntSample<int, uint32_t>(v, f, 0, 1 << 15, 512);
        generateIntSample<int, uint32_t>(v, f, -(1 << 15), 1 << 15, 512);
        generateIntSample<int, uint32_t>(v, f, 0, 1ul << 31, 512);
        generateIntSample<int, uint32_t>(v, f, 1ul << 31, 1ul << 31, 512);
        return v;
    }
};

template<>
struct Sample<num64>: SampleBase<num64>
{
    static std::vector<num64> const &get() {
        if(v.size() > 0)
            return v;
        auto f = [](uint64_t i) { num64 x; x.v = i; return x; };
        generateIntSample<num64, uint64_t>(v, f, 0, 1 << 15, 512);
        generateIntSample<num64, uint64_t>(v, f, -(1 << 15), 1 << 15, 512);
        generateIntSample<num64, uint64_t>(v, f, 0, 1ul << 31, 512);
        generateIntSample<num64, uint64_t>(v, f, -(1ul << 31), 1ul << 31, 512);
        generateIntSample<num64, uint64_t>(v, f, 0, 1ull << 63, 512);
        generateIntSample<num64, uint64_t>(v, f, 1ull << 63, 1ull << 63, 512);
        return v;
    }
};

//---
// Automatic test functions
//---

template<typename T> requires(has_sample<T>)
void runOnSampleInputs(std::function<void(T)> f)
{
    for(auto t: Sample<T>::get())
        f(t);
}

template<typename T, typename U> requires(has_sample<T> && has_sample<U>)
void runOnSampleInputs(std::function<void(T, U)> f)
{
    for(auto t: Sample<T>::get())
    for(auto u: Sample<U>::get())
        f(t, u);
}
