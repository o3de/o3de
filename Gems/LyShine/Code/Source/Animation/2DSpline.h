/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once


#include <ISplines.h>

namespace AZ
{
    class ReflectContext;
}

namespace UiSpline
{
    //////////////////////////////////////////////////////////////////////////
    // General Spline class (this is a modified version of spline::TSpline)
    // It is modified so that we can use AZ::Serialization
    //////////////////////////////////////////////////////////////////////////
    template <class KeyType, class BasisType>
    class TSpline
    {
    public:
        typedef KeyType     key_type;
        typedef typename KeyType::value_type    value_type;
        typedef BasisType   basis_type;

        // Out of range types.
        enum
        {
            ORT_CONSTANT                =   0x0001, // Constant track.
            ORT_CYCLE                       =   0x0002, // Cycle track
            ORT_LOOP                        =   0x0003, // Loop track.
            ORT_OSCILLATE               =   0x0004, // Oscillate track.
            ORT_LINEAR                  =   0x0005, // Linear track.
            ORT_RELATIVE_REPEAT =   0x0007  // Relative repeat track.
        };
        // Spline flags.
        enum
        {
            MODIFIED        =   0x0001, // Track modified.
            MUST_SORT       =   0x0002, // Track modified and must be sorted.
        };

        /////////////////////////////////////////////////////////////////////////////
        // Methods.
        inline TSpline()
        {
            m_flags = MODIFIED;
            m_ORT = 0;
            m_curr = 0;
            m_rangeStart = 0;
            m_rangeEnd = 0;
            m_refCount = 0;
        }

        virtual ~TSpline() {};

        ILINE void flag_set(int flag) { m_flags |= flag; };
        ILINE void flag_clr(int flag) { m_flags &= ~flag; };
        ILINE int  flag(int flag)  { return m_flags & flag; };

        ILINE void ORT(int ort) { m_ORT = static_cast<uint8>(ort); };
        ILINE int  ORT() const { return m_ORT; };
        ILINE int  isORT(int o) const { return (m_ORT == o); };

        ILINE void SetRange(float start, float end) { m_rangeStart = start; m_rangeEnd = end; };
        ILINE float GetRangeStart() const { return m_rangeStart; };
        ILINE float GetRangeEnd() const { return m_rangeEnd; };

        // Keys access methods.
        ILINE void reserve_keys(int n) { m_keys.reserve(n); };              // Reserve memory for more keys.
        ILINE void clear()               { m_keys.clear(); };
        ILINE void resize(int num)     { m_keys.resize(num); SetModified(true); };              // Set new key count.
        ILINE bool empty() const         { return m_keys.empty(); };        // Check if curve empty (no keys).
        ILINE int num_keys() const       { return (int)m_keys.size(); };        // Return number of keys in curve.

        ILINE key_type& key(int n)     { return m_keys[n]; };               // Return n key.
        ILINE float&    time(int n)      { return m_keys[n].time; };    // Shortcut to key n time.
        ILINE value_type&   value(int n) { return m_keys[n].value; };   // Shortcut to key n value.
        ILINE value_type&   ds(int n)    { return m_keys[n].ds; };          // Shortcut to key n incoming tangent.
        ILINE value_type&   dd(int n)    { return m_keys[n].dd; };          // Shortcut to key n outgoing tangent.
        ILINE int& flags(int n)        { return m_keys[n].flags; };   // Shortcut to key n flags.

        ILINE key_type const&   key(int n) const          { return m_keys[n]; };                // Return n key.
        ILINE float time(int n) const                             { return m_keys[n].time; };       // Shortcut to key n time.
        ILINE value_type const& value(int n) const    { return m_keys[n].value; };      // Shortcut to key n value.
        ILINE value_type const& ds(int n) const           { return m_keys[n].ds; };             // Shortcut to key n incoming tangent.
        ILINE value_type const& dd(int n) const           { return m_keys[n].dd; };             // Shortcut to key n outgoing tangent.
        ILINE int   flags(int n) const                { return m_keys[n].flags; };    // Shortcut to key n flags.

        ILINE int GetInTangentType(int nkey) const  { return (flags(nkey) & SPLINE_KEY_TANGENT_IN_MASK)  >> SPLINE_KEY_TANGENT_IN_SHIFT; }
        ILINE int GetOutTangentType(int nkey) const { return (flags(nkey) & SPLINE_KEY_TANGENT_OUT_MASK) >> SPLINE_KEY_TANGENT_OUT_SHIFT; }

