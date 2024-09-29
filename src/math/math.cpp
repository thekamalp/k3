// k3 graphics library
// math fucntions

#include "k3internal.h"

/* operations to a single vector */
K3API float* k3v_Negate(uint32_t vec_length, float* d)
{
    uint32_t i;
    for (i = 0; i < vec_length; i++) d[i] = -d[i];
    return d;
}

K3API float* k3v_Swizzle(uint32_t vec_length, float* d, const uint32_t* indices)
{
    uint32_t i;
    float static_copy[K3_MATH_STATIC_ARRAY_SIZE];
    float* temp_copy;

    if (vec_length > K3_MATH_STATIC_ARRAY_SIZE)
        temp_copy = new float[vec_length];
    else
        temp_copy = static_copy;

    for (i = 0; i < vec_length; i++)
        temp_copy[i] = d[indices[i]];

    if (temp_copy != static_copy) {
        memcpy(d, temp_copy, vec_length * sizeof(float));
        delete[] temp_copy;
    } else {
        for (i = 0; i < vec_length; i++)
            d[i] = temp_copy[i];
    }

    return d;
}

K3API float  k3v_Length(uint32_t vec_length, const float* s)
{
    return sqrtf(k3v_Dot(vec_length, s, s));
}

/* operations of two vectors of the same length */
K3API float* k3v_Add(uint32_t vec_length, float* d, const float* s1, const float* s2)
{
    uint32_t i;
    float* dp = d;
    for (i = 0; i < vec_length; i++, dp++, s1++, s2++) *dp = *s1 + *s2;
    return d;
}

K3API float* k3v_Sub(uint32_t vec_length, float* d, const float* s1, const float* s2)
{
    uint32_t i;
    float* dp = d;
    for (i = 0; i < vec_length; i++, dp++, s1++, s2++) *dp = *s1 - *s2;
    return d;
}

K3API float* k3v_Mul(uint32_t vec_length, float* d, const float* s1, const float* s2)
{
    uint32_t i;
    float* dp = d;
    for (i = 0; i < vec_length; i++, dp++, s1++, s2++) *dp = *s1 * *s2;
    return d;
}

K3API float* k3v_Div(uint32_t vec_length, float* d, const float* s1, const float* s2)
{
    uint32_t i;
    float* dp = d;
    for (i = 0; i < vec_length; i++, dp++, s1++, s2++) *dp = *s1 / *s2;
    return d;
}

K3API float* k3v_Cross(uint32_t vec_length, float* d, const float* s1, const float* s2)
{
    switch (vec_length) {
    case 2:
        *d = s1[0] * s2[1] - s1[1] * s2[0];
        break;
    case 3:
    {
        float temp_copy[3];
        temp_copy[0] = s1[1] * s2[2] - s1[2] * s2[1];
        temp_copy[1] = s1[2] * s2[0] - s1[0] * s2[2];
        temp_copy[2] = s1[0] * s2[1] - s1[1] * s2[0];
        d[0] = temp_copy[0];
        d[1] = temp_copy[1];
        d[2] = temp_copy[2];
    }
    break;
    default:
    {
    }
    }

    return d;
}

K3API bool k3v_Equals(uint32_t vec_length, const float* s1, const float* s2)
{
    uint32_t i;
    bool result = true;
    for (i = 0; i < vec_length; i++, s1++, s2++) if (*s1 != *s2) result = false;
    return result;
}

K3API float k3v_Dot(uint32_t vec_length, const float* s1, const float* s2)
{
    uint32_t i;
    float result = 0.0f;
    for (i = 0; i < vec_length; i++, s1++, s2++) result += *s1 * *s2;
    return result;
}

K3API float* k3v_Min(uint32_t vec_length, float* d, const float* s1, const float* s2)
{
    uint32_t i;
    float* dp = d;
    for (i = 0; i < vec_length; i++, dp++, s1++, s2++) *dp = (*s1 < *s2) ? *s1 : *s2;
    return d;
}

K3API float* k3v_Max(uint32_t vec_length, float* d, const float* s1, const float* s2)
{
    uint32_t i;
    float* dp = d;
    for (i = 0; i < vec_length; i++, dp++, s1++, s2++) *dp = (*s1 > *s2) ? *s1 : *s2;
    return d;
}

