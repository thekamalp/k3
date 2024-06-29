// k3 grpahics library
// Date: 10/10/2021

#pragma once

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

#if defined(WIN32)
#include <direct.h>
#else
#include <unistd.h>
#endif

#define K3CALLBACK __cdecl
#ifndef K3DECLSPEC
#define K3DECLSPEC __declspec( dllimport )
#endif
#define K3API K3DECLSPEC

// ------------------------------------------------------------
// k3 smart pointer classes
template <class T>
class k3ptr
{
private:
    T* _ptr;

public:
    template <class T2>
    friend class k3ptr;

    k3ptr() : _ptr(NULL)
    { }

    k3ptr(T* p) : _ptr(p)
    {
        if (_ptr) _ptr->incRef();
    }

    k3ptr(const k3ptr<T>& p) : _ptr(p._ptr)
    {
        if (_ptr) _ptr->incRef();
    }

    virtual ~k3ptr()
    {
        if (_ptr) _ptr->decRef();
    }

    template <class T2>
    operator k3ptr<T2>()
    {
        return k3ptr<T2>(static_cast<T2*>(_ptr));
    }

    T& operator*()
    {
        return *_ptr;
    }

    T* operator->()
    {
        return _ptr;
    }

    const T* operator->() const
    {
        return _ptr;
    }

    k3ptr<T>& operator=(const k3ptr<T>& a)
    {
        T* a_ptr = static_cast<T*>(a._ptr);
        if (_ptr != a._ptr) {
            if (_ptr) _ptr->decRef();
            _ptr = a_ptr;
            if (_ptr) _ptr->incRef();
        }
        return *this;
    }

    bool operator==(k3ptr<T> p)
    {
        return (p._ptr == _ptr);
    }

    bool operator!=(k3ptr<T> p)
    {
        return (p._ptr != _ptr);
    }

    bool operator==(T* p)
    {
        return (p == _ptr);
    }

    bool operator!=(T* p)
    {
        return (p != _ptr);
    }

    k3ptr<T>& operator=(T* p)
    {
        if (_ptr != p) {
            if (_ptr) _ptr->decRef();
            _ptr = p;
            if (_ptr) _ptr->incRef();
        }
        return *this;
    }

    template <class T2>
    k3ptr<T>& operator=(const k3ptr<T2>& a)
    {
        T* a_ptr = static_cast<T*>(a._ptr);
        if (_ptr != a_ptr) {
            if (_ptr) _ptr->decRef();
            _ptr = a_ptr;
            if (_ptr) _ptr->incRef();
        }
        return *this;
    }

    template <class T2>
    k3ptr<T>& operator=(T2* p)
    {
        T* p1 = static_cast<T*>(p);
        if (_ptr != p1) {
            if (_ptr) _ptr->decRef();
            _ptr = p1;
            if (_ptr) _ptr->incRef();
        }
        return *this;
    }

    bool operator==(const T* p) const
    {
        return (_ptr == p);
    }

    bool operator!=(const T* p) const
    {
        return (_ptr != p);
    }
};

enum class k3objType {
    UNKNOWN,
    IMAGE,
    TIMER,
    SOUND_BUF,
    FENCE,
    RESOURCE,
    SURF,
    SAMPLER,
    BUFFER,
    BLAS,
    TLAS,
    FONT,
    CMD_BUF,
    SHADER_BINDING,
    SHADER,
    RT_STATE,
    RT_STATE_TABLE,
    GFX_STATE,
    MEM_POOL,
    UPLOAD_IMAGE,
    UPLOAD_BUFFER,
    GFX,
    WIN,
    MESH,
    DOWNLOAD_IMAGE
};

class k3obj
{
    template <class T>
    friend class k3ptr;
public:
    k3obj() : _ref(0)
    { }
    virtual ~k3obj()
    { }
    virtual K3API k3objType getObjType() const = 0;

private:
    uint32_t _ref;
    uint32_t incRef()
    {
        _ref++;
        return _ref;
    }
    uint32_t decRef()
    {
        if (_ref) _ref--;
        if (_ref == 0) {
            delete this;
            return 0;
        }
        return _ref;
    }
};

typedef k3ptr<k3obj> k3objPtr;

// ------------------------------------------------------------
// k3 error handling
typedef void (K3CALLBACK* k3error_handler_ptr) (const char* error_msg, const char* title);

void K3CALLBACK k3error_StdOutHandler(const char* error_msg, const char* title);
void K3CALLBACK k3error_MsgBoxHandler(const char* error_msg, const char* title);

class k3error
{
public:
    static K3API void SetHandler(k3error_handler_ptr error_handler);
    static K3API k3error_handler_ptr GetHandler();
    static K3API void Handler(const char* error, const char* title = NULL);

private:
    static k3error_handler_ptr _handler;
};

// ------------------------------------------------------------
// k3 math functions
const float PI = 3.1415926535898f;

inline float rad2deg(float x)
{
    return (180 * x) / PI;
}

inline float deg2rad(float x)
{
    return (PI * x) / 180;
}

/* operations to a single vector */
K3API float* k3v_Negate(uint32_t vec_length, float* d);
K3API float* k3v_Swizzle(uint32_t vec_length, float* d, const uint32_t* indices);
K3API float  k3v_Length(uint32_t vec_length, const float* s);

/* operations of two vectors of the same length */
K3API float* k3v_Add(uint32_t vec_length, float* d, const float* s1, const float* s2);
K3API float* k3v_Sub(uint32_t vec_length, float* d, const float* s1, const float* s2);
K3API float* k3v_Mul(uint32_t vec_length, float* d, const float* s1, const float* s2);
K3API float* k3v_Div(uint32_t vec_length, float* d, const float* s1, const float* s2);
K3API float* k3v_Cross(uint32_t vec_length, float* d, const float* s1, const float* s2);
K3API bool k3v_Equals(uint32_t vec_length, const float* s1, const float* s2);
K3API float k3v_Dot(uint32_t vec_length, const float* s1, const float* s2);
K3API float* k3v_Min(uint32_t vec_length, float* d, const float* s1, const float* s2);
K3API float* k3v_Max(uint32_t vec_length, float* d, const float* s1, const float* s2);

/* Generate a tangent and Bitangent vectors; all vectors are 3 components except u* and which are 2 */
K3API void k3v3_SetTangentBitangent(float* t, float* b, const float* p0, const float* p1, const float* p2, const float* u0, const float* u1, const float* u2);

/* operations where s1 is scalar and s2 is a vector */
K3API float* k3sv_Add(uint32_t vec_length, float* d, const float s1, const float* s2);
K3API float* k3sv_Sub(uint32_t vec_length, float* d, const float s1, const float* s2);
K3API float* k3sv_Mul(uint32_t vec_length, float* d, const float s1, const float* s2);
K3API float* k3sv_Div(uint32_t vec_length, float* d, const float s1, const float* s2);

/* operations on a single matrix */
K3API float  k3m_Determinant(uint32_t rows, const float* s);
K3API float* k3m_Transpose(uint32_t rows, uint32_t cols, float* d);
K3API float* k3m_Inverse(uint32_t rows, float* d);
K3API float* k3m_Swizzle(uint32_t rows, uint32_t cols, float* d, const uint32_t* row_indices, const uint32_t* col_indices);
K3API float* k3m_SetIdentity(uint32_t rows, float* d);
K3API float* k3m_SetRotation(uint32_t rows, float* d, float angle, const float* axis);

/* operations on a single 4x4 matrix */
K3API float* k3m4_SetPerspectiveOffCenter(float* d, float left, float right, float bottom, float top, float znear, float zfar, bool left_handed, bool dx_style, bool reverse_z);
K3API float* k3m4_SetPerspectiveFov(float* d, float fovy, float aspect, float znear, float zfar, bool left_handed, bool dx_style, bool reverse_z);
K3API float* k3m4_SetOrthoOffCenter(float* d, float left, float right, float bottom, float top, float znear, float zfar, bool left_handed, bool dx_style);
K3API float* k3m4_SetLookAt(float* d, const float* eye, const float* at, const float* up_dir, bool left_handed);

/* operations on 2 matrices */
K3API float* k3m_Mul(uint32_t s1_rows, uint32_t s2_rows, uint32_t s2_cols, float* d, const float* s1, const float* s2);

/* shortcuts to normalize vector */
inline float* k3v_Normalize(uint32_t l, float* d) {
    float f = k3v_Length((l), (d));
    f = (f == 0.0f) ? 0.0f : (1.0f / f);
    return k3sv_Mul((l), (d), f, (d));
}

/* shortcuts for vector to scalar operations */
inline float* k3vs_Add(uint32_t l, float* d, const float* s1, float s2) { return k3sv_Add((l), (d), (s2), (s1)); }
inline float* k3vs_Sub(uint32_t l, float* d, const float* s1, float s2) { return k3sv_Add((l), (d), -(s2), (s1)); }
inline float* k3vs_Mul(uint32_t l, float* d, const float* s1, float s2) { return k3sv_Mul((l), (d), (s2), (s1)); }
inline float* k3vs_Div(uint32_t l, float* d, const float* s1, float s2) { return k3sv_Mul((l), (d), 1.0f / (s2), (s1)); }

/* shortcut for not equals */
inline bool k3v_NotEquals(uint32_t l, const float* s1, const float* s2) { return (!k3v_Equals(l, s1, s2)); }

/* shortcuts for vectors of length 2 through 4  operations */
/* also shortcuts for 2x2, 3x3, and 4x4 matrices */
inline float* k3v2_Negate(float* d) { return k3v_Negate(  2, (d) ); }
inline float* k3v3_Negate(float* d) { return k3v_Negate(  3, (d) ); }
inline float* k3v4_Negate(float* d) { return k3v_Negate(  4, (d) ); }
inline float* k3m2_Negate(float* d) { return k3v_Negate(  4, (d) ); }
inline float* k3m3_Negate(float* d) { return k3v_Negate(  9, (d) ); }
inline float* k3m4_Negate(float* d) { return k3v_Negate( 16, (d) ); }

inline float* k3v2_Normalize(float* d) { return k3v_Normalize( 2, (d) ); }
inline float* k3v3_Normalize(float* d) { return k3v_Normalize( 3, (d) ); }
inline float* k3v4_Normalize(float* d) { return k3v_Normalize( 4, (d) ); }

inline float* k3v2_Swizzle(float* d, const uint32_t* i) { return k3v_Swizzle( 2, (d), (i) ); }
inline float* k3v3_Swizzle(float* d, const uint32_t* i) { return k3v_Swizzle( 3, (d), (i) ); }
inline float* k3v4_Swizzle(float* d, const uint32_t* i) { return k3v_Swizzle( 4, (d), (i) ); }
inline float* k3m2_Swizzle(float* d, const uint32_t* r, const uint32_t* c) { return k3m_Swizzle( 2, 2, (d), (r), (c) ); }
inline float* k3m3_Swizzle(float* d, const uint32_t* r, const uint32_t* c) { return k3m_Swizzle( 3, 3, (d), (r), (c) ); }
inline float* k3m4_Swizzle(float* d, const uint32_t* r, const uint32_t* c) { return k3m_Swizzle( 4, 4, (d), (r), (c) ); }

inline float k3v2_Length(const float* s) { return k3v_Length( 2, (s) ); }
inline float k3v3_Length(const float* s) { return k3v_Length( 3, (s) ); }
inline float k3v4_Length(const float* s) { return k3v_Length( 4, (s) ); }

inline float* k3v2_Add(float* d, const float* s1, const float* s2) { return k3v_Add(  2, (d), (s1), (s2) ); }
inline float* k3v3_Add(float* d, const float* s1, const float* s2) { return k3v_Add(  3, (d), (s1), (s2) ); }
inline float* k3v4_Add(float* d, const float* s1, const float* s2) { return k3v_Add(  4, (d), (s1), (s2) ); }
inline float* k3m2_Add(float* d, const float* s1, const float* s2) { return k3v_Add(  4, (d), (s1), (s2) ); }
inline float* k3m3_Add(float* d, const float* s1, const float* s2) { return k3v_Add(  9, (d), (s1), (s2) ); }
inline float* k3m4_Add(float* d, const float* s1, const float* s2) { return k3v_Add( 16, (d), (s1), (s2) ); }
inline float* k3v2_Sub(float* d, const float* s1, const float* s2) { return k3v_Sub(  2, (d), (s1), (s2) ); }
inline float* k3v3_Sub(float* d, const float* s1, const float* s2) { return k3v_Sub(  3, (d), (s1), (s2) ); }
inline float* k3v4_Sub(float* d, const float* s1, const float* s2) { return k3v_Sub(  4, (d), (s1), (s2) ); }
inline float* k3m2_Sub(float* d, const float* s1, const float* s2) { return k3v_Sub(  4, (d), (s1), (s2) ); }
inline float* k3m3_Sub(float* d, const float* s1, const float* s2) { return k3v_Sub(  9, (d), (s1), (s2) ); }
inline float* k3m4_Sub(float* d, const float* s1, const float* s2) { return k3v_Sub( 16, (d), (s1), (s2) ); }
inline float* k3v2_Mul(float* d, const float* s1, const float* s2) { return k3v_Mul(  2, (d), (s1), (s2) ); }
inline float* k3v3_Mul(float* d, const float* s1, const float* s2) { return k3v_Mul(  3, (d), (s1), (s2) ); }
inline float* k3v4_Mul(float* d, const float* s1, const float* s2) { return k3v_Mul(  4, (d), (s1), (s2) ); }
inline float* k3m2_ComponentMul(float* d, const float* s1, const float* s2) { return k3v_Mul(  4, (d), (s1), (s2) ); }
inline float* k3m3_ComponentMul(float* d, const float* s1, const float* s2) { return k3v_Mul(  9, (d), (s1), (s2) ); }
inline float* k3m4_ComponentMul(float* d, const float* s1, const float* s2) { return k3v_Mul( 16, (d), (s1), (s2) ); }
inline float* k3v2_Div(float* d, const float* s1, const float* s2) { return k3v_Div(  2, (d), (s1), (s2) ); }
inline float* k3v3_Div(float* d, const float* s1, const float* s2) { return k3v_Div(  3, (d), (s1), (s2) ); }
inline float* k3v4_Div(float* d, const float* s1, const float* s2) { return k3v_Div(  4, (d), (s1), (s2) ); }
inline float* k3m2_Div(float* d, const float* s1, const float* s2) { return k3v_Div(  4, (d), (s1), (s2) ); }
inline float* k3m3_Div(float* d, const float* s1, const float* s2) { return k3v_Div(  9, (d), (s1), (s2) ); }
inline float* k3m4_Div(float* d, const float* s1, const float* s2) { return k3v_Div( 16, (d), (s1), (s2) ); }

