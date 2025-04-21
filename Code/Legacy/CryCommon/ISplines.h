/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_CRYCOMMON_ISPLINES_H
#define CRYINCLUDE_CRYCOMMON_ISPLINES_H
#pragma once

#include <IXml.h>
#include <AzCore/std/containers/vector.h>

//////////////////////////////////////////////////////////////////////////

namespace AZ
{
    class ReflectContext;
}

// These flags are mostly applicable for hermit based splines.
enum ESplineKeyTangentType
{
    SPLINE_KEY_TANGENT_NONE    = 0,
    SPLINE_KEY_TANGENT_CUSTOM  = 1,
    SPLINE_KEY_TANGENT_ZERO    = 2,
    SPLINE_KEY_TANGENT_STEP    = 3,
    SPLINE_KEY_TANGENT_LINEAR  = 4,
    SPLINE_KEY_TANGENT_BEZIER  = 5
};

#define SPLINE_KEY_TANGENT_IN_SHIFT  (0)
#define SPLINE_KEY_TANGENT_IN_MASK   (0x07)                                  // 0000111
#define SPLINE_KEY_TANGENT_OUT_SHIFT (3)
#define SPLINE_KEY_TANGENT_OUT_MASK  (0x07 << (SPLINE_KEY_TANGENT_OUT_SHIFT))  // 0111000
#define SPLINE_KEY_TANGENT_UNIFY_SHIFT (6)
#define SPLINE_KEY_TANGENT_UNIFY_MASK  (0x01 << (SPLINE_KEY_TANGENT_UNIFY_SHIFT))  // 1000000

#define SPLINE_KEY_TANGENT_ALL_MASK (SPLINE_KEY_TANGENT_IN_MASK | SPLINE_KEY_TANGENT_OUT_MASK | SPLINE_KEY_TANGENT_UNIFY_MASK)
#define SPLINE_KEY_TANGENT_UNIFIED ((SPLINE_KEY_TANGENT_CUSTOM << SPLINE_KEY_TANGENT_IN_SHIFT)    \
                                    | (SPLINE_KEY_TANGENT_CUSTOM << SPLINE_KEY_TANGENT_OUT_SHIFT) \
                                    | (0x01 << SPLINE_KEY_TANGENT_UNIFY_SHIFT))
#define SPLINE_KEY_TANGENT_BROKEN ((SPLINE_KEY_TANGENT_CUSTOM << SPLINE_KEY_TANGENT_IN_SHIFT)    \
                                   | (SPLINE_KEY_TANGENT_CUSTOM << SPLINE_KEY_TANGENT_OUT_SHIFT) \
                                   | (0x00 << SPLINE_KEY_TANGENT_UNIFY_SHIFT))

enum ESplineKeyFlags
{
    ESPLINE_KEY_UI_SELECTED_SHIFT = 16,
    ESPLINE_KEY_UI_SELECTED_MAX_DIMENSION_COUNT = 4,   // should be power of 2 (see ESPLINE_KEY_UI_SELECTED_MASK)
    ESPLINE_KEY_UI_SELECTED_MASK = ((1 << ESPLINE_KEY_UI_SELECTED_MAX_DIMENSION_COUNT) - 1) << ESPLINE_KEY_UI_SELECTED_SHIFT
};


// Return value closest to 0 if same sign, or 0 if opposite.
template<class T>
inline T minmag(T const& a, T const& b)
{
    if (a * b <= T(0.f))
    {
        return T(0.f);
    }
    else if (a < T(0.f))
    {
        return max(a, b);
    }
    else
    {
        return min(a, b);
    }
}

template<class T>
inline Vec3_tpl<T> minmag(Vec3_tpl<T> const& a, Vec3_tpl<T> const& b)
{
    return Vec3_tpl<T>(minmag(a.x, b.x), minmag(a.y, b.y), minmag(a.z, b.z));
}

template<class T>
T abs(Vec3_tpl<T> v)
{
    return v.GetLength();
}

//////////////////////////////////////////////////////////////////////////
// Interface returned by backup methods of ISplineInterpolator.
//////////////////////////////////////////////////////////////////////////
struct ISplineBackup
{
    // <interfuscator:shuffle>
    virtual ~ISplineBackup(){}
    virtual void AddRef() = 0;
    virtual void Release() = 0;
    // </interfuscator:shuffle>
};

