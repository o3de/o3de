/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Common matrix class


#ifndef CRYINCLUDE_CRYCOMMON_CRY_MATRIX44_H
#define CRYINCLUDE_CRYCOMMON_CRY_MATRIX44_H
#pragma once


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// struct Matrix44_tpl
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
template<typename F>
struct Matrix44_tpl
{
    F m00, m01, m02, m03;
    F m10, m11, m12, m13;
    F m20, m21, m22, m23;
    F m30, m31, m32, m33;

    //---------------------------------------------------------------------------------

#if defined(_DEBUG)
    ILINE Matrix44_tpl()
    {
        if constexpr (sizeof(F) == 4)
        {
            uint32* p = (uint32*)&m00;
            p[ 0] = F32NAN;
            p[ 1] = F32NAN;
            p[ 2] = F32NAN;
            p[ 3] = F32NAN;
            p[ 4] = F32NAN;
            p[ 5] = F32NAN;
            p[ 6] = F32NAN;
            p[ 7] = F32NAN;
            p[ 8] = F32NAN;
            p[ 9] = F32NAN;
            p[10] = F32NAN;
            p[11] = F32NAN;
            p[12] = F32NAN;
            p[13] = F32NAN;
            p[14] = F32NAN;
            p[15] = F32NAN;
        }
        if constexpr (sizeof(F) == 8)
        {
            uint64* p = (uint64*)&m00;
            p[ 0] = F64NAN;
            p[ 1] = F64NAN;
            p[ 2] = F64NAN;
            p[ 3] = F64NAN;
            p[ 4] = F64NAN;
            p[ 5] = F64NAN;
            p[ 6] = F64NAN;
            p[ 7] = F64NAN;
            p[ 8] = F64NAN;
            p[ 9] = F64NAN;
            p[10] = F64NAN;
            p[11] = F64NAN;
            p[12] = F64NAN;
            p[13] = F64NAN;
            p[14] = F64NAN;
            p[15] = F64NAN;
        }
    }
#else
    ILINE Matrix44_tpl(){};
#endif

    //initialize with zeros
    ILINE Matrix44_tpl(type_zero)
    {
        m00 = 0;
        m01 = 0;
        m02 = 0;
        m03 = 0;
        m10 = 0;
        m11 = 0;
        m12 = 0;
        m13 = 0;
        m20 = 0;
        m21 = 0;
        m22 = 0;
        m23 = 0;
        m30 = 0;
        m31 = 0;
        m32 = 0;
        m33 = 0;
    }


    //ASSIGNMENT OPERATOR of identical Matrix44 types.
    //The assignment operator has precedence over assignment constructor
    //Matrix44 m; m=m44;
    ILINE Matrix44_tpl<F>& operator = (const Matrix44_tpl<F>& m)
    {
        assert(m.IsValid());
        m00 = m.m00;
        m01 = m.m01;
        m02 = m.m02;
        m03 = m.m03;
        m10 = m.m10;
        m11 = m.m11;
        m12 = m.m12;
        m13 = m.m13;
        m20 = m.m20;
        m21 = m.m21;
        m22 = m.m22;
        m23 = m.m23;
        m30 = m.m30;
        m31 = m.m31;
        m32 = m.m32;
        m33 = m.m33;
        return *this;
    }



    //--------------------------------------------------------------------
    //----   implementation of the constructors                        ---
    //--------------------------------------------------------------------
    ILINE Matrix44_tpl<F>(F v00, F v01, F v02, F v03,
                          F v10, F v11, F v12, F v13,
                          F v20, F v21, F v22, F v23,
                          F v30, F v31, F v32, F v33)
    {
        m00 = v00;
        m01 = v01;
        m02 = v02;
        m03 = v03;
        m10 = v10;
        m11 = v11;
        m12 = v12;
        m13 = v13;
        m20 = v20;
        m21 = v21;
        m22 = v22;
        m23 = v23;
        m30 = v30;
        m31 = v31;
        m32 = v32;
        m33 = v33;
    }






