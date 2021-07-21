/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Classes for TCB Spline curves
// Notice      : Deprecated by 2DSpline h


#ifndef CRYINCLUDE_CRYMOVIE_TCBSPLINE_H
#define CRYINCLUDE_CRYMOVIE_TCBSPLINE_H
#pragma once


#include <ISplines.h>




namespace spline
{
    //! Quaternion interpolation for angles > 2PI.
    ILINE static Quat CreateSquadRev(f32 angle,                 // angle of rotation
        const Vec3& axis,   // the axis of rotation
        const Quat& p,      // start quaternion
        const Quat& a,  // start tangent quaternion
        const Quat& b,  // end tangent quaternion
        const Quat& q,      // end quaternion
        f32 t)                          // Time parameter, in range [0,1]
    {
        f32 s, v;
        f32 omega = 0.5f * angle;
        f32 nrevs = 0;
        Quat r, pp, qq;

        if (omega < (gf_PI - 0.00001f))
        {
            return Quat::CreateSquad(p, a, b, q, t);
        }

        while (omega > (gf_PI - 0.00001f))
        {
            omega -= gf_PI;
            nrevs += 1.0f;
        }
        if (omega < 0)
        {
            omega = 0;
        }
        s = t * angle / gf_PI;      // 2t(omega+Npi)/pi

        if (s < 1.0f)
        {
            pp = p * Quat(0.0f, axis);//pp = p.Orthog( axis );
            r = Quat::CreateSquad(p, a, pp, pp, s); // in first 90 degrees.
        }
        else
        {
            v = s + 1.0f - 2.0f * (nrevs + (omega / gf_PI));
            if (v <= 0.0f)
            {
                // middle part, on great circle(p,q).
                while (s >= 2.0f)
                {
                    s -= 2.0f;
                }
                pp = p * Quat(0.0f, axis);//pp = p.Orthog(axis);
                r = Quat::CreateSlerp(p, pp, s);
            }
            else
            {
                // in last 90 degrees.
                qq = -q* Quat(0.0f, axis);
                r = Quat::CreateSquad(qq, qq, b, q, v);
            }
        }
        return r;
    }




    /** TCB spline key extended for tangent unify/break.
    */
    template    <class T>
    struct TCBSplineKeyEx
        :  public TCBSplineKey<T>
    {
        float theta_from_dd_to_ds;
        float scale_from_dd_to_ds;

        void ComputeThetaAndScale() { assert(0); }
        void SetOutTangentFromIn() { assert(0); }
        void SetInTangentFromOut() { assert(0); }

        TCBSplineKeyEx()
            : theta_from_dd_to_ds(gf_PI)
            , scale_from_dd_to_ds(1.0f) {}
    };

    template <>
    inline void TCBSplineKeyEx<float>::ComputeThetaAndScale()
    {
        scale_from_dd_to_ds = (easeto + 1.0f) / (easefrom + 1.0f);
        float out = atan_tpl(dd);
        float in = atan_tpl(ds);
        theta_from_dd_to_ds = in + gf_PI - out;
    }

    template<>
    inline void TCBSplineKeyEx<float>::SetOutTangentFromIn()
    {
        assert((flags & SPLINE_KEY_TANGENT_ALL_MASK) == SPLINE_KEY_TANGENT_UNIFIED);
        easefrom = (easeto + 1.0f) / scale_from_dd_to_ds - 1.0f;
        if (easefrom > 1)
        {
            easefrom = 1.0f;
        }
        else if (easefrom < 0)
        {
            easefrom = 0;
        }
        float in = atan_tpl(ds);
        dd = tan_tpl(in + gf_PI - theta_from_dd_to_ds);
    }

    template<>
    inline void TCBSplineKeyEx<float>::SetInTangentFromOut()
    {
        assert((flags & SPLINE_KEY_TANGENT_ALL_MASK) == SPLINE_KEY_TANGENT_UNIFIED);
        easeto = scale_from_dd_to_ds * (easefrom + 1.0f) - 1.0f;
        if (easeto > 1)
        {
            easeto = 1.0f;
        }
        else if (easeto < 0)
        {
            easeto = 0;
        }
        float out = atan_tpl(dd);
        ds = tan_tpl(out + theta_from_dd_to_ds - gf_PI);
    }