inline float* k3sv2_Add(float* d, const float s1, const float* s2) { return k3sv_Add(  2, (d), (s1), (s2) ); }
inline float* k3sv3_Add(float* d, const float s1, const float* s2) { return k3sv_Add(  3, (d), (s1), (s2) ); }
inline float* k3sv4_Add(float* d, const float s1, const float* s2) { return k3sv_Add(  4, (d), (s1), (s2) ); }
inline float* k3sm2_Add(float* d, const float s1, const float* s2) { return k3sv_Add(  4, (d), (s1), (s2) ); }
inline float* k3sm3_Add(float* d, const float s1, const float* s2) { return k3sv_Add(  9, (d), (s1), (s2) ); }
inline float* k3sm4_Add(float* d, const float s1, const float* s2) { return k3sv_Add( 16, (d), (s1), (s2) ); }
inline float* k3sv2_Sub(float* d, const float s1, const float* s2) { return k3sv_Sub(  2, (d), (s1), (s2) ); }
inline float* k3sv3_Sub(float* d, const float s1, const float* s2) { return k3sv_Sub(  3, (d), (s1), (s2) ); }
inline float* k3sv4_Sub(float* d, const float s1, const float* s2) { return k3sv_Sub(  4, (d), (s1), (s2) ); }
inline float* k3sm2_Sub(float* d, const float s1, const float* s2) { return k3sv_Sub(  4, (d), (s1), (s2) ); }
inline float* k3sm3_Sub(float* d, const float s1, const float* s2) { return k3sv_Sub(  9, (d), (s1), (s2) ); }
inline float* k3sm4_Sub(float* d, const float s1, const float* s2) { return k3sv_Sub( 16, (d), (s1), (s2) ); }
inline float* k3sv2_Mul(float* d, const float s1, const float* s2) { return k3sv_Mul(  2, (d), (s1), (s2) ); }
inline float* k3sv3_Mul(float* d, const float s1, const float* s2) { return k3sv_Mul(  3, (d), (s1), (s2) ); }
inline float* k3sv4_Mul(float* d, const float s1, const float* s2) { return k3sv_Mul(  4, (d), (s1), (s2) ); }
inline float* k3sm2_Mul(float* d, const float s1, const float* s2) { return k3sv_Mul(  4, (d), (s1), (s2) ); }
inline float* k3sm3_Mul(float* d, const float s1, const float* s2) { return k3sv_Mul(  9, (d), (s1), (s2) ); }
inline float* k3sm4_Mul(float* d, const float s1, const float* s2) { return k3sv_Mul( 16, (d), (s1), (s2) ); }
inline float* k3sv2_Div(float* d, const float s1, const float* s2) { return k3sv_Div(  2, (d), (s1), (s2) ); }
inline float* k3sv3_Div(float* d, const float s1, const float* s2) { return k3sv_Div(  3, (d), (s1), (s2) ); }
inline float* k3sv4_Div(float* d, const float s1, const float* s2) { return k3sv_Div(  4, (d), (s1), (s2) ); }
inline float* k3sm2_Div(float* d, const float s1, const float* s2) { return k3sv_Div(  4, (d), (s1), (s2) ); }
inline float* k3sm3_Div(float* d, const float s1, const float* s2) { return k3sv_Div(  9, (d), (s1), (s2) ); }
inline float* k3sm4_Div(float* d, const float s1, const float* s2) { return k3sv_Div( 16, (d), (s1), (s2) ); }

inline float* k3v2s_Add(float* d, const float* s1, const float s2) { return k3vs_Add(  2, (d), (s1), (s2) ); }
inline float* k3v3s_Add(float* d, const float* s1, const float s2) { return k3vs_Add(  3, (d), (s1), (s2) ); }
inline float* k3v4s_Add(float* d, const float* s1, const float s2) { return k3vs_Add(  4, (d), (s1), (s2) ); }
inline float* k3m2s_Add(float* d, const float* s1, const float s2) { return k3vs_Add(  4, (d), (s1), (s2) ); }
inline float* k3m3s_Add(float* d, const float* s1, const float s2) { return k3vs_Add(  9, (d), (s1), (s2) ); }
inline float* k3m4s_Add(float* d, const float* s1, const float s2) { return k3vs_Add( 16, (d), (s1), (s2) ); }
inline float* k3v2s_Sub(float* d, const float* s1, const float s2) { return k3vs_Sub(  2, (d), (s1), (s2) ); }
inline float* k3v3s_Sub(float* d, const float* s1, const float s2) { return k3vs_Sub(  3, (d), (s1), (s2) ); }
inline float* k3v4s_Sub(float* d, const float* s1, const float s2) { return k3vs_Sub(  4, (d), (s1), (s2) ); }
inline float* k3m2s_Sub(float* d, const float* s1, const float s2) { return k3vs_Sub(  4, (d), (s1), (s2) ); }
inline float* k3m3s_Sub(float* d, const float* s1, const float s2) { return k3vs_Sub(  9, (d), (s1), (s2) ); }
inline float* k3m4s_Sub(float* d, const float* s1, const float s2) { return k3vs_Sub( 16, (d), (s1), (s2) ); }
inline float* k3v2s_Mul(float* d, const float* s1, const float s2) { return k3vs_Mul(  2, (d), (s1), (s2) ); }
inline float* k3v3s_Mul(float* d, const float* s1, const float s2) { return k3vs_Mul(  3, (d), (s1), (s2) ); }
inline float* k3v4s_Mul(float* d, const float* s1, const float s2) { return k3vs_Mul(  4, (d), (s1), (s2) ); }
inline float* k3m2s_Mul(float* d, const float* s1, const float s2) { return k3vs_Mul(  4, (d), (s1), (s2) ); }
inline float* k3m3s_Mul(float* d, const float* s1, const float s2) { return k3vs_Mul(  9, (d), (s1), (s2) ); }
inline float* k3m4s_Mul(float* d, const float* s1, const float s2) { return k3vs_Mul( 16, (d), (s1), (s2) ); }
inline float* k3v2s_Div(float* d, const float* s1, const float s2) { return k3vs_Div(  2, (d), (s1), (s2) ); }
inline float* k3v3s_Div(float* d, const float* s1, const float s2) { return k3vs_Div(  3, (d), (s1), (s2) ); }
inline float* k3v4s_Div(float* d, const float* s1, const float s2) { return k3vs_Div(  4, (d), (s1), (s2) ); }
inline float* k3m2s_Div(float* d, const float* s1, const float s2) { return k3vs_Div(  4, (d), (s1), (s2) ); }
inline float* k3m3s_Div(float* d, const float* s1, const float s2) { return k3vs_Div(  9, (d), (s1), (s2) ); }
inline float* k3m4s_Div(float* d, const float* s1, const float s2) { return k3vs_Div( 16, (d), (s1), (s2) ); }

inline float* k3v2_Cross(float* d, const float* s1, const float* s2) { return k3v_Cross( 2, (d), (s1), (s2) ); }
inline float* k3v3_Cross(float* d, const float* s1, const float* s2) { return k3v_Cross( 3, (d), (s1), (s2) ); }

inline bool k3v2_Equals(const float* s1, const float* s2) { return k3v_Equals(  2, (s1), (s2) ); }
inline bool k3v3_Equals(const float* s1, const float* s2) { return k3v_Equals(  3, (s1), (s2) ); }
inline bool k3v4_Equals(const float* s1, const float* s2) { return k3v_Equals(  4, (s1), (s2) ); }
inline bool k3m2_Equals(const float* s1, const float* s2) { return k3v_Equals(  4, (s1), (s2) ); }
inline bool k3m3_Equals(const float* s1, const float* s2) { return k3v_Equals(  9, (s1), (s2) ); }
inline bool k3m4_Equals(const float* s1, const float* s2) { return k3v_Equals( 16, (s1), (s2) ); }
inline bool k3v2_NotEquals(const float* s1, const float* s2) { return k3v_NotEquals(  2, (s1), (s2) ); }
inline bool k3v3_NotEquals(const float* s1, const float* s2) { return k3v_NotEquals(  3, (s1), (s2) ); }
inline bool k3v4_NotEquals(const float* s1, const float* s2) { return k3v_NotEquals(  4, (s1), (s2) ); }
inline bool k3m2_NotEquals(const float* s1, const float* s2) { return k3v_NotEquals(  4, (s1), (s2) ); }
inline bool k3m3_NotEquals(const float* s1, const float* s2) { return k3v_NotEquals(  9, (s1), (s2) ); }
inline bool k3m4_NotEquals(const float* s1, const float* s2) { return k3v_NotEquals( 16, (s1), (s2) ); }

inline float k3v2_Dot(const float* s1, const float* s2) { return k3v_Dot( 2, (s1), (s2) ); }
inline float k3v3_Dot(const float* s1, const float* s2) { return k3v_Dot( 3, (s1), (s2) ); }
inline float k3v4_Dot(const float* s1, const float* s2) { return k3v_Dot( 4, (s1), (s2) ); }

inline float* k3v2_Min(float* d, const float* s1, const float* s2) { return k3v_Min( 2, (d), (s1), (s2) ); }
inline float* k3v3_Min(float* d, const float* s1, const float* s2) { return k3v_Min( 3, (d), (s1), (s2) ); }
inline float* k3v4_Min(float* d, const float* s1, const float* s2) { return k3v_Min( 4, (d), (s1), (s2) ); }
inline float* k3v2_Max(float* d, const float* s1, const float* s2) { return k3v_Max( 2, (d), (s1), (s2) ); }
inline float* k3v3_Max(float* d, const float* s1, const float* s2) { return k3v_Max( 3, (d), (s1), (s2) ); }
inline float* k3v4_Max(float* d, const float* s1, const float* s2) { return k3v_Max( 4, (d), (s1), (s2) ); }

inline float k3m2_Determinant(const float* s1) { return k3m_Determinant( 2, (s1) ); }
inline float k3m3_Determinant(const float* s1) { return k3m_Determinant( 3, (s1) ); }
inline float k3m4_Determinant(const float* s1) { return k3m_Determinant( 4, (s1) ); }
inline float* k3m2_Transpose(float* d) { return k3m_Transpose( 2, 2, (d) ); }
inline float* k3m3_Transpose(float* d) { return k3m_Transpose( 3, 3, (d) ); }
inline float* k3m4_Transpose(float* d) { return k3m_Transpose( 4, 4, (d) ); }
inline float* k3m2_Inverse(float* d) { return k3m_Inverse( 2, (d) ); }
inline float* k3m3_Inverse(float* d) { return k3m_Inverse( 3, (d) ); }
inline float* k3m4_Inverse(float* d) { return k3m_Inverse( 4, (d) ); }
inline float* k3m2_SetIdentity(float* d) { return k3m_SetIdentity( 2, (d) ); }
inline float* k3m3_SetIdentity(float* d) { return k3m_SetIdentity( 3, (d) ); }
inline float* k3m4_SetIdentity(float* d) { return k3m_SetIdentity( 4, (d) ); }
inline float* k3m2_SetRotation(float* d, float a)                 { return k3m_SetRotation( 2, (d), (a), NULL ); }
inline float* k3m3_SetRotation(float* d, float a, const float* x) { return k3m_SetRotation( 3, (d), (a), (x)  ); }
inline float* k3m4_SetRotation(float* d, float a, const float* x) { return k3m_SetRotation( 4, (d), (a), (x)  ); }