    //CONSTRUCTOR for different types. It converts a Matrix33 into a Matrix44.
    //Matrix44(m33);
    ILINE Matrix44_tpl<F>(const Matrix33_tpl<F>&m)
    {
        assert(m.IsValid());
        m00 = m.m00;
        m01 = m.m01;
        m02 = m.m02;
        m03 = 0;
        m10 = m.m10;
        m11 = m.m11;
        m12 = m.m12;
        m13 = 0;
        m20 = m.m20;
        m21 = m.m21;
        m22 = m.m22;
        m23 = 0;
        m30 = 0;
        m31 = 0;
        m32 = 0;
        m33 = 1;
    }
    //CONSTRUCTOR for different types. It converts a Matrix33 into a Matrix44 and also converts between double/float.
    //Matrix44(Matrix33);
    template<class F1>
    ILINE Matrix44_tpl<F>(const Matrix33_tpl<F1>&m)
    {
        assert(m.IsValid());
        m00 = F(m.m00);
        m01 = F(m.m01);
        m02 = F(m.m02);
        m03 = 0;
        m10 = F(m.m10);
        m11 = F(m.m11);
        m12 = F(m.m12);
        m13 = 0;
        m20 = F(m.m20);
        m21 = F(m.m21);
        m22 = F(m.m22);
        m23 = 0;
        m30 = 0;
        m31 = 0;
        m32 = 0;
        m33 = 1;
    }


    //CONSTRUCTOR for different types. It converts a Matrix34 into a Matrix44.
    //Matrix44(m34);
    ILINE Matrix44_tpl<F>(const Matrix34_tpl<F>&m)
    {
        assert(m.IsValid());
        m00 = m.m00;
        m01 = m.m01;
        m02 = m.m02;
        m03 = m.m03;
        m10 = m.m10;
        m11 = m.m11;
        m12 = m.m12;
        m13 = m.m13;
        m20 = m.m20;
        m21 = m.m21;
        m22 = m.m22;
        m23 = m.m23;
        m30 = 0;
        m31 = 0;
        m32 = 0;
        m33 = 1;
    }
    //CONSTRUCTOR for different types. It converts a Matrix34 into a Matrix44 and also converts between double/float.
    //Matrix44(Matrix34);
    template<class F1>
    ILINE Matrix44_tpl<F>(const Matrix34_tpl<F1>&m)
    {
        assert(m.IsValid());
        m00 = F(m.m00);
        m01 = F(m.m01);
        m02 = F(m.m02);
        m03 = F(m.m03);
        m10 = F(m.m10);
        m11 = F(m.m11);
        m12 = F(m.m12);
        m13 = F(m.m13);
        m20 = F(m.m20);
        m21 = F(m.m21);
        m22 = F(m.m22);
        m23 = F(m.m23);
        m30 = 0;
        m31 = 0;
        m32 = 0;
        m33 = 1;
    }


    //CONSTRUCTOR for identical types
    //Matrix44 m=m44;
    ILINE Matrix44_tpl<F>(const Matrix44_tpl<F>&m)
    {
        assert(m.IsValid());
        m00 = m.m00;
        m01 = m.m01;
        m02 = m.m02;
        m03 = m.m03;
        m10 = m.m10;
        m11 = m.m11;
        m12 = m.m12;
        m13 = m.m13;
        m20 = m.m20;
        m21 = m.m21;
        m22 = m.m22;
        m23 = m.m23;
        m30 = m.m30;
        m31 = m.m31;
        m32 = m.m32;
        m33 = m.m33;
    }

    //---------------------------------------------------------------------

    //! multiply all m1 matrix's values by f and return the matrix
    friend  ILINE Matrix44_tpl<F> operator * (const Matrix44_tpl<F>& m, const f32 f)
    {
        assert(m.IsValid());
        Matrix44_tpl<F> r;
        r.m00 = m.m00 * f;
        r.m01 = m.m01 * f;
        r.m02 = m.m02 * f;
        r.m03 = m.m03 * f;
        r.m10 = m.m10 * f;
        r.m11 = m.m11 * f;
        r.m12 = m.m12 * f;
        r.m13 = m.m13 * f;
        r.m20 = m.m20 * f;
        r.m21 = m.m21 * f;
        r.m22 = m.m22 * f;
        r.m23 = m.m23 * f;
        r.m30 = m.m30 * f;
        r.m31 = m.m31 * f;
        r.m32 = m.m32 * f;
        r.m33 = m.m33 * f;
        return r;
    }