K3API void k3v3_SetTangentBitangent(float* t, float* b, const float* p0, const float* p1, const float* p2, const float* u0, const float* u1, const float* u2)
{
    //float x1 = p1[0] - p0[0];
    //float x2 = p2[0] - p0[0];
    //float y1 = p1[1] - p0[0];
    //float y2 = p2[1] - p0[1];
    //float z1 = p1[2] - p0[2];
    //float z2 = p2[2] - p0[2];
    //
    //float s1 = u1[0] - u0[0];
    //float s2 = u2[0] - u0[0];
    //float t1 = u1[1] - u0[1];
    //float t2 = u2[1] - u0[0];
    //
    //float r = (s1 * y2 - s2 * t1);
    //if (r != 0.0f) r = 1.0f / r;
    //
    //if (t) {
    //    t[0] = (s2 * x1 - s1 * x2) * r;
    //    t[1] = (s2 * y1 - s1 * y2) * r;
    //    t[2] = (s2 * z1 - s2 * z2) * r;
    //}
    //if (b) {
    //    b[0] = (t2 * x1 - t1 * x2) * r;
    //    b[1] = (t2 * y1 - t1 * y2) * r;
    //    b[2] = (t2 * z1 - t2 * z2) * r;
    //}


    //float v[3][3];
    //float e1[3];
    //float e2[3];
    //uint32_t i;
    //k3v2_Sub(e1 + 1, u1, u0);
    //k3v2_Sub(e2 + 1, u2, u0);
    //if (e1[1] == 0.0f && e1[2] == 0.0f) {
    //    e1[1] = 1.0f;
    //}
    //if (e2[1] == 0.0f && e2[2] == 0.0f) {
    //    e2[2] = 1.0f;
    //}
    //if (e1[1] == e2[1] && e1[2] == e2[2]) {
    //    e2[1] = -e1[2];
    //    e2[2] = e1[1];
    //}
    ////e2[1] = 0.0f; e2[2] = 1.0f;
    ////e1[1] = 1.0f; e1[2] = 0.0f;
    //
    //for (i = 0; i < 3; i++) {
    //    e1[0] = p1[i] - p0[i];
    //    e2[0] = p2[i] - p0[i];
    //    k3v3_Cross(v[i], e1, e2);
    //}
    //for (i = 0; i < 3; i++) {
    //    if (v[0][i] == 0.0f)
    //        v[0][i] = 1.0f;
    //}
    //if (t) {
    //    if (v[1][0] == 0.0f && v[1][1] == 0.0f && v[1][2] == 0.0f) {
    //        v[1][0] = 1.0f;
    //    } else {
    //        k3v3_Div(t, v[1], v[0]);
    //        k3v3_Normalize(t);
    //    }
    //}
    //if (b) {
    //    if (v[2][0] == 0.0f && v[2][1] == 0.0f && v[2][2] == 0.0f) {
    //        v[2][1] = 1.0f;
    //    } else {
    //        k3v3_Div(b, v[2], v[0]);
    //        k3v2_Normalize(b);
    //    }
    //}

    float e[6];
    float udelta[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
    k3v3_Sub(e + 0, p1, p0);
    k3v3_Sub(e + 3, p2, p0);
    k3v2_Sub(udelta + 0, u1, u0);
    k3v2_Sub(udelta + 2, u2, u0);
    k3m2_Inverse(udelta);
    if (t) {
        k3m_Mul(1, 2, 3, t, udelta + 0, e);
    }
    if (b) {
        k3m_Mul(1, 2, 3, b, udelta + 2, e);
    }
}

/* operations where s1 is scalar and s2 is a vector */
K3API float* k3sv_Add(uint32_t vec_length, float* d, const float s1, const float* s2)
{
    uint32_t i;
    float* dp = d;
    for (i = 0; i < vec_length; i++, dp++, s2++) *dp = s1 + *s2;
    return d;
}

K3API float* k3sv_Sub(uint32_t vec_length, float* d, const float s1, const float* s2)
{
    uint32_t i;
    float* dp = d;
    for (i = 0; i < vec_length; i++, dp++, s2++) *dp = s1 - *s2;
    return d;
}

K3API float* k3sv_Mul(uint32_t vec_length, float* d, const float s1, const float* s2)
{
    uint32_t i;
    float* dp = d;
    for (i = 0; i < vec_length; i++, dp++, s2++) *dp = s1 * *s2;
    return d;
}

K3API float* k3sv_Div(uint32_t vec_length, float* d, const float s1, const float* s2)
{
    uint32_t i;
    float* dp = d;
    for (i = 0; i < vec_length; i++, dp++, s2++) *dp = s1 / *s2;
    return d;
}

/* operations on a single matrix */
float* k3m_Shrink(uint32_t rows, uint32_t cols, float* d, const float* s, uint32_t remove_row, uint32_t remove_col)
{
    uint32_t srow, scol, drow, dcol;
    float* dptr = d;

    srow = (remove_row + 1);
    scol = (remove_col + 1);
    for (drow = 0; drow < rows - 1; drow++) {
        if (srow >= rows) srow = 0;
        for (dcol = 0; dcol < cols - 1; dcol++) {
            if (scol >= cols) scol = 0;
            *dptr = s[(srow * cols) + scol];
            dptr++;
            scol++;
        }
        srow++;
    }

    return d;
}

K3API float  k3m_Determinant(uint32_t rows, const float* s)
{
    switch (rows) {
    case 2: return s[0] * s[3] - s[1] * s[2];
    case 3: return (s[0] * (s[4] * s[8] - s[5] * s[7]) +
                    s[1] * (s[5] * s[6] - s[3] * s[8]) +
                    s[2] * (s[3] * s[7] - s[4] * s[6]));
    case 4: return (s[0] * (s[5] * (s[10] * s[15] - s[11] * s[14]) +
                            s[6] * (s[11] * s[13] - s[9] * s[15]) +
                            s[7] * (s[9] * s[14] - s[10] * s[13])) +
                    s[1] * (s[6] * (s[11] * s[12] - s[8] * s[15]) +
                            s[7] * (s[8] * s[14] - s[10] * s[12]) +
                            s[4] * (s[10] * s[15] - s[11] * s[14])) +
                    s[2] * (s[7] * (s[8] * s[13] - s[9] * s[12]) +
                            s[4] * (s[9] * s[15] - s[11] * s[13]) +
                            s[5] * (s[11] * s[12] - s[8] * s[15])) +
                    s[3] * (s[4] * (s[9] * s[14] - s[10] * s[13]) +
                            s[5] * (s[10] * s[12] - s[8] * s[14]) +
                            s[6] * (s[8] * s[13] - s[9] * s[12])));
    default: {
        float* temp_mat = new float[(rows - 1) * (rows - 1)];
        float result = 0.0;
        uint32_t i;
        for (i = 0; i < rows; i++) {
            if (s[i] != 0.0f) {
                k3m_Shrink(rows, rows, temp_mat, s, 0, i);
                result += s[i] * k3m_Determinant(rows - 1, temp_mat);
            }
        }
        delete[] temp_mat;
        return result;
    }
    }
}

K3API float* k3m_Transpose(uint32_t rows, uint32_t cols, float* d)
{
    uint32_t r, c;
    if (rows == cols) {
        float temp;
        for (r = 0; r < rows; r++) {
            for (c = r + 1; c < cols; c++) {
                temp = d[r * cols + c];
                d[r * cols + c] = d[c * rows + r];
                d[c * rows + r] = temp;
            }
        }
    } else {
        float* temp_matrix = new float[rows * cols];
        for (r = 0; r < rows; r++) {
            for (c = 0; c < cols; c++) {
                temp_matrix[c * rows + r] = d[r * cols + c];
            }
        }
        memcpy(d, temp_matrix, rows * cols * sizeof(float));
        delete[] temp_matrix;
    }
    return d;
}

K3API float* k3m_Inverse(uint32_t rows, float* d)
{
    switch (rows) {
    case 2:
    {
        float copy[4];
        copy[0] = d[0]; copy[1] = d[1]; copy[2] = d[2]; copy[3] = d[3];
        d[0] = copy[3];
        d[1] = -copy[2];
        d[2] = -copy[1];
        d[3] = copy[0];
        float det = k3m_Determinant(2, d);
        if (det != 0) {
            det = 1.0f / det;
        }
        k3sv_Mul(4, d, det, d);
        return d;
    }
    break;
    case 3:
    {
        float copy[9];
        copy[0] = d[4] * d[8] - d[5] * d[7];
        copy[1] = d[5] * d[6] - d[3] * d[8];
        copy[2] = d[3] * d[7] - d[4] * d[6];

        copy[3] = d[2] * d[7] - d[1] * d[8];
        copy[4] = d[0] * d[8] - d[2] * d[6];
        copy[5] = d[1] * d[6] - d[0] * d[7];

        copy[6] = d[1] * d[5] - d[2] * d[4];
        copy[7] = d[2] * d[3] - d[0] * d[5];
        copy[8] = d[0] * d[4] - d[1] * d[3];

        float det = k3v_Dot(3, d, copy);
        if (det != 0) {
            det = 1.0f / det;
        }
        k3m_Transpose(3, 3, copy);
        k3sv_Mul(9, d, det, copy);
    }
    break;
    case 4:
    {
        float copy[16];
        float tmp[12];
        float det;

        uint32_t r, c;
        for (r = 0; r < 4; r++) {
            for (c = 0; c < 4; c++) {
                copy[r * rows + c] = d[c * rows + r];
            }
        }

        // calculate pairs for first 8 elements (cofactors)
        tmp[0] = copy[10] * copy[15];
        tmp[1] = copy[11] * copy[14];
        tmp[2] = copy[9] * copy[15];
        tmp[3] = copy[11] * copy[13];
        tmp[4] = copy[9] * copy[14];
        tmp[5] = copy[10] * copy[13];
        tmp[6] = copy[8] * copy[15];
        tmp[7] = copy[11] * copy[12];
        tmp[8] = copy[8] * copy[14];
        tmp[9] = copy[10] * copy[12];
        tmp[10] = copy[8] * copy[13];
        tmp[11] = copy[9] * copy[12];

        // calculate first 8 elements (cofactors)
        d[0] = tmp[0] * copy[5] + tmp[3] * copy[6] + tmp[4] * copy[7];
        d[0] -= tmp[1] * copy[5] + tmp[2] * copy[6] + tmp[5] * copy[7];
        d[1] = tmp[1] * copy[4] + tmp[6] * copy[6] + tmp[9] * copy[7];
        d[1] -= tmp[0] * copy[4] + tmp[7] * copy[6] + tmp[8] * copy[7];
        d[2] = tmp[2] * copy[4] + tmp[7] * copy[5] + tmp[10] * copy[7];
        d[2] -= tmp[3] * copy[4] + tmp[6] * copy[5] + tmp[11] * copy[7];
        d[3] = tmp[5] * copy[4] + tmp[8] * copy[5] + tmp[11] * copy[6];
        d[3] -= tmp[4] * copy[4] + tmp[9] * copy[5] + tmp[10] * copy[6];
        d[4] = tmp[1] * copy[1] + tmp[2] * copy[2] + tmp[5] * copy[3];
        d[4] -= tmp[0] * copy[1] + tmp[3] * copy[2] + tmp[4] * copy[3];
        d[5] = tmp[0] * copy[0] + tmp[7] * copy[2] + tmp[8] * copy[3];
        d[5] -= tmp[1] * copy[0] + tmp[6] * copy[2] + tmp[9] * copy[3];
        d[6] = tmp[3] * copy[0] + tmp[6] * copy[1] + tmp[11] * copy[3];
        d[6] -= tmp[2] * copy[0] + tmp[7] * copy[1] + tmp[10] * copy[3];
        d[7] = tmp[4] * copy[0] + tmp[9] * copy[1] + tmp[10] * copy[2];
        d[7] -= tmp[5] * copy[0] + tmp[8] * copy[1] + tmp[11] * copy[2];

        // calculate pairs for second 8 elements (cofactors)
        tmp[0] = copy[2] * copy[7];
        tmp[1] = copy[3] * copy[6];
        tmp[2] = copy[1] * copy[7];
        tmp[3] = copy[3] * copy[5];
        tmp[4] = copy[1] * copy[6];
        tmp[5] = copy[2] * copy[5];
        tmp[6] = copy[0] * copy[7];
        tmp[7] = copy[3] * copy[4];
        tmp[8] = copy[0] * copy[6];
        tmp[9] = copy[2] * copy[4];
        tmp[10] = copy[0] * copy[5];
        tmp[11] = copy[1] * copy[4];

        // calculate second 8 elements (cofactors)
        d[8] = tmp[0] * copy[13] + tmp[3] * copy[14] + tmp[4] * copy[15];
        d[8] -= tmp[1] * copy[13] + tmp[2] * copy[14] + tmp[5] * copy[15];
        d[9] = tmp[1] * copy[12] + tmp[6] * copy[14] + tmp[9] * copy[15];
        d[9] -= tmp[0] * copy[12] + tmp[7] * copy[14] + tmp[8] * copy[15];
        d[10] = tmp[2] * copy[12] + tmp[7] * copy[13] + tmp[10] * copy[15];
        d[10] -= tmp[3] * copy[12] + tmp[6] * copy[13] + tmp[11] * copy[15];
        d[11] = tmp[5] * copy[12] + tmp[8] * copy[13] + tmp[11] * copy[14];
        d[11] -= tmp[4] * copy[12] + tmp[9] * copy[13] + tmp[10] * copy[14];
        d[12] = tmp[2] * copy[10] + tmp[5] * copy[11] + tmp[1] * copy[9];
        d[12] -= tmp[4] * copy[11] + tmp[0] * copy[9] + tmp[3] * copy[10];
        d[13] = tmp[8] * copy[11] + tmp[0] * copy[8] + tmp[7] * copy[10];
        d[13] -= tmp[6] * copy[10] + tmp[9] * copy[11] + tmp[1] * copy[8];
        d[14] = tmp[6] * copy[9] + tmp[11] * copy[11] + tmp[3] * copy[8];
        d[14] -= tmp[10] * copy[11] + tmp[2] * copy[8] + tmp[7] * copy[9];
        d[15] = tmp[10] * copy[10] + tmp[4] * copy[8] + tmp[9] * copy[9];
        d[15] -= tmp[8] * copy[9] + tmp[11] * copy[10] + tmp[5] * copy[8];

        // calculate determinant
        det = k3v_Dot(4, copy, d);
        if (det != 0) {
            det = 1 / det;
        }
        k3sv_Mul(16, d, det, d);
    }
    break;
    default:
    {
    }
    }

    return d;
}

K3API float* k3m_Swizzle(uint32_t rows, uint32_t cols, float* d, const uint32_t* row_indices, const uint32_t* col_indices)
{
    uint32_t i, len = rows * cols;
    float static_copy[K3_MATH_STATIC_ARRAY_SIZE];
    float* temp_copy;

    if (len > K3_MATH_STATIC_ARRAY_SIZE) {
        temp_copy = new float[len];
    } else {
        temp_copy = static_copy;
    }

    for (i = 0; i < len; i++) {
        temp_copy[i] = d[row_indices[i] * cols + col_indices[i]];
    }

    if (temp_copy != static_copy) {
        memcpy(d, temp_copy, len);
        delete[] temp_copy;
    } else {
        for (i = 0; i < len; i++) d[i] = temp_copy[i];
    }

    return d;
}

K3API float* k3m_SetIdentity(uint32_t rows, float* d)
{
    uint32_t r, c;
    float* dptr = d;

    for (r = 0; r < rows; r++) {
        for (c = 0; c < rows; c++) {
            *dptr = (r == c) ? 1.0f : 0.0f;
            dptr++;
        }
    }

    return d;
}

K3API float* k3m_SetRotation(uint32_t rows, float* d, float angle, const float* axis)
{
    float cos_ang = cosf(angle);
    float sin_ang = sinf(angle);

    switch (rows) {
    case 2:
        d[0] = cos_ang;
        d[1] = -sin_ang;
        d[2] = sin_ang;
        d[3] = cos_ang;
        break;
    case 3:
        d[0] = cos_ang + (1 - cos_ang) * axis[0] * axis[0];
        d[1] = sin_ang * axis[2] + (1 - cos_ang) * axis[0] * axis[1];
        d[2] = -sin_ang * axis[1] + (1 - cos_ang) * axis[0] * axis[2];

        d[3] = -sin_ang * axis[2] + (1 - cos_ang) * axis[1] * axis[0];
        d[4] = cos_ang + (1 - cos_ang) * axis[1] * axis[1];
        d[5] = sin_ang * axis[0] + (1 - cos_ang) * axis[1] * axis[2];

        d[6] = sin_ang * axis[1] + (1 - cos_ang) * axis[2] * axis[0];
        d[7] = -sin_ang * axis[0] + (1 - cos_ang) * axis[2] * axis[1];
        d[8] = cos_ang + (1 - cos_ang) * axis[2] * axis[2];
        break;
    case 4:
        d[0] = cos_ang + (1 - cos_ang) * axis[0] * axis[0];
        d[1] = sin_ang * axis[2] + (1 - cos_ang) * axis[0] * axis[1];
        d[2] = -sin_ang * axis[1] + (1 - cos_ang) * axis[0] * axis[2];
        d[3] = 0.0f;

        d[4] = -sin_ang * axis[2] + (1 - cos_ang) * axis[1] * axis[0];
        d[5] = cos_ang + (1 - cos_ang) * axis[1] * axis[1];
        d[6] = sin_ang * axis[0] + (1 - cos_ang) * axis[1] * axis[2];
        d[7] = 0.0f;

        d[8] = sin_ang * axis[1] + (1 - cos_ang) * axis[2] * axis[0];
        d[9] = -sin_ang * axis[0] + (1 - cos_ang) * axis[2] * axis[1];
        d[10] = cos_ang + (1 - cos_ang) * axis[2] * axis[2];
        d[11] = 0.0f;

        d[12] = 0.0f;
        d[13] = 0.0f;
        d[14] = 0.0f;
        d[15] = 1.0f;
        break;
    }

    return d;
}

/* operations on a single 4x4 matrix */
K3API float* k3m4_SetPerspectiveOffCenter(float* d, float left, float right, float bottom, float top, float znear, float zfar, bool left_handed, bool dx_style, bool reverse_z)
{
    d[0] = 2.0f * znear / (right - left);
    d[1] = 0.0f;
    d[2] = (left + right) / (right - left);
    d[3] = 0.0f;

    d[4] = 0.0f;
    d[5] = 2.0f * znear / (top - bottom);
    d[6] = (bottom + top) / (top - bottom);
    d[7] = 0.0f;

    d[8] = 0.0f;
    d[9] = 0.0f;
    if (dx_style) {
        if (zfar == INFINITY) {
            if (reverse_z) {
                d[10] = 0.0f;
                d[11] = znear;
            } else {
                d[10] = -1.0f;
                d[11] = -znear;
            }
        } else {
            if (reverse_z) {
                d[10] = znear / (zfar - znear);
                d[11] = znear * zfar / (zfar - znear);
            } else {
                d[10] = zfar / (znear - zfar);
                d[11] = znear * zfar / (znear - zfar);
            }
        }
    } else {
        if (zfar == INFINITY) {
            if (reverse_z) {
                d[10] = 0.0f;
                d[11] = znear;
            } else {
                d[10] = -1.0;
                d[11] = -2.0f * znear;
            }
        } else {
            if (reverse_z) {
                d[10] = znear / (zfar - znear);
                d[11] = znear * zfar / (zfar - znear);
            } else {
                d[10] = (zfar + znear) / (znear - zfar);
                d[11] = (2.0f * znear * zfar) / (znear - zfar);
            }
        }
    }

    d[12] = 0.0f;
    d[13] = 0.0f;
    d[14] = -1.0f;
    d[15] = 0.0f;

    if (left_handed) {
        d[2] = -d[2];
        d[6] = -d[6];
        d[10] = -d[10];
        d[14] = -d[14];
    }
    return d;
}

K3API float* k3m4_SetPerspectiveFov(float* d, float fovy, float aspect, float znear, float zfar, bool left_handed, bool dx_style, bool reverse_z)
{
    float tan_f = tanf(fovy / 2.0f);

    d[0] = 1.0f / (aspect * tan_f);
    d[1] = 0.0f;
    d[2] = 0.0f;
    d[3] = 0.0f;

    d[4] = 0.0f;
    d[5] = 1.0f / tan_f;
    d[6] = 0.0f;
    d[7] = 0.0f;

    d[8] = 0.0f;
    d[9] = 0.0f;
    if (dx_style) {
        if (zfar == INFINITY) {
            if (reverse_z) {
                d[10] = 0.0f;
                d[11] = znear;
            } else {
                d[10] = -1.0f;
                d[11] = -znear;
            }
        } else {
            if (reverse_z) {
                d[10] = znear / (zfar - znear);
                d[11] = znear * zfar / (zfar - znear);
            } else {
                d[10] = zfar / (znear - zfar);
                d[11] = znear * zfar / (znear - zfar);
            }
        }
    } else {
        if (zfar == INFINITY) {
            if (reverse_z) {
                d[10] = 0.0f;
                d[11] = znear;
            } else {
                d[10] = -1.0;
                d[11] = -2.0f * znear;
            }
        } else {
            if (reverse_z) {
                d[10] = znear / (zfar - znear);
                d[11] = znear * zfar / (zfar - znear);
            } else {
                d[10] = (zfar + znear) / (znear - zfar);
                d[11] = (2.0f * znear * zfar) / (znear - zfar);
            }
        }
    }

    d[12] = 0.0f;
    d[13] = 0.0f;
    d[14] = -1.0f;
    d[15] = 0.0f;

    if (left_handed) {
        d[10] = -d[10];
        d[14] = -d[14];
    }

    return d;
}

K3API float* k3m4_SetOrthoOffCenter(float* d, float left, float right, float bottom, float top, float znear, float zfar, bool left_handed, bool dx_style, bool reverse_z)
{
    d[0] = 2.0f / (right - left);
    d[1] = 0.0f;
    d[2] = 0.0f;
    d[3] = (left + right) / (left - right);

    d[4] = 0.0f;
    d[5] = 2.0f / (top - bottom);
    d[6] = 0.0f;
    d[7] = (bottom + top) / (bottom - top);

    d[8] = 0.0f;
    d[9] = 0.0f;
    if (dx_style) {
        if (reverse_z) {
            d[10] = 1.0f / (zfar - znear);
            d[11] = zfar / (zfar - znear);
        } else {
            d[10] = 1.0f / (znear - zfar);
            d[11] = znear / (znear - zfar);
        }
    } else {
        if (reverse_z) {
            d[10] = 2.0f / (zfar - znear);
            d[11] = (znear + zfar) / (zfar - znear);
        } else {
            d[10] = 2.0f / (znear - zfar);
            d[11] = (znear + zfar) / (znear - zfar);
        }
    }

    d[12] = 0.0f;
    d[13] = 0.0f;
    d[14] = 0.0f;
    d[15] = 1.0f;

    if (left_handed)
        d[10] = -d[10];

    return d;
}

K3API float* k3m4_SetLookAt(float* d, const float* eye, const float* at, const float* up_dir, bool left_handed)
{
    float* xaxis = &(d[0]), * yaxis = &(d[4]), * zaxis = &(d[8]);

    if (left_handed) {
        k3v3_Normalize(k3v3_Sub(zaxis, at, eye));
    } else {
        k3v3_Normalize(k3v3_Sub(zaxis, eye, at));
    }
    k3v3_Normalize(k3v3_Cross(xaxis, up_dir, zaxis));
    k3v3_Cross(yaxis, zaxis, xaxis);

    d[3] = -k3v3_Dot(xaxis, eye);
    d[7] = -k3v3_Dot(yaxis, eye);
    d[11] = -k3v3_Dot(zaxis, eye);
    d[12] = 0.0f;
    d[13] = 0.0f;
    d[14] = 0.0f;
    d[15] = 1.0f;

    return d;
}

K3API float* k3m4_SetRotAngleScaleXlat(float* d, const float* r3, const float* s3, const float* t3)
{
    float mat[16];
    float x_axis[3] = { 1.0f, 0.0f, 0.0f };
    float y_axis[3] = { 0.0f, 1.0f, 0.0f };
    float z_axis[3] = { 0.0f, 0.0f, 1.0f };
    k3m4_SetRotation(d, -deg2rad(r3[0]), x_axis);
    k3m4_SetRotation(mat, -deg2rad(r3[1]), y_axis);
    k3m4_Mul(d, mat, d);
    k3m4_SetRotation(mat, -deg2rad(r3[2]), z_axis);
    k3m4_Mul(d, mat, d);
    k3m4_SetIdentity(mat);
    mat[0] = s3[0];
    mat[5] = s3[1];
    mat[10] = s3[2];
    k3m4_Mul(d, mat, d);
    mat[0] = 1.0f;
    mat[5] = 1.0f;
    mat[10] = 1.0f;
    mat[3] = t3[0];
    mat[7] = t3[1];
    mat[11] = t3[2];
    k3m4_Mul(d, mat, d);
    return d;
}

K3API float* k3m4_SetScaleRotAngleXlat(float* d, const float* s3, const float* r3, const float* t3)
{
    float mat[16];
    float x_axis[3] = { 1.0f, 0.0f, 0.0f };
    float y_axis[3] = { 0.0f, 1.0f, 0.0f };
    float z_axis[3] = { 0.0f, 0.0f, 1.0f };
    k3m4_SetIdentity(d);
    d[0] = s3[0];
    d[5] = s3[1];
    d[10] = s3[2];
    k3m4_SetRotation(mat, -deg2rad(r3[0]), x_axis);
    k3m4_Mul(d, mat, d);
    k3m4_SetRotation(mat, -deg2rad(r3[1]), y_axis);
    k3m4_Mul(d, mat, d);
    k3m4_SetRotation(mat, -deg2rad(r3[2]), z_axis);
    k3m4_Mul(d, mat, d);
    k3m4_SetIdentity(mat);
    mat[3] = t3[0];
    mat[7] = t3[1];
    mat[11] = t3[2];
    k3m4_Mul(d, mat, d);
    return d;
}


/* operations on 2 matrices */
K3API float* k3m_Mul(uint32_t s1_rows, uint32_t s2_rows, uint32_t s2_cols, float* d, const float* s1, const float* s2)
{
    float static_copy1[K3_MATH_STATIC_ARRAY_SIZE];
    float static_copy2[K3_MATH_STATIC_ARRAY_SIZE];
    const float* s1ptr;
    float* s2ptr;
    float* dptr;
    uint32_t s1size = s1_rows * s2_rows;
    uint32_t s2size = s2_rows * s2_cols;
    uint32_t r, c;
    const float* v1;
    float* v2;

    // if s1 == d, then make a copy of s1
    if (s1 == d) {
        float* s1temp;
        if (s1size > K3_MATH_STATIC_ARRAY_SIZE) {
            s1temp = new float[s1size];
            memcpy(s1temp, s1, s1size);
        } else {
            s1temp = static_copy1;
            for (r = 0; r < s1size; r++) s1temp[r] = s1[r];
        }
        s1ptr = s1temp;
    } else {
        s1ptr = s1;
    }

    // Make a tranposed copy of s2
    if (s2size > K3_MATH_STATIC_ARRAY_SIZE) {
        s2ptr = new float[s2size];
    } else {
        s2ptr = static_copy2;
    }
    memcpy(s2ptr, s2, s2size * sizeof(float));
    k3m_Transpose(s2_rows, s2_cols, s2ptr);

    // Loop through every row of s1, and coumn of s2,
    // and take the dot product of the row/column pair, and assign to it the
    // (rth, cth) entry of the destination matrix
    dptr = d;
    for (r = 0, v1 = s1ptr; r < s1_rows; r++, v1 += s2_rows) {
        for (c = 0, v2 = s2ptr; c < s2_cols; c++, v2 += s2_rows) {
            *dptr = k3v_Dot(s2_rows, v1, v2);
            dptr++;
        }
    }

    // Free any memory that was allocated
    if ((s1ptr != s1) && (s1ptr != static_copy1)) delete[] s1ptr;
    if (s2ptr != static_copy2) delete[] s2ptr;

    return d;
}

#ifdef _WIN32
#include <xmmintrin.h>
K3API float* k3m4_Mul(float* d, const float* s1, const float* s2)
{
    uint64_t s1_addr = (uint64_t)s1;
    uint64_t s2_addr = (uint64_t)s2;
    if ((s1_addr & 0xf) || (s2_addr & 0xf)) {
        return k3m_Mul(4, 4, 4, (d), (s1), (s2));
    }
    __m128 row1 = _mm_load_ps(&s2[0]);
    __m128 row2 = _mm_load_ps(&s2[4]);
    __m128 row3 = _mm_load_ps(&s2[8]);
    __m128 row4 = _mm_load_ps(&s2[12]);
    uint32_t i;
    for (i = 0; i < 4; i++) {
        __m128 brod1 = _mm_set1_ps(s1[4 * i + 0]);
        __m128 brod2 = _mm_set1_ps(s1[4 * i + 1]);
        __m128 brod3 = _mm_set1_ps(s1[4 * i + 2]);
        __m128 brod4 = _mm_set1_ps(s1[4 * i + 3]);
        __m128 row = _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(brod1, row1),
                _mm_mul_ps(brod2, row2)),
            _mm_add_ps(
                _mm_mul_ps(brod3, row3),
                _mm_mul_ps(brod4, row4)));
        _mm_store_ps(&d[4 * i], row);
    }
    return d;
}
#else
K3API float* k3m4_Mul(float* d, const float* s1, const float* s2)
{
    return k3m_Mul(4, 4, 4, (d), (s1), (s2));
}
#endif

