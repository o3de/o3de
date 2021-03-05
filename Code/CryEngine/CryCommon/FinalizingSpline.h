/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_CRYCOMMON_FINALIZINGSPLINE_H
#define CRYINCLUDE_CRYCOMMON_FINALIZINGSPLINE_H
#pragma once


#include <ISplines.h> // <> required for Interfuscator
#include <AzCore/Casting/numeric_cast.h>
#include "CryCustomTypes.h"

inline bool IsEquivalent(float a, float b, float eps)
{
    return abs(a - b) <= eps;
}

// change this value to 1 run spline verification code; this was previously enabled in debug and
// was causing issues because triggering the assert would steal the focus from the editor
// and cause the user to put a control point in the wrong spot.  we don't understand this code
// enough to triage the asserts yet, but things seem to be functioning properly.
#define VERIFY_SPLINE_CONVERSION 0

namespace spline
{
    //////////////////////////////////////////////////////////////////////////
    // FinalizingSpline
    //////////////////////////////////////////////////////////////////////////

    template <class Source, class Final>
    class FinalizingSpline
        : public CBaseSplineInterpolator<typename Source::value_type, Source>
    {
    public:
        using_type(Source, value_type);
        using_type(Source, key_type);
        using_type(ISplineInterpolator, ValueType);

        FinalizingSpline()
            : m_pFinal(0)
        {}

        void SetFinal(Final* pFinal)
        {
            m_pFinal = pFinal;
            m_pFinal->to_source(*this);
        }

        // Most spline functions use source spline (base class).
        // interpolate() uses dest, for fidelity.
        virtual void Interpolate(float time, ValueType& value)
        {
            Source::update();
            assert(m_pFinal);
            m_pFinal->interpolate(time, *(value_type*)&value);
        }

        virtual void EvalInTangent(float time, ValueType& value)
        {
            Source::update();
            assert(m_pFinal);
            m_pFinal->evalInTangent(time, *(value_type*)&value);

        }

        virtual void EvalOutTangent(float time, ValueType& value)
        {
            Source::update();
            assert(m_pFinal);
            m_pFinal->evalOutTangent(time, *(value_type*)&value);

        }

        virtual void eval(float time, ValueType& value)
        {
            Source::update();
            assert(m_pFinal);
            m_pFinal->interpolate(time, *(value_type*)&value);
        }

        // Update dest when source modified.
        // Should be called for every update to source spline.
        virtual void SetModified(bool bOn, bool bSort = false)
        {
            Source::SetModified(bOn, bSort);
            assert(m_pFinal);
            m_pFinal->from_source(*this);
        }

        virtual bool GetKeyTangents(int k, ValueType& tin, ValueType& tout)
        {
            if (k >= 0 && k < this->num_keys())
            {
                this->ToValueType(Source::ds(k), tin);
                this->ToValueType(Source::dd(k), tout);
                return true;
            }
            else
            {
                return false;
            }
        }

        virtual int   GetKeyFlags(int k)
        {
            return Source::flags(k);
        }

        ///////////////////////////////////////////

        virtual void SerializeSpline([[maybe_unused]] XmlNodeRef& node, [[maybe_unused]] bool bLoading)
        {}

    protected:

        Final*          m_pFinal;
    };

#if 0

    //////////////////////////////////////////////////////////////////////////
    // LookupTableSpline
    //////////////////////////////////////////////////////////////////////////

    template <class S, class value_type, const int nMAX_ENTRIES>
    class LookupTableSplineInterpolater
    {
    public:
        ILINE static void fast_interpolate(float t, value_type& val, S* m_table)
        {
            t *= nMAX_ENTRIES - 1.0f;
            float frac = t - floorf(t);
            int idx = int(t);
            val = value_type(m_table[idx]) * (1.f - frac);
            val += value_type(m_table[idx + 1]) * frac;
        }
    };

    template <class value_type, const int nMAX_ENTRIES>
    class LookupTableSplineInterpolater <UnitFloat8, value_type, nMAX_ENTRIES>
    {
        enum
        {
            SHIFT_AMOUNT = 24
        };

