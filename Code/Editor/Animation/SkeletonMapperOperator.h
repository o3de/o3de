/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_ANIMATION_SKELETONMAPPEROPERATOR_H
#define CRYINCLUDE_EDITOR_ANIMATION_SKELETONMAPPEROPERATOR_H
#pragma once


#include "../Util/Variable.h"

#undef GetClassName

#define SkeletonMapperOperatorRegister(name, className)               \
    class CMapperOperatorDesc_##name                                  \
        : public CMapperOperatorDesc                                  \
    {                                                                 \
    public:                                                           \
        CMapperOperatorDesc_##name()                                  \
            : CMapperOperatorDesc(#name) { }                          \
    protected:                                                        \
        virtual const char* GetName() { return #name; }               \
        virtual CMapperOperator* Create() { return new className(); } \
    } mapperOperatorDesc__##name;

namespace Skeleton {
    class CMapperOperator;

    class CMapperOperatorDesc
    {
    public:
        static uint32 GetCount() { return uint32(s_descs.size()); }
        static const char* GetName(uint32 index) { return s_descs[index]->GetName(); }
        static CMapperOperator* Create(uint32 index) { return s_descs[index]->Create(); }

    private:
        static std::vector<CMapperOperatorDesc*> s_descs;

    public:
        CMapperOperatorDesc(const char* name);

    protected:
        virtual const char* GetName() = 0;
        virtual CMapperOperator* Create() = 0;
    };

    class CMapperOperator
        : public _reference_target_t
    {
    protected:
        CMapperOperator(const char* className, uint32 positionCount, uint32 orientationCount);
        ~CMapperOperator();

    public:
        const char* GetClassName() { return m_className; }

        uint32 GetPositionCount() const { return uint32(m_position.size()); }
        void SetPosition(uint32 index, CMapperOperator* pOperator) { m_position[index] = pOperator; }
        CMapperOperator* GetPosition(uint32 index) { return m_position[index]; }

        uint32 GetOrientationCount() const { return uint32(m_orientation.size()); }
        void SetOrientation(uint32 index, CMapperOperator* pOperator) { m_orientation[index] = pOperator; }
        CMapperOperator* GetOrientation(uint32 index) { return m_orientation[index]; }

        uint32 GetParameterCount() { return uint32(m_parameters.size()); }
        IVariable* GetParameter(uint32 index) { return m_parameters[index]; }

        bool IsOfClass(const char* className);
        uint32 HasLinksOfClass(const char* className);

        bool SerializeTo(XmlNodeRef& node);
        bool SerializeFrom(XmlNodeRef& node);

        bool SerializeWithLinksTo(XmlNodeRef& node);
        bool SerializeWithLinksFrom(XmlNodeRef& node);

    protected:
        void AddParameter(IVariable& variable) { m_parameters.push_back(&variable); }

    public:
        virtual QuatT Compute() = 0;

    private:
        const char* m_className;
        std::vector<_smart_ptr<CMapperOperator> > m_position;
        std::vector<_smart_ptr<CMapperOperator> > m_orientation;

        std::vector<IVariablePtr> m_parameters;
    };

    class CMapperLocation
        : public CMapperOperator
    {
    public:
        CMapperLocation()
            : CMapperOperator("Location", 0, 0)
        {
            m_pName = new CVariable<CString>();
            m_pName->SetName("name");
            m_pName->SetFlags(m_pName->GetFlags() | IVariable::UI_INVISIBLE);
            AddParameter(*m_pName);

            m_pAxis = new CVariable<Vec3>();
            m_pAxis->SetName("axis");
            m_pAxis->SetLimits(-3.0f, +3.0f);
            m_pAxis->Set(Vec3(1.0f, 2.0f, 3.0f));
            AddParameter(*m_pAxis);

            m_location = QuatT(IDENTITY);
        }

    public:
        void SetName(const char* name) { m_pName->Set(name); }
        CString GetName() const { CString s; m_pName->Get(s); return s; }

        void SetLocation(const QuatT& location) { m_location = location; }
        const QuatT& GetLocation() const { return m_location; }

        // CMapperOperator
    public:
        virtual QuatT Compute()
        {
            Vec3 axis;
            m_pAxis->Get(axis);

            uint32 x = fabs_tpl(axis.x);
            uint32 y = fabs_tpl(axis.y);
            uint32 z = fabs_tpl(axis.z);
            if (x < 1 || y < 1 || z < 1 ||
                x > 3 || y > 3 || y > 3 ||
                x == y || x == z || y == z)
            {
                return QuatT(IDENTITY);
            }

            Matrix33 matrix;
            matrix.SetFromVectors(
                m_location.q.GetColumn(x - 1) * f32(::sgn(axis.x)),
                m_location.q.GetColumn(y - 1) * f32(::sgn(axis.y)),
                m_location.q.GetColumn(z - 1) * f32(::sgn(axis.z)));
            if (!matrix.IsOrthonormalRH(0.01f))
            {
                return QuatT(IDENTITY);
            }

            QuatT result = m_location;
            result.q = Quat(matrix);
            return result;
        }

    private:
        CVariable<CString>* m_pName;
        CVariable<Vec3>* m_pAxis;

        QuatT m_location;
    };
} // namespace Skeleton

#endif // CRYINCLUDE_EDITOR_ANIMATION_SKELETONMAPPEROPERATOR_H
