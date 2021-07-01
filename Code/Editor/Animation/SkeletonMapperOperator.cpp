/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "SkeletonMapperOperator.h"

using namespace Skeleton;

/*

  CMapperOperatorDesc

*/

std::vector<CMapperOperatorDesc*> CMapperOperatorDesc::s_descs;

//

CMapperOperatorDesc::CMapperOperatorDesc(const char* name)
{
    s_descs.push_back(this);
}

/*

  CMapperOperator

*/

CMapperOperator::CMapperOperator(const char* className, uint32 positionCount, uint32 orientationCount)
{
    m_className = className;
    m_position.resize(positionCount, NULL);
    m_orientation.resize(orientationCount, NULL);
}

CMapperOperator::~CMapperOperator()
{
}

//

bool CMapperOperator::IsOfClass(const char* className)
{
    if (::_stricmp(m_className, className))
    {
        return false;
    }

    return true;
}

uint32 CMapperOperator::HasLinksOfClass(const char* className)
{
    uint32 count = 0;

    uint32 positionCount = m_position.size();
    for (uint32 i = 0; i < positionCount; ++i)
    {
        CMapperOperator* pOperator = m_position[i];
        if (!pOperator)
        {
            continue;
        }

        if (pOperator->IsOfClass(className))
        {
            ++count;
        }
    }

    uint32 orientationCount = m_orientation.size();
    for (uint32 i = 0; i < orientationCount; ++i)
    {
        CMapperOperator* pOperator = m_orientation[i];
        if (!pOperator)
        {
            continue;
        }

        if (pOperator->IsOfClass(className))
        {
            ++count;
        }
    }

    return count;
}

//

bool CMapperOperator::SerializeTo(XmlNodeRef& node)
{
    node->setAttr("class", m_className);

    uint32 parameterCount = uint32(m_parameters.size());
    for (uint32 i = 0; i < parameterCount; ++i)
    {
        m_parameters[i]->Serialize(node, false);
    }

    return true;
}

bool CMapperOperator::SerializeFrom(XmlNodeRef& node)
{
    uint32 parameterCount = uint32(m_parameters.size());
    for (uint32 i = 0; i < parameterCount; ++i)
    {
        m_parameters[i]->Serialize(node, true);
    }

    return true;
}

bool CMapperOperator::SerializeWithLinksTo(XmlNodeRef& node)
{
    if (!SerializeTo(node))
    {
        return false;
    }

    uint32 positionCount = uint32(m_position.size());
    for (uint32 i = 0; i < positionCount; ++i)
    {
        CMapperOperator* pOperator = m_position[i];
        if (!pOperator)
        {
            continue;
        }

        XmlNodeRef position = node->newChild("Position");
        position->setAttr("index", i);

        XmlNodeRef child = position->newChild("Operator");
        if (!pOperator->SerializeWithLinksTo(child))
        {
            return false;
        }
    }

    uint32 orientationCount = uint32(m_orientation.size());
    for (uint32 i = 0; i < orientationCount; ++i)
    {
        CMapperOperator* pOperator = m_orientation[i];
        if (!pOperator)
        {
            continue;
        }

        XmlNodeRef orientation = node->newChild("Orientation");
        orientation->setAttr("index", i);

        XmlNodeRef child = orientation->newChild("Operator");
        if (!pOperator->SerializeWithLinksTo(child))
        {
            return false;
        }
    }

    return true;
}

bool CMapperOperator::SerializeWithLinksFrom(XmlNodeRef& node)
{
    if (!SerializeFrom(node))
    {
        return false;
    }

    return true;
}
/*

  CMapperOperator_Transform

*/

class CMapperOperator_Transform
    : public CMapperOperator
{
public:
    CMapperOperator_Transform()
        : CMapperOperator("Transform", 1, 1)
    {
        m_pAngles = new CVariable<Vec3>();
        m_pAngles->SetName("rotation");
        m_pAngles->Set(Vec3(0.0f, 0.0f, 0.0f));
        m_pAngles->SetLimits(-180.0f, 180.0f);
        AddParameter(*m_pAngles);

        m_pVector = new CVariable<Vec3>();
        m_pVector->SetName("vector");
        m_pVector->Set(Vec3(0.0f, 0.0f, 0.0f));
        AddParameter(*m_pVector);

        m_pScale = new CVariable<Vec3>();
        m_pScale->SetName("scale");
        m_pScale->Set(Vec3(1.0f, 1.0f, 1.0f));
        AddParameter(*m_pScale);
    }

    // CMapperOperator
public:
    virtual QuatT CMapperOperator_Transform::Compute()
    {
        QuatT result(IDENTITY);
        m_pVector->Get(result.t);

        Vec3 scale;
        m_pScale->Get(scale);

        Vec3 angles;
        m_pAngles->Get(angles);

        result.q = Quat::CreateRotationXYZ(
                Ang3(DEG2RAD(angles.x), DEG2RAD(angles.y), DEG2RAD(angles.z)));

        if (CMapperOperator* pOperator = GetPosition(0))
        {
            result.t = pOperator->Compute().t.CompMul(scale) + result.t;
        }
        if (CMapperOperator* pOperator = GetOrientation(0))
        {
            result.q = pOperator->Compute().q * result.q;
        }
        return result;
    }

private:
    CVariable<Vec3>* m_pVector;
    CVariable<Vec3>* m_pAngles;
    CVariable<Vec3>* m_pScale;
};

SkeletonMapperOperatorRegister(Transform, CMapperOperator_Transform)

class CMapperOperator_PositionsToOrientation
    : public CMapperOperator
{
public:
    CMapperOperator_PositionsToOrientation()
        : CMapperOperator("PositionsToOrientation", 3, 0)
    {
    }

    // CMapperOperator
public:
    virtual QuatT Compute()
    {
        CMapperOperator* pOperator0 = GetPosition(0);
        CMapperOperator* pOperator1 = GetPosition(1);
        CMapperOperator* pOperator2 = GetPosition(2);
        if (!pOperator0 || !pOperator1 || !pOperator2)
        {
            return QuatT(IDENTITY);
        }

        Vec3 p0 = pOperator0->Compute().t;
        Vec3 p1 = pOperator1->Compute().t;
        Vec3 p2 = pOperator2->Compute().t;

        Vec3 m = (p1 + p2) * 0.5f;
        Vec3 y = (m - p0).GetNormalized();
        Vec3 z = (p1 - p2).GetNormalized();
        Vec3 x = y % z;
        z = x % y;

        Matrix33 m33;
        m33.SetFromVectors(x, y, z);
        QuatT result(IDENTITY);
        result.q = Quat(m33);
        return result;
    }
};

SkeletonMapperOperatorRegister(PositionsToOrientation, CMapperOperator_PositionsToOrientation)