    public:
        ILINE static void fast_interpolate(float t, value_type& val, UnitFloat8* m_table)
        {
            const float scale = (float)(1 << SHIFT_AMOUNT);
            uint32 ti = uint32(t * scale * (nMAX_ENTRIES - 1.0f));
            uint32 idx = ti >> SHIFT_AMOUNT;
            uint32 frac = ti - (idx << SHIFT_AMOUNT);
            uint32 vali = (uint32)m_table[idx + 1].GetStore() * frac;
            frac = (1 << SHIFT_AMOUNT) - frac;
            vali += (uint32)m_table[idx].GetStore() * frac;
            val = (value_type)vali * (1.0f / (255.0f * scale));
        }
    };


    template <class S, class Source>
    class LookupTableSpline
        : public FinalizingSpline<Source>
    {
        typedef FinalizingSpline<Source> super_type;
        using super_type::m_pSource;
        using super_type::is_updated;

        enum
        {
            nSTORE_SIZE = 128
        };
        enum
        {
            nMAX_ENTRIES = nSTORE_SIZE - 1
        };
        enum
        {
            nMIN_VALUE = nMAX_ENTRIES
        };

    public:

        using_type(super_type, value_type);

        LookupTableSpline()
        {
            init();
        }

        LookupTableSpline(const LookupTableSpline& other)
            : super_type(other)
        {
            init();
            update();
        }
        void operator=(const LookupTableSpline& other)
        {
            super_type::operator=(other);
            update();
        }

        ~LookupTableSpline()
        {
            delete[] m_table;
        }

        void interpolate(float t, value_type& val)
        {
            if (!is_updated())
            {
                update();
            }
            fast_interpolate(clamp_tpl(t, 0.0f, 1.0f), val);
        }

        void fast_interpolate(float t, value_type& val) const
        {
            LookupTableSplineInterpolater<S, value_type, nMAX_ENTRIES>::fast_interpolate(t, val, m_table);
        }

        ILINE void min_value(value_type& val) const
        {
            val = value_type(m_table[nMIN_VALUE]);
        }

        void finalize()
        {
            if (!is_updated())
            {
                update();
            }
            super_type::finalize();
        }

        void GetMemoryUsage(ICrySizer* pSizer, bool bSelf = false) const
        {
            if (bSelf && !pSizer->AddObjectSize(this))
            {
                return;
            }
            super_type::GetMemoryUsage(pSizer);
            if (m_table)
            {
                pSizer->AddObject(m_table, nSTORE_SIZE * sizeof(S));
            }
        }

    protected:

        void update()
        {
            value_type minVal(1.0f);
            if (!m_table)
            {
                m_table = new S[nSTORE_SIZE];
            }
            if (!m_pSource || m_pSource->empty())
            {
                for (int i = 0; i < nMAX_ENTRIES; i++)
                {
                    m_table[i] = value_type(1.f);
                }
            }
            else
            {
                m_pSource->update();
                for (int i = 0; i < nMAX_ENTRIES; i++)
                {
                    value_type val;
                    float t = float(i) * (1.0f / (float)(nMAX_ENTRIES - 1));
                    m_pSource->interpolate(t, val);
                    minVal = min(minVal, val);
                    m_table[i] = val;
                }
            }
            m_table[nMIN_VALUE] = minVal;
        }

        void init()
        {
            m_table = NULL;
        }

        bool is_updated() const
        {
            return super_type::is_updated() && m_table;
        }

        S*          m_table;
    };

#endif // LookupTableSpline

    //////////////////////////////////////////////////////////////////////////
    // OptSpline
    // Minimises memory for key-based storage. Uses 8-bit compressed key values.
    //////////////////////////////////////////////////////////////////////////

    /*
        Choose basis vars t, u = 1-t, ttu, uut.
        This produces exact values at t = 0 and 1, even with compressed coefficients.
        For end points and slopes v0, v1, s0, s1,
        solve for coefficients a, b, c, d:

            v(t) = a u + b t + c uut + d utt
            s(t) = v'(t) = -a + b + c (1-4t+3t^2) + d (2t-3t^2)

            v(0) = a
            v(1) = b
            s(0) = -a + b + c
            s(1) = -a + b - d

        So

            a = v0
            b = v1
            c = s0 + v0 - v1
            d = -s1 - v0 + v1

            s0 = c + v1 - v0
            s1 = -d + v1 - v0

        For compression, all values of v and t are limited to [0..1].
        Find the max possible slope values, such that values never exceed this range.

        If v0 = v1 = 0, then max slopes would have

            c = d
            v(1/2) = 1
            c/8 + d/8 = 1
            c = 4

        If v0 = 0 and v1 = 1, then max slopes would have

            c = d
            v(1/3) = 1
            1/3 + c 4/9 + d 2/9 = 1
            c = 1
    */