K3API float* k3m_QuatToMat(uint32_t cols, float* d, const float* s)
{
    // cols must >= 3
    d[0 * cols + 0] = s[3] * s[3] + s[0] * s[0] - s[1] * s[1] - s[2] * s[2];
    d[0 * cols + 1] = 2 * s[0] * s[1] - 2 * s[3] * s[2];
    d[0 * cols + 2] = 2 * s[0] * s[2] + 2 * s[3] * s[1];
    d[1 * cols + 0] = 2 * s[0] * s[1] + 2 * s[3] * s[2];
    d[1 * cols + 1] = s[3] * s[3] - s[0] * s[0] + s[1] * s[1] - s[2] * s[2];
    d[1 * cols + 2] = 2 * s[1] * s[2] - 2 * s[3] * s[0];
    d[2 * cols + 0] = 2 * s[0] * s[2] - 2 * s[3] * s[1];
    d[2 * cols + 1] = 2 * s[1] * s[2] + 2 * s[3] * s[0];
    d[2 * cols + 2] = s[3] * s[3] - s[0] * s[0] - s[1] * s[1] + s[2] * s[2];
    if (cols > 3) {
        d[0 * cols + 3] = 0.0f;
        d[1 * cols + 3] = 0.0f;
        d[2 * cols + 3] = 0.0f;
        d[3 * cols + 0] = 0.0f;
        d[3 * cols + 1] = 0.0f;
        d[3 * cols + 2] = 0.0f;
        d[3 * cols + 3] = 1.0f;
    }
    return d;
}