    //! add all m matrix's values and return the matrix
    friend    ILINE Matrix44_tpl<F> operator + (const Matrix44_tpl<F>& mm0, const Matrix44_tpl<F>& mm1)
    {
        assert(mm0.IsValid());
        assert(mm1.IsValid());
        Matrix44_tpl<F> r;
        r.m00 = mm0.m00 + mm1.m00;
        r.m01 = mm0.m01 + mm1.m01;
        r.m02 = mm0.m02 + mm1.m02;
        r.m03 = mm0.m03 + mm1.m03;
        r.m10 = mm0.m10 + mm1.m10;
        r.m11 = mm0.m11 + mm1.m11;
        r.m12 = mm0.m12 + mm1.m12;
        r.m13 = mm0.m13 + mm1.m13;
        r.m20 = mm0.m20 + mm1.m20;
        r.m21 = mm0.m21 + mm1.m21;
        r.m22 = mm0.m22 + mm1.m22;
        r.m23 = mm0.m23 + mm1.m23;
        r.m30 = mm0.m30 + mm1.m30;
        r.m31 = mm0.m31 + mm1.m31;
        r.m32 = mm0.m32 + mm1.m32;
        r.m33 = mm0.m33 + mm1.m33;
        return r;
    }

    ILINE void SetIdentity()
    {
        m00 = 1;
        m01 = 0;
        m02 = 0;
        m03 = 0;
        m10 = 0;
        m11 = 1;
        m12 = 0;
        m13 = 0;
        m20 = 0;
        m21 = 0;
        m22 = 1;
        m23 = 0;
        m30 = 0;
        m31 = 0;
        m32 = 0;
        m33 = 1;
    }
    ILINE Matrix44_tpl(type_identity) { SetIdentity(); }


    ILINE void Transpose()
    {
        Matrix44_tpl<F> tmp = *this;
        m00 = tmp.m00;
        m01 = tmp.m10;
        m02 = tmp.m20;
        m03 = tmp.m30;
        m10 = tmp.m01;
        m11 = tmp.m11;
        m12 = tmp.m21;
        m13 = tmp.m31;
        m20 = tmp.m02;
        m21 = tmp.m12;
        m22 = tmp.m22;
        m23 = tmp.m32;
        m30 = tmp.m03;
        m31 = tmp.m13;
        m32 = tmp.m23;
        m33 = tmp.m33;
    }
    ILINE Matrix44_tpl<F> GetTransposed() const
    {
        Matrix44_tpl<F> tmp;
        tmp.m00 = m00;
        tmp.m01 = m10;
        tmp.m02 = m20;
        tmp.m03 = m30;
        tmp.m10 = m01;
        tmp.m11 = m11;
        tmp.m12 = m21;
        tmp.m13 = m31;
        tmp.m20 = m02;
        tmp.m21 = m12;
        tmp.m22 = m22;
        tmp.m23 = m32;
        tmp.m30 = m03;
        tmp.m31 = m13;
        tmp.m32 = m23;
        tmp.m33 = m33;
        return tmp;
    }