    template<class T>
    class OptSpline
    {
        typedef OptSpline<T> self_type;

    public:

        typedef T value_type;
        typedef SplineKey<T> key_type;
        typedef TSplineSlopes<T, key_type, true> source_spline;

    protected:

        static const int DIM = sizeof(value_type) / sizeof(float);

        template<class S>
        struct array
        {
            S   elems[DIM];

            ILINE array()
            {
                for (int i = 0; i < DIM; i++)
                {
                    elems[i] = 0;
                }
            }
            ILINE array(value_type const& val)
            {
                const float* aVal = reinterpret_cast<const float*>(&val);
                for (int i = 0; i < DIM; i++)
                {
                    elems[i] = aVal[i];
                }
            }
            ILINE array& operator=(value_type const& val)
            {
                new(this)array<S>(val);
                return *this;
            }

            ILINE operator value_type() const
            {
                PREFAST_SUPPRESS_WARNING(6001)
                value_type val;
                float* aVal = reinterpret_cast<float*>(&val);
                for (int i = 0; i < DIM; i++)
                {
                    aVal[i] = elems[i];
                }
                return val;
            }

            ILINE bool operator !() const
            {
                for (int i = 0; i < DIM; i++)
                {
                    if (elems[i])
                    {
                        return false;
                    }
                }
                return true;
            }
            ILINE bool operator ==(array<S> const& o) const
            {
                for (int i = 0; i < DIM; i++)
                {
                    if (!(elems[i] == o.elems[i]))
                    {
                        return false;
                    }
                }
                return true;
            }
            ILINE S& operator [](int i)
            {
                assert(i >= 0 && i < DIM);
                return elems[i];
            }
            ILINE const S& operator [](int i) const
            {
                assert(i >= 0 && i < DIM);
                return elems[i];
            }
        };

        typedef UnitFloat8 TStore;
        typedef array< UnitFloat8 > VStore;
        typedef array< float> FStore;
        typedef array< TFixed<int8, 2, 127, true> > SStore;

        //
        // Element storage
        //
        struct Point
        {
            TStore  st;             // Time of this point.
            VStore  sv;             // Value at this point.
            FStore  dd;             // Out tangent 
            FStore  ds;             // In tangent
            int flags;              // key type flags 

            void set_key(float t, value_type v)
            {
                st = t;
                sv = v;
            }
            void set_flags(int f)
            {
                flags = f;
            }

            void set_tangent(value_type _dd, value_type _ds)
            {
                dd = _dd;
                ds = _ds;
            }

        };

        struct Elem
            : Point
        {
            using Point::st;    // Total BS required for idiotic gcc.
            using Point::sv;

            SStore  sc, sd;     // Coefficients for uut and utt.

            // Compute coeffs based on 2 endpoints & slopes.
            void set_slopes(value_type s0, value_type s1)
            {
                value_type dv = value_type((this)[1].sv) - value_type(sv);
                sc = s0 - dv;
                sd = dv - s1;
            }

            ILINE void eval(value_type& val, float t) const
            {
                float u = 1.f - t,
                      tu = t * u;

                float* aF = reinterpret_cast<float*>(&val);
                for (int i = 0; i < DIM; i++)
                {
                    float elem = float(sv[i]) * u + float(this[1].sv[i]) * t;
                    elem += (float(sc[i]) * u + float(sd[i]) * t) * tu;
                    elem = elem < 0.f ? 0.f : elem;
                    elem = elem > 1.f ? 1.f : elem;
                    aF[i] = elem;
                }
            }

            // Use the derivative of eval formular to caculate the tangent of t
            ILINE void dev_eval(value_type& val, float t, value_type value) const
            {
                float u = 1.f - t,
                    tu = t * u;

                float* aF = reinterpret_cast<float*>(&val);
                float* currentValue = reinterpret_cast<float*>(&value);
                for (int i = 0; i < DIM; i++)
                {
                    float elem = -float(sv[i]) + float(currentValue[i]);
                    elem += float(sc[i]) * (1 - 4 * t + 3 * t * t)
                        + float(sd[i]) * (2 * t - 3 * t * t);
                    aF[i] = elem;
                }
            }

            value_type value() const
            {
                return sv;
            }

            // Slopes
            // v(t) = v0 u + v1 t + (c u + d t) t u
            // v\t(t) = v1 - v0 + (d - c) t u + (d t + c u) (u-t)
            // v\t(0) = v1 - v0 + c
            // v\t(1) = v1 - v0 - d
            value_type start_slope() const
            {
                return value_type((this)[1].sv) - value_type(sv) + value_type(sc);
            }
            value_type end_slope() const
            {
                return value_type((this)[1].sv) - value_type(sv) - value_type(sd);
            }
        };