        ILINE void erase(int key)                     { m_keys.erase(m_keys.begin() + key); SetModified(true); };
        ILINE bool closed()                                     { return (ORT() == ORT_LOOP); } // return True if curve closed.

        ILINE void SetModified(bool bOn, bool bSort = false)
        {
            if (bOn)
            {
                m_flags |= MODIFIED;
            }
            else
            {
                m_flags &= ~(MODIFIED);
            }
            if (bSort)
            {
                m_flags |= MUST_SORT;
            }
            m_curr = 0;
        }

        ILINE void sort_keys()
        {
            std::sort(m_keys.begin(), m_keys.end());
            m_flags &= ~MUST_SORT;
        }

        ILINE void push_back(const key_type& k)
        {
            m_keys.push_back(k);
            SetModified(true);
        };
        ILINE int insert_key(const key_type& k)
        {
            int num = num_keys();
            for (int i = 0; i < num; i++)
            {
                if (m_keys[i].time > k.time)
                {
                    m_keys.insert(m_keys.begin() + i, k);
                    SetModified(true);
                    return i;
                }
            }
            m_keys.push_back(k);
            SetModified(true);
            return num_keys() - 1;
        };

        ILINE int insert_key(float t, value_type const& val)
        {
            key_type key;
            key.time = t;
            key.value = val;
            key.flags = 0;
            spline::Zero(key.ds);
            spline::Zero(key.dd);
            return insert_key(key);
        }

        inline void update()
        {
            if (m_flags & MODIFIED)
            {
                sort_keys();
                if (m_flags & MODIFIED)
                {
                    comp_deriv();
                }
            }
        }

        inline bool is_updated() const
        {
            return (m_flags & MODIFIED) == 0;
        }

        // Interpolate the value along the spline.
        inline bool interpolate(float t, value_type& val)
        {
            update();

            if (empty())
            {
                return false;
            }

            if (t < time(0))
            {
                val = value(0);
                return true;
            }

            adjust_time(t);

            int curr = seek_key(t);
            if (curr < num_keys() - 1)
            {
                assert(t >= time(curr));
                float u = (t - time(curr)) / (time(curr + 1) - time(curr));
                interp_keys(curr, curr + 1, u, val);
            }
            else
            {
                val = value(num_keys() - 1);
            }
            return true;
        }

        size_t mem_size() const
        {
            return this->m_keys.capacity() * sizeof(this->m_keys[0]);
        }

        size_t sizeofThis() const
        {
            return sizeof(*this) + mem_size();
        }

        void swap(TSpline<KeyType, BasisType>& b)
        {
            using std::swap;

            m_keys.swap(b.m_keys);
            swap(m_flags, b.m_flags);
            swap(m_ORT, b.m_ORT);
            swap(m_curr, b.m_curr);
            swap(m_rangeStart, b.m_rangeStart);
            swap(m_rangeEnd, b.m_rangeEnd);
        }

        //////////////////////////////////////////////////////////////////////////
        // This two functions must be overridden in derived spline classes.
        //////////////////////////////////////////////////////////////////////////
        // Pre-compute spline tangents.
        virtual void comp_deriv() = 0;
        // Interpolate value between two keys.
        virtual void interp_keys(int key1, int key2, float u, value_type& val) = 0;
        //////////////////////////////////////////////////////////////////////////

        static void Reflect(AZ::ReflectContext*) {}

    protected:
        AZStd::vector<key_type> m_keys;     // List of keys.
        uint8           m_flags;
        uint8           m_ORT;                          // Out-Of-Range type.
        int16           m_curr;                         // Current key in track.

        float           m_rangeStart;
        float           m_rangeEnd;

        // Return key before or equal to this time.
        inline int seek_key(float t)
        {
            assert(num_keys() < (1 << 15));
            if ((m_curr >= num_keys()) || (time(m_curr) > t))
            {
                // Search from begining.
                m_curr = 0;
            }
            while ((m_curr < num_keys() - 1) && (time(m_curr + 1) <= t))
            {
                ++m_curr;
            }
            return m_curr;
        }