    /****************************************************************************
    **                        TCBSpline class implementation                                       **
    ****************************************************************************/
    template <class T, class Key = TCBSplineKey<T> >
    class TCBSpline
        : public TSpline< Key, HermitBasis >
    {
    public:
        virtual void comp_deriv();

    protected:
        virtual void interp_keys(int key1, int key2, float u, T& val);
        float calc_ease(float t, float a, float b);

    private:
        void compMiddleDeriv(int curr);
        void compFirstDeriv();
        void compLastDeriv();
        void comp2KeyDeriv();
    };

    //////////////////////////////////////////////////////////////////////////
    template    <class T, class Key>
    inline  void TCBSpline<T, Key>::compMiddleDeriv(int curr)
    {
        float   dsA, dsB, ddA, ddB;
        float   A, B, cont1, cont2;
        int last = this->num_keys() - 1;

        dsA = 0;
        ddA = 0;

        // dsAdjust,ddAdjust apply speed correction when continuity is 0.
        // Middle key.
        if (curr == 0)
        {
            // First key.
            float dts = (this->GetRangeEnd() - this->time(last)) + (this->time(0) - this->GetRangeStart());
            float fDiv = (dts + this->time(1) - this->time(0));
            if (fDiv != 0.0f)
            {
                float dt = 2.0f / fDiv;
                dsA = dt * dts;
                ddA = dt * (this->time(1) - this->time(0));
            }
        }
        else
        {
            if (curr == last)
            {
                // Last key.
                float dts = (this->GetRangeEnd() - this->time(last)) + (this->time(0) - this->GetRangeStart());
                float fDiv = (dts + this->time(last) - this->time(last - 1));
                if (fDiv != 0.0f)
                {
                    float dt = 2.0f / fDiv;
                    dsA = dt * dts;
                    ddA = dt * (this->time(last) - this->time(last - 1));
                }
            }
            else
            {
                // Middle key.
                float fDiv = this->time(curr + 1) - this->time(curr - 1);
                if (fDiv != 0.f)
                {
                    float dt = 2.0f / fDiv;
                    dsA = dt * (this->time(curr) - this->time(curr - 1));
                    ddA = dt * (this->time(curr + 1) - this->time(curr));
                }
            }
        }
        typename TSpline<Key, HermitBasis >::key_type& k = this->key(curr);
        T ds0 = k.ds;
        T dd0 = k.dd;

        float c = (float)fabs(k.cont);
        float sa = dsA + c * (1.0f - dsA);
        float da = ddA + c * (1.0f - ddA);

        A = 0.5f * (1.0f - k.tens) * (1.0f + k.bias);
        B = 0.5f * (1.0f - k.tens) * (1.0f - k.bias);
        cont1 = (1.0f - k.cont);
        cont2 = (1.0f + k.cont);
        //dsA = dsA * A * cont1;
        //dsB = dsA * B * cont2;
        //ddA = ddA * A * cont2;
        //ddB = ddA * B * cont1;
        dsA = sa * A * cont1;
        dsB = sa * B * cont2;
        ddA = da * A * cont2;
        ddB = da * B * cont1;

        T qp, qn;
        if (curr > 0)
        {
            qp = this->value(curr - 1);
        }
        else
        {
            qp = this->value(last);
        }
        if (curr < last)
        {
            qn = this->value(curr + 1);
        }
        else
        {
            qn = this->value(0);
        }
        k.ds = Concatenate(dsA * Subtract(k.value, qp), dsB * Subtract(qn, k.value));
        k.dd = Concatenate(ddA * Subtract(k.value, qp), ddB * Subtract(qn, k.value));

        switch (this->GetInTangentType(curr))
        {
        case SPLINE_KEY_TANGENT_STEP:
        case SPLINE_KEY_TANGENT_ZERO:
            Zero(k.ds);
            break;
        case SPLINE_KEY_TANGENT_LINEAR:
            k.ds = Subtract(k.value, qp);
            break;
        case SPLINE_KEY_TANGENT_CUSTOM:
            k.ds = ds0;
            break;
        }
        switch (this->GetOutTangentType(curr))
        {
        case SPLINE_KEY_TANGENT_STEP:
        case SPLINE_KEY_TANGENT_ZERO:
            Zero(k.dd);
            break;
        case SPLINE_KEY_TANGENT_LINEAR:
            k.dd = Subtract(qn, k.value);
            break;
        case SPLINE_KEY_TANGENT_CUSTOM:
            k.dd = dd0;
            break;
        }
    }