        struct Spline
        {
            uint8       nKeys;                      // Total number of keys.
            Elem        aElems[1];              // Points and slopes. Last element is just Point without slopes.

            Spline()
                : nKeys(0)
            {
                // Empty spline sets dummy values to max, for consistency.
                aElems[0].st = TStore(1);
                aElems[0].sv = value_type(1);
                aElems[0].flags = 0;
                aElems[0].dd = FStore(0);
                aElems[0].ds = FStore(0);
            }

            Spline(int keys)
                : nKeys(aznumeric_caster(keys))
            {
                #ifdef _DEBUG
                if (nKeys)
                {
                    ((char*)this)[alloc_size()] = 77;
                }
                #endif
            }

            static size_t alloc_size(int keys)
            {
                assert(keys > 0);
                return sizeof(Spline) + max(keys - 1, 0) * sizeof(Elem);
            }
            size_t alloc_size() const
            {
                return alloc_size(nKeys);
            }

            key_type key(int n) const
            {
                key_type key;
                if (n < nKeys)
                {
                    key.time = aElems[n].st;
                    key.value = aElems[n].sv;

                    // Infer slope flags from slopes.
                    if (n >= 0) // bezier curve inTangent and outtangent will be asigned by user
                    {
                        key.flags = aElems[n].flags;
                        key.dd = aElems[n].dd;
                        key.ds = aElems[n].ds;
                    }
                }
                return key;
            }

            void interpolate(float t, value_type& val) const
            {
                float prev_t = aElems[0].st;
                if (t <= prev_t)
                {
                    val = aElems[0].sv;
                }
                else
                {
                    // Find spline segment.
                    const Elem* pEnd = aElems + nKeys - 1;
                    const Elem* pElem = aElems;
                    for (; pElem < pEnd; ++pElem)
                    {
                        float cur_t = pElem[1].st;
                        if (t <= cur_t)
                        {
                            // Eval
                            pElem->eval(val, (t - prev_t) / (cur_t - prev_t));
                            return;
                        }
                        prev_t = cur_t;
                    }

                    // Last point value.
                    val = pElem->sv;
                        }
                    }


            void EvalInTangent(float t, value_type& val) const
            {
                // Compute coeffs dynamically.

                float prev_t = aElems[0].st;
                value_type prev_v = aElems[0].sv;
                if (t <= prev_t)
                {
                    val = 0;
                    }
                else
                {
                    // Find spline segment.
                    const Elem* pEnd = aElems + nKeys - 1;
                    const Elem* pElem = aElems;
                    for (; pElem < pEnd; ++pElem)
                    {
                        float cur_t = pElem[1].st;
                        value_type cur_v = pElem[1].sv;
                        value_type Tvalue = value_type(0);
                        interpolate(t, Tvalue);

                        if (t <= cur_t)
                    {

                            Elem newElement;
                            newElement.set_key(prev_t, prev_v);
                            value_type dd = Tvalue - prev_v;

                            value_type ds = aElems[0].ds;

                            newElement.set_tangent(dd, ds);
                            
                            
                            value_type tds = Tvalue - prev_v;
                            if (pElem[0].dd == FStore(0))
                        {
                                tds = 2 * tds;
                            }
                            
                            newElement.sc = dd - dd;
                            newElement.sd = dd - value_type(tds);

                            newElement.dev_eval(val, 1, Tvalue);
                            return;
                        }
                        prev_t = cur_t;
                        prev_v = cur_v;
                    }

                    // Last point value.
                    val = 0.0f;
                }

            }