        inline void adjust_time(float& t)
        {
            if (isORT(ORT_CYCLE) || isORT(ORT_LOOP))
            {
                if (num_keys() > 0)
                {
                    float endtime = time(num_keys() - 1);
                    if (t > endtime)
                    {
                        // Warp time.
                        t = spline::fast_fmod(t, endtime);
                    }
                }
            }
        }

    private:
        int m_refCount;

    public:

        inline void add_ref()
        {
            ++m_refCount;
        };

        inline void release()
        {
            AZ_Assert(m_refCount > 0, "Reference count logic error, trying to decrement reference when refCount is 0");
            if (--m_refCount == 0)
            {
                delete this;
            }
        }
    };

    template    <class T>
    struct  SplineKey
    {
        typedef T   value_type;

        float               time;       //!< Key time.
        int                 flags;  //!< Key flags.
        value_type  value;  //!< Key value.
        value_type  ds;         //!< Incoming tangent.
        value_type  dd;         //!< Outgoing tangent.
        SplineKey()
        {
            memset(this, 0, sizeof(SplineKey));
        }

        SplineKey& operator=(const SplineKey& src) { memcpy(this, &src, sizeof(*this)); return *this; }

        static void Reflect(AZ::ReflectContext*) {}
    };

    template    <class T>
    bool    operator ==(const SplineKey<T>& k1, const SplineKey<T>& k2) { return k1.time == k2.time; };
    template    <class T>
    bool    operator !=(const SplineKey<T>& k1, const SplineKey<T>& k2) { return k1.time != k2.time; };
    template    <class T>
    bool    operator < (const SplineKey<T>& k1, const SplineKey<T>& k2) { return k1.time < k2.time; };
    template    <class T>
    bool    operator > (const SplineKey<T>& k1, const SplineKey<T>& k2) { return k1.time > k2.time; };

    template<>
    void SplineKey<Vec2>::Reflect(AZ::ReflectContext* context);

    /** Bezier spline key extended for tangent unify/break.
    */
    template    <class T>
    struct SplineKeyEx
        :  public UiSpline::SplineKey<T>
    {
        float theta_from_dd_to_ds;
        float scale_from_dd_to_ds;

        void ComputeThetaAndScale() { assert(0); }
        void SetOutTangentFromIn() { assert(0); }
        void SetInTangentFromOut() { assert(0); }

        SplineKeyEx()
            : theta_from_dd_to_ds(gf_PI)
            , scale_from_dd_to_ds(1.0f) {}

        static void Reflect(AZ::ReflectContext*) {}
    };

    template <>
    inline void SplineKeyEx<Vec2>::ComputeThetaAndScale()
    {
        scale_from_dd_to_ds = (ds.GetLength() + 1.0f) / (dd.GetLength() + 1.0f);
        float out = fabs(dd.x) > 0.000001f ? atan_tpl(dd.y / dd.x) : (dd.x * dd.y >= 0 ? gf_PI / 2.0f : -gf_PI / 2.0f);
        float in  = fabs(ds.x) > 0.000001f ? atan_tpl(ds.y / ds.x) : (ds.x * ds.y >= 0 ? gf_PI / 2.0f : -gf_PI / 2.0f);
        theta_from_dd_to_ds = in + gf_PI - out;
    }

    template<>
    inline void SplineKeyEx<Vec2>::SetOutTangentFromIn()
    {
        assert((flags & SPLINE_KEY_TANGENT_ALL_MASK) == SPLINE_KEY_TANGENT_UNIFIED);
        float outLength = (ds.GetLength() + 1.0f) / scale_from_dd_to_ds - 1.0f;
        float in  = fabs(ds.x) > 0.000001f ? atan_tpl(ds.y / ds.x) : (ds.x * ds.y >= 0 ? gf_PI / 2.0f : -gf_PI / 2.0f);
        dd.x = 1.0f;
        dd.y = tan_tpl(in + gf_PI - theta_from_dd_to_ds);
        dd.Normalize();
        dd *= outLength;
    }

    template<>
    inline void SplineKeyEx<Vec2>::SetInTangentFromOut()
    {
        assert((flags & SPLINE_KEY_TANGENT_ALL_MASK) == SPLINE_KEY_TANGENT_UNIFIED);
        float inLength = scale_from_dd_to_ds * (dd.GetLength() + 1.0f) - 1.0f;
        float out = fabs(dd.x) > 0.000001f ? atan_tpl(dd.y / dd.x) : (dd.x * dd.y >= 0 ? gf_PI / 2.0f : -gf_PI / 2.0f);
        ds.x = 1.0f;
        ds.y = tan_tpl(out + theta_from_dd_to_ds - gf_PI);
        ds.Normalize();
        ds *= inLength;
    }