    template    <class T, class Key>
    inline  void TCBSpline<T, Key>::compFirstDeriv()
    {
        typename TSpline<Key, HermitBasis >::key_type& k = this->key(0);

        if (this->GetInTangentType(0) != SPLINE_KEY_TANGENT_CUSTOM)
        {
            Zero(k.ds);
        }

        if (this->GetOutTangentType(0) != SPLINE_KEY_TANGENT_CUSTOM)
        {
            k.dd = 0.5f *
                (1.0f - k.tens) * (3.0f *
                                   Subtract(Subtract(this->value(1), k.value), this->ds(1)));
        }
    }

    template    <class T, class Key>
    inline  void TCBSpline<T, Key>::compLastDeriv()
    {
        int last = this->num_keys() - 1;
        typename TSpline<Key, HermitBasis >::key_type& k = this->key(last);

        if (this->GetInTangentType(last) != SPLINE_KEY_TANGENT_CUSTOM)
        {
            k.ds = -0.5f * (1.0f -
                            k.tens) * (3.0f *
                                       Concatenate(Subtract(this->value(last - 1), k.value), this->dd(last - 1)));
        }

        if (this->GetOutTangentType(last) != SPLINE_KEY_TANGENT_CUSTOM)
        {
            Zero(k.dd);
        }
    }

    template    <class T, class Key>
    inline  void TCBSpline<T, Key>::comp2KeyDeriv()
    {
        typename TSpline<Key, HermitBasis >::key_type& k1 = this->key(0);
        typename TSpline<Key, HermitBasis >::key_type& k2 = this->key(1);

        typename TSpline<Key, HermitBasis >::value_type val = Subtract(this->value(1), this->value(0));

        if (this->GetInTangentType(0) != SPLINE_KEY_TANGENT_CUSTOM)
        {
            Zero(k1.ds);
        }
        if (this->GetOutTangentType(0) != SPLINE_KEY_TANGENT_CUSTOM)
        {
            k1.dd = (1.0f - k1.tens) * val;
        }
        if (this->GetInTangentType(1) != SPLINE_KEY_TANGENT_CUSTOM)
        {
            k2.ds = (1.0f - k2.tens) * val;
        }
        if (this->GetOutTangentType(1) != SPLINE_KEY_TANGENT_CUSTOM)
        {
            Zero(k2.dd);
        }
    }

    template    <class T, class Key>
    inline  void    TCBSpline<T, Key>::comp_deriv()
    {
        if (this->num_keys() > 1)
        {
            if ((this->num_keys() == 2) && !this->closed())
            {
                comp2KeyDeriv();
                return;
            }
            if (this->closed())
            {
                for (int i = 0; i < this->num_keys(); ++i)
                {
                    compMiddleDeriv(i);
                }
            }
            else
            {
                for (int i = 1; i < (this->num_keys() - 1); ++i)
                {
                    compMiddleDeriv(i);
                }
                compFirstDeriv();
                compLastDeriv();
            }
        }
        this->SetModified(false);
    }

    template    <class T, class Key>
    inline  float   TCBSpline<T, Key>::calc_ease(float t, float a, float b)
    {
        float k;
        float s = a + b;

        if (t == 0.0f || t == 1.0f)
        {
            return t;
        }
        if (s == 0.0f)
        {
            return t;
        }
        if (s > 1.0f)
        {
            k = 1.0f / s;
            a *= k;
            b *= k;
        }
        k = 1.0f / (2.0f - a - b);
        if (t < a)
        {
            return ((k / a) * t * t);
        }
        else
        {
            if (t < 1.0f - b)
            {
                return (k * (2.0f * t - a));
            }
            else
            {
                t = 1.0f - t;
                return (1.0f - (k / b) * t * t);
            }
        }
    }

    template    <class T, class Key>
    inline  void    TCBSpline<T, Key>::interp_keys(int from, int to, float u, T& val)
    {
        if (this->GetOutTangentType(from) == SPLINE_KEY_TANGENT_STEP)
        {
            val = this->value(to);
        }
        else if (this->GetInTangentType(to) == SPLINE_KEY_TANGENT_STEP)
        {
            val = this->value(from);
        }
        else
        {
            u = calc_ease(u, this->key(from).easefrom, this->key(to).easeto);
            typename TSpline<Key, HermitBasis >::basis_type basis(u);
            val = Concatenate(
                    Concatenate(
                        Concatenate(
                            (basis[0] * this->value(from)), (basis[1] * this->value(to))
                            ),
                        (basis[2] * this->dd(from))
                        ),
                    (basis[3] * this->ds(to))
                    );
        }
    }



