//---------------------------------------------------------------------------//
//  ,"  /\  ",    Azur: A game engine for CASIO fx-CG and PC                 //
// |  _/__\_  |   Designed by Lephe' and the Planète Casio community.        //
//  "._`\/'_."    License: MIT <https://opensource.org/licenses/MIT>         //
//---------------------------------------------------------------------------//
// num.rect: 2D rectangles
//
// Lke other basic geometric primitives, rectangles are pure, passed by value,
// and methods modifying them return new obejcts. Compilers are fully able to
// scalarize them, sometimes even in function calls (some targets have ABIs
// that allow 128-bit objects in registers). `r = r.modify()` expands to an in-
// place replacement in GCC 15 with just -O1.
//---

#pragma once

#include <num/num.h>
#include <num/vec.h>

namespace libnum {

/* rect<T> is available for any numerical type T, including unsigned integers.
   As such, empty rectangles are represented only with width/height 0. */
template<typename T>
struct rect
{
    rect():
        m_x {0}, m_y {0}, m_w {0}, m_h {0} {}
    rect(T x, T y, T w, T h):
        m_x {x}, m_y {y}, m_w {w}, m_h {h} {}
    rect(T x, T y, vec<T,2> size):
        m_x {x}, m_y {y}, m_w {size.x}, m_h {size.y} {}
    rect(vec<T,2> pos, T w, T h):
        m_x {pos.x}, m_y {pos.y}, m_w {w}, m_h {h} {}
    rect(vec<T,2> pos, vec<T,2> size):
        m_x {pos.x}, m_y {pos.y}, m_w {size.x}, m_h {size.y} {}

    T x()       const { return m_x; }
    T y()       const { return m_y; }
    T width()   const { return m_w; }
    T height()  const { return m_h; }

    T left()    const { return m_x; }
    T right()   const { return m_x + m_w; }
    T top()     const { return m_y; }
    T bottom()  const { return m_y + m_h; }
    T centerX() const { return m_x + m_w / 2; }
    T centerY() const { return m_x + m_h / 2; }

    vec<T,2> size()        const { return {m_w, m_h}; }
    vec<T,2> topLeft()     const { return {left(),  top()}; }
    vec<T,2> topRight()    const { return {right(), top()}; }
    vec<T,2> bottomLeft()  const { return {left(),  bottom()}; }
    vec<T,2> bottomRight() const { return {right(), bottom()}; }
    vec<T,2> center()      const { return {centerX(), centerY()}; }

    bool intersects(rect<T> const &other) const {
        return right() > other.left()
            && left() < other.right()
            && bottom() > other.top()
            && top() < other.bottom();
    }

    rect<T> translate(num dx, num dy) {
        return rect<T>(m_x + dx, m_y + dy, m_w, m_h);
    }
    rect<T> translate(vec<T,2> xy) {
        return rect<T>(topLeft() + xy, m_w, m_h);
    }

private:
    T m_x, m_y, m_w, m_h;
};

} /* namespace libnum */