inline float* k3m4_SetPerspectiveOffCenterLH(float* d, float l, float r, float b, float t, float n, float f, bool s) { return k3m4_SetPerspectiveOffCenter( (d), (l), (r), (b), (t), (n), (f), true, (s), false  ); }
inline float* k3m4_SetPerspectiveOffCenterRH(float* d, float l, float r, float b, float t, float n, float f, bool s) { return k3m4_SetPerspectiveOffCenter( (d), (l), (r), (b), (t), (n), (f), false, (s), false); }
inline float* k3m4_SetPerspectiveLH(float* d, float w, float h, float n, float f, bool s) { return k3m4_SetPerspectiveOffCenter( (d), -(w)/2.0f, (w)/2.0f, -(h)/2.0f, (h)/2.0f, (n), (f), true, (s), false); }
inline float* k3m4_SetPerspectiveRH(float* d, float w, float h, float n, float f, bool s) { return k3m4_SetPerspectiveOffCenter( (d), -(w)/2.0f, (w)/2.0f, -(h)/2.0f, (h)/2.0f, (n), (f), false, (s), false); }
inline float* k3m4_SetPerspectiveFovLH(float* d, float fy, float a, float n, float f, bool s) { return k3m4_SetPerspectiveFov( (d), (fy), (a), (n), (f), true, (s), false); }
inline float* k3m4_SetPerspectiveFovRH(float* d, float fy, float a, float n, float f, bool s) { return k3m4_SetPerspectiveFov( (d), (fy), (a), (n), (f), false, (s), false); }
inline float* k3m4_SetPerspectiveOffCenterRevZLH(float* d, float l, float r, float b, float t, float n, float f, bool s) { return k3m4_SetPerspectiveOffCenter((d), (l), (r), (b), (t), (n), (f), true, (s), true); }
inline float* k3m4_SetPerspectiveOffCenterRevZRH(float* d, float l, float r, float b, float t, float n, float f, bool s) { return k3m4_SetPerspectiveOffCenter((d), (l), (r), (b), (t), (n), (f), false, (s), true); }
inline float* k3m4_SetPerspectiveRevZLH(float* d, float w, float h, float n, float f, bool s) { return k3m4_SetPerspectiveOffCenter((d), -(w) / 2.0f, (w) / 2.0f, -(h) / 2.0f, (h) / 2.0f, (n), (f), true, (s), true); }
inline float* k3m4_SetPerspectiveRevZRH(float* d, float w, float h, float n, float f, bool s) { return k3m4_SetPerspectiveOffCenter((d), -(w) / 2.0f, (w) / 2.0f, -(h) / 2.0f, (h) / 2.0f, (n), (f), false, (s), true); }
inline float* k3m4_SetPerspectiveFovRevZLH(float* d, float fy, float a, float n, float f, bool s) { return k3m4_SetPerspectiveFov((d), (fy), (a), (n), (f), true, (s), true); }
inline float* k3m4_SetPerspectiveFovRevZRH(float* d, float fy, float a, float n, float f, bool s) { return k3m4_SetPerspectiveFov((d), (fy), (a), (n), (f), false, (s), true); }
inline float* k3m4_SetOrthoOffCenterLH(float* d, float l, float r, float b, float t, float n, float f, bool s) { return k3m4_SetOrthoOffCenter( (d), (l), (r), (b), (t), (n), (f), true, (s)  ); }
inline float* k3m4_SetOrthoOffCenterRH(float* d, float l, float r, float b, float t, float n, float f, bool s) { return k3m4_SetOrthoOffCenter( (d), (l), (r), (b), (t), (n), (f), false, (s) ); }
inline float* k3m4_SetOrthLH(float* d, float w, float h, float n, float f, bool s) { return k3m4_SetOrthoOffCenter( (d), -(w)/2.0f, (w)/2.0f, -(h)/2.0f, (h)/2.0f, (n), (f), true, (s)  ); }
inline float* k3m4_SetOrthRH(float* d, float w, float h, float n, float f, bool s) { return k3m4_SetOrthoOffCenter( (d), -(w)/2.0f, (w)/2.0f, -(h)/2.0f, (h)/2.0f, (n), (f), false, (s) ); }

inline float* k3m4_SetDXPerspectiveOffCenterLH(float* d, float l, float r, float b, float t, float n, float f) { return k3m4_SetPerspectiveOffCenter( (d), (l), (r), (b), (t), (n), (f), true, true, false); }
inline float* k3m4_SetDXPerspectiveOffCenterRH(float* d, float l, float r, float b, float t, float n, float f) { return k3m4_SetPerspectiveOffCenter( (d), (l), (r), (b), (t), (n), (f), false, true, false); }
inline float* k3m4_SetDXPerspectiveLH(float* d, float w, float h, float n, float f) { return k3m4_SetPerspectiveOffCenter( (d), -(w)/2.0f, (w)/2.0f, -(h)/2.0f, (h)/2.0f, (n), (f), true, true, false); }
inline float* k3m4_SetDXPerspectiveRH(float* d, float w, float h, float n, float f) { return k3m4_SetPerspectiveOffCenter( (d), -(w)/2.0f, (w)/2.0f, -(h)/2.0f, (h)/2.0f, (n), (f), false, true, false); }
inline float* k3m4_SetDXPerspectiveFovLH(float* d, float fy, float a, float n, float f) { return k3m4_SetPerspectiveFov( (d), (fy), (a), (n), (f), true, true, false); }
inline float* k3m4_SetDXPerspectiveFovRH(float* d, float fy, float a, float n, float f) { return k3m4_SetPerspectiveFov( (d), (fy), (a), (n), (f), false, true, false); }
inline float* k3m4_SetDXPerspectiveOffCenterRevZLH(float* d, float l, float r, float b, float t, float n, float f) { return k3m4_SetPerspectiveOffCenter((d), (l), (r), (b), (t), (n), (f), true, true, true); }
inline float* k3m4_SetDXPerspectiveOffCenterRevZRH(float* d, float l, float r, float b, float t, float n, float f) { return k3m4_SetPerspectiveOffCenter((d), (l), (r), (b), (t), (n), (f), false, true, true); }
inline float* k3m4_SetDXPerspectiveRevZLH(float* d, float w, float h, float n, float f) { return k3m4_SetPerspectiveOffCenter((d), -(w) / 2.0f, (w) / 2.0f, -(h) / 2.0f, (h) / 2.0f, (n), (f), true, true, true); }
inline float* k3m4_SetDXPerspectiveRevZRH(float* d, float w, float h, float n, float f) { return k3m4_SetPerspectiveOffCenter((d), -(w) / 2.0f, (w) / 2.0f, -(h) / 2.0f, (h) / 2.0f, (n), (f), false, true, true); }
inline float* k3m4_SetDXPerspectiveFovRevZLH(float* d, float fy, float a, float n, float f) { return k3m4_SetPerspectiveFov((d), (fy), (a), (n), (f), true, true, true); }
inline float* k3m4_SetDXPerspectiveFovRevZRH(float* d, float fy, float a, float n, float f) { return k3m4_SetPerspectiveFov((d), (fy), (a), (n), (f), false, true, true); }
inline float* k3m4_SetDXOrthoOffCenterLH(float* d, float l, float r, float b, float t, float n, float f) { return k3m4_SetOrthoOffCenter( (d), (l), (r), (b), (t), (n), (f), true, true  ); }
inline float* k3m4_SetDXOrthoOffCenterRH(float* d, float l, float r, float b, float t, float n, float f) { return k3m4_SetOrthoOffCenter( (d), (l), (r), (b), (t), (n), (f), false, true ); }
inline float* k3m4_SetDXOrthLH(float* d, float w, float h, float n, float f) { return k3m4_SetOrthoOffCenter( (d), -(w)/2.0f, (w)/2.0f, -(h)/2.0f, (h)/2.0f, (n), (f), true, true  ); }
inline float* k3m4_SetDXOrthRH(float* d, float w, float h, float n, float f) { return k3m4_SetOrthoOffCenter( (d), -(w)/2.0f, (w)/2.0f, -(h)/2.0f, (h)/2.0f, (n), (f), false, true ); }

inline float* k3m4_SetGLPerspectiveOffCenterLH(float* d, float l, float r, float b, float t, float n, float f) { return k3m4_SetPerspectiveOffCenter( (d), (l), (r), (b), (t), (n), (f), true, false, false); }
inline float* k3m4_SetGLPerspectiveOffCenterRH(float* d, float l, float r, float b, float t, float n, float f) { return k3m4_SetPerspectiveOffCenter( (d), (l), (r), (b), (t), (n), (f), false, false, false); }
inline float* k3m4_SetGLPerspectiveLH(float* d, float w, float h, float n, float f) { return k3m4_SetPerspectiveOffCenter( (d), -(w)/2.0f, (w)/2.0f, -(h)/2.0f, (h)/2.0f, (n), (f), true, false, false); }
inline float* k3m4_SetGLPerspectiveRH(float* d, float w, float h, float n, float f) { return k3m4_SetPerspectiveOffCenter( (d), -(w)/2.0f, (w)/2.0f, -(h)/2.0f, (h)/2.0f, (n), (f), false, false, false); }
inline float* k3m4_SetGLPerspectiveFovLH(float* d, float fy, float a, float n, float f) { return k3m4_SetPerspectiveFov( (d), (fy), (a), (n), (f), true, false, false); }
inline float* k3m4_SetGLPerspectiveFovRH(float* d, float fy, float a, float n, float f) { return k3m4_SetPerspectiveFov( (d), (fy), (a), (n), (f), false, false, false); }
inline float* k3m4_SetGLPerspectiveOffCenterRevZLH(float* d, float l, float r, float b, float t, float n, float f) { return k3m4_SetPerspectiveOffCenter((d), (l), (r), (b), (t), (n), (f), true, false, true); }
inline float* k3m4_SetGLPerspectiveOffCenterRevZRH(float* d, float l, float r, float b, float t, float n, float f) { return k3m4_SetPerspectiveOffCenter((d), (l), (r), (b), (t), (n), (f), false, false, true); }
inline float* k3m4_SetGLPerspectiveRevZLH(float* d, float w, float h, float n, float f) { return k3m4_SetPerspectiveOffCenter((d), -(w) / 2.0f, (w) / 2.0f, -(h) / 2.0f, (h) / 2.0f, (n), (f), true, false, true); }
inline float* k3m4_SetGLPerspectiveRevZRH(float* d, float w, float h, float n, float f) { return k3m4_SetPerspectiveOffCenter((d), -(w) / 2.0f, (w) / 2.0f, -(h) / 2.0f, (h) / 2.0f, (n), (f), false, false, true); }
inline float* k3m4_SetGLPerspectiveFovRevZLH(float* d, float fy, float a, float n, float f) { return k3m4_SetPerspectiveFov((d), (fy), (a), (n), (f), true, false, true); }
inline float* k3m4_SetGLPerspectiveFovRevZRH(float* d, float fy, float a, float n, float f) { return k3m4_SetPerspectiveFov((d), (fy), (a), (n), (f), false, false, true); }
inline float* k3m4_SetGLOrthoOffCenterLH(float* d, float l, float r, float b, float t, float n, float f) { return k3m4_SetOrthoOffCenter( (d), (l), (r), (b), (t), (n), (f), true, false  ); }
inline float* k3m4_SetGLOrthoOffCenterRH(float* d, float l, float r, float b, float t, float n, float f) { return k3m4_SetOrthoOffCenter( (d), (l), (r), (b), (t), (n), (f), false, false ); }
inline float* k3m4_SetGLOrthLH(float* d, float w, float h, float n, float f) { return k3m4_SetOrthoOffCenter( (d), -(w)/2.0f, (w)/2.0f, -(h)/2.0f, (h)/2.0f, (n), (f), true, false  ); }
inline float* k3m4_SetGLOrthRH(float* d, float w, float h, float n, float f) { return k3m4_SetOrthoOffCenter( (d), -(w)/2.0f, (w)/2.0f, -(h)/2.0f, (h)/2.0f, (n), (f), false, false ); }

inline float* k3m4_SetLookAtLH(float* d, const float* e, const float* a, const float* u) { return k3m4_SetLookAt( (d), (e), (a), (u), true  ); }
inline float* k3m4_SetLookAtRH(float* d, const float* e, const float* a, const float* u) { return k3m4_SetLookAt( (d), (e), (a), (u), false ); }

inline float* k3m2_Mul(float* d, const float* s1, const float* s2) { return k3m_Mul( 2, 2, 2, (d), (s1), (s2) ); }
inline float* k3m3_Mul(float* d, const float* s1, const float* s2) { return k3m_Mul( 3, 3, 3, (d), (s1), (s2) ); }
inline float* k3m4_Mul(float* d, const float* s1, const float* s2) { return k3m_Mul( 4, 4, 4, (d), (s1), (s2) ); }

inline float* k3vm2_Mul(float* d, const float* s1, const float* s2) { return k3m_Mul( 1, 2, 2, (d), (s1), (s2) ); }
inline float* k3vm3_Mul(float* d, const float* s1, const float* s2) { return k3m_Mul( 1, 3, 3, (d), (s1), (s2) ); }
inline float* k3vm4_Mul(float* d, const float* s1, const float* s2) { return k3m_Mul( 1, 4, 4, (d), (s1), (s2) ); }

inline float* k3mv2_Mul(float* d, const float* s1, const float* s2) { return k3m_Mul( 2, 2, 1, (d), (s1), (s2) ); }
inline float* k3mv3_Mul(float* d, const float* s1, const float* s2) { return k3m_Mul( 3, 3, 1, (d), (s1), (s2) ); }
inline float* k3mv4_Mul(float* d, const float* s1, const float* s2) { return k3m_Mul( 4, 4, 1, (d), (s1), (s2) ); }