    /****************************************************************************
    **                        TCBQuatSpline class implementation                             **
    ****************************************************************************/
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    class TCBQuatSpline
        : public TCBSpline<Quat>
    {
    public:
        //void interpolate( float time,value_type& val );
        void comp_deriv();

    protected:
        void interp_keys(int key1, int key2, float u, value_type& val);

    private:
        void compKeyDeriv(int curr);

        // Loacal function to add quaternions.
        Quat AddQuat(const Quat& q1, const Quat& q2)
        {
            return Quat(q1.w + q2.w, q1.v.x + q2.v.x, q1.v.y + q2.v.y, q1.v.z + q2.v.z);
        }
    };

    inline  void    TCBQuatSpline::interp_keys(int from, int to, float u, value_type& val)
    {
        u = calc_ease(u, key(from).easefrom, key(to).easeto);
        basis_type basis(u);
        //val = SquadRev( angle(to),axis(to), value(from), dd(from), ds(to), value(to), u );
        val = Quat::CreateSquad(value(from), dd(from), ds(to), value(to), u);
        val = (val).GetNormalized();    // Normalize quaternion.
    }


    inline  void TCBQuatSpline::comp_deriv()
    {
        if (num_keys() > 1)
        {
            for (int i = 0; i < num_keys(); ++i)
            {
                compKeyDeriv(i);
            }
        }
        this->SetModified(false);
    }



    inline  void TCBQuatSpline::compKeyDeriv(int curr)
    {
        Quat  qp, qm;
        float fp, fn;
        int last = num_keys() - 1;

        if (curr > 0 || closed())
        {
            int prev = (curr != 0) ? curr - 1 : last;
            qm = value(prev);
            if ((qm | (value(curr))) < 0.0f)
            {
                qm = -qm;
            }
            qm = Quat::LnDif(qm.GetNormalizedSafe(), value(curr).GetNormalizedSafe());
        }

        if (curr < last || closed())
        {
            int next = (curr != last) ? curr + 1 : 0;
            Quat qnext = value(next);
            if ((qnext | (value(curr))) < 0.0f)
            {
                qnext = -qnext;
            }
            qp = value(curr);
            qp = Quat::LnDif(qp.GetNormalizedSafe(), qnext.GetNormalizedSafe());
        }

        if (curr == 0 && !closed())
        {
            qm = qp;
        }
        if (curr == last && !closed())
        {
            qp = qm;
        }

        key_type& k = key(curr);
        float c = (float)fabs(k.cont);

        fp = fn = 1.0f;
        if ((curr > 0 && curr < last) || closed())
        {
            if (curr == 0)
            {
                // First key.
                float dts = (this->GetRangeEnd() - this->time(last)) + (this->time(0) - this->GetRangeStart());
                float fDiv = (dts + this->time(1) - this->time(0));
                if (fDiv != 0.0f)
                {
                    float dt = 2.0f / fDiv;
                    fp = dt * dts;
                    fn = dt * (this->time(1) - this->time(0));
                }
            }
            else
            {
                if (curr == last)
                {
                    // Last key.
                    float dts = (this->GetRangeEnd() - this->time(last)) + (this->time(0) - this->GetRangeStart());
                    float fDiv = (dts + this->time(last) - this->time(last - 1));
                    if (fDiv != 0.0f)
                    {
                        float dt = 2.0f / fDiv;
                        fp = dt * dts;
                        fn = dt * (this->time(last) - this->time(last - 1));
                    }
                }
                else
                {
                    // Middle key.
                    float fDiv = (this->time(curr + 1) - this->time(curr - 1));
                    if (fDiv != 0.0f)
                    {
                        float dt = 2.0f / fDiv;
                        fp = dt * (this->time(curr) - this->time(curr - 1));
                        fn = dt * (this->time(curr + 1) - this->time(curr));
                    }
                }
            }
            fp += c - c * fp;
            fn += c - c * fn;
        }

        float tm, cm, cp, bm, bp, tmcm, tmcp, ksm, ksp, kdm, kdp;

        cm = 1.0f - k.cont;
        tm = 0.5f * (1.0f - k.tens);
        cp = 2.0f - cm;
        bm = 1.0f - k.bias;
        bp = 2.0f - bm;
        tmcm = tm * cm;
        tmcp = tm * cp;
        ksm  = 1.0f - tmcm * bp * fp;
        ksp  = -tmcp * bm * fp;
        kdm  = tmcp * bp * fn;
        kdp  = tmcm * bm * fn - 1.0f;

        Quat qa = 0.5f * AddQuat(kdm * qm, kdp * qp);
        Quat qb = 0.5f * AddQuat(ksm * qm, ksp * qp);
        qa = Quat::exp(qa.v);
        qb = Quat::exp(qb.v);

        // ds = qb, dd = qa.
        k.ds = value(curr) * qb;
        k.dd = value(curr) * qa;
    }