K3API float* k3m_MatToQuat(uint32_t cols, float* d, const float* s)
{
    float r11 = s[0 * cols + 0];
    float r12 = s[0 * cols + 1];
    float r13 = s[0 * cols + 2];
    float r21 = s[1 * cols + 0];
    float r22 = s[1 * cols + 1];
    float r23 = s[1 * cols + 2];
    float r31 = s[2 * cols + 0];
    float r32 = s[2 * cols + 1];
    float r33 = s[2 * cols + 2];
    d[0] = (1 + r11 - r22 - r33) / 4.0f;
    d[1] = (1 - r11 + r22 - r33) / 4.0f;
    d[2] = (1 - r11 - r22 + r33) / 4.0f;
    d[3] = (1 + r11 + r22 + r33) / 4.0f;
    d[0] = (d[0] < 0.0f) ? 0.0f : sqrtf(d[0]);
    d[1] = (d[1] < 0.0f) ? 0.0f : sqrtf(d[1]);
    d[2] = (d[2] < 0.0f) ? 0.0f : sqrtf(d[2]);
    d[3] = (d[3] < 0.0f) ? 0.0f : sqrtf(d[3]);
    if (d[3] >= d[2] && d[3] >= d[1] && d[3] >= d[0]) {
        d[0] = (r32 - r23) / (4.0f * d[3]);
        d[1] = (r13 - r31) / (4.0f * d[3]);
        d[2] = (r21 - r12) / (4.0f * d[3]);
    } else if (d[0] >= d[2] && d[0] >= d[1]) {
        d[3] = (r32 - r23) / (4.0f * d[0]);
        d[1] = (r12 + r21) / (4.0f * d[0]);
        d[2] = (r13 + r31) / (4.0f * d[0]);
    } else if (d[1] >= d[2]) {
        d[3] = (r13 - r31) / (4.0f * d[1]);
        d[0] = (r12 + r21) / (4.0f * d[1]);
        d[2] = (r23 + r32) / (4.0f * d[1]);
    } else {
        d[3] = (r21 - r12) / (4.0f * d[2]);
        d[0] = (r13 + r31) / (4.0f * d[2]);
        d[1] = (r23 + r32) / (4.0f * d[2]);
    }
    return d;
}