/* Quaternion converstion functions */
K3API float* k3m_QuatToMat(uint32_t cols, float* d, const float* s);
K3API float* k3m_MatToQuat(uint32_t cols, float* d, const float* s);
K3API float* k3v4_SetQuatRotation(float* d, float angle, const float* axis);
K3API float k3v3_GetQuatRotation(float* axis, float* angle, const float* quat);
K3API float* k3v4_SetQuatEuler(float* d, const float* angles);
K3API float* k3v3_GetQuatEuler(float* d, const float* quat);

inline float* k3m3_QuatToMat(float* d, const float* s) { return k3m_QuatToMat(3, d, s); }
inline float* k3m4_QuatToMat(float* d, const float* s) { return k3m_QuatToMat(4, d, s); }
inline float* k3m3_MatToQuat(float* d, const float* s) { return k3m_MatToQuat(3, d, s); }
inline float* k3m4_MatToQuat(float* d, const float* s) { return k3m_MatToQuat(4, d, s); }

// ------------------------------------------------------------
// k3 image classes

#define FOURCC4(A,B,C,D) ( (A) | ((B) << 8) | ((C) << 16) | ((D) << 24) )

#define FOURCC(cc) ( (static_cast<uint32_t>((cc)[0])) |                 \
                     (static_cast<uint32_t>((cc)[1]) << 8) |            \
                     (static_cast<uint32_t>((cc)[2]) << 16) |           \
                     (static_cast<uint32_t>((cc)[3]) << 24) )

// Compressed formats
const uint32_t FOURCC_DXT1 = FOURCC4('D', 'X', 'T', '1');
const uint32_t FOURCC_DXT3 = FOURCC4('D', 'X', 'T', '3');
const uint32_t FOURCC_DXT5 = FOURCC4('D', 'X', 'T', '5');
const uint32_t FOURCC_ATI1 = FOURCC4('A', 'T', 'I', '1');
const uint32_t FOURCC_ATI2 = FOURCC4('A', 'T', 'I', '2');
const uint32_t FOURCC_DX10 = FOURCC4('D', 'X', '1', '0');

// Float formats
const uint32_t FOURCC_RGBA16U = 36;
const uint32_t FOURCC_R16F    = 111;
const uint32_t FOURCC_RG16F   = 112;
const uint32_t FOURCC_RGBA16F = 113;
const uint32_t FOURCC_R32F    = 114;
const uint32_t FOURCC_RG32F   = 115;
const uint32_t FOURCC_RGBA32F = 116;

enum class k3fmt {
    UNKNOWN,
    // 4 component
    RGBA8_UNORM, BGRA8_UNORM,
    RGBX8_UNORM, BGRX8_UNORM,
    RGBA16_UNORM, RGBA16_FLOAT,
    RGBA32_UNORM, RGBA32_FLOAT,
    RGB10A2_UNORM, BGR5A1_UNORM,
    RGBA32_UINT, RGBA16_UINT,
    // 3 component
    RGB8_UNORM, BGR8_UNORM,
    RGB32_FLOAT, B5G6R5_UNORM,
    RGB32_UINT,
    // 2 component
    RG8_UNORM, RG16_UNORM,
    RG16_FLOAT, RG32_UNORM,
    RG32_FLOAT,
    RG32_UINT, RG16_UINT,
    // 1 component
    R8_UNORM, A8_UNORM,
    R16_UNORM, R16_FLOAT,
    R32_UNORM, R32_FLOAT,
    R32_UINT, R16_UINT,
    // Compressed formats
    BC1_UNORM, BC2_UNORM,
    BC3_UNORM, BC4_UNORM,
    BC5_UNORM, BC6_UNORM,
    BC7_UNORM,
    // Depth/Stecnil formats
    D16_UNORM, D24X8_UNORM,
    D24_UNORM_S8_UINT, D32_FLOAT,
    D32_FLOAT_S8X24_UINT,
    // Shared exponent
    RGB9E5_FLOAT
};

enum class k3component {
    NONE, RED, GREEN, BLUE, ALPHA, DEPTH, STENCIL
};

enum class k3texAddr { WRAP, MIRROR, CLAMP, MIRROR_ONCE };

typedef void (K3CALLBACK* k3image_file_handler_loadheaderinfo_ptr)(FILE* file_handle, uint32_t* width, uint32_t* height,
    uint32_t* depth, k3fmt* format);

typedef void (K3CALLBACK* k3image_file_handler_loaddata_ptr)(FILE* file_handle, uint32_t pitch, uint32_t slice_pitch, void* data);

typedef void (K3CALLBACK* k3image_file_handler_savedata_ptr)(FILE* file_handle, uint32_t width, uint32_t height,
    uint32_t depth, uint32_t pitch, uint32_t slice_pitch, k3fmt format, const void* data);

struct k3image_file_handler_t
{
    k3image_file_handler_loadheaderinfo_ptr LoadHeaderInfo;
    k3image_file_handler_loaddata_ptr LoadData;
    k3image_file_handler_savedata_ptr SaveData;
};

struct k3DXT1Block {
    uint16_t color0;
    uint16_t color1;
    uint32_t pixmap;
};

struct k3DXT3Block {
    uint64_t alphas;
    k3DXT1Block dxt1;
};

struct k3ATI2NBlock {
    uint64_t reds;
    uint64_t greens;
};

class k3imageImpl;
class k3imageObj;
typedef k3ptr<k3imageObj> k3image;
class k3imageObj : public k3obj
{
protected:
    static const uint32_t MAX_FILE_HANDLERS = 8;
    static uint32_t _num_file_handlers;
    static k3image_file_handler_t* _fh[MAX_FILE_HANDLERS];
    k3imageImpl* _data;

    k3imageObj();

public:
    static const uint32_t FILE_HANDLER_DDS = 0;
    static const uint32_t FILE_HANDLER_PNG = 1;
    static const uint32_t FILE_HANDLER_JPG = 2;

    virtual K3API k3objType getObjType() const
    {
        return k3objType::IMAGE;
    }

    static K3API uint32_t AddImageFileHandler(k3image_file_handler_t* fh);
    static K3API void RemoveImageFileHandler(k3image_file_handler_t* fh);
    static K3API k3image Create();

    static K3API void ReformatFromImage(k3image img, k3image src,
        uint32_t dest_width, uint32_t dest_height, uint32_t dest_depth,
        k3fmt dest_format, const float* transform,
        k3texAddr x_addr_mode, k3texAddr y_addr_mode, k3texAddr z_addr_mode);

    static void LoadFromImage(k3image img, k3image src)
    {
        ReformatFromImage((img), (src), 0, 0, 0, k3fmt::UNKNOWN, NULL, k3texAddr::CLAMP, k3texAddr::CLAMP, k3texAddr::CLAMP);
    }
    static void ReformatFromImage(k3image img, k3image src, uint32_t w, uint32_t h, uint32_t d, k3fmt f)
    {
        ReformatFromImage((img), (src), (w), (h), (d), (f), NULL, k3texAddr::CLAMP, k3texAddr::CLAMP, k3texAddr::CLAMP);
    }
    static void TransformFromImage(k3image img, k3image src, uint32_t w, uint32_t h, uint32_t d, k3fmt f, const float* t, k3texAddr xa, k3texAddr ya, k3texAddr za)
    {
        ReformatFromImage((img), (src), (w), (h), (d), (f), (t), (xa), (ya), (za));
    }

    static K3API void ReformatFromFile(k3image img, const char* file_name,
        uint32_t dest_width, uint32_t dest_height, uint32_t dest_depth,
        k3fmt dest_format, const float* transform,
        k3texAddr x_addr_mode, k3texAddr y_addr_mode, k3texAddr z_addr_mode);

    static K3API void ReformatFromFileHandle(k3image img, FILE* file_handle,
        uint32_t dest_width, uint32_t dest_height, uint32_t dest_depth,
        k3fmt dest_format, const float* transform,
        k3texAddr x_addr_mode, k3texAddr y_addr_mode, k3texAddr z_addr_mode);

    static void LoadFromFile(k3image img, const char* src)
    {
        ReformatFromFile((img), (src), 0, 0, 0, k3fmt::UNKNOWN, NULL, k3texAddr::CLAMP, k3texAddr::CLAMP, k3texAddr::CLAMP);
    }
    static void ReformatFromFile(k3image img, const char* src, uint32_t dw, uint32_t dh, uint32_t dd, k3fmt df)
    {
        ReformatFromFile((img), (src), (dw), (dh), (dd), (df), NULL, k3texAddr::CLAMP, k3texAddr::CLAMP, k3texAddr::CLAMP);
    }
    static void TransformFromFile(k3image img, const char* src, uint32_t dw, uint32_t dh, uint32_t dd, k3fmt df, const float* t, k3texAddr xa, k3texAddr ya, k3texAddr za)
    {
        ReformatFromFile((img), (src), (dw), (dh), (dd), (df), (t), (xa), (ya), (za));
    }

    static void LoadFromFileHandle(k3image img, FILE* src)
    {
        ReformatFromFileHandle((img), (src), 0, 0, 0, k3fmt::UNKNOWN, NULL, k3texAddr::CLAMP, k3texAddr::CLAMP, k3texAddr::CLAMP);
    }
    static void ReformatFromFileHandle(k3image img, FILE* src, uint32_t dw, uint32_t dh, uint32_t dd, k3fmt df)
    {
        ReformatFromFileHandle((img), (src), (dw), (dh), (dd), (df), NULL, k3texAddr::CLAMP, k3texAddr::CLAMP, k3texAddr::CLAMP);
    }
    static void TransformFromFileHandle(k3image img, FILE* src, uint32_t dw, uint32_t dh, uint32_t dd, k3fmt df, const float* t, k3texAddr xa, k3texAddr ya, k3texAddr za)
    {
        ReformatFromFileHandle((img), (src), (dw), (dh), (dd), (df), (t), (xa), (ya), (za));
    }

    static K3API void ReformatFromMemory(k3image img, uint32_t src_width, uint32_t src_height, uint32_t src_depth,
        uint32_t src_pitch, uint32_t src_slice_pitch,
        k3fmt src_format, const void* src_data,
        uint32_t dest_width, uint32_t dest_height, uint32_t dest_depth,
        k3fmt dest_format, const float* transform,
        k3texAddr x_addr_mode, k3texAddr y_addr_mode, k3texAddr z_addr_mode);

    static void LoadFromMemory(k3image img, uint32_t sw, uint32_t sh, uint32_t sd, uint32_t sp, uint32_t ssp, k3fmt sf, const void* sdata)
    {
        ReformatFromMemory((img), (sw), (sh), (sd), (sp), (ssp), (sf), (sdata), 0, 0, 0, k3fmt::UNKNOWN, NULL, k3texAddr::CLAMP, k3texAddr::CLAMP, k3texAddr::CLAMP);
    }
    static void ReformatFromMemory(k3image img, uint32_t sw, uint32_t sh, uint32_t sd, uint32_t sp, uint32_t ssp, k3fmt sf, const void* sdata, uint32_t dw, uint32_t dh, uint32_t dd, k3fmt df)
    {
        ReformatFromMemory((img), (sw), (sh), (sd), (sp), (ssp), (sf), (sdata), (dw), (dh), (dd), (df), NULL, k3texAddr::CLAMP, k3texAddr::CLAMP, k3texAddr::CLAMP);
    }
    static void TransformFromMemory(k3image img, uint32_t sw, uint32_t sh, uint32_t sd, uint32_t sp, uint32_t ssp, k3fmt sf, const void* sdata, uint32_t dw, uint32_t dh, uint32_t dd, k3fmt df, const float* t, k3texAddr xa, k3texAddr ya, k3texAddr za)
    {
        ReformatFromMemory((img), (sw), (sh), (sd), (sp), (ssp), (sf), (sdata), (dw), (dh), (dd), (df), (t), (xa), (ya), (za));
    }

    virtual ~k3imageObj();

    K3API void SetDimensions(uint32_t width, uint32_t height, uint32_t depth, k3fmt format);
    K3API uint32_t GetWidth() const;
    K3API uint32_t GetHeight() const;
    K3API uint32_t GetDepth() const;
    K3API k3fmt GetFormat() const;
    K3API uint32_t GetPitch() const;
    K3API uint32_t GetSlicePitch() const;

    K3API void SaveToFile(const char* filename, uint32_t fh_index);
    virtual K3API const void* MapForRead();
    virtual K3API void* MapForWrite();
    virtual K3API void Unmap();
    K3API void SampleImage(float* color,
        float x, float y, float z,
        k3texAddr x_addr_mode, k3texAddr y_addr_mode, k3texAddr z_addr_mode);

    void SampleImage1D(float* c, float x, k3texAddr ax) { SampleImage((c), (x), 0.0f, 0.0f, (ax), k3texAddr::CLAMP, k3texAddr::CLAMP); }
    void SampleImage2D(float* c, float x, float y, k3texAddr ax, k3texAddr ay) { SampleImage((c), (x), (y), 0.0f, (ax), (ay), k3texAddr::CLAMP); }
    void SampleImage3D(float* c, float x, float y, float z, k3texAddr ax, k3texAddr ay, k3texAddr az) { SampleImage((c), (x), (y), (z), (ax), (ay), (az)); }