            void EvalOutTangent(float t, value_type& val) const
            {
                // Compute coeffs dynamically.

                float prev_t = aElems[0].st;
                value_type prev_v = aElems[0].sv;
                if (t <= prev_t)
                {
                    val = 0;
                }
                else
                {
                    // Find spline segment.
                    const Elem* pEnd = aElems + nKeys - 1;
                    const Elem* pElem = aElems;
                    for (; pElem < pEnd; ++pElem)
                    {
                        float cur_t = pElem[1].st;
                        value_type cur_v = pElem[1].sv;
                        value_type Tvalue = value_type(0);
                        interpolate(t, Tvalue);

                        if (t <= cur_t)
                        {
                            // Create a temporary Elem to hold the info of the inserted key.
                            // Since we will insert a key between pre_key and curr_key, the 
                            // slop and in/out tangent need to be recaculated.
                            Elem newElement;
                            // Set value and time for new key
                            newElement.set_key(t, Tvalue); 
                            
                            //Caculate out tangent of new key.
                            value_type dd = cur_v - Tvalue;
                            if (pElem[1].ds == FStore(0))
                            {
                                dd = 2 * dd;
                            }
                            //Caculate in tangent of new key
                            value_type ds = Tvalue - prev_v;
                            if (pElem[0].dd == FStore(0))
                            {
                                ds = 2 * ds;
                            }
                            newElement.set_tangent(dd,ds);

                            value_type dv = cur_v - Tvalue;
                            newElement.sc = dd - dv;
                            newElement.sd = dv - value_type(pElem[1].ds);

                            newElement.dev_eval(val, 0, cur_v);
                            return;
                        }
                        prev_t = cur_t;
                        prev_v = cur_v;
                    }

                    // Last point value.
                    val = 0.0f;
                }
            }

            void min_value(value_type& val) const
            {
                VStore sval = aElems[0].sv;
                for (int n = 1; n < nKeys; n++)
                {
                    for (int i = 0; i < DIM; i++)
                    {
                        sval[i] = min(sval[i], aElems[n].sv[i]);
                    }
                }
                val = sval;
            }

            void max_value(value_type& val) const
            {
                VStore sval = aElems[0].sv;
                for (int n = 1; n < nKeys; n++)
                {
                    for (int i = 0; i < DIM; i++)
                    {
                        sval[i] = max(sval[i], aElems[n].sv[i]);
                    }
                }
                val = sval;
            }

            value_type default_slope(int n) const
            {
                return n > 0 && n < nKeys - 1 ?
                       minmag(aElems[n].value() - aElems[n - 1].value(), aElems[n + 1].value() - aElems[n].value())
                       : value_type(0.f);
            }

            void validate() const
            {
                #ifdef _DEBUG
                if (nKeys)
                {
                    assert(((char const*)this)[alloc_size()] == 77);
                }
                #endif
            }
        };

        Spline*     m_pSpline;

        void alloc(int nKeys)
        {
            if (nKeys)
            {
                size_t nAlloc = Spline::alloc_size(nKeys)
                    #ifdef _DEBUG
                    + 1
                    #endif
                ;

                //set the memory to 0 since the comparison between two splines are comparing memory directly. 
                //And set functions won't set the memory because of alignment.
                m_pSpline = new(CryModuleCalloc(1, nAlloc))Spline(nKeys);
            }
            else
            {
                m_pSpline = NULL;
            }
        }

        void dealloc()
        {
            CryModuleFree(m_pSpline);
            m_pSpline = nullptr;
        }

    public:

        ~OptSpline()
        {
            dealloc();
        }

        OptSpline()
        {
            m_pSpline = NULL;
        }

        OptSpline(const self_type& in)
        {
            if (!in.empty() && in.num_keys() != 0)
            {
                alloc(in.num_keys());
                memcpy(m_pSpline, in.m_pSpline, in.m_pSpline->alloc_size());
                m_pSpline->validate();
            }
            else
            {
                m_pSpline = NULL;
            }
        }

        self_type& operator=(const self_type& in)
        {
            dealloc();
            new(this)self_type(in);
            return *this;
        }

        bool operator == (const self_type& o) const
        {
            if (empty() && o.empty())
            {
                return true;
            }
            if (num_keys() != o.num_keys())
            {
                return false;
            }
            return !memcmp(m_pSpline, o.m_pSpline, m_pSpline->alloc_size());
        }

        //
        // Adaptors for CBaseSplineInterpolator
        //
        bool empty() const
        {
            return !m_pSpline;
        }
        void clear()
        {
            dealloc();
            m_pSpline = NULL;
        }
        ILINE int num_keys() const
        {
            return m_pSpline ? m_pSpline->nKeys : 0;
        }

