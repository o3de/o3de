/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Classes for 2D Bezier Spline curves
// Notice      : some extra helpful information

#pragma once

#include <ISplines.h>

namespace AZ
{
    class ReflectContext;
}

namespace spline
{
    const float g_tanEpsilon = 0.000001f;

    /** Bezier spline key extended for tangent unify/break.
    */
    template    <class T>
    struct SplineKeyEx
        :  public SplineKey<T>
    {
        float theta_from_dd_to_ds;
        float scale_from_dd_to_ds;

        void ComputeThetaAndScale()
        {
            AZ_Assert(false, "Not expected to be used");
        }

        void SetOutTangentFromIn()
        {
            AZ_Assert(false, "Not expected to be used");
        }

        void SetInTangentFromOut()
        {
            AZ_Assert(false, "Not expected to be used");
        }

        SplineKeyEx()
            : theta_from_dd_to_ds(gf_PI)
            , scale_from_dd_to_ds(1.0f) {}

        static void Reflect(AZ::ReflectContext*) {}
    };

    inline void ComputeUnifiedTangent(Vec2& destTan, float angle, float length)
    {
        // "Unifying" tangents really means we try to maintain the angle between them

        // clamp the out tangent between +/- 90 degrees
        if (angle <= -gf_halfPI)
        {
            destTan.x = .0f;
            destTan.y = -1.0f;
        }
        else if (angle >= gf_halfPI)
        {
            destTan.x = .0f;
            destTan.y = 1.0f;
        }
        else
        {
            destTan.x = 1.0f;
            destTan.y = tan_tpl(angle);
            destTan.Normalize();
        }

        // lower clamp length so the destTan is never 'inverted' nor completely zero
        destTan *= max(length, g_tanEpsilon);
    }

    template <>
    inline void SplineKeyEx<Vec2>::ComputeThetaAndScale()
    {
        scale_from_dd_to_ds = (ds.GetLength() + 1.0f) / (dd.GetLength() + 1.0f);
        float out = fabs(dd.x) > g_tanEpsilon ? atan_tpl(dd.y / dd.x) : (dd.y >= .0f ? gf_halfPI : -gf_halfPI);
        float in = fabs(ds.x) > g_tanEpsilon ? atan_tpl(ds.y / ds.x) : (ds.y >= .0f ? gf_halfPI : -gf_halfPI);
        theta_from_dd_to_ds = in + gf_PI - out;
    }

    template<>
    inline void SplineKeyEx<Vec2>::SetOutTangentFromIn()
    {
        // "Unifying" tangents really means we try to maintain the angle between them
        AZ_Assert((flags & SPLINE_KEY_TANGENT_ALL_MASK) == SPLINE_KEY_TANGENT_UNIFIED, "Invalid spline key flag");
        float outLength = (ds.GetLength() + 1.0f) / scale_from_dd_to_ds - 1.0f;
        float in = fabs(ds.x) > g_tanEpsilon ? atan_tpl(ds.y / ds.x) : (ds.y >= .0f ? gf_halfPI : -gf_halfPI);
        float outAngle = in + gf_PI - theta_from_dd_to_ds;

        ComputeUnifiedTangent(dd, outAngle, outLength);
    }

    template<>
    inline void SplineKeyEx<Vec2>::SetInTangentFromOut()
    {
        // "Unifying" tangents really means we try to maintain the angle between them
        AZ_Assert((flags & SPLINE_KEY_TANGENT_ALL_MASK) == SPLINE_KEY_TANGENT_UNIFIED, "Invalid spline key flag");
        float inLength = scale_from_dd_to_ds * (dd.GetLength() + 1.0f) - 1.0f;
        float out = fabs(dd.x) > g_tanEpsilon ? atan_tpl(dd.y / dd.x) : (dd.y >= .0f ? gf_halfPI : -gf_halfPI);
        float inAngle = out + theta_from_dd_to_ds - gf_PI;

        ComputeUnifiedTangent(ds, inAngle, inLength);
    }

    template<>
    void SplineKeyEx<Vec2>::Reflect(AZ::ReflectContext* context);

    template <class T>
    class TrackSplineInterpolator;