    /****************************************************************************
    **                    TCBAngleAxisSpline class implementation                            **
    ****************************************************************************/
    ///////////////////////////////////////////////////////////////////////////////
    //
    // TCBAngleAxisSpline takes as input relative Angle-Axis values.
    // Interpolated result is returned as Normalized quaternion.
    //
    //////////////////////////////////////////////////////////////////////////
    struct SAngleAxis
    {
        float angle;
        Vec3 axis;
    };


    class TCBAngleAxisSpline
        : public TCBSpline<Quat, TCBAngAxisKey>
    {
    public:
        //void interpolate( float time,value_type& val );
        void comp_deriv();

        // Angle axis used for quaternion.
        float&  angle(int i)  { return key(i).angle; };
        Vec3& axis(int i) { return key(i).axis; };

    protected:
        void interp_keys(int key1, int key2, float u, value_type& val);

    private:
        virtual void compKeyDeriv(int curr);

        // Loacal function to add quaternions.
        Quat AddQuat(const Quat& q1, const Quat& q2)
        {
            return Quat(q1.w + q2.w, q1.v.x + q2.v.x, q1.v.y + q2.v.y, q1.v.z + q2.v.z);
        }
    };

    //////////////////////////////////////////////////////////////////////////
    inline  void    TCBAngleAxisSpline::interp_keys(int from, int to, float u, value_type& val)
    {
        u = calc_ease(u, key(from).easefrom, key(to).easeto);
        basis_type basis(u);
        val = CreateSquadRev(angle(to), axis(to), value(from), dd(from), ds(to), value(to), u);
        val = (val).GetNormalized();    // Normalize quaternion.
    }

    //////////////////////////////////////////////////////////////////////////
    inline  void TCBAngleAxisSpline::comp_deriv()
    {
        // Convert from relative angle-axis to absolute quaternion.
        Quat q, lastq;
        lastq.SetIdentity();
        for (int i = 0; i < num_keys(); ++i)
        {
            q.SetRotationAA(angle(i), axis(i));
            q.Normalize(); // Normalize quaternion
            q = lastq * q;
            lastq = q;
            value(i) = q;
        }

        if (num_keys() > 1)
        {
            for (int i = 0; i < num_keys(); ++i)
            {
                compKeyDeriv(i);
            }
        }
        this->SetModified(false);
    }