K3API float* k3v4_SetQuatRotation(float* d, float angle, const float* axis)
{
    float cos_ang2 = cosf(angle / 2.0f);
    float sin_ang2 = sinf(angle / 2.0f);
    d[0] = axis[0] * sin_ang2;
    d[1] = axis[1] * sin_ang2;
    d[2] = axis[2] * sin_ang2;
    d[3] = cos_ang2;
    return d;
}

K3API float k3v3_GetQuatRotation(float* axis, float* angle, const float* quat)
{
    float ang = 2 * acosf(quat[3]);
    float sin_ang2 = sinf(ang / 2.0f);
    if (angle) *angle = ang;
    axis[0] = quat[0] / sin_ang2;
    axis[1] = quat[1] / sin_ang2;
    axis[2] = quat[2] / sin_ang2;
    return ang;
}

K3API float* k3v4_SetQuatEuler(float* d, const float* angles)
{
    float sin_x2 = sinf(angles[0] / 2.0f);
    float cos_x2 = cosf(angles[0] / 2.0f);
    float sin_y2 = sinf(angles[1] / 2.0f);
    float cos_y2 = cosf(angles[1] / 2.0f);
    float sin_z2 = sinf(angles[2] / 2.0f);
    float cos_z2 = cosf(angles[2] / 2.0f);
    d[0] = sin_x2 * cos_y2 * cos_z2 - cos_x2 * sin_y2 * sin_z2;
    d[1] = cos_x2 * sin_y2 * cos_z2 + sin_x2 * cos_y2 * sin_z2;
    d[2] = cos_x2 * cos_y2 * sin_z2 - sin_x2 * sin_y2 * cos_y2;
    d[3] = cos_x2 * cos_y2 * cos_z2 + sin_x2 * sin_y2 * sin_z2;
    return d;
}