    /*!
    *
    * Calculate a real inversion of a Matrix44.
    *
    * Uses Cramer's Rule which is faster (branchless) but numerically more unstable
    * than other methods like Gaussian Elimination.
    *
    *  Example 1:
    *       Matrix44 im44; im44.Invert();
    *
    *  Example 2:
    *   Matrix44 im44 = m33.GetInverted();
    */
    void Invert(void)
    {
        F   tmp[12];
        Matrix44_tpl<F> m = *this;

        // Calculate pairs for first 8 elements (cofactors)
        tmp[0] = m.m22 * m.m33;
        tmp[1] = m.m32 * m.m23;
        tmp[2] = m.m12 * m.m33;
        tmp[3] = m.m32 * m.m13;
        tmp[4] = m.m12 * m.m23;
        tmp[5] = m.m22 * m.m13;
        tmp[6] = m.m02 * m.m33;
        tmp[7] = m.m32 * m.m03;
        tmp[8] = m.m02 * m.m23;
        tmp[9] = m.m22 * m.m03;
        tmp[10] = m.m02 * m.m13;
        tmp[11] = m.m12 * m.m03;

        // Calculate first 8 elements (cofactors)
        m00 = tmp[0] * m.m11 + tmp[3] * m.m21 + tmp[ 4] * m.m31;
        m00 -= tmp[1] * m.m11 + tmp[2] * m.m21 + tmp[ 5] * m.m31;
        m01 = tmp[1] * m.m01 + tmp[6] * m.m21 + tmp[ 9] * m.m31;
        m01 -= tmp[0] * m.m01 + tmp[7] * m.m21 + tmp[ 8] * m.m31;
        m02 = tmp[2] * m.m01 + tmp[7] * m.m11 + tmp[10] * m.m31;
        m02 -= tmp[3] * m.m01 + tmp[6] * m.m11 + tmp[11] * m.m31;
        m03 = tmp[5] * m.m01 + tmp[8] * m.m11 + tmp[11] * m.m21;
        m03 -= tmp[4] * m.m01 + tmp[9] * m.m11 + tmp[10] * m.m21;
        m10 = tmp[1] * m.m10 + tmp[2] * m.m20 + tmp[ 5] * m.m30;
        m10 -= tmp[0] * m.m10 + tmp[3] * m.m20 + tmp[ 4] * m.m30;
        m11 = tmp[0] * m.m00 + tmp[7] * m.m20 + tmp[ 8] * m.m30;
        m11 -= tmp[1] * m.m00 + tmp[6] * m.m20 + tmp[ 9] * m.m30;
        m12 = tmp[3] * m.m00 + tmp[6] * m.m10 + tmp[11] * m.m30;
        m12 -= tmp[2] * m.m00 + tmp[7] * m.m10 + tmp[10] * m.m30;
        m13 = tmp[4] * m.m00 + tmp[9] * m.m10 + tmp[10] * m.m20;
        m13 -= tmp[5] * m.m00 + tmp[8] * m.m10 + tmp[11] * m.m20;

        // Calculate pairs for second 8 elements (cofactors)
        tmp[ 0] = m.m20 * m.m31;
        tmp[ 1] = m.m30 * m.m21;
        tmp[ 2] = m.m10 * m.m31;
        tmp[ 3] = m.m30 * m.m11;
        tmp[ 4] = m.m10 * m.m21;
        tmp[ 5] = m.m20 * m.m11;
        tmp[ 6] = m.m00 * m.m31;
        tmp[ 7] = m.m30 * m.m01;
        tmp[ 8] = m.m00 * m.m21;
        tmp[ 9] = m.m20 * m.m01;
        tmp[10] = m.m00 * m.m11;
        tmp[11] = m.m10 * m.m01;

        // Calculate second 8 elements (cofactors)
        m20 = tmp[ 0] * m.m13 + tmp[ 3] * m.m23 + tmp[ 4] * m.m33;
        m20 -= tmp[ 1] * m.m13 + tmp[ 2] * m.m23 + tmp[ 5] * m.m33;
        m21 = tmp[ 1] * m.m03 + tmp[ 6] * m.m23 + tmp[ 9] * m.m33;
        m21 -= tmp[ 0] * m.m03 + tmp[ 7] * m.m23 + tmp[ 8] * m.m33;
        m22 = tmp[ 2] * m.m03 + tmp[ 7] * m.m13 + tmp[10] * m.m33;
        m22 -= tmp[ 3] * m.m03 + tmp[ 6] * m.m13 + tmp[11] * m.m33;
        m23 = tmp[ 5] * m.m03 + tmp[ 8] * m.m13 + tmp[11] * m.m23;
        m23 -= tmp[ 4] * m.m03 + tmp[ 9] * m.m13 + tmp[10] * m.m23;
        m30 = tmp[ 2] * m.m22 + tmp[ 5] * m.m32 + tmp[ 1] * m.m12;
        m30 -= tmp[ 4] * m.m32 + tmp[ 0] * m.m12 + tmp[ 3] * m.m22;
        m31 = tmp[ 8] * m.m32 + tmp[ 0] * m.m02 + tmp[ 7] * m.m22;
        m31 -= tmp[ 6] * m.m22 + tmp[ 9] * m.m32 + tmp[ 1] * m.m02;
        m32 = tmp[ 6] * m.m12 + tmp[11] * m.m32 + tmp[ 3] * m.m02;
        m32 -= tmp[10] * m.m32 + tmp[ 2] * m.m02 + tmp[ 7] * m.m12;
        m33 = tmp[10] * m.m22 + tmp[ 4] * m.m02 + tmp[ 9] * m.m12;
        m33 -= tmp[ 8] * m.m12 + tmp[11] * m.m22 + tmp[ 5] * m.m02;

        // Calculate determinant
        F det = (m.m00 * m00 + m.m10 * m01 + m.m20 * m02 + m.m30 * m03);
        //if (fabs_tpl(det)<0.0001f) assert(0);

        // Divide the cofactor-matrix by the determinant
        F idet = (F)1.0 / det;
        m00 *= idet;
        m01 *= idet;
        m02 *= idet;
        m03 *= idet;
        m10 *= idet;
        m11 *= idet;
        m12 *= idet;
        m13 *= idet;
        m20 *= idet;
        m21 *= idet;
        m22 *= idet;
        m23 *= idet;
        m30 *= idet;
        m31 *= idet;
        m32 *= idet;
        m33 *= idet;
    }