//////////////////////////////////////////////////////////////////////////
// General Interpolation interface.
//////////////////////////////////////////////////////////////////////////
struct ISplineInterpolator
{
    typedef float ElemType;
    typedef ElemType ValueType[4];

    // <interfuscator:shuffle>
    virtual ~ISplineInterpolator(){}

    // Dimension of the spline from 0 to 3, number of parameters used in ValueType.
    virtual int GetNumDimensions() = 0;

    // Insert`s a new key, returns index of the key.
    virtual int  InsertKey(float time, ValueType value) = 0;
    virtual void RemoveKey(int key) = 0;

    virtual void FindKeysInRange(float startTime, float endTime, int& firstFoundKey, int& numFoundKeys) = 0;
    virtual void RemoveKeysInRange(float startTime, float endTime) = 0;

    virtual int   GetKeyCount() = 0;
    virtual void  SetKeyTime(int key, float time) = 0;
    virtual float GetKeyTime(int key) = 0;
    virtual void  SetKeyValue(int key, ValueType value) = 0;
    virtual bool  GetKeyValue(int key, ValueType& value) = 0;

    virtual void  SetKeyInTangent(int key, ValueType tin) = 0;
    virtual void  SetKeyOutTangent(int key, ValueType tout) = 0;
    virtual void  SetKeyTangents(int key, ValueType tin, ValueType tout) = 0;
    virtual bool  GetKeyTangents(int key, ValueType& tin, ValueType& tout) = 0;

    // Changes key flags, @see ESplineKeyFlags
    virtual void  SetKeyFlags(int key, int flags) = 0;
    // Retrieve key flags, @see ESplineKeyFlags
    virtual int   GetKeyFlags(int key) = 0;

    virtual void Interpolate(float time, ValueType& value) = 0;
    virtual void EvalInTangent([[maybe_unused]] float time, [[maybe_unused]] ValueType& value) {};
    virtual void EvalOutTangent([[maybe_unused]] float time, [[maybe_unused]] ValueType& value) {};

    virtual void SerializeSpline(XmlNodeRef& node, bool bLoading) = 0;

    virtual ISplineBackup* Backup() = 0;
    virtual void Restore(ISplineBackup* pBackup) = 0;

    void ClearAllKeys()
    {
        while (GetKeyCount() > 0)
        {
            RemoveKey(0);
        }
        Update();
    }

    // </interfuscator:shuffle>

    //////////////////////////////////////////////////////////////////////////
    // Helper functions.
    //////////////////////////////////////////////////////////////////////////
    inline bool IsKeySelectedAtAnyDimension(const int key)
    {
        const int flags = GetKeyFlags(key);
        const int dimensionCount = GetNumDimensions();
        const int mask = ((1 << dimensionCount) - 1) << ESPLINE_KEY_UI_SELECTED_SHIFT;
        return (flags & mask) != 0;
    }

    inline bool IsKeySelectedAtDimension(const int key, const int dimension)
    {
        const int flags = GetKeyFlags(key);
        const int mask = 1 << (ESPLINE_KEY_UI_SELECTED_SHIFT + dimension);
        return (flags & mask) != 0;
    }

    void SelectKeyAllDimensions(int key, bool select)
    {
        const int flags = GetKeyFlags(key);
        if (select)
        {
            const int dimensionCount = GetNumDimensions();
            const int mask = ((1 << dimensionCount) - 1) << ESPLINE_KEY_UI_SELECTED_SHIFT;
            SetKeyFlags(key, (flags & (~ESPLINE_KEY_UI_SELECTED_MASK)) | mask);
        }
        else
        {
            SetKeyFlags(key, flags & (~ESPLINE_KEY_UI_SELECTED_MASK));
        }
    }

    void SelectKeyAtDimension(int key, int dimension, bool select)
    {
        const int flags = GetKeyFlags(key);
        const int mask = 1 << (ESPLINE_KEY_UI_SELECTED_SHIFT + dimension);
        SetKeyFlags(key, (select ? (flags | mask) : (flags & (~mask))));
    }