K3API float* k3v3_GetQuatEuler(float* d, const float* quat)
{
    float t0, t1, t2, t3, t4;
    t0 = 2.0f * (quat[3] * quat[0] + quat[1] * quat[2]);
    t1 = 1.0f - 2.0f * (quat[0] * quat[0] + quat[1] * quat[1]);

    t2 = 2.0f * (quat[3] * quat[1] - quat[2] * quat[0]);
    t2 = (t2 > 1.0f) ? 1.0f : t2;
    t2 = (t2 < -1.0f) ? -1.0f : t2;

    t3 = 2.0f * (quat[3] * quat[2] + quat[0] * quat[1]);
    t4 = 1.0f - 2.0f * (quat[1] * quat[1] + quat[2] * quat[2]);

    d[0] = atan2f(t0, t1);
    d[1] = asinf(t2);
    d[2] = atan2f(t3, t4);

    return d;
}

K3API float* k3v4_QuatConjugate(float* d)
{
    d[0] = -d[0];
    d[1] = -d[1];
    d[2] = -d[2];
    return d;
}

K3API float* k3v4_QuatMul(float* d, const float* s1, const float* s2)
{
    float a[4];

    a[3] = s1[3] * s2[3] - s1[0] * s2[0] - s1[1] * s2[1] - s1[2] * s2[2];
    a[0] = s1[3] * s2[0] + s1[0] * s2[3] + s1[1] * s2[2] - s1[2] * s2[1];
    a[1] = s1[3] * s2[1] + s1[1] * s2[3] - s1[0] * s2[2] + s1[2] * s2[0];
    a[2] = s1[3] * s2[2] + s1[2] * s2[3] + s1[0] * s2[1] - s1[1] * s2[0];

    d[0] = a[0];
    d[1] = a[1];
    d[2] = a[2];
    d[3] = a[3];

    return d;
}