    static void InterpolateFloat(uint32_t num_channels, const float* src0, const float* src1, uint32_t levels, float* out);
    static void DecompressDXT1Palette(const k3DXT1Block* src, float* palette, bool allow_alpha);
    static void DecompressDXT1Block(const k3DXT1Block* src, float* dest, bool allow_alpha = true);
    static void DecompressDXT3Block(const k3DXT3Block* src, float* dest);
    static void DecompressDXT5Block(const k3DXT3Block* src, float* dest);
    static void DecompressATI1NBlock(const uint64_t* src, float* dest, bool clear_others = true);
    static void DecompressATI2NBlock(const k3ATI2NBlock* src, float* dest);
    static float CalcRGBLuminance(const float* src);
    static float CompressDXT1Pixmap(const float** palette, const float* src, uint32_t* dest);
    static float CompressATI1NPixmap(const float** palette, const float* src, uint64_t* dest);
    static void CompressDXT1Block(const float* src, k3DXT1Block* dest, bool allow_alpha = true);
    static void CompressDXT3Block(const float* src, k3DXT3Block* dest);
    static void CompressDXT5Block(const float* src, k3DXT3Block* dest);
    static void CompressATI1NBlock(const float* src, uint64_t* dest);
    static void CompressATI2NBlock(const float* src, k3ATI2NBlock* dest);

    static void InterpolateUnorm8(uint32_t num_channels, const uint8_t* src0, const uint8_t* src1, uint8_t levels, uint8_t* out);
    static void DecompressDXT1Palette(const k3DXT1Block* src, uint8_t* palette, bool allow_alpha);
    static void DecompressDXT1Block(const k3DXT1Block* src, uint8_t* dest, bool allow_alpha = true);
    static void DecompressDXT3Block(const k3DXT3Block* src, uint8_t* dest);
    static void DecompressDXT5Block(const k3DXT3Block* src, uint8_t* dest);
    static void DecompressATI1NBlock(const uint64_t* src, uint8_t* dest, bool clear_others = true);
    static void DecompressATI2NBlock(const k3ATI2NBlock* src, uint8_t* dest);
    static uint32_t CalcRGBLuminance(const uint8_t* src);
    static uint32_t CompressDXT1Pixmap(const uint8_t** palette, const uint8_t* src, uint32_t* dest);
    static uint32_t CompressATI1NPixmap(const uint8_t** palette, const uint8_t* src, uint64_t* dest);
    static void CompressDXT1Block(const uint8_t* src, k3DXT1Block* dest, bool allow_alpha = true);
    static void CompressDXT3Block(const uint8_t* src, k3DXT3Block* dest);
    static void CompressDXT5Block(const uint8_t* src, k3DXT3Block* dest);
    static void CompressATI1NBlock(const uint8_t* src, uint64_t* dest);
    static void CompressATI2NBlock(const uint8_t* src, k3ATI2NBlock* dest);

    static void ConvertRGB9E5ToFloat32(uint32_t src, float* dest);
    static void ConvertFloat32ToRGB9E5(const float* src, uint32_t* dest);
    static float ConvertFloat16ToFloat32(uint16_t f16);
    static uint16_t ConvertFloat32ToFloat16(float f32);
    static void ConvertToFloat4(k3fmt format, const void* src, float* dest);
    static void ConvertFromFloat4(k3fmt format, const float* src, void* dest);
    static void ConvertToUnorm8(k3fmt format, const void* src, uint8_t* dest);
    static void ConvertFromUnorm8(k3fmt format, const uint8_t* src, void* dest);
    static uint32_t GetFormatSize(k3fmt format);
    static uint32_t GetFormatBlockSize(k3fmt format);
    static uint32_t GetImageSize(uint32_t width, uint32_t height, uint32_t depth, k3fmt format);
    static uint32_t GetFormatNumComponents(k3fmt format);
    static uint32_t GetComponentBits(k3component component, k3fmt format);
    static uint32_t GetMaxComponentBits(k3fmt format);

    static int32_t CalcFinalAddress(int32_t x, int32_t length, k3texAddr addr_mode);
    static const void* GetSamplePointer(int32_t x, int32_t y, int32_t z,
        int32_t width, int32_t height, int32_t depth,
        uint32_t pitch, uint32_t slice_pitch,
        uint32_t format_size, uint32_t block_size,
        const void* data,
        k3texAddr x_addr_mode = k3texAddr::CLAMP,
        k3texAddr y_addr_mode = k3texAddr::CLAMP,
        k3texAddr z_addr_mode = k3texAddr::CLAMP,
        uint32_t* block_offset = NULL);
    static void* GetSamplePointer(int32_t x, int32_t y, int32_t z,
        int32_t width, int32_t height, int32_t depth,
        uint32_t pitch, uint32_t slice_pitch,
        uint32_t format_size, uint32_t block_size,
        void* data,
        k3texAddr x_addr_mode = k3texAddr::CLAMP,
        k3texAddr y_addr_mode = k3texAddr::CLAMP,
        k3texAddr z_addr_mode = k3texAddr::CLAMP,
        uint32_t* block_offset = NULL);

    static void GetWeights(float start, float end, int32_t& istart, int32_t& iend, float* weights);
    static void ReformatBuffer(uint32_t src_width, uint32_t src_height, uint32_t src_depth,
        uint32_t src_pitch, uint32_t src_slice_pitch,
        k3fmt src_format, const void* src_data,
        uint32_t dest_width, uint32_t dest_height, uint32_t dest_depth,
        uint32_t dest_pitch, uint32_t dest_slice_pitch,
        k3fmt dest_format, void* dest_data,
        const float* transform = NULL,
        k3texAddr x_addr_mode = k3texAddr::CLAMP,
        k3texAddr y_addr_mode = k3texAddr::CLAMP,
        k3texAddr z_addr_mode = k3texAddr::CLAMP);

    static void SampleBuffer(float x, float y, float z,
        uint32_t width, uint32_t height, uint32_t depth,
        uint32_t pitch, uint32_t slice_pitch,
        k3fmt format, const void* data, float* color,
        k3texAddr x_addr_mode = k3texAddr::CLAMP,
        k3texAddr y_addr_mode = k3texAddr::CLAMP,
        k3texAddr z_addr_mode = k3texAddr::CLAMP);
};

// ------------------------------------------------------------
// k3 key enums
enum class k3key {
    NONE = 0x00000,
    ESCAPE = 0x0001b,
    F1 = 0x10001,
    F2 = 0x10002,
    F3 = 0x10003,
    F4 = 0x10004,
    F5 = 0x10005,
    F6 = 0x10006,
    F7 = 0x10007,
    F8 = 0x10008,
    F9 = 0x10009,
    F10 = 0x1000a,
    F11 = 0x1000b,
    F12 = 0x1000c,

    LSHIFT = 0x1000d,
    RSHIFT = 0x1000e,
    LCONTROL = 0x1000f,
    RCONTROL = 0x10010,
    LALT = 0x10011,
    RALT = 0x10012,
    LMETA = 0x10013,
    RMETA = 0x10014,
    LWIN = 0x10015,
    RWIN = 0x10016,
    MENU = 0x10017,

    BACKSPACE = 0x00008,
    TAB = 0x00009,
    CAPS_LOCK = 0x10018,
    ENTER = 0x0000d,
    SPACE = 0x00020,

    SYS_REQ = 0x10019,
    SCROLL_LOCK = 0x1001a,
    PAUSE = 0x1001b,
    INSERT = 0x1001c,
    DEL = 0x0007f,
    HOME = 0x1001d,
    END = 0x1001e,
    PAGE_UP = 0x1001f,
    PAGE_DOWN = 0x10020,

    UP = 0x10021,
    DOWN = 0x10022,
    LEFT = 0x10023,
    RIGHT = 0x10024,

    NUM_LOCK = 0x10025,
    NUM_0 = 0x10026,
    NUM_1 = 0x10027,
    NUM_2 = 0x10028,
    NUM_3 = 0x10029,
    NUM_4 = 0x1002a,
    NUM_5 = 0x1002b,
    NUM_6 = 0x1002c,
    NUM_7 = 0x1002d,
    NUM_8 = 0x1002e,
    NUM_9 = 0x1002f,
    NUM_DECIMAL = 0x10030,
    NUM_PLUS = 0x10031,
    NUM_MINUS = 0x10032,
    NUM_TIMES = 0x10033,
    NUM_DIVIDE = 0x10034,
    NUM_ENTER = 0x10035,

    MINUS = 0x2d,
    PLUS = 0x2b,
    LBRACKET = 0x5b,
    RBRACKET = 0x5d,
    BACKSLASH = 0x5c,
    SEMICOLON = 0x3b,
    TICK = 0x27,
    BACKTICK = 0x60,

    A = 0x41,
    B = 0x42,
    C = 0x43,
    D = 0x44,
    E = 0x45,
    F = 0x46,
    G = 0x47,
    H = 0x48,
    I = 0x49,
    J = 0x4a,
    K = 0x4b,
    L = 0x4c,
    M = 0x4d,
    N = 0x4e,
    O = 0x4f,
    P = 0x50,
    Q = 0x51,
    R = 0x52,
    S = 0x53,
    T = 0x54,
    U = 0x55,
    V = 0x56,
    W = 0x57,
    X = 0x58,
    Y = 0x59,
    Z = 0x5a,

    KEY_0 = 0x30,
    KEY_1 = 0x31,
    KEY_2 = 0x32,
    KEY_3 = 0x33,
    KEY_4 = 0x34,
    KEY_5 = 0x35,
    KEY_6 = 0x36,
    KEY_7 = 0x37,
    KEY_8 = 0x38,
    KEY_9 = 0x39

 };

enum class k3keyState {
    NONE = 0x0,
    PRESSED = 0x01,
    RELEASED = 0x02,
    REPEATED = 0x04
};

// ------------------------------------------------------------
// k3 joystick information
const uint32_t K3JOY_MAX_AXES = 32;
const uint32_t K3JOY_MAX_BUTTONS = 32;
const uint32_t K3JOY_NAME_MAX = 128;
const uint32_t K3JOY_MAX_ATTR = 8;

enum class k3joyAxis {
    UNKNOWN,
    X,
    Y,
    Z,
    R,
    U,
    V,
    POV,
    GX,
    GY,
    GZ,
    AX,
    AY,
    AZ,
    TPAD_X,
    TPAD_Y
};

enum class k3joyAttr {
    UNKNOWN,
    RUMBLE,
    LIGHT_BAR
};

struct k3joyInfo {
    uint32_t num_axes;
    uint32_t num_buttons;
    uint32_t num_attr;
    k3joyAxis axis[K3JOY_MAX_AXES];
    uint32_t axis_ordinal[K3JOY_MAX_AXES];
    uint32_t attr_values[K3JOY_MAX_ATTR];
    k3joyAttr attr_type[K3JOY_MAX_ATTR];
    char name[K3JOY_NAME_MAX];
};

struct k3joyState {
    uint32_t buttons_pressed;
    float axis[K3JOY_MAX_AXES];
};

// ------------------------------------------------------------
// k3 timer classes

class k3timerImpl;
class k3timerObj;
typedef k3ptr<k3timerObj> k3timer;

class k3timerObj : public k3obj
{
private:
    k3timerImpl* _data;

public:
    k3timerObj();
    virtual ~k3timerObj();
    k3timerImpl* getImpl();
    const k3timerImpl* getImpl() const;

    virtual K3API k3objType getObjType() const
    {
        return k3objType::TIMER;
    }

    K3API uint32_t GetTime();
    K3API uint32_t GetDeltaTime();
    K3API void Sleep(uint32_t time);
    K3API void Pause(bool pause);
    K3API bool IsPaused();
    K3API void Reset();
    uint32_t getSystemTime();
};

// ------------------------------------------------------------
// k3 sound classes

class k3soundBufImpl;
class k3soundBufObj;
typedef k3ptr<k3soundBufObj> k3soundBuf;

class k3soundBufObj : public k3obj
{
private:
    k3soundBufImpl* _data;

public:
    k3soundBufObj();
    virtual ~k3soundBufObj();
    k3soundBufImpl* getImpl();
    const k3soundBufImpl* getImpl() const;

    virtual K3API k3objType getObjType() const
    {
        return k3objType::SOUND_BUF;
    }

    K3API void UpdateSBuffer(uint32_t offset, const void* data, uint32_t size);
    K3API void* MapForWrite(uint32_t offset, uint32_t size, void** aux, uint32_t* aux_size);
    K3API void Unmap(void* p, uint32_t offset, uint32_t size);
    K3API void PlaySBuffer(uint32_t offset = 0xFFFFFFFF);
    K3API uint32_t GetPlayPosition();
    K3API uint32_t GetWritePosition();
    K3API void StopSBuffer();
};

// ------------------------------------------------------------
// k3 graphics classes

const uint32_t K3_MAX_RENDER_TARGETS = 8;

struct k3rect {
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
};

class k3gfxImpl;
class k3gfxObj;
typedef k3ptr<k3gfxObj> k3gfx;

class k3fenceImpl;
class k3fenceObj;
typedef k3ptr<k3fenceObj> k3fence;

class k3resourceImpl;
class k3resourceObj;
typedef k3ptr<k3resourceObj> k3resource;

class k3surfImpl;
class k3surfObj;
typedef k3ptr<k3surfObj> k3surf;

class k3samplerImpl;
class k3samplerObj;
typedef k3ptr<k3samplerObj> k3sampler;

class k3bufferImpl;
class k3bufferObj;
typedef k3ptr<k3bufferObj> k3buffer;

class k3cmdBufImpl;
class k3cmdBufObj;
typedef k3ptr<k3cmdBufObj> k3cmdBuf;