    template <>
    class TrackSplineInterpolator<Vec2>
        : public spline::CBaseSplineInterpolator<Vec2, spline::BezierSpline<Vec2, spline::SplineKeyEx<Vec2>>>
    {
    public:
        AZ_CLASS_ALLOCATOR(TrackSplineInterpolator<Vec2>, AZ::SystemAllocator);

        int GetNumDimensions() override
        {
            // It's actually one-dimensional since the x component curve is for a time-warping.
            return 1;
        }
        void SerializeSpline([[maybe_unused]] XmlNodeRef& node, [[maybe_unused]] bool bLoading) override {}
    private:
        // An utility function for the Newton-Raphson method
        float comp_time_deriv(int from, int to, float u) const
        {
            float u2 = u * u;
            float b0 = -3.0f * u2 + 6.0f * u - 3;
            float b1 = 9.0f * u2 - 12.0f * u + 3;
            float b2 = -9.0f * u2 + 6.0f * u;
            float b3 = 3.0f * u2;

            float p0 = this->value(from).x;
            float p3 = this->value(to).x;
            float p1 = p0 + this->dd(from).x;
            float p2 = p3 - this->ds(to).x;

            return (b0 * p0) + (b1 * p1) + (b2 * p2) + (b3 * p3);
        }

        float comp_value_deriv(int from, int to, float u) const
        {
            float u2 = u * u;
            float b0 = -3.0f * u2 + 6.0f * u - 3;
            float b1 = 9.0f * u2 - 12.0f * u + 3;
            float b2 = -9.0f * u2 + 6.0f * u;
            float b3 = 3.0f * u2;

            float p0 = this->value(from).y;
            float p3 = this->value(to).y;
            float p1 = p0 + this->dd(from).y;
            float p2 = p3 - this->ds(to).y;

            return (b0 * p0) + (b1 * p1) + (b2 * p2) + (b3 * p3);
        }

        float comp_area(int from, int to, float u = 1.0f) const
        {
            if (GetOutTangentType(from) == SPLINE_KEY_TANGENT_STEP || GetInTangentType(to) == SPLINE_KEY_TANGENT_STEP)
            {
                float value = this->value(from).y;
                if (GetOutTangentType(from) == SPLINE_KEY_TANGENT_STEP)
                {
                    value = this->value(to).y;
                }
                float timeDelta = this->time(to) - this->time(from);
                return value * timeDelta * u;
            }

            float p0 = this->value(from).y;
            float p3 = this->value(to).y;
            float p1 = p0 + this->dd(from).y;
            float p2 = p3 - this->ds(to).y;

            // y = A*t^3 + B*t^2 + C*t + D
            float A = -p0 + 3 * p1 - 3 * p2 + p3;
            float B = 3 * p0 - 6 * p1 + 3 * p2;
            float C = -3 * p0 + 3 * p1;
            float D = p0;

            p0 = this->value(from).x;
            p3 = this->value(to).x;
            p1 = p0 + this->dd(from).x;
            p2 = p3 - this->ds(to).x;

            // dx/dt = a*t^2 + b*t + c
            float a = 3 * (-p0 + 3 * p1 - 3 * p2 + p3);
            float b = 2 * (3 * p0 - 6 * p1 + 3 * p2);
            float c = 1 * (-3 * p0 + 3 * p1);

            // y * (dx/dt) = k5*t^5 + k4*t^4 + k3*t^3 + k2*t^2 + k1*t + k0
            float k5 = A * a;
            float k4 = B * a + A * b;
            float k3 = C * a + B * b + A * c;
            float k2 = D * a + C * b + B * c;
            float k1 = D * b + C * c;
            float k0 = D * c;

            // Integral (y*(dx/dt) dt from 0 to u
            float u2 = u * u;
            float u3 = u2 * u;
            float u4 = u3 * u;
            float u5 = u4 * u;
            float u6 = u5 * u;
            return (k5 / 6) * u6 + (k4 / 5) * u5 + (k3 / 4) * u4 + (k2 / 3) * u3 + (k1 / 2) * u2 + k0 * u;
        }

        float search_u(float time, ISplineInterpolator::ValueType& value)
        {
            float time_to_check = time;
            int count = 0;
            int curr = seek_key(time);
            int next = (curr < num_keys() - 1) ? curr + 1 : curr;
            // Clamp the time first.
            if (time < this->time(0))
            {
                time = this->time(0);
            }
            else if (time > this->time(num_keys() - 1))
            {
                time = this->time(num_keys() - 1);
            }
            // It's somewhat tricky here. We should find the 't' where the x element
            // of the 2D Bezier curve equals to the specified 'time'.
            // The y component of the curve there is our value.
            // We use the 'Newton's method' to find the root.
            float u = 0;
            const float epsilon = 0.00001f;
            float timeDelta = this->time(next) - this->time(curr);
            if (timeDelta == 0)
            {
                timeDelta = epsilon;
            }
            // In case of stepping tangents, we don't need this special processing.
            if (GetOutTangentType(curr) == SPLINE_KEY_TANGENT_STEP || GetInTangentType(next) == SPLINE_KEY_TANGENT_STEP)
            {
                spline::CBaseSplineInterpolator<Vec2, spline::BezierSpline<Vec2, spline::SplineKeyEx<Vec2> > >::Interpolate(time_to_check, value);
                return (time_to_check - this->time(curr)) / timeDelta;
            }
            do
            {
                spline::CBaseSplineInterpolator<Vec2, spline::BezierSpline<Vec2, spline::SplineKeyEx<Vec2> > >::Interpolate(time_to_check, value);

                u = (time_to_check - this->time(curr)) / timeDelta;

                if (fabs(value[0] - time) < epsilon)
                {
                    // Finally, we got the solution.
                    break;
                }
                else
                {
                    // Apply the Newton's method to compute the next time value to try.
                    AZ_Assert(next != curr, "Next time to try equal current");
                    float dt = comp_time_deriv(curr, next, u);
                    double dfdt = (double(value[0]) - double(time)) / (double(dt) + epsilon);
                    u -= float(dfdt);
                    if (u < 0)
                    {
                        u = 0;
                    }
                    else if (u > 1)
                    {
                        u = 1;
                    }
                    time_to_check = u * (this->time(next) - this->time(curr)) + this->time(curr);
                }
                ++count;
            }
            while (count < 10);
            return u;
        }

        Vec2 interpolate_tangent(float time, float& u)
        {
            Vec2 tangent;
            int curr = seek_key(time);

            // special case for time == last key.
            // Use the last two keys.
            if (curr == num_keys() - 1)
            {
                curr--;
            }

            int next = curr + 1;

            AZ_Assert(0 <= curr && next < num_keys(), "Keys indicies out of range");

            ISplineInterpolator::ValueType value;
            u = search_u(time, value);
            tangent.x = comp_time_deriv(curr, next, u);
            tangent.y = comp_value_deriv(curr, next, u);
            tangent /= 3.0f;
            return tangent;
        }
    public:
        // We should override following 4 methods to make it act like an 1D curve although it's actually a 2D curve.
        void SetKeyTime(int key, float time) override
        {
            ISplineInterpolator::ValueType value;
            ISplineInterpolator::ZeroValue(value);
            spline::CBaseSplineInterpolator<Vec2, spline::BezierSpline<Vec2, spline::SplineKeyEx<Vec2> > >::GetKeyValue(key, value);
            value[0] = time;
            spline::CBaseSplineInterpolator<Vec2, spline::BezierSpline<Vec2, spline::SplineKeyEx<Vec2> > >::SetKeyValue(key, value);
            spline::CBaseSplineInterpolator<Vec2, spline::BezierSpline<Vec2, spline::SplineKeyEx<Vec2> > >::SetKeyTime(key, time);
        }

        void SetKeyValue(int key, ISplineInterpolator::ValueType value) override
        {
            ISplineInterpolator::ValueType value0;
            ISplineInterpolator::ZeroValue(value0);
            value0[0] = GetKeyTime(key);
            value0[1] = value[0];
            spline::CBaseSplineInterpolator<Vec2, spline::BezierSpline<Vec2, spline::SplineKeyEx<Vec2> > >::SetKeyValue(key, value0);
        }

        bool GetKeyValue(int key, ISplineInterpolator::ValueType& value) override
        {
            if (spline::CBaseSplineInterpolator<Vec2, spline::BezierSpline<Vec2, spline::SplineKeyEx<Vec2> > >::GetKeyValue(key, value))
            {
                value[0] = value[1];
                value[1] = 0;
                return true;
            }
            return false;
        }

        void Interpolate(float time, ISplineInterpolator::ValueType& value) override
        {
            if (empty())
            {
                return;
            }
            adjust_time(time);
            search_u(time, value);

            value[0] = value[1];
            value[1] = 0;
        }

        float Integrate(float time)
        {
            if (empty())
            {
                return 0;
            }
            if (time < this->time(0))
            {
                return 0;
            }
            int curr = seek_key(time);
            int next = curr + 1;

            float area = 0;
            for (int i = 0; i < curr; ++i)
            {
                area += comp_area(i, i + 1);
            }
            if (next < this->num_keys())
            {
                ISplineInterpolator::ValueType value;
                float u = search_u(time, value);
                area += comp_area(curr, next, u);
            }
            else
            {
                area += (time - this->time(curr)) * this->value(curr).y;
            }
            return area;
        }

        void SetKeyFlags(int k, int flags) override
        {
            if (k >= 0 && k < this->num_keys())
            {
                if ((this->key(k).flags & SPLINE_KEY_TANGENT_ALL_MASK) != SPLINE_KEY_TANGENT_UNIFIED
                    && (flags & SPLINE_KEY_TANGENT_ALL_MASK) == SPLINE_KEY_TANGENT_UNIFIED)
                {
                    this->key(k).ComputeThetaAndScale();
                }
            }
            spline::CBaseSplineInterpolator<Vec2, spline::BezierSpline<Vec2, spline::SplineKeyEx<Vec2> > >::SetKeyFlags(k, flags);
        }

        void SetKeyInTangent(int k, ISplineInterpolator::ValueType tin) override
        {
            if (k >= 0 && k < this->num_keys())
            {
                FromValueType(tin, this->key(k).ds);
                if ((this->key(k).flags & SPLINE_KEY_TANGENT_ALL_MASK) == SPLINE_KEY_TANGENT_UNIFIED)
                {
                    this->key(k).SetOutTangentFromIn();
                    ConstrainOutTangentsOf(k);
                }
                this->SetModified(true);
            }
        }

        void SetKeyOutTangent(int k, ISplineInterpolator::ValueType tout) override
        {
            if (k >= 0 && k < this->num_keys())
            {
                FromValueType(tout, this->key(k).dd);
                if ((this->key(k).flags & SPLINE_KEY_TANGENT_ALL_MASK) == SPLINE_KEY_TANGENT_UNIFIED)
                {
                    this->key(k).SetInTangentFromOut();
                    ConstrainInTangentsOf(k);
                }
                this->SetModified(true);
            }
        }

        // A pair of utility functions to constrain the time range
        // so that the time curve is always monotonically increasing.
        void ConstrainOutTangentsOf(int k)
        {
            if (k < num_keys() - 1
                && this->key(k).dd.x > (this->time(k + 1) - this->time(k)))
            {
                this->key(k).dd *= (this->time(k + 1) - this->time(k)) / this->key(k).dd.x;
            }
        }

        void ConstrainInTangentsOf(int k)
        {
            if (k > 0
                && this->key(k).ds.x > (this->time(k) - this->time(k - 1)))
            {
                this->key(k).ds *= (this->time(k) - this->time(k - 1)) / this->key(k).ds.x;
            }
        }

        void comp_deriv() override
        {
            spline::BezierSpline<Vec2, spline::SplineKeyEx<Vec2> >::comp_deriv();

            // To process the 'zero tangent' case more properly,
            // here we override the tangent behavior for the case of SPLINE_KEY_TANGENT_ZERO.
            if (this->num_keys() > 1)
            {
                const float oneThird = 1 / 3.0f;

                const int last = this->num_keys() - 1;

                {
                    if (GetOutTangentType(0) == SPLINE_KEY_TANGENT_ZERO)
                    {
                        this->key(0).dd.x = oneThird * (this->value(1).x - this->value(0).x);
                        this->key(0).dd.y = 0;
                    }
                    else
                    {
                        ConstrainOutTangentsOf(0);
                    }
                    // Set the in-tangent same to the out.
                    if (GetInTangentType(0) == SPLINE_KEY_TANGENT_ZERO)
                    {
                        this->key(0).ds.x = oneThird * (this->value(1).x - this->value(0).x);
                        this->key(0).ds.y = 0;
                    }
                    else
                    {
                        ConstrainInTangentsOf(0);
                    }

                    if (GetInTangentType(last) == SPLINE_KEY_TANGENT_ZERO)
                    {
                        this->key(last).ds.x = oneThird * (this->value(last).x - this->value(last - 1).x);
                        this->key(last).ds.y = 0;
                    }
                    else
                    {
                        ConstrainInTangentsOf(last);
                    }
                    // Set the out-tangent same to the in.
                    if (GetOutTangentType(last) == SPLINE_KEY_TANGENT_ZERO)
                    {
                        this->key(last).dd.x = oneThird * (this->value(last).x - this->value(last - 1).x);
                        this->key(last).dd.y = 0;
                    }
                    else
                    {
                        ConstrainOutTangentsOf(last);
                    }
                }

                for (int i = 1; i < last; ++i)
                {
                    key_type& key = this->key(i);

                    switch (GetInTangentType(i))
                    {
                    case SPLINE_KEY_TANGENT_ZERO:
                        key.ds.x = oneThird * (this->value(i).x - this->value(i - 1).x);
                        key.ds.y = 0;
                        break;
                    default:
                        ConstrainInTangentsOf(i);
                        break;
                    }

                    switch (GetOutTangentType(i))
                    {
                    case SPLINE_KEY_TANGENT_ZERO:
                        key.dd.x = oneThird * (this->value(i + 1).x - this->value(i).x);
                        key.dd.y = 0;
                        break;
                    default:
                        ConstrainOutTangentsOf(i);
                        break;
                    }
                }
            }
        }

        int InsertKey(float t, ISplineInterpolator::ValueType val) override
        {
            Vec2 tangent;
            float u = 0;
            bool inRange = false;
            if (num_keys() > 1 && this->time(0) <= t && t <= this->time(num_keys() - 1))
            {
                tangent = interpolate_tangent(t, u);
                inRange = true;
            }

            val[1] = val[0];
            val[0] = t;
            int keyIndex = spline::CBaseSplineInterpolator<Vec2, spline::BezierSpline<Vec2, spline::SplineKeyEx<Vec2> > >::InsertKey(t, val);
            // Sets the default tangents properly.
            if (inRange)
            {
                this->key(keyIndex).ds = tangent * u;
                this->key(keyIndex).dd = tangent * (1 - u);
                ConstrainInTangentsOf(keyIndex);
                ConstrainOutTangentsOf(keyIndex);
            }
            else
            {
                const float oneThird = 1 / 3.0f;
                if (keyIndex == 0)
                {
                    u = 0;
                    if (num_keys() > 1)
                    {
                        this->key(0).dd.x = oneThird * (this->value(1).x - this->value(0).x);
                    }
                    else
                    {
                        this->key(0).dd.x = 1.0f;   // Just an arbitrary value
                    }
                    this->key(0).dd.y = 0;
                    // Set the in-tangent same to the out.
                    this->key(0).ds.x = this->key(0).dd.x;
                    this->key(0).ds.y = 0;
                }
                else if (keyIndex == num_keys() - 1)
                {
                    u = 1;
                    int last = num_keys() - 1;
                    this->key(last).ds.x = oneThird * (this->value(last).x - this->value(last - 1).x);
                    this->key(last).ds.y = 0;
                    // Set the out-tangent same to the in.
                    this->key(last).dd.x = this->key(last).ds.x;
                    this->key(last).dd.y = 0;
                }
                else
                {
                    AZ_Assert(false, "Invalid keyIndex %i", keyIndex);
                }
            }
            // Sets the unified tangent handles to the default.
            SetKeyFlags(keyIndex, SPLINE_KEY_TANGENT_UNIFIED);
            // Adjusts neighbors.
            if (keyIndex - 1 >= 0)
            {
                this->key(keyIndex - 1).dd *= u;
                ConstrainOutTangentsOf(keyIndex - 1);
            }
            if (keyIndex + 1 < num_keys())
            {
                this->key(keyIndex + 1).ds *= (1 - u);
                ConstrainInTangentsOf(keyIndex + 1);
            }
            return keyIndex;
        }

        static void Reflect(AZ::ReflectContext* context);
    };

} // namespace spline