    template<>
    void SplineKeyEx<Vec2>::Reflect(AZ::ReflectContext* context);

    //////////////////////////////////////////////////////////////////////////
    // Bezier Spline.
    // This is a modified version of spline::BezierSpline
    // It is modified so that we can use AZ serialization
    //////////////////////////////////////////////////////////////////////////
    template <class T, class Key = SplineKeyEx<T> >
    class BezierSpline
        : public TSpline< Key, spline::BezierBasis >
    {
    public:
        typedef TSpline< Key, spline::BezierBasis > super_type;
        using_type(super_type, key_type);
        using_type(super_type, value_type);

        virtual void comp_deriv()
        {
            this->SetModified(false);

            if (this->num_keys() > 1)
            {
                const float oneThird = 1 / 3.0f;

                const int last = this->num_keys() - 1;

                {
                    if (this->GetInTangentType(0) != SPLINE_KEY_TANGENT_CUSTOM)
                    {
                        spline::Zero(this->key(0).ds);
                    }
                    if (this->GetOutTangentType(0) != SPLINE_KEY_TANGENT_CUSTOM)
                    {
                        this->key(0).dd = oneThird * (this->value(1) - this->value(0));
                    }

                    if (this->GetInTangentType(last) != SPLINE_KEY_TANGENT_CUSTOM)
                    {
                        this->key(last).ds = oneThird * (this->value(last) - this->value(last - 1));
                    }
                    if (this->GetOutTangentType(last) != SPLINE_KEY_TANGENT_CUSTOM)
                    {
                        spline::Zero(this->key(last).dd);
                    }
                }

                for (int i = 1; i < last; ++i)
                {
                    key_type& key = this->key(i);
                    T ds0 = key.ds;
                    T dd0 = key.dd;

                    const float deltaTime = this->time(i + 1) - this->time(i - 1);
                    if (deltaTime <= 0)
                    {
                        spline::Zero(key.ds);
                        spline::Zero(key.dd);
                    }
                    else
                    {
                        const float k = (this->time(i) - this->time(i - 1)) / deltaTime;
                        const value_type deltaValue = this->value(i + 1) - this->value(i - 1);
                        key.ds = oneThird * deltaValue * k;
                        key.dd = oneThird * deltaValue * (1 - k);
                    }

                    switch (this->GetInTangentType(i))
                    {
                    case SPLINE_KEY_TANGENT_STEP:
                        spline::Zero(key.ds);
                        break;
                    case SPLINE_KEY_TANGENT_ZERO:
                        spline::Zero(key.ds);
                        break;
                    case SPLINE_KEY_TANGENT_LINEAR:
                        key.ds = oneThird * (this->value(i) - this->value(i - 1));
                        break;
                    case SPLINE_KEY_TANGENT_CUSTOM:
                        key.ds = ds0;
                        break;
                    }

                    switch (this->GetOutTangentType(i))
                    {
                    case SPLINE_KEY_TANGENT_STEP:
                        spline::Zero(key.dd);
                        break;
                    case SPLINE_KEY_TANGENT_ZERO:
                        spline::Zero(key.dd);
                        break;
                    case SPLINE_KEY_TANGENT_LINEAR:
                        key.dd = oneThird * (this->value(i + 1) - this->value(i));
                        break;
                    case SPLINE_KEY_TANGENT_CUSTOM:
                        key.dd = dd0;
                        break;
                    }
                }
            }
        }

        static void Reflect(AZ::ReflectContext*) {}

    protected:
        virtual void interp_keys(int from, int to, float u, T& val)
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
                typename TSpline<Key, spline::BezierBasis >::basis_type basis(u);

                const T p0 = this->value(from);
                const T p3 = this->value(to);
                const T p1 = p0 + this->dd(from);
                const T p2 = p3 - this->ds(to);

                val = (basis[0] * p0) + (basis[1] * p1) + (basis[2] * p2) + (basis[3] * p3);
            }
        }
    };

    template <class T>
    class TrackSplineInterpolator;

    template <>
    class TrackSplineInterpolator<Vec2>
        : public spline::CBaseSplineInterpolator<Vec2, UiSpline::BezierSpline<Vec2, UiSpline::SplineKeyEx<Vec2> > >
    {
    public:
        AZ_CLASS_ALLOCATOR(TrackSplineInterpolator<Vec2>, AZ::SystemAllocator)

        virtual int GetNumDimensions()
        {
            // It's actually one-dimensional since the x component curve is for a time-warping.
            return 1;
        }
        virtual void SerializeSpline([[maybe_unused]] XmlNodeRef& node, [[maybe_unused]] bool bLoading) {};
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
                spline::CBaseSplineInterpolator<Vec2, UiSpline::BezierSpline<Vec2, UiSpline::SplineKeyEx<Vec2> > >::Interpolate(time_to_check, value);
                return (time_to_check - this->time(curr)) / timeDelta;
            }
            do
            {
                spline::CBaseSplineInterpolator<Vec2, UiSpline::BezierSpline<Vec2, UiSpline::SplineKeyEx<Vec2> > >::Interpolate(time_to_check, value);

                u = (time_to_check - this->time(curr)) / timeDelta;

                if (fabs(value[0] - time) < epsilon)
                {
                    // Finally, we got the solution.
                    break;
                }
                else
                {
                    // Apply the Newton's method to compute the next time value to try.
                    assert(next != curr);
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
            int next = curr + 1;
            assert(0 <= curr && next < num_keys());

            ISplineInterpolator::ValueType value;
            u = search_u(time, value);
            tangent.x = comp_time_deriv(curr, next, u);
            tangent.y = comp_value_deriv(curr, next, u);
            tangent /= 3.0f;
            return tangent;
        }
    public:
        // We should override following 4 methods to make it act like an 1D curve although it's actually a 2D curve.
        virtual void  SetKeyTime(int key, float time)
        {
            ISplineInterpolator::ValueType value;
            ISplineInterpolator::ZeroValue(value);
            spline::CBaseSplineInterpolator<Vec2, UiSpline::BezierSpline<Vec2, UiSpline::SplineKeyEx<Vec2> > >::GetKeyValue(key, value);
            value[0] = time;
            spline::CBaseSplineInterpolator<Vec2, UiSpline::BezierSpline<Vec2, UiSpline::SplineKeyEx<Vec2> > >::SetKeyValue(key, value);
            spline::CBaseSplineInterpolator<Vec2, UiSpline::BezierSpline<Vec2, UiSpline::SplineKeyEx<Vec2> > >::SetKeyTime(key, time);
        }
        virtual void  SetKeyValue(int key, ISplineInterpolator::ValueType value)
        {
            ISplineInterpolator::ValueType value0;
            ISplineInterpolator::ZeroValue(value0);
            value0[0] = GetKeyTime(key);
            value0[1] = value[0];
            spline::CBaseSplineInterpolator<Vec2, UiSpline::BezierSpline<Vec2, UiSpline::SplineKeyEx<Vec2> > >::SetKeyValue(key, value0);
        }
        virtual bool  GetKeyValue(int key, ISplineInterpolator::ValueType& value)
        {
            if (spline::CBaseSplineInterpolator<Vec2, UiSpline::BezierSpline<Vec2, UiSpline::SplineKeyEx<Vec2> > >::GetKeyValue(key, value))
            {
                value[0] = value[1];
                value[1] = 0;
                return true;
            }
            return false;
        }
        virtual void Interpolate(float time, ISplineInterpolator::ValueType& value)
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
            spline::CBaseSplineInterpolator<Vec2, UiSpline::BezierSpline<Vec2, UiSpline::SplineKeyEx<Vec2> > >::SetKeyFlags(k, flags);
        }
        virtual void SetKeyInTangent(int k, ISplineInterpolator::ValueType tin)
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
        virtual void SetKeyOutTangent(int k, ISplineInterpolator::ValueType tout)
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

        virtual void comp_deriv()
        {
            UiSpline::BezierSpline<Vec2, UiSpline::SplineKeyEx<Vec2> >::comp_deriv();

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

        virtual int InsertKey(float t, ISplineInterpolator::ValueType val)
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
            int keyIndex = spline::CBaseSplineInterpolator<Vec2, UiSpline::BezierSpline<Vec2, UiSpline::SplineKeyEx<Vec2> > >::InsertKey(t, val);
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
                    assert(0);
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
}; // namespace spline