    ILINE Matrix44_tpl<F> GetInverted() const
    {
        Matrix44_tpl<F> dst = *this;
        dst.Invert();
        return dst;
    }


    ILINE f32 Determinant() const
    {
        //determinant is ambiguous: only the upper-left-submatrix's determinant is calculated
        return (m00 * m11 * m22) + (m01 * m12 * m20) + (m02 * m10 * m21) - (m02 * m11 * m20) - (m00 * m12 * m21) - (m01 * m10 * m22);
    }

    //! transform a vector
    ILINE Vec3 TransformVector(const Vec3& b) const
    {
        assert(b.IsValid());
        Vec3 v;
        v.x = m00 * b.x + m01 * b.y + m02 * b.z;
        v.y = m10 * b.x + m11 * b.y + m12 * b.z;
        v.z = m20 * b.x + m21 * b.y + m22 * b.z;
        return v;
    }
    //! transform a point
    ILINE Vec3 TransformPoint(const Vec3& b) const
    {
        assert(b.IsValid());
        Vec3 v;
        v.x = m00 * b.x + m01 * b.y + m02 * b.z + m03;
        v.y = m10 * b.x + m11 * b.y + m12 * b.z + m13;
        v.z = m20 * b.x + m21 * b.y + m22 * b.z + m23;
        return v;
    }


    //--------------------------------------------------------------------------------
    //----                  helper functions to access matrix-members     ------------
    //--------------------------------------------------------------------------------
    ILINE F* GetData() { return &m00; }
    ILINE const F* GetData() const { return &m00; }

    ILINE F operator ()  (uint32 i, uint32 j) const {   assert ((i < 4) && (j < 4));    F* p_data = (F*)(&m00);   return p_data[i * 4 + j];   }
    ILINE F& operator () (uint32 i, uint32 j)   {   assert ((i < 4) && (j < 4));    F* p_data = (F*)(&m00);       return p_data[i * 4 + j];   }

    ILINE void SetRow(int i, const Vec3_tpl<F>& v)  {   assert(i < 4);    F* p = (F*)(&m00);    p[0 + 4 * i] = v.x;   p[1 + 4 * i] = v.y;   p[2 + 4 * i] = v.z;       }
    ILINE void SetRow4(int i, const Vec4& v)   {   assert(i < 4);    F* p = (F*)(&m00);    p[0 + 4 * i] = v.x;   p[1 + 4 * i] = v.y;   p[2 + 4 * i] = v.z;   p[3 + 4 * i] = v.w;   }
    ILINE const Vec3_tpl<F>& GetRow(int i) const    {   assert(i < 4); return *(const Vec3_tpl<F>*)(&m00 + 4 * i);  }