class k3blasImpl;
class k3blasObj;
typedef k3ptr<k3blasObj> k3blas;

class k3tlasImpl;
class k3tlasObj;
typedef k3ptr<k3tlasObj> k3tlas;

class k3shaderBindingImpl;
class k3shaderBindingObj;
typedef k3ptr<k3shaderBindingObj> k3shaderBinding;

class k3shaderImpl;
class k3shaderObj;
typedef k3ptr<k3shaderObj> k3shader;

class k3gfxStateImpl;
class k3gfxStateObj;
typedef k3ptr<k3gfxStateObj> k3gfxState;

class k3rtStateImpl;
class k3rtStateObj;
typedef k3ptr<k3rtStateObj> k3rtState;

class k3rtStateTableImpl;
class k3rtStateTableObj;
typedef k3ptr<k3rtStateTableObj> k3rtStateTable;

class k3memPoolImpl;
class k3memPoolObj;
typedef k3ptr<k3memPoolObj> k3memPool;

class k3uploadImageImpl;
class k3uploadImageObj;
typedef k3ptr<k3uploadImageObj> k3uploadImage;

class k3downloadImageImpl;
class k3downloadImageObj;
typedef k3ptr<k3downloadImageObj> k3downloadImage;

class k3uploadBufferImpl;
class k3uploadBufferObj;
typedef k3ptr<k3uploadBufferObj> k3uploadBuffer;

class k3fontImpl;
class k3fontObj;
typedef k3ptr<k3fontObj> k3font;

class k3meshImpl;
class k3meshObj;
typedef k3ptr<k3meshObj> k3mesh;

enum class k3gpuQueue {
    NONE,
    GRAPHICS,
    COMPUTE,
    COPY
};

enum class k3drawPrimType {
    UNDEFINED,
    POINTLIST,
    LINELIST,
    LINESTRIP,
    TRIANGLELIST,
    TRIANGLESTRIP,
    LINELIST_ADJ,
    LINESTRIP_ADJ,
    TRIANGLELIST_ADJ,
    TRIANGLESTRIP_ADJ,
    CONTROL_POINT_PATCHLIST_1,
    CONTROL_POINT_PATCHLIST_2,
    CONTROL_POINT_PATCHLIST_3,
    CONTROL_POINT_PATCHLIST_4,
    CONTROL_POINT_PATCHLIST_5,
    CONTROL_POINT_PATCHLIST_6,
    CONTROL_POINT_PATCHLIST_7,
    CONTROL_POINT_PATCHLIST_8,
    CONTROL_POINT_PATCHLIST_9,
    CONTROL_POINT_PATCHLIST_10,
    CONTROL_POINT_PATCHLIST_11,
    CONTROL_POINT_PATCHLIST_12,
    CONTROL_POINT_PATCHLIST_13,
    CONTROL_POINT_PATCHLIST_14,
    CONTROL_POINT_PATCHLIST_15,
    CONTROL_POINT_PATCHLIST_16,
    CONTROL_POINT_PATCHLIST_17,
    CONTROL_POINT_PATCHLIST_18,
    CONTROL_POINT_PATCHLIST_19,
    CONTROL_POINT_PATCHLIST_20,
    CONTROL_POINT_PATCHLIST_21,
    CONTROL_POINT_PATCHLIST_22,
    CONTROL_POINT_PATCHLIST_23,
    CONTROL_POINT_PATCHLIST_24,
    CONTROL_POINT_PATCHLIST_25,
    CONTROL_POINT_PATCHLIST_26,
    CONTROL_POINT_PATCHLIST_27,
    CONTROL_POINT_PATCHLIST_28,
    CONTROL_POINT_PATCHLIST_29,
    CONTROL_POINT_PATCHLIST_30,
    CONTROL_POINT_PATCHLIST_31,
    CONTROL_POINT_PATCHLIST_32
};

class k3fenceObj : public k3obj
{
private:
    k3fenceImpl* _data;

public:
    k3fenceObj();
    virtual ~k3fenceObj();
    k3fenceImpl* getImpl();
    const k3fenceImpl* getImpl() const;

    virtual K3API k3objType getObjType() const
    {
        return k3objType::FENCE;
    }

    K3API uint64_t SetCpuFence();
    K3API uint64_t SetGpuFence(k3gpuQueue queue);
    K3API bool CheckFence(uint64_t value);
    K3API void WaitCpuFence(uint64_t value);
    K3API void WaitGpuFence(uint64_t value);
};

enum class k3resourceState
{
    COMMON,
    SHADER_BUFFER,
    INDEX_BUFFER,
    RENDER_TARGET,
    UAV,
    DEPTH_WRITE,
    DEPTH_READ,
    FRONT_END_SHADER_RESOURCE,
    PIXEL_SHADER_RESOURCE,
    SHADER_RESOURCE,
    STREAM_OUT,
    COPY_DEST,
    COPY_SOURCE,
    RESOLVE_DEST,
    RESOLVE_SOURCE,
    SHADING_RATE_SOURCE,
    RT_ACCEL_STRUCT
};

struct k3resourceDesc {
    k3memPool mem_pool;
    uint64_t mem_offset;
    uint64_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t mip_levels;
    k3fmt format;
    uint32_t num_samples;
};

class k3resourceObj : public k3obj
{
private:
    k3resourceImpl* _data;

public:
    k3resourceObj();
    virtual ~k3resourceObj();
    k3resourceImpl* getImpl();
    const k3resourceImpl* getImpl() const;

    virtual K3API k3objType getObjType() const
    {
        return k3objType::RESOURCE;
    }

    K3API void getDesc(k3resourceDesc* desc);
};

enum class k3depthSelect
{
    NONE = 0,
    DEPTH = 1,
    STENCIL = 2,
    DEPTH_STENCIL = 3
};


enum class k3texFilter {
    MIN_MAG_MIP_POINT,
    MIN_MAG_POINT_MIP_LINEAR,
    MIN_POINT_MAG_LINEAR_MIP_POINT,
    MIN_POINT_MAG_MIP_LINEAR,
    MIN_LINEAR_MAG_MIP_POINT,
    MIN_LINEAR_MAG_POINT_MIP_LINEAR,
    MIN_MAG_LINEAR_MIP_POINT,
    MIN_MAG_MIP_LINEAR,
    ANISOTROPIC,
    COMPARISON_MIN_MAG_MIP_POINT,
    COMPARISON_MIN_MAG_POINT_MIP_LINEAR,
    COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT,
    COMPARISON_MIN_POINT_MAG_MIP_LINEAR,
    COMPARISON_MIN_LINEAR_MAG_MIP_POINT,
    COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
    COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
    COMPARISON_MIN_MAG_MIP_LINEAR,
    COMPARISON_ANISOTROPIC,
    MINIMUM_MIN_MAG_MIP_POINT,
    MINIMUM_MIN_MAG_POINT_MIP_LINEAR,
    MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT,
    MINIMUM_MIN_POINT_MAG_MIP_LINEAR,
    MINIMUM_MIN_LINEAR_MAG_MIP_POINT,
    MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
    MINIMUM_MIN_MAG_LINEAR_MIP_POINT,
    MINIMUM_MIN_MAG_MIP_LINEAR,
    MINIMUM_ANISOTROPIC,
    MAXIMUM_MIN_MAG_MIP_POINT,
    MAXIMUM_MIN_MAG_POINT_MIP_LINEAR,
    MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT,
    MAXIMUM_MIN_POINT_MAG_MIP_LINEAR,
    MAXIMUM_MIN_LINEAR_MAG_MIP_POINT,
    MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
    MAXIMUM_MIN_MAG_LINEAR_MIP_POINT,
    MAXIMUM_MIN_MAG_MIP_LINEAR,
    MAXIMUM_ANISOTROPIC
};

enum class k3testFunc {
    NEVER,
    LESS,
    EQUAL,
    LESS_EQUAL,
    GREATER,
    NOT_EQUAL,
    GREATER_EQUAL,
    ALWAYS
};

struct k3viewDesc {
    uint32_t view_index;
    uint32_t array_start;
    uint32_t array_size;
    uint32_t mip_start;
    k3depthSelect read_only;
    bool is_cube;
    bool is_array;
    float clear_value[4];
};

struct k3samplerDesc {
    uint32_t sampler_index;
    k3texFilter filter;
    k3texAddr addr_u;
    k3texAddr addr_v;
    k3texAddr addr_w;
    float mip_load_bias;
    uint32_t max_anisotropy;
    k3testFunc compare_func;
    float border_color[4];
    float min_lod;
    float max_lod;
};

struct k3bufferDesc {
    k3memPool mem_pool;
    uint64_t mem_offset;
    uint32_t size;
    uint32_t stride;
    uint32_t view_index;  // for CBV, SRV
    k3fmt format;         // for index buffer
    bool shader_resource; // SRV buffer
};

// Raytracing acceleration structure size
struct k3rtasSize {
    uint64_t rtas_size;
    uint64_t create_size;
    uint64_t update_size;
};

struct k3rtasAllocDesc {
    k3memPool rtas_mem_pool;
    k3memPool create_mem_pool;
    k3memPool update_mem_pool;
    uint64_t rtas_mem_offset;
    uint64_t create_mem_offset;
    uint64_t update_mem_offset;
};

struct k3blasCreateDesc {
    float* xform_3x4;
    k3buffer ib;
    k3buffer vb;
    bool alloc;
    uint32_t start_prim;
    uint32_t num_prims;
};

struct k3tlasInstance {
    float transform[12];  // 3x4 transform matrix
    uint32_t id;
    uint32_t hit_group;
    uint8_t mask;
    uint8_t flags;
    k3blas blas;
};

struct k3tlasCreateDesc {
    uint32_t num_instances;
    k3tlasInstance* instances;
    uint32_t view_index;
    bool alloc;
};

class k3surfObj : public k3obj
{
private:
    k3surfImpl* _data;

public:
    k3surfObj();
    virtual ~k3surfObj();
    k3surfImpl* getImpl();
    const k3surfImpl* getImpl() const;

    virtual K3API k3objType getObjType() const
    {
        return k3objType::SURF;
    }

    K3API k3resource GetResource();
    K3API uint32_t GetSRVViewIndex() const;
    K3API uint32_t GetUAVViewIndex() const;
};

class k3samplerObj : public k3obj
{
private:
    k3samplerImpl* _data;

public:
    k3samplerObj();
    virtual ~k3samplerObj();
    k3samplerImpl* getImpl();
    const k3samplerImpl* getImpl() const;

    virtual K3API k3objType getObjType() const
    {
        return k3objType::SAMPLER;
    }

    K3API uint32_t GetViewIndex() const;
};

class k3bufferObj : public k3obj
{
private:
    k3bufferImpl* _data;

public:
    k3bufferObj();
    virtual ~k3bufferObj();
    k3bufferImpl* getImpl();
    const k3bufferImpl* getImpl() const;

    virtual K3API k3objType getObjType() const
    {
        return k3objType::BUFFER;
    }

    K3API k3resource GetResource();
    K3API uint32_t GetViewIndex() const;
};

class k3blasObj : public k3obj
{
private:
    k3blasImpl* _data;

public:
    k3blasObj();
    virtual ~k3blasObj();
    k3blasImpl* getImpl();
    const k3blasImpl* getImpl() const;

    virtual K3API k3objType getObjType() const
    {
        return k3objType::BLAS;
    }

    K3API void getSize(k3rtasSize* size);
};

class k3tlasObj : public k3obj
{
private:
    k3tlasImpl* _data;

public:
    k3tlasObj();
    virtual ~k3tlasObj();
    k3tlasImpl* getImpl();
    const k3tlasImpl* getImpl() const;

    virtual K3API k3objType getObjType() const
    {
        return k3objType::TLAS;
    }

    K3API uint32_t GetViewIndex() const;
    K3API void UpdateInstance(uint64_t inst_id, k3tlasInstance* inst);
    K3API void UpdateTransform(uint64_t inst_id, float* xform);
};

struct k3rtStateTableDesc {
    uint32_t num_entries;
    uint32_t num_args;
    k3memPool mem_pool;
    uint64_t mem_offset;
};

enum class k3rtStateTableArgType {
    HANDLE,
    CONSTANT
};

enum class k3shaderBindType {
    CBV,
    SRV,
    UAV,
    SAMPLER
};

struct k3rtStateTableArg {
    k3rtStateTableArgType type;
    k3shaderBindType bind_type;
    k3objPtr obj;
    uint32_t c[2];
};

struct k3rtStateTableEntryDesc {
    const char* shader;
    uint32_t num_args;
    k3rtStateTableArg* args;
};

struct k3rtStateTableUpdate {
    k3rtState state;
    k3uploadBuffer copy_buffer;
    uint32_t start;
    uint32_t num_entries;
    const k3rtStateTableEntryDesc* entries;
};

struct k3rtDispatch {
    k3rtStateTable state_table;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t raygen_index;
    uint32_t raygen_entries;
    uint32_t miss_index;
    uint32_t miss_entries;
    uint32_t hit_group_index;
    uint32_t hit_group_entries;
};

class k3rtStateTableObj : public k3obj
{
private:
    k3rtStateTableImpl* _data;

public:
    k3rtStateTableObj();
    virtual ~k3rtStateTableObj();
    k3rtStateTableImpl* getImpl();
    const k3rtStateTableImpl* getImpl() const;