    inline int InsertKeyFloat(float time, float val) { ValueType v = {val, 0, 0, 0}; return InsertKey(time, v); }
    inline int InsertKeyFloat3(float time, float* vals) { ValueType v = {vals[0], vals[1], vals[2], 0}; return InsertKey(time, v); }
    inline bool GetKeyValueFloat(int key, float& value) { ValueType v = {value}; bool b = GetKeyValue(key, v); value = v[0]; return b; }
    inline void SetKeyValueFloat(int key, float value) { ValueType v = {value, 0, 0, 0}; SetKeyValue(key, v); }
    inline void SetKeyValueFloat3(int key, float* vals) { ValueType v = {vals[0], vals[1], vals[2], 0}; SetKeyValue(key, v); }
    inline void InterpolateFloat(float time, float& val) { ValueType v = {val}; Interpolate(time, v); val = v[0]; }
    inline void InterpolateFloat3(float time, float* vals) { ValueType v = {vals[0], vals[1], vals[2]}; Interpolate(time, v); vals[0] = v[0]; vals[1] = v[1]; vals[2] = v[2]; }

    inline void EvalInTangentFloat(float time, float& val) { ValueType v = { val }; EvalInTangent(time, v); val = v[0]; }
    inline void EvalOutTangentFloat(float time, float& val) { ValueType v = { val }; EvalOutTangent(time, v); val = v[0]; }


    // Return Key closest to the specified time.
    inline int FindKey(float fTime, float fEpsilon = 0.01f)
    {
        int nKey = -1;
        // Find key.
        for (int k = 0; k < GetKeyCount(); k++)
        {
            if (fabs(GetKeyTime(k) - fTime) < fEpsilon)
            {
                nKey = k;
                break;
            }
        }
        return nKey;
    }

    // Force update.
    void Update() { ValueType val; Interpolate(0.f, val); }
    static void ZeroValue(ValueType& value) { value[0] = 0; value[1] = 0; value[2] = 0; value[3] = 0; }
};

//////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////
namespace spline
{
    template    <int N>
    class BasisFunction
    {
    public:
        const float& operator[](int i) const { return m_f[i]; };
    protected:
        float   m_f[N];
    };

    // Special functions that makes parameter zero.
    template <class T>
    inline void Zero(T& val)
    {
        memset(&val, 0, sizeof(val));
    }
    // Specialized Zero functions
    template <>
    inline void Zero(float& val) { val = 0.0f; }
    template <>
    inline void Zero(Vec2& val) { val = Vec2(0.0f, 0.0f); }
    template <>
    inline void Zero(Vec3& val) { val = Vec3(0.0f, 0.0f, 0.0f); }
    template <>
    inline void Zero(Quat& val) { val.SetIdentity(); }

    inline float Concatenate(float left, float right) { return left + right; }
    inline Vec3 Concatenate(const Vec3& left, const Vec3& right) { return left + right; }
    inline Quat Concatenate(const Quat& left, const Quat& right) { return left * right; }
    inline float Subtract (float left, float right) { return left - right; }
    inline Vec3 Subtract (const Vec3& left, const Vec3& right) { return left - right; }
    inline Quat Subtract(const Quat& left, const Quat& right) { return left / right; }

    ///////////////////////////////////////////////////////////////////////////////
    // HermitBasis.
    class HermitBasis
        : public BasisFunction<4>
    {
    public:
        HermitBasis(float t)
        {
            float t2, t3, t2_3, t3_2, t3_t2;

            t2 = t * t;                                               // t2 = t^2;
            t3 = t2 * t;                                          // t3 = t^3;

            t3_2 = t3 + t3;
            t2_3 = 3 * t2;
            t3_t2 = t3 - t2;
            m_f[0] = t3_2 - t2_3 + 1;
            m_f[1] = -t3_2 + t2_3;
            m_f[2] = t3_t2 - t2 + t;
            m_f[3] = t3_t2;
        }
    };

    ///////////////////////////////////////////////////////////////////////////////
    // BezierBasis.
    class BezierBasis
        : public BasisFunction<4>
    {
    public:
        BezierBasis(const float t)
        {
            const float t2 = t * t;
            const float t3 = t2 * t;
            m_f[0] = -t3 + 3 * t2 - 3 * t + 1;
            m_f[1] = 3 * t3 - 6 * t2 + 3 * t;
            m_f[2] = -3 * t3 + 3 * t2;
            m_f[3] = t3;
        }
    };

    template<class T>
    struct TCoeffBasis
    {
        // Coefficients for a cubic polynomial.
        T   m_c[4];