    ILINE void SetColumn(int i, const Vec3_tpl<F>& v)   {   assert(i < 4);    F* p = (F*)(&m00);    p[i + 4 * 0] = v.x;   p[i + 4 * 1] = v.y;   p[i + 4 * 2] = v.z;       }
    ILINE Vec3_tpl<F> GetColumn(int i) const    {   assert(i < 4);    F* p = (F*)(&m00);    return Vec3(p[i + 4 * 0], p[i + 4 * 1], p[i + 4 * 2]);    }
    ILINE Vec4 GetColumn4(int i) const {   assert(i < 4);    F* p = (F*)(&m00);    return Vec4(p[i + 4 * 0], p[i + 4 * 1], p[i + 4 * 2], p[i + 4 * 3]);   }

    ILINE Vec3 GetTranslation() const { return Vec3(m03, m13, m23);   }
    ILINE void SetTranslation(const Vec3& t)  {   m03 = t.x; m13 = t.y; m23 = t.z; }

    bool IsValid() const
    {
        if (!NumberValid(m00))
        {
            return false;
        }
        if (!NumberValid(m01))
        {
            return false;
        }
        if (!NumberValid(m02))
        {
            return false;
        }
        if (!NumberValid(m03))
        {
            return false;
        }
        if (!NumberValid(m10))
        {
            return false;
        }
        if (!NumberValid(m11))
        {
            return false;
        }
        if (!NumberValid(m12))
        {
            return false;
        }
        if (!NumberValid(m13))
        {
            return false;
        }
        if (!NumberValid(m20))
        {
            return false;
        }
        if (!NumberValid(m21))
        {
            return false;
        }
        if (!NumberValid(m22))
        {
            return false;
        }
        if (!NumberValid(m23))
        {
            return false;
        }
        if (!NumberValid(m30))
        {
            return false;
        }
        if (!NumberValid(m31))
        {
            return false;
        }
        if (!NumberValid(m32))
        {
            return false;
        }
        if (!NumberValid(m33))
        {
            return false;
        }
        return true;
    }
    static bool IsEquivalent(const Matrix44_tpl<F>& m0, const Matrix44_tpl<F>& m1, F e = VEC_EPSILON)
    {
        return  (
            (fabs_tpl(m0.m00 - m1.m00) <= e) && (fabs_tpl(m0.m01 - m1.m01) <= e) && (fabs_tpl(m0.m02 - m1.m02) <= e) && (fabs_tpl(m0.m03 - m1.m03) <= e) &&
            (fabs_tpl(m0.m10 - m1.m10) <= e) && (fabs_tpl(m0.m11 - m1.m11) <= e) && (fabs_tpl(m0.m12 - m1.m12) <= e) && (fabs_tpl(m0.m13 - m1.m13) <= e) &&
            (fabs_tpl(m0.m20 - m1.m20) <= e) && (fabs_tpl(m0.m21 - m1.m21) <= e) && (fabs_tpl(m0.m22 - m1.m22) <= e) && (fabs_tpl(m0.m23 - m1.m23) <= e) &&
            (fabs_tpl(m0.m30 - m1.m30) <= e) && (fabs_tpl(m0.m31 - m1.m31) <= e) && (fabs_tpl(m0.m32 - m1.m32) <= e) && (fabs_tpl(m0.m33 - m1.m33) <= e)
            );
    }
};

///////////////////////////////////////////////////////////////////////////////
// Typedefs                                                                  //
///////////////////////////////////////////////////////////////////////////////

typedef Matrix44_tpl<f32>  Matrix44;   //always 32 bit

//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------
//-------------       implementation of Matrix44      ------------------------------
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------