    virtual K3API k3objType getObjType() const
    {
        return k3objType::RT_STATE_TABLE;
    }
};

struct k3renderTargets {
    k3surf render_targets[K3_MAX_RENDER_TARGETS];
    k3surf depth_target;
};

enum class k3fontStyle {
    NORMAL,
    ITALIC
};

enum class k3fontWeight {
    NORMAL,
    BOLD
};

enum class k3fontAlignment {
    TOP_LEFT,
    TOP_CENTER,
    TOP_RIGHT,
    MID_LEFT,
    MID_CENTER,
    MID_RIGHT,
    BOTTOM_LEFT,
    BOTTOM_CENTER,
    BOTTOM_RIGHT
};

struct k3fontDesc {
    uint32_t view_index;
    const char* name;
    float point_size;
    k3fontStyle style;
    k3fontWeight weight;
    k3cmdBuf cmd_buf;
    k3fmt format;
    bool transparent;
};

class k3fontObj : public k3obj
{
private:
    k3fontImpl* _data;

public:
    k3fontObj();
    virtual ~k3fontObj();
    k3fontImpl* getImpl();
    const k3fontImpl* getImpl() const;

    virtual K3API k3objType getObjType() const
    {
        return k3objType::FONT;
    }
};

struct k3lightBufferData {
    static const uint32_t POINT = 0;
    static const uint32_t SPOT = 1;
    static const uint32_t DIRECTIONAL = 2;
    static const uint32_t DECAY_NONE = 0;
    static const uint32_t DECAY_LINEAR = 1;
    static const uint32_t DECAY_QUADRATIC = 2;
    static const uint32_t DECAY_CUBIC = 3;

    float position[3];
    float intensity;
    float color[3];
    float decay_start;
    uint32_t light_type;
    uint32_t decay_type;
    uint32_t cast_shadows;
    float spot_angle;
};

struct k3meshDesc {
    uint32_t view_index;
    const char* name;
    k3cmdBuf cmd_buf;
    k3uploadBuffer up_buf;
};

class k3meshObj : public k3obj
{
private:
    k3meshImpl * _data;

public:
    static const uint32_t ANIM_FLAG_NONE = 0x0;
    static const uint32_t ANIM_FLAG_INCREMENTAL = 0x1;

    k3meshObj();
    virtual ~k3meshObj();
    k3meshImpl* getImpl();
    const k3meshImpl* getImpl() const;

    virtual K3API k3objType getObjType() const
    {
        return k3objType::MESH;
    }


    K3API uint32_t getNumObjects();
    K3API uint32_t getNumTextures();
    K3API uint32_t getNumMeshes();
    K3API uint32_t getNumCameras();
    K3API uint32_t getNumBones();
    K3API uint32_t getNumAnims();
    K3API k3surf getTexture(uint32_t tex);
    K3API uint32_t getMeshStartPrim(uint32_t mesh);
    K3API uint32_t getMeshNumPrims(uint32_t mesh);
    K3API uint32_t getMeshIndex(uint32_t obj);
    K3API uint32_t getStartPrim(uint32_t obj);
    K3API uint32_t getNumPrims(uint32_t obj);
    K3API float* getTransform(uint32_t obj);
    K3API float* getDiffuseColor(uint32_t obj);
    K3API uint32_t getDiffuseMapIndex(uint32_t obj);
    K3API uint32_t getNormalMapIndex(uint32_t obj);
    K3API float* getCameraPerspective(float* d, uint32_t camera, bool left_handed = false, bool dx_style = true, bool reverse_z = true);
    K3API float* getCameraView(float* d, uint32_t camera, bool left_handed = false);
    K3API float* getCameraPosition(uint32_t camera);
    K3API float* getCameraLookAt(uint32_t camera);
    K3API float* getCameraUp(uint32_t camera);
    K3API void getCameraResolution(uint32_t camera, uint32_t* width, uint32_t* height);
    K3API float getCameraNearPlane(uint32_t camera);
    K3API float getCameraFarPlane(uint32_t camera);
    K3API void setCameraResolution(uint32_t camera, uint32_t width, uint32_t height);
    K3API void setCameraNearPlane(uint32_t camera, float near);
    K3API void setCameraFarPlane(uint32_t camera, float far);
    K3API void genBoneMatrices(float* mat, bool gen_inv);
    K3API uint32_t findAnim(const char* name);
    K3API const char* getAnimName(uint32_t a);
    K3API void setAnimation(uint32_t anim_index, uint32_t time_msec, uint32_t flags);

    K3API k3buffer getIndexBuffer();
    K3API k3buffer getVertexBuffer();
    K3API k3buffer getAttribBuffer();
    K3API k3buffer getLightBuffer();
    K3API k3buffer getSkinBuffer();
};

class k3cmdBufObj : public k3obj
{
private:
    k3cmdBufImpl* _data;

public:
    k3cmdBufObj();
    virtual ~k3cmdBufObj();
    k3cmdBufImpl* getImpl();
    const k3cmdBufImpl* getImpl() const;

    virtual K3API k3objType getObjType() const
    {
        return k3objType::CMD_BUF;
    }

    K3API void Reset();
    K3API void Close();

    K3API void TransitionResource(k3resource surf, k3resourceState new_state);

    K3API void ClearRenderTarget(k3surf surf, const float* color, const k3rect* rect);
    K3API void ClearDepthTarget(k3surf surf, k3depthSelect clear, const float depth, uint8_t stencil, const k3rect* rect);
    K3API void UploadImage(k3uploadImage img, k3resource resource);
    K3API void DownloadImage(k3downloadImage img, k3resource resource);
    K3API void UploadBuffer(k3uploadBuffer buf, k3resource resource, uint64_t start = 0);
    K3API void UploadBufferSrcRange(k3uploadBuffer buf, k3resource resource, uint64_t src_start, uint64_t size, uint64_t dst_start = 0);
    K3API void GetCurrentViewport(k3rect* viewport);

    K3API void SetGfxState(k3gfxState state);
    K3API void SetDrawPrim(k3drawPrimType draw_prim);
    K3API void SetViewToSurface(k3resource surface);
    K3API void SetViewport(k3rect* rect);
    K3API void SetScissor(k3rect* rect);
    K3API void SetRenderTargets(k3renderTargets* rt);
    K3API void SetIndexBuffer(k3buffer index_buffer);
    K3API void SetVertexBuffer(uint32_t slot, k3buffer vertex_buffer);
    K3API void SetConstant(uint32_t index, uint32_t value, uint32_t offset = 0);
    K3API void SetConstants(uint32_t index, uint32_t num_constants, const void* values, uint32_t offset = 0);
    K3API void SetConstantBuffer(uint32_t index, k3buffer constant_buffer);
    K3API void SetShaderView(uint32_t index, k3surf surf);
    K3API void SetSampler(uint32_t index, k3sampler sampler);
    K3API void SetBlendFactor(const float* blend_factor);
    K3API void SetStencilRef(uint8_t stencil_ref);

    K3API void BuildBlas(k3blas blas);
    K3API void BuildTlas(k3tlas tlas);
    K3API void UpdateRTStateTable(k3rtStateTable table, k3rtStateTableUpdate* desc);
    K3API void SetRTState(k3rtState state);
    K3API void RTDispatch(const k3rtDispatch* desc);

    K3API void Copy(k3resource dest, k3resource source);
    K3API void Draw(uint32_t vertex_count, uint32_t vertex_start = 0, uint32_t instance_count = 1, uint32_t instance_start = 0, uint32_t index_start = 0);
    K3API void DrawText(const char* text, k3font font, const float fg_color[4], const float bg_color[4], int32_t x, int32_t y, k3fontAlignment alignment = k3fontAlignment::TOP_LEFT);
};

enum class k3bindingType {
    CONSTANT,
    CBV,
    SRV,
    UAV,
    VIEW_SET,
    VIEW_SET_TABLE
};

struct k3bindingConst {
    uint32_t reg;
    uint32_t space;
    uint32_t num_const;
};

struct k3bindingView {
    uint32_t reg;
    uint32_t space;
};

struct k3bindingViewSet {
    k3shaderBindType type;
    uint32_t num_views;
    uint32_t reg;
    uint32_t space;
    uint32_t offset;
};

struct k3bindingViewSetTable {
    uint32_t num_view_sets;
    k3bindingViewSet* view_sets;
};

struct k3bindingParam {
    k3bindingType type;
    union {
        k3bindingConst constant;
        k3bindingView view_desc;
        k3bindingViewSet view_set_desc;
        k3bindingViewSetTable view_set_table_desc;
    };
};

enum class k3shaderBindingType {
    GLOBAL,
    LOCAL
};

class k3shaderBindingObj : public k3obj
{
private:
    k3shaderBindingImpl* _data;

public:
    k3shaderBindingObj();
    virtual ~k3shaderBindingObj();
    k3shaderBindingImpl* getImpl();
    const k3shaderBindingImpl* getImpl() const;

    virtual K3API k3objType getObjType() const
    {
        return k3objType::SHADER_BINDING;
    }

    K3API k3shaderBindingType getType() const;
};

enum class k3shaderType
{
    VERTEX_SHADER,
    PIXEL_SHADER,
    GEOMETRY_SHADER,
    HULL_SHADER,
    DOMAIN_SHADER,
    LIBRARY
};

class k3shaderObj : public k3obj
{
private:
    k3shaderImpl* _data;

public:
    k3shaderObj();
    virtual ~k3shaderObj();
    k3shaderImpl* getImpl();
    const k3shaderImpl* getImpl() const;

    virtual K3API k3objType getObjType() const
    {
        return k3objType::SHADER;
    }
};

struct k3rtShaderStateDesc {
    k3shader obj;
    uint32_t num_entries;
    const char** entries;
};

enum class k3rtHitGroupType {
    TRIANGLES,
    PROCEDURAL
};

struct k3rtHitGroupStateDesc {
    const char* name;
    const char* any_hit_shader;
    const char* closest_hit_shader;
    const char* intersection_shader;
    k3rtHitGroupType type;
};

struct k3rtExportAssociationStateDesc {
    uint32_t num_exports;
    const char** export_names;
    uint32_t association_index;
};

struct k3rtShaderConfigStateDesc {
    uint32_t attrib_size;
    uint32_t payload_size;
};

struct k3rtPipelineConfigStateDesc {
    uint32_t max_recursion;
};

union k3rtStateElementDesc {
    k3rtHitGroupStateDesc hit_group;
    k3rtExportAssociationStateDesc export_association;
    k3rtShaderConfigStateDesc shader_config;
    k3rtPipelineConfigStateDesc pipeline_config;
};

enum class k3rtStateType {
    SHADER,
    HIT_GROUP,
    SHADER_BINDING,
    EXPORT_ASSOCIATION,
    SHADER_CONFIG,
    PIPELINE_CONFIG
};

struct k3rtStateDesc {
    k3rtStateType type;
    k3rtShaderStateDesc shader;
    k3shaderBinding shader_binding;
    k3rtStateElementDesc elem;
};

class k3rtStateObj : public k3obj
{
private:
    k3rtStateImpl* _data;

public:
    k3rtStateObj();
    virtual ~k3rtStateObj();
    k3rtStateImpl* getImpl();
    const k3rtStateImpl* getImpl() const;

    virtual K3API k3objType getObjType() const
    {
        return k3objType::RT_STATE;
    }
};

enum class k3blend {
    ZERO,
    ONE,
    SRC_COLOR,
    INV_SRC_COLOR,
    SRC_ALPHA,
    INV_SRC_ALPHA,
    DEST_ALPHA,
    INV_DEST_ALPHA,
    DEST_COLOR,
    INV_DEST_COLOR,
    SRC_ALPHA_SAT,
    BLEND_FACTOR,
    INV_BLEND_FACTOR,
    SRC1_COLOR,
    INV_SRC1_COLOR,
    SRC1_ALPHA,
    INV_SRC1_ALPHA
};

enum class k3blendOp {
    ADD,
    SUBTRACT,
    REV_SUBTRACT,
    MIN,
    MAX
};

enum class k3rop {
    CLEAR,
    SET,
    COPY,
    COPY_INVERTED,
    NOOP,
    INVERT,
    AND,
    NAND,
    OR,
    NOR,
    XOR,
    EQUIV,
    AND_REVERSE,
    AND_INVERTED,
    OR_REVERSE,
    OR_INVERTED
};

struct k3blendOpState {
    bool blend_enable;
    bool rop_enable;
    k3blend src_blend;
    k3blend dst_blend;
    k3blendOp blend_op;
    k3blend alpha_src_blend;
    k3blend alpha_dst_blend;
    k3blendOp alpha_blend_op;
    k3rop rop;
    uint8_t rt_write_mask;
};

struct k3blendState {
    bool alpha_to_mask;
    bool independent_blend;
    k3blendOpState blend_op[K3_MAX_RENDER_TARGETS];
};

enum class k3fill {
    WIREFRAME,
    SOLID
};

enum class k3cull {
    NONE,
    FRONT,
    BACK
};

struct k3rastState {
    k3fill fill_mode;
    k3cull cull_mode;
    bool front_counter_clockwise;
    int32_t depth_bias;
    float depth_bias_clamp;
    float slope_scale_depth_bias;
    bool depth_clip_enable;
    bool msaa_enable;
    bool aa_line_enable;
    bool conservative_rast_enable;
};