        ILINE key_type key(int n) const
        {
            assert(n < num_keys());
            return m_pSpline->key(n);
        }

        ILINE void interpolate(float t, value_type& val) const
        {
            if (!empty())
            {
                m_pSpline->interpolate(t, val);
            }
            else
            {
                val = value_type(1.f);
            }
        }


        ILINE void evalInTangent(float t, value_type& val) const
        {
            if (!empty())
            {
                m_pSpline->EvalInTangent(t, val);
            }
            else
            {
                val = value_type(0.f);
            }
        }

        ILINE void evalOutTangent(float t, value_type& val) const
        {
            if (!empty())
            {
                m_pSpline->EvalOutTangent(t, val);
            }
            else
            {
                val = value_type(0.f);
            }
        }

        void GetMemoryUsage(ICrySizer* pSizer) const
        {
            if (!empty())
            {
                pSizer->AddObject(m_pSpline, m_pSpline->alloc_size());
            }
        }

        //
        // Additional methods.
        //
        void min_value(value_type& val) const
        {
            if (!empty())
            {
                m_pSpline->min_value(val);
            }
            else
            {
                val = value_type(1.f);
            }
        }
        void max_value(value_type& val) const
        {
            if (!empty())
            {
                m_pSpline->max_value(val);
            }
            else
            {
                val = value_type(1.f);
            }
        }

        void from_source(source_spline& source)
        {
            dealloc();
            source.update();
            int nKeys = source.num_keys();

            // Check for trivial spline.
            bool is_default = true;
            for (int i = 0; i < nKeys; i++)
            {
                if (source.value(i) != value_type(1))
                {
                    is_default = false;
                    break;
                }
            }
            if (is_default)
            {
                nKeys = 0;
            }

            alloc(nKeys);
            if (nKeys)
            {
                // First set key values, then compute slope coefficients.
                for (int i = 0; i < nKeys; i++)
                {
                    m_pSpline->aElems[i].set_key(source.time(i), source.value(i));
                    m_pSpline->aElems[i].set_flags(source.flags(i));
                    m_pSpline->aElems[i].set_tangent(source.dd(i), source.ds(i));

                }
                for (int i = 0; i < nKeys - 1; i++)
                {
                    m_pSpline->aElems[i].set_slopes(source.dd(i), source.ds(i + 1));
                }

#if VERIFY_SPLINE_CONVERSION
                // Verify accurate conversion to OptSpline.
                for (int i = 0; i < nKeys; i++)
                {
                    key_type ks = source.key(i);
                    key_type kf = key(i);

                    assert(TStore(ks.time) == TStore(kf.time));
                    assert(VStore(ks.value) == VStore(kf.value));

                    static const float fSlopeEquivalence = 1.f / 60.f;
                    assert(IsEquivalent(ks.ds, kf.ds, fSlopeEquivalence));
                    assert(IsEquivalent(ks.dd, kf.dd, fSlopeEquivalence));

                    // Verify accurate reconstruction of slope flags.
                    ks.flags &= (SPLINE_KEY_TANGENT_IN_MASK | SPLINE_KEY_TANGENT_OUT_MASK);

                    value_type default_slope = i > 0 && i < nKeys - 1 ?
                        minmag(ks.value - source.key(i - 1).value, source.key(i + 1).value - ks.value)
                        : value_type(0.f);
                    if (i == 0 || IsEquivalent(ks.ds, default_slope, fSlopeEquivalence))
                    {
                        ks.flags &= ~SPLINE_KEY_TANGENT_IN_MASK;
                    }
                    if (i == nKeys - 1 || IsEquivalent(ks.dd, default_slope, fSlopeEquivalence))
                    {
                        ks.flags &= ~SPLINE_KEY_TANGENT_OUT_MASK;
                    }
                    assert(ks.flags == kf.flags);
                }
#endif //VERIFY_SPLINE_CONVERSION
            }
        }

        void to_source(source_spline& source) const
        {
            int nKeys = num_keys();
            source.resize(nKeys);
            for (int i = 0; i < nKeys; i++)
            {
                source.key(i) = key(i);
            }
            source.update();
        }
    };
};

#endif // CRYINCLUDE_CRYCOMMON_FINALIZINGSPLINE_H