    //////////////////////////////////////////////////////////////////////////
    inline  void TCBAngleAxisSpline::compKeyDeriv(int curr)
    {
        Quat  qp, qm;
        float fp, fn;
        int last = num_keys() - 1;

        if (curr > 0 || closed())
        {
            int prev = (curr != 0) ? curr - 1 : last;
            if (angle(curr) > gf_PI2)
            {
                Vec3 a = axis(curr);
                qm = Quat(0, Quat::log(Quat(0, a.x, a.y, a.z)));
            }
            else
            {
                qm = value(prev);
                if ((qm | (value(curr))) < 0.0f)
                {
                    qm = -qm;
                }
                qm = Quat::LnDif(qm, value(curr));
            }
        }

        if (curr < last || closed())
        {
            int next = (curr != last) ? curr + 1 : 0;
            if (angle(next) > gf_PI2)
            {
                Vec3 a = axis(next);
                qp = Quat(0, Quat::log(Quat(0, a.x, a.y, a.z)));
            }
            else
            {
                Quat qnext = value(next);
                if ((qnext | (value(curr))) < 0.0f)
                {
                    qnext = -qnext;
                }
                qp = value(curr);
                qp = Quat::LnDif(qp, qnext);
            }
        }

        if (curr == 0 && !closed())
        {
            qm = qp;
        }
        if (curr == last && !closed())
        {
            qp = qm;
        }

        key_type& k = key(curr);
        float c = (float)fabs(k.cont);

        fp = fn = 1.0f;
        if ((curr > 0 && curr < last) || closed())
        {
            if (curr == 0)
            {
                // First key.
                float dts = (this->GetRangeEnd() - this->time(last)) + (this->time(0) - this->GetRangeStart());
                float dt = 2.0f / (dts + this->time(1) - this->time(0));
                fp = dt * dts;
                fn = dt * (this->time(1) - this->time(0));
            }
            else
            {
                if (curr == last)
                {
                    // Last key.
                    float dts = (this->GetRangeEnd() - this->time(last)) + (this->time(0) - this->GetRangeStart());
                    float dt = 2.0f / (dts + this->time(last) - this->time(last - 1));
                    fp = dt * dts;
                    fn = dt * (this->time(last) - this->time(last - 1));
                }
                else
                {
                    // Middle key.
                    float dt = 2.0f / (this->time(curr + 1) - this->time(curr - 1));
                    fp = dt * (this->time(curr) - this->time(curr - 1));
                    fn = dt * (this->time(curr + 1) - this->time(curr));
                }
            }
            fp += c - c * fp;
            fn += c - c * fn;
        }

        float tm, cm, cp, bm, bp, tmcm, tmcp, ksm, ksp, kdm, kdp;

        cm = 1.0f - k.cont;
        tm = 0.5f * (1.0f - k.tens);
        cp = 2.0f - cm;
        bm = 1.0f - k.bias;
        bp = 2.0f - bm;
        tmcm = tm * cm;
        tmcp = tm * cp;
        ksm  = 1.0f - tmcm * bp * fp;
        ksp  = -tmcp * bm * fp;
        kdm  = tmcp * bp * fn;
        kdp  = tmcm * bm * fn - 1.0f;

        const Vec3 va = 0.5f * (kdm * qm.v + kdp * qp.v);
        const Vec3 vb = 0.5f * (ksm * qm.v + ksp * qp.v);

        const Quat qa = Quat::exp(va);
        const Quat qb = Quat::exp(vb);

        // ds = qb, dd = qa.
        k.ds = value(curr) * qb;
        k.dd = value(curr) * qa;
    }


    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    template <class T>
    class TrackSplineInterpolator
        : public spline::CBaseSplineInterpolator< T, spline::TCBSpline<T, spline::TCBSplineKeyEx<T> > >
    {
        typedef typename spline::CBaseSplineInterpolator< T, spline::TCBSpline<T, spline::TCBSplineKeyEx<T> > > I;
    public:
        virtual void SerializeSpline(XmlNodeRef& node, bool bLoading) {};
        virtual void SetKeyFlags(int k, int flags)
        {
            if (k >= 0 && k < this->num_keys())
            {
                if ((this->key(k).flags & SPLINE_KEY_TANGENT_ALL_MASK) != SPLINE_KEY_TANGENT_UNIFIED
                    && (flags & SPLINE_KEY_TANGENT_ALL_MASK) == SPLINE_KEY_TANGENT_UNIFIED)
                {
                    this->key(k).ComputeThetaAndScale();
                }
            }
            spline::CBaseSplineInterpolator< T, spline::TCBSpline<T, spline::TCBSplineKeyEx<T> > >::SetKeyFlags(k, flags);
        }
        virtual void SetKeyInTangent(int k, ISplineInterpolator::ValueType tin)
        {
            if (k >= 0 && k < this->num_keys())
            {
                I::FromValueType(tin, this->key(k).ds);
                if ((this->key(k).flags & SPLINE_KEY_TANGENT_ALL_MASK) == SPLINE_KEY_TANGENT_UNIFIED)
                {
                    this->key(k).SetOutTangentFromIn();
                }
                this->SetModified(true);
            }
        }
        virtual void SetKeyOutTangent(int k, ISplineInterpolator::ValueType tout)
        {
            if (k >= 0 && k < this->num_keys())
            {
                I::FromValueType(tout, this->key(k).dd);
                if ((this->key(k).flags & SPLINE_KEY_TANGENT_ALL_MASK) == SPLINE_KEY_TANGENT_UNIFIED)
                {
                    this->key(k).SetInTangentFromOut();
                }
                this->SetModified(true);
            }
        }
    };

    template <>
    class TrackSplineInterpolator<Quat>
        : public spline::CBaseSplineInterpolator< Quat, spline::TCBQuatSpline >
    {
    public:
        virtual void SerializeSpline([[maybe_unused]] XmlNodeRef& node, [[maybe_unused]] bool bLoading) {};
    };
}; // namespace spline

#endif // CRYINCLUDE_CRYMOVIE_TCBSPLINE_H