        inline T eval(float t) const
        {
            return m_c[0] + t * (m_c[1] + t * (m_c[2] + t * m_c[3]));
        }
        // Compute coeffs based on 2 endpoints & slopes.
        void set(float t0, T v0, T s0, float t1, T v1, T s1)
        {
            /*
                Solve cubic equation:
                    v(u) = d t^3 + c t^2 + b t + a
                for
                    v(0) = v0, v'(0) = s0, v(t1) = v1, v'(t1) = s1

                Solution:
                    a   = v0
                    b   =                       s0
                    c   = -3v0 +3v1 -2s0    -s1
                    d = +2v0 -2v1 +s0       +s1
            */

            /*
                Polynomial will be evaluated on adjusted parameter u == t-t0. u0 = 0, u1 = t1-t0.
                The range is normalised to start at 0 to avoid extra terms in the coefficient
                computation that can compromise precision. However, the range is not normalised
                to length 1, because that would require a division at runtime. Instead, we perform
                the division on the coefficients.
            */
            m_c[0] = v0;

            if (t1 <= t0)
            {
                m_c[1] = m_c[2] = m_c[3] = T(0.f);
            }
            else
            {
                float idt = 1.f / (t1 - t0);

                m_c[1] = T(s0 * idt);
                m_c[2] = T((-3.0f * v0 + 3.0f * v1 - 2.0f * s0 - s1) * idt * idt);
                m_c[3] = T((2.0f * v0 - 2.0f * v1 + s0 + s1) * idt * idt * idt);
            }
        }
    };

    inline float fast_fmod(float x, float y)
    {
        return fmod_tpl(x, y);
        //int ival = ftoi(x/y);
        //return x - ival*y;
    }

    /****************************************************************************
    **                            Key classes                                                                    **
    ****************************************************************************/
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