/*!
*  Implements the multiplication operator: Matrix44=Matrix44*Matrix33
*
*  Matrix44 and Matrix33 are specified in collumn order.
*  AxB = rotation B followed by rotation A.
*  This operation takes 48 mults and 24 adds.
*
*  Example:
*   Matrix33 m34=CreateRotationX33(1.94192f);;
*   Matrix44 m44=CreateRotationZ33(3.14192f);
*     Matrix44 result=m44*m33;
*/
template<class F1,  class F2>
ILINE Matrix44_tpl<F1> operator * (const Matrix44_tpl<F1>& l, const Matrix33_tpl<F2>& r)
{
    assert(l.IsValid());
    assert(r.IsValid());
    Matrix44_tpl<F1> m;
    m.m00 = l.m00 * r.m00 + l.m01 * r.m10 + l.m02 * r.m20;
    m.m10 = l.m10 * r.m00 + l.m11 * r.m10 + l.m12 * r.m20;
    m.m20 = l.m20 * r.m00 + l.m21 * r.m10 + l.m22 * r.m20;
    m.m30 = l.m30 * r.m00 + l.m31 * r.m10 + l.m32 * r.m20;
    m.m01 = l.m00 * r.m01 + l.m01 * r.m11 + l.m02 * r.m21;
    m.m11 = l.m10 * r.m01 + l.m11 * r.m11 + l.m12 * r.m21;
    m.m21 = l.m20 * r.m01 + l.m21 * r.m11 + l.m22 * r.m21;
    m.m31 = l.m30 * r.m01 + l.m31 * r.m11 + l.m32 * r.m21;
    m.m02 = l.m00 * r.m02 + l.m01 * r.m12 + l.m02 * r.m22;
    m.m12 = l.m10 * r.m02 + l.m11 * r.m12 + l.m12 * r.m22;
    m.m22 = l.m20 * r.m02 + l.m21 * r.m12 + l.m22 * r.m22;
    m.m32 = l.m30 * r.m02 + l.m31 * r.m12 + l.m32 * r.m22;
    m.m03 = l.m03;
    m.m13 = l.m13;
    m.m23 = l.m23;
    m.m33 = l.m33;
    return m;
}


/*!
*  Implements the multiplication operator: Matrix44=Matrix44*Matrix34
*
*  Matrix44 and Matrix34 are specified in collumn order.
*  AxB = rotation B followed by rotation A.
*  This operation takes 48 mults and 36 adds.
*
*  Example:
*   Matrix34 m34=CreateRotationX33(1.94192f);;
*   Matrix44 m44=CreateRotationZ33(3.14192f);
*     Matrix44 result=m44*m34;
*/
template<class F>
ILINE Matrix44_tpl<F> operator * (const Matrix44_tpl<F>& l, const Matrix34_tpl<F>& r)
{
    assert(l.IsValid());
    assert(r.IsValid());
    Matrix44_tpl<F> m;
    m.m00 = l.m00 * r.m00 + l.m01 * r.m10 + l.m02 * r.m20;
    m.m10 = l.m10 * r.m00 + l.m11 * r.m10 + l.m12 * r.m20;
    m.m20 = l.m20 * r.m00 + l.m21 * r.m10 + l.m22 * r.m20;
    m.m30 = l.m30 * r.m00 + l.m31 * r.m10 + l.m32 * r.m20;
    m.m01 = l.m00 * r.m01 + l.m01 * r.m11 + l.m02 * r.m21;
    m.m11 = l.m10 * r.m01 + l.m11 * r.m11 + l.m12 * r.m21;
    m.m21 = l.m20 * r.m01 + l.m21 * r.m11 + l.m22 * r.m21;
    m.m31 = l.m30 * r.m01 + l.m31 * r.m11 + l.m32 * r.m21;
    m.m02 = l.m00 * r.m02 + l.m01 * r.m12 + l.m02 * r.m22;
    m.m12 = l.m10 * r.m02 + l.m11 * r.m12 + l.m12 * r.m22;
    m.m22 = l.m20 * r.m02 + l.m21 * r.m12 + l.m22 * r.m22;
    m.m32 = l.m30 * r.m02 + l.m31 * r.m12 + l.m32 * r.m22;
    m.m03 = l.m00 * r.m03 + l.m01 * r.m13 + l.m02 * r.m23 + l.m03;
    m.m13 = l.m10 * r.m03 + l.m11 * r.m13 + l.m12 * r.m23 + l.m13;
    m.m23 = l.m20 * r.m03 + l.m21 * r.m13 + l.m22 * r.m23 + l.m23;
    m.m33 = l.m30 * r.m03 + l.m31 * r.m13 + l.m32 * r.m23 + l.m33;
    return m;
}


