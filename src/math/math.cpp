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
K3API float* k3m4_SetPerspectiveOffCenter(float* d, float left, float right, float bottom, float top, float znear, float zfar, bool left_handed, bool dx_style)
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
        d[10] = zfar / (znear - zfar);
        d[11] = znear * zfar / (znear - zfar);
    } else {
        d[10] = (zfar + znear) / (znear - zfar);
        d[11] = (2.0f * znear * zfar) / (znear - zfar);
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

K3API float* k3m4_SetPerspectiveFov(float* d, float fovy, float aspect, float znear, float zfar, bool left_handed, bool dx_style)
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
        d[10] = zfar / (znear - zfar);
        d[11] = znear * zfar / (znear - zfar);
    } else {
        d[10] = (zfar + znear) / (znear - zfar);
        d[11] = (2.0f * znear * zfar) / (znear - zfar);
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

K3API float* k3m4_SetOrthoOffCenter(float* d, float left, float right, float bottom, float top, float znear, float zfar, bool left_handed, bool dx_style)
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
        d[10] = 1.0f / (znear - zfar);
        d[11] = znear / (znear - zfar);
    } else {
        d[10] = 2.0f / (znear - zfar);
        d[11] = (znear + zfar) / (znear - zfar);
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