enum class k3stencilOp {
    KEEP,
    ZERO,
    REPLACE,
    INCR_SAT,
    DECR_SAT,
    INVERT,
    INCR,
    DECR
};

struct k3stencilState {
    k3stencilOp fail_op;
    k3stencilOp z_fail_op;
    k3stencilOp pass_op;
    k3testFunc stencil_test;
};

struct k3depthState {
    bool depth_enable;
    bool depth_write_enable;
    k3testFunc depth_test;
    bool stencil_enable;
    uint8_t stencil_read_mask;
    uint8_t stencil_write_mask;
    k3stencilState front;
    k3stencilState back;
};

enum class k3inputType {
    VERTEX,
    INSTANCE
};

struct k3inputElement {
    const char* name;
    uint32_t index;
    k3fmt format;
    uint32_t slot;
    uint32_t offset;
    k3inputType in_type;
    uint32_t instance_step;
};

enum class k3stripCut {
    NONE,
    CUT_FFFF,
    CUT_FFFF_FFFF
};

enum class k3primType {
    UNDEFINED,
    POINT,
    LINE,
    TRIANGLE,
    PATCH
};

struct k3gfxStateDesc
{
    k3shaderBinding shader_binding;
    k3shader vertex_shader;
    k3shader pixel_shader;
    k3shader hull_shader;
    k3shader domain_shader;
    k3shader geometry_shader;
    k3blendState blend_state;
    uint32_t sample_mask;
    k3rastState rast_state;
    k3depthState depth_state;
    uint32_t num_input_elements;
    k3inputElement* input_elements;
    k3stripCut cut_index;
    k3primType prim_type;
    uint32_t num_render_targets;
    k3fmt rtv_format[K3_MAX_RENDER_TARGETS];
    k3fmt dsv_format;
    uint32_t msaa_samples;
};

class k3gfxStateObj : public k3obj
{
private:
    k3gfxStateImpl* _data;

public:
    k3gfxStateObj();
    virtual ~k3gfxStateObj();
    k3gfxStateImpl* getImpl();
    const k3gfxStateImpl* getImpl() const;

    virtual K3API k3objType getObjType() const
    {
        return k3objType::GFX_STATE;
    }
};

enum class k3memType {
    GPU,
    UPLOAD,
    READBACK
};

class k3memPoolObj : public k3obj
{
private:
    k3memPoolImpl* _data;

public:
    k3memPoolObj();
    virtual ~k3memPoolObj();
    k3memPoolImpl* getImpl();
    const k3memPoolImpl* getImpl() const;

    virtual K3API k3objType getObjType() const
    {
        return k3objType::MEM_POOL;
    }

    K3API k3memType getMemType();
    K3API uint64_t getSize();
};

class k3uploadImageObj : public k3imageObj
{
private:
    k3uploadImageImpl* _upload_data;

public:
    k3uploadImageObj();
    virtual ~k3uploadImageObj();
    k3uploadImageImpl* getUploadImageImpl();
    const k3uploadImageImpl* getUploadImageImpl() const;

    virtual K3API k3objType getObjType() const
    {
        return k3objType::UPLOAD_IMAGE;
    }

    virtual K3API const void* MapForRead();
    virtual K3API void* MapForWrite();
    virtual K3API void Unmap();
    K3API void GetDesc(k3resourceDesc* desc);
};

class k3downloadImageObj : public k3imageObj
{
private:
    k3downloadImageImpl* _download_data;

public:
    k3downloadImageObj();
    virtual ~k3downloadImageObj();
    k3downloadImageImpl* getDownloadImageImpl();
    const k3downloadImageImpl* getDownloadImageImpl() const;

    virtual K3API k3objType getObjType() const
    {
        return k3objType::DOWNLOAD_IMAGE;
    }

    virtual K3API const void* MapForRead();
    virtual K3API void* MapForWrite();
    virtual K3API void Unmap();
    K3API void GetDesc(k3resourceDesc* desc);
};

class k3uploadBufferObj : public k3obj
{
private:
    k3uploadBufferImpl* _data;

public:
    k3uploadBufferObj();
    virtual ~k3uploadBufferObj();
    k3uploadBufferImpl* getImpl();
    const k3uploadBufferImpl* getImpl() const;

    virtual K3API k3objType getObjType() const
    {
        return k3objType::UPLOAD_BUFFER;
    }

    K3API void* MapForWrite(uint64_t size);
    K3API void Unmap();
    K3API void* MapRange(uint64_t offset, uint64_t size);
    K3API void UnmapRange(uint64_t offset, uint64_t size);
};

const uint32_t K3_MEM_FLAG_MSAA = 0x1;

class k3gfxObj : public k3obj
{
private:
    k3gfxImpl* _data;
    k3gfxObj();
    void GetFontShaders(k3shader& vs, k3shader& ps);

public:
    static k3gfx Create(uint32_t num_views, uint32_t num_samplers);
    virtual ~k3gfxObj();
    k3gfxImpl* getImpl();
    const k3gfxImpl* getImpl() const;

    virtual K3API k3objType getObjType() const
    {
        return k3objType::GFX;
    }

    K3API const char* AdapterName();
    K3API uint32_t GetRayTracingSupport();

    K3API k3fence CreateFence();
    K3API void WaitGpuIdle();

    K3API k3cmdBuf CreateCmdBuf();
    K3API void SubmitCmdBuf(k3cmdBuf cmd);

    K3API k3shaderBinding CreateShaderBinding(uint32_t num_params, k3bindingParam* params, uint32_t num_samplers, k3samplerDesc* samplers);
    K3API k3shaderBinding CreateTypedShaderBinding(uint32_t num_params, k3bindingParam* params, uint32_t num_samplers, k3samplerDesc* samplers, k3shaderBindingType sh_bind_type);
    K3API k3shader CreateShaderFromCompiledFile(const char* file_name);
    K3API k3shader CompileShaderFromString(const char* code, k3shaderType shader_type);
    K3API k3shader CompileShaderFromFile(const char* file_name, k3shaderType shader_type);
    K3API k3gfxState CreateGfxState(const k3gfxStateDesc* desc);
    K3API k3rtState CreateRTState(uint32_t num_elements, const k3rtStateDesc* desc);
    K3API k3rtStateTable CreateRTStateTable(const k3rtStateTableDesc* desc);

    K3API k3memPool CreateMemPool(uint64_t size, k3memType mem_type, uint32_t flag);
    K3API k3uploadImage CreateUploadImage();
    K3API k3downloadImage CreateDownloadImage();
    K3API k3uploadBuffer CreateUploadBuffer();
    K3API k3surf CreateSurface(k3resourceDesc* rdesc, k3viewDesc* rtv_desc, k3viewDesc* srv_desc, k3viewDesc* uav_desc);
    K3API k3surf CreateSurfaceAlias(k3resource resource, k3viewDesc* rtv_desc, k3viewDesc* srv_desc, k3viewDesc* uav_desc);
    K3API k3sampler CreateSampler(const k3samplerDesc* sdesc);
    K3API k3buffer CreateBuffer(const k3bufferDesc* bdesc);
    K3API k3blas CreateBlas(const k3blasCreateDesc* bldesc);
    K3API void AllocBlas(k3blas bl, const k3rtasAllocDesc* rtadesc);
    K3API k3tlas CreateTlas(const k3tlasCreateDesc* tldesc);
    K3API void AllocTlas(k3tlas tl, const k3rtasAllocDesc* rtadesc);

    K3API k3font CreateFont(k3fontDesc* desc);
    K3API k3mesh CreateMesh(k3meshDesc* desc);
};

// ------------------------------------------------------------
// k3 windows classes

typedef void (K3CALLBACK* k3win_display_ptr)(void* data);
typedef void (K3CALLBACK* k3win_idle_ptr)(void* data);
typedef void (K3CALLBACK* k3win_keyboard_ptr)(void* data, k3key k, char c, k3keyState state);
typedef void (K3CALLBACK* k3win_mouse_move_ptr)(void* data, uint32_t x, uint32_t y);
typedef void (K3CALLBACK* k3win_mouse_button_ptr)(void* data, uint32_t x, uint32_t y, uint32_t b, k3keyState state);
typedef void (K3CALLBACK* k3win_mouse_scroll_ptr)(void* data, uint32_t x, uint32_t y, int32_t vscroll, int32_t hscroll);
typedef void (K3CALLBACK* k3win_resize_ptr)(void* data, uint32_t width, uint32_t height);
typedef void (K3CALLBACK* k3win_joystick_added_ptr)(void* data, uint32_t joystick, const k3joyInfo* joy_info, const k3joyState* joy_state);
typedef void (K3CALLBACK* k3win_joystick_removed_ptr)(void* data, uint32_t joystick);
typedef void (K3CALLBACK* k3win_joystick_move_ptr)(void* data, uint32_t joystick, uint32_t axis_num, k3joyAxis axis, uint32_t ordinal, float position);
typedef void (K3CALLBACK* k3win_joystick_button_ptr)(void* data, uint32_t joystick, uint32_t button, k3keyState state);
typedef void (K3CALLBACK* k3win_destroy_ptr)(void* data);

class k3winImpl;
class k3winObj;
typedef k3ptr<k3winObj> k3win;
class k3winObj : public k3obj
{
private:
    k3winImpl* _data;
    k3winObj();

public:
    static K3API k3win Create(const char* title,
        uint32_t x, uint32_t y, uint32_t width, uint32_t height,
        k3fmt color_format, bool fullscreen,
        uint32_t num_views, uint32_t num_samplers, k3gfx gfx);

    static k3win CreateFullscreen(const char* t, uint32_t w, uint32_t h, uint32_t nv, uint32_t ns) { return Create((t), 0, 0, (w), (h), k3fmt::RGBA8_UNORM, true, nv, ns, NULL); }
    static k3win CreateFullscreenWithFormat(const char* t, uint32_t w, uint32_t h, k3fmt cf, uint32_t nv, uint32_t ns) { return Create((t), 0, 0, (w), (h), (cf), true, nv, ns, NULL); }
    static k3win CreateFullscreenWithGfx(const char* t, uint32_t w, uint32_t h, k3fmt cf, k3gfx gfx) { return Create((t), 0, 0, (w), (h), (cf), true, 0, 0, (gfx)); }

    static k3win CreateWindowed(const char* t, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t nv, uint32_t ns) { return Create((t), (x), (y), (w), (h), k3fmt::RGBA8_UNORM, false, nv, ns, NULL); }
    static k3win CreateWindowedWithFormat(const char* t, uint32_t x, uint32_t y, uint32_t w, uint32_t h, k3fmt cf, uint32_t nv, uint32_t ns) { return Create((t), (x), (y), (w), (h), (cf), false, nv, ns, NULL); }
    static k3win CreateWindowedWithGfx(const char* t, uint32_t x, uint32_t y, uint32_t w, uint32_t h, k3fmt cf, k3gfx gfx) { return Create((t), (x), (y), (w), (h), (cf), false, 0, 0, (gfx)); }

    static K3API void Destroy(k3win win);

    virtual ~k3winObj();

    virtual K3API k3objType getObjType() const
    {
        return k3objType::WIN;
    }

    K3API void SetTitle(const char* title);
    K3API void SetSize(uint32_t width, uint32_t height);
    K3API void SetPosition(uint32_t x, uint32_t y);
    K3API void SetCursorPosition(int32_t x, int32_t y);
    K3API void SetVisible(bool visible);
    K3API void SetCursorVisible(bool visible);
    K3API void SetFullscreen(bool fullscreen);
    K3API void SetVsyncInterval(uint32_t interval);
    K3API void SetJoystickAttribute(uint32_t joystick, k3joyAttr attr_type, uint32_t num_values, float* values);
    K3API void SwapBuffer();
    K3API void SetDataPtr(void* data);

    K3API void SetDisplayFunc(k3win_display_ptr Display);
    K3API void SetIdleFunc(k3win_idle_ptr Idle);
    K3API void SetKeyboardFunc(k3win_keyboard_ptr Keyboard);
    K3API void SetMouseFunc(k3win_mouse_move_ptr MouseMove, k3win_mouse_button_ptr MouseButton, k3win_mouse_scroll_ptr MouseScroll);
    K3API void SetResizeFunc(k3win_resize_ptr Resize);
    K3API void SetJoystickFunc(k3win_joystick_added_ptr JoystickAdded, k3win_joystick_removed_ptr JoystickRemoved, k3win_joystick_move_ptr JoystickMove, k3win_joystick_button_ptr JoystickButton);
    K3API void SetDestroyFunc(k3win_destroy_ptr Destroy);

    K3API k3timer CreateTimer();
    K3API k3soundBuf CreateSoundBuffer(uint32_t num_channels, uint32_t samples_per_second, uint32_t bits_per_sample, uint32_t num_samples);

    K3API k3gfx GetGfx() const;
    K3API const char* GetTitle() const;
    K3API uint32_t GetWidth() const;
    K3API uint32_t GetHeight() const;
    K3API uint32_t GetXPosition() const;
    K3API uint32_t GetYPosition() const;
    K3API bool IsVisible() const;
    K3API bool IsCursorVisible() const;
    K3API bool IsFullscreen() const;
    K3API uint32_t GetVsyncInterval() const;
    K3API k3surf GetBackBuffer();

    static void WindowLoop();
    static void ExitLoop();
};