        static void Reflect(AZ::ReflectContext* context) {}
    };

    template    <class T>
    bool    operator ==(const SplineKey<T>& k1, const SplineKey<T>& k2) { return k1.time == k2.time; };
    template    <class T>
    bool    operator !=(const SplineKey<T>& k1, const SplineKey<T>& k2) { return k1.time != k2.time; };
    template    <class T>
    bool    operator < (const SplineKey<T>& k1, const SplineKey<T>& k2) { return k1.time < k2.time; };
    template    <class T>
    bool    operator > (const SplineKey<T>& k1, const SplineKey<T>& k2) { return k1.time > k2.time; };

    //////////////////////////////////////////////////////////////////////////
    // TCBSplineKey class
    //////////////////////////////////////////////////////////////////////////

    template    <class T>
    struct TCBSplineKey
        :  public SplineKey<T>
    {
        // Key controls.
        float tens;         //!< Key tension value.
        float cont;         //!< Key continuity value.
        float bias;         //!< Key bias value.
        float easeto;       //!< Key ease to value.
        float easefrom;     //!< Key ease from value.

        TCBSplineKey() { tens = 0, cont = 0, bias = 0, easeto = 0, easefrom = 0; };
    };

    //! TCB spline key used in quaternion spline with angle axis as input.
    struct TCBAngAxisKey
        :  public TCBSplineKey<Quat>
    {
        float angle;
        Vec3 axis;

        TCBAngAxisKey()
            : axis(0, 0, 0)
            , angle(0) {};
    };

    //////////////////////////////////////////////////////////////////////////
    // General Spline class
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
            std::stable_sort(m_keys.begin(), m_keys.end());
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
            Zero(key.ds);
            Zero(key.dd);
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

        static void Reflect(AZ::ReflectContext* context) {}

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

    private:
        int m_refCount;

    protected:
        AZStd::vector<key_type> m_keys;                 // List of keys.
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
                        t = fast_fmod(t, endtime);
                    }
                }
            }
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // TSplineSlopes is default implementation of slope computation
    //////////////////////////////////////////////////////////////////////////
    template < class T, class Key = SplineKey<T>, bool bCLAMP = false >
    class TSplineSlopes
        : public TSpline< Key, TCoeffBasis<T> >
    {
    public:
        typedef TSpline< Key, TCoeffBasis<T> > super_type;
        using_type(super_type, key_type);
        using_type(super_type, value_type);
        static const bool clamp_range = bCLAMP;

        void comp_deriv()
        {
            this->SetModified(false);

            int last = this->num_keys() - 1;
            if (last <= 0)
            {
                return;
            }

            if (bCLAMP)
            {
                // Change discontinuous slopes.
                for (int i = 0; i <= last; ++i)
                {
                    // Out slopes.
                    if (i < last && this->GetOutTangentType(i) == SPLINE_KEY_TANGENT_LINEAR)
                    {
                        // Set linear between points.
                        this->dd(i) = this->value(i + 1) - this->value(i);
                        if (this->GetInTangentType(i + 1) == SPLINE_KEY_TANGENT_NONE)
                        {
                            // Match continuous slope on right.
                            this->dd(i) = 2.0f * this->dd(i) - this->ds(i + 1);
                        }
                    }
                    else if (i < last && this->GetOutTangentType(i) == SPLINE_KEY_TANGENT_NONE)
                    {
                        this->dd(i) = value_type(0.f);
                    }

                    // In slopes.
                    if (i > 0 && this->GetInTangentType(i) == SPLINE_KEY_TANGENT_LINEAR)
                    {
                        // Set linear between points.
                        this->ds(i) = this->value(i) - this->value(i - 1);
                        if (this->GetOutTangentType(i - 1) == SPLINE_KEY_TANGENT_NONE)
                        {
                            // Match continuous slope on left.
                            this->ds(i) = 2.0f * this->ds(i) - this->dd(i - 1);
                        }
                    }
                    else if (i < last && this->GetInTangentType(i) == SPLINE_KEY_TANGENT_NONE)
                    {
                        this->ds(i) = value_type(0.f);
                    }
                }
            }
            else
            {
                key_type& k0 = this->key(0);
                key_type& k1 = this->key(last);

                Zero(k0.ds);
                k0.dd = (0.5f) * (this->value(1) - this->value(0));
                k1.ds = (0.5f) * (this->value(last) - this->value(last - 1));
                Zero(k1.dd);

                for (int i = 1; i < (this->num_keys() - 1); ++i)
                {
                    key_type& key = this->key(i);
                    key.ds = 0.5f * (this->value(i + 1) - this->value(i - 1));
                    key.dd = key.ds;
                }
            }
        }

        virtual void interp_keys(int key1, int key2, float u, value_type& val)
        {
            // Compute coeffs dynamically.
            TCoeffBasis<T> coeff;
            coeff.set(0.f, this->value(key1), this->dd(key1), 1.f, this->value(key2), this->ds(key2));
            val = coeff.eval(u);
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // CatmullRomSpline class implementation
    //////////////////////////////////////////////////////////////////////////
    template <class T, class Key = SplineKey<T>, bool bRangeLimit = false >
    class CatmullRomSpline
        : public TSplineSlopes< T, Key, bRangeLimit >
    {
    protected:

        typedef TSplineSlopes< T, Key, bRangeLimit > super_type;

        std::vector< TCoeffBasis<T> > m_coeffs;

        virtual void comp_deriv()
        {
            super_type::comp_deriv();

            // Store coeffs for each segment.
            m_coeffs.resize(this->num_keys());

            if (this->num_keys() > 0)
            {
                unsigned i;
                for (i = 0; i < m_coeffs.size() - 1; i++)
                {
                    m_coeffs[i].set(this->time(i), this->value(i), this->dd(i), this->time(i + 1), this->value(i + 1), this->ds(i + 1));
                }

                // Last segment is just constant value.
                m_coeffs[i].set(this->time(i), this->value(i), T(0.f), this->time(i) + 1.f, this->value(i), T(0.f));
            }
        }

        virtual void interp_keys(int key1, int key2, float u, typename TSpline<Key, HermitBasis>::value_type& val)
        {
            u *= this->time(key2) - this->time(key1);
            val = m_coeffs[key1].eval(u);
        }

    public:

        size_t mem_size() const
        {
            return super_type::mem_size() + this->m_coeffs.capacity() * sizeof(this->m_coeffs[0]);
        }
        size_t sizeofThis() const
        {
            return sizeof(*this) + mem_size();
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // Extended version of Hermit Spline.
    // Provides more control on key tangents.
    //////////////////////////////////////////////////////////////////////////
    template <class T, class Key = SplineKey<T> >
    class HermitSplineEx
        : public TSpline< Key, HermitBasis >
    {
    public:
        typedef TSpline< Key, HermitBasis > super_type;
        using_type(super_type, key_type);
        using_type(super_type, value_type);

        virtual void comp_deriv()
        {
            this->SetModified(false);
            if (this->num_keys() > 1)
            {
                int last = this->num_keys() - 1;
                key_type& k0 = this->key(0);
                key_type& k1 = this->key(last);
                Zero(k0.ds);
                Zero(k0.dd);
                Zero(k1.ds);
                Zero(k1.dd);

                Zero(k0.ds);
                k0.dd = (0.5f) * (this->value(1) - this->value(0));
                k1.ds = (0.5f) * (this->value(last) - this->value(last - 1));
                Zero(k1.dd);

                for (int i = 1; i < (this->num_keys() - 1); ++i)
                {
                    key_type& key = this->key(i);
                    key.ds = 0.5f * (this->value(i + 1) - this->value(i - 1));
                    key.dd = key.ds;
                    switch (this->GetInTangentType(i))
                    {
                    case SPLINE_KEY_TANGENT_STEP:
                        key.ds = value_type();
                        break;
                    case SPLINE_KEY_TANGENT_ZERO:
                        key.ds = value_type();
                        break;
                    case SPLINE_KEY_TANGENT_LINEAR:
                        key.ds = this->value(i) - this->value(i - 1);
                        break;
                    }
                    switch (this->GetOutTangentType(i))
                    {
                    case SPLINE_KEY_TANGENT_STEP:
                        key.dd = value_type();
                        break;
                    case SPLINE_KEY_TANGENT_ZERO:
                        key.dd = value_type();
                        break;
                    case SPLINE_KEY_TANGENT_LINEAR:
                        key.dd = this->value(i + 1) - this->value(i);
                        break;
                    }
                }
            }
        }

    protected:
        virtual void interp_keys(int from, int to, float u, T& val)
        {
            if (this->GetInTangentType(to) == SPLINE_KEY_TANGENT_STEP || this->GetOutTangentType(from) == SPLINE_KEY_TANGENT_STEP)
            {
                val = this->value(from);
                return;
            }
            typename TSpline<Key, HermitBasis >::basis_type basis(u);
            val = (basis[0] * this->value(from)) + (basis[1] * this->value(to)) + (basis[2] * this->dd(from)) + (basis[3] * this->ds(to));
        }
    };


    //////////////////////////////////////////////////////////////////////////
    // Bezier Spline.
    //////////////////////////////////////////////////////////////////////////
    template <class T, class Key = SplineKey<T> >
    class BezierSpline
        : public TSpline< Key, BezierBasis >
    {
    public:
        typedef TSpline< Key, BezierBasis > super_type;
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
                        Zero(this->key(0).ds);
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
                        Zero(this->key(last).dd);
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
                        Zero(key.ds);
                        Zero(key.dd);
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
                        Zero(key.ds);
                        break;
                    case SPLINE_KEY_TANGENT_ZERO:
                        Zero(key.ds);
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
                        Zero(key.dd);
                        break;
                    case SPLINE_KEY_TANGENT_ZERO:
                        Zero(key.dd);
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

        static void Reflect(AZ::ReflectContext* context) {}

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
                typename TSpline<Key, BezierBasis >::basis_type basis(u);

                const T p0 = this->value(from);
                const T p3 = this->value(to);
                const T p1 = p0 + this->dd(from);
                const T p2 = p3 - this->ds(to);

                val = (basis[0] * p0) + (basis[1] * p1) + (basis[2] * p2) + (basis[3] * p3);
            }
        }
    };


    //////////////////////////////////////////////////////////////////////////
    // Base class for spline interpolators.
    //////////////////////////////////////////////////////////////////////////

    template <typename spline_type>
    struct SSplineBackup
        : public ISplineBackup
        , public spline_type
    {
        int refCount;

        SSplineBackup(spline_type const& s)
            : spline_type(s)
            , refCount(0) {}
        virtual void AddRef() {++refCount; }
        virtual void Release()
        {
            if (--refCount <= 0)
            {
                delete this;
            }
        }
    };

    template <class value_type, class spline_type>
    class CBaseSplineInterpolator
        : public ISplineInterpolator
        , public spline_type
    {
    public:
        static const int DIM = sizeof(value_type) / sizeof(ElemType);

        //////////////////////////////////////////////////////////////////////////
        inline void ToValueType(const value_type& t, ValueType& v) { *(value_type*)v = t; }
        inline void FromValueType(ValueType v, value_type& t) { t = *(value_type*)v; }
        //////////////////////////////////////////////////////////////////////////

        virtual void SetModified(bool b, bool bSort = false)
        {
            spline_type::SetModified(b, bSort);
        }
        virtual int GetNumDimensions()
        {
            assert(sizeof(value_type) % sizeof(ElemType) == 0);
            return DIM;
        }

        virtual int InsertKey(float t, ValueType val)
        {
            value_type value;
            FromValueType(val, value);
            return spline_type::insert_key(t, value);
        }
        virtual void RemoveKey(int key)
        {
            if (key >= 0 && key < this->num_keys())
            {
                this->erase(key);
            }
        }
        virtual void FindKeysInRange(float startTime, float endTime, int& firstFoundKey, int& numFoundKeys)
        {
            int count = this->num_keys();
            int start = 0;
            int end = count;
            for (int i = 0; i < count; ++i)
            {
                float keyTime = this->key(i).time;
                if (keyTime < startTime)
                {
                    start = i + 1;
                }
                if (keyTime > endTime && end > i)
                {
                    end = i;
                }
            }
            if (start < end)
            {
                firstFoundKey = start;
                numFoundKeys = end - start;
            }
            else
            {
                firstFoundKey = -1;
                numFoundKeys = 0;
            }
        }
        virtual void RemoveKeysInRange(float startTime, float endTime)
        {
            int firstFoundKey, numFoundKeys;
            FindKeysInRange(startTime, endTime, firstFoundKey, numFoundKeys);
            while (numFoundKeys-- > 0)
            {
                this->erase(firstFoundKey++);
            }
        }
        virtual int GetKeyCount()
        {
            return this->num_keys();
        };
        virtual float GetKeyTime(int key)
        {
            if (key >= 0 && key < this->num_keys())
            {
                return this->key(key).time;
            }
            return 0;
        }
        virtual bool GetKeyValue(int key, ValueType& val)
        {
            if (key >= 0 && key < this->num_keys())
            {
                ToValueType(this->key(key).value, val);
                return true;
            }
            return false;
        }
        virtual void SetKeyValue(int k, ValueType val)
        {
            if (k >= 0 && k < this->num_keys())
            {
                FromValueType(val, this->key(k).value);
                this->SetModified(true);
            }
        }
        virtual void SetKeyTime(int k, float fTime)
        {
            if (k >= 0 && k < this->num_keys())
            {
                this->key(k).time = fTime;
                this->SetModified(true, true);
            }
        }
        virtual void  SetKeyInTangent(int k, ValueType tin)
        {
            if (k >= 0 && k < this->num_keys())
            {
                FromValueType(tin, this->key(k).ds);
                this->SetModified(true);
            }
        }
        virtual void  SetKeyOutTangent(int k, ValueType tout)
        {
            if (k >= 0 && k < this->num_keys())
            {
                FromValueType(tout, this->key(k).dd);
                this->SetModified(true);
            }
        }
        virtual void  SetKeyTangents(int k, ValueType tin, ValueType tout)
        {
            if (k >= 0 && k < this->num_keys())
            {
                FromValueType(tin, this->key(k).ds);
                FromValueType(tout, this->key(k).dd);
                this->SetModified(true);
            }
        }
        virtual bool GetKeyTangents(int k, ValueType& tin, ValueType& tout)
        {
            if (k >= 0 && k < this->num_keys())
            {
                ToValueType(this->key(k).ds, tin);
                ToValueType(this->key(k).dd, tout);
                return true;
            }
            else
            {
                return false;
            }
        }
        virtual void  SetKeyFlags(int k, int flags)
        {
            if (k >= 0 && k < this->num_keys())
            {
                this->key(k).flags = flags;
                this->SetModified(true);
            }
        }
        virtual int   GetKeyFlags(int k)
        {
            if (k >= 0 && k < this->num_keys())
            {
                return this->key(k).flags;
            }
            return 0;
        }

        virtual void Interpolate(float time, ValueType& value)
        {
            value_type v;
            if (spline_type::interpolate(time, v))
            {
                ToValueType(v, value);
            }
        }

        virtual ISplineBackup* Backup()
        {
            return new SSplineBackup<spline_type>(*this);
        }

        virtual void Restore(ISplineBackup* p)
        {
            SSplineBackup<spline_type>* pBackup = static_cast<SSplineBackup<spline_type>*>(p);
            static_cast<spline_type&>(*this) = *pBackup;
        }
    };
}

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(spline::SplineKey<Vec2>, "{24A4D7E5-C36D-427D-AB49-CD86573B7288}");
}
#endif // CRYINCLUDE_CRYCOMMON_ISPLINES_H