/*!
*  Implements the multiplication operator: Matrix44=Matrix44*Matrix44
*
*  Matrix44 and Matrix34 are specified in collumn order.
*    AxB = rotation B followed by rotation A.
*  This operation takes 48 mults and 36 adds.
*
*  Example:
*   Matrix44 m44=CreateRotationX33(1.94192f);;
*   Matrix44 m44=CreateRotationZ33(3.14192f);
*     Matrix44 result=m44*m44;
*/
template<class F1, class F2>
ILINE Matrix44_tpl<F1> operator * (const Matrix44_tpl<F1>& l, const Matrix44_tpl<F2>& r)
{
    assert(l.IsValid());
    assert(r.IsValid());
    Matrix44_tpl<F1> res;
    res.m00 = l.m00 * r.m00 + l.m01 * r.m10 + l.m02 * r.m20 + l.m03 * r.m30;
    res.m10 = l.m10 * r.m00 + l.m11 * r.m10 + l.m12 * r.m20 + l.m13 * r.m30;
    res.m20 = l.m20 * r.m00 + l.m21 * r.m10 + l.m22 * r.m20 + l.m23 * r.m30;
    res.m30 = l.m30 * r.m00 + l.m31 * r.m10 + l.m32 * r.m20 + l.m33 * r.m30;
    res.m01 = l.m00 * r.m01 + l.m01 * r.m11 + l.m02 * r.m21 + l.m03 * r.m31;
    res.m11 = l.m10 * r.m01 + l.m11 * r.m11 + l.m12 * r.m21 + l.m13 * r.m31;
    res.m21 = l.m20 * r.m01 + l.m21 * r.m11 + l.m22 * r.m21 + l.m23 * r.m31;
    res.m31 = l.m30 * r.m01 + l.m31 * r.m11 + l.m32 * r.m21 + l.m33 * r.m31;
    res.m02 = l.m00 * r.m02 + l.m01 * r.m12 + l.m02 * r.m22 + l.m03 * r.m32;
    res.m12 = l.m10 * r.m02 + l.m11 * r.m12 + l.m12 * r.m22 + l.m13 * r.m32;
    res.m22 = l.m20 * r.m02 + l.m21 * r.m12 + l.m22 * r.m22 + l.m23 * r.m32;
    res.m32 = l.m30 * r.m02 + l.m31 * r.m12 + l.m32 * r.m22 + l.m33 * r.m32;
    res.m03 = l.m00 * r.m03 + l.m01 * r.m13 + l.m02 * r.m23 + l.m03 * r.m33;
    res.m13 = l.m10 * r.m03 + l.m11 * r.m13 + l.m12 * r.m23 + l.m13 * r.m33;
    res.m23 = l.m20 * r.m03 + l.m21 * r.m13 + l.m22 * r.m23 + l.m23 * r.m33;
    res.m33 = l.m30 * r.m03 + l.m31 * r.m13 + l.m32 * r.m23 + l.m33 * r.m33;
    return res;
}

//post-multiply
template<class F2>
ILINE Vec4 operator*(const Matrix44_tpl<F2>& m, const Vec4& v)
{
    assert(m.IsValid());
    assert(v.IsValid());
    return Vec4(v.x * m.m00 + v.y * m.m01 + v.z * m.m02 + v.w * m.m03,
        v.x * m.m10 + v.y * m.m11 + v.z * m.m12 + v.w * m.m13,
        v.x * m.m20 + v.y * m.m21 + v.z * m.m22 + v.w * m.m23,
        v.x * m.m30 + v.y * m.m31 + v.z * m.m32 + v.w * m.m33);
}

//pre-multiply
template<class F2>
ILINE Vec4 operator*(const Vec4& v, const Matrix44_tpl<F2>& m)
{
    assert(m.IsValid());
    assert(v.IsValid());
    return Vec4(v.x * m.m00 + v.y * m.m10 + v.z * m.m20 + v.w * m.m30,
        v.x * m.m01 + v.y * m.m11 + v.z * m.m21 + v.w * m.m31,
        v.x * m.m02 + v.y * m.m12 + v.z * m.m22 + v.w * m.m32,
        v.x * m.m03 + v.y * m.m13 + v.z * m.m23 + v.w * m.m33);
}


#endif // CRYINCLUDE_CRYCOMMON_CRY_MATRIX44_H

