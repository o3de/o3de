/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <IMovieSystem.h>
#include <AzCore/Serialization/SerializeContext.h>

AZ_TYPE_INFO_WITH_NAME_IMPL(CAnimParamType, "CAnimParamType", "{E2F34955-3B07-4241-8D34-EA3BEF3B33D2}");

CAnimParamType::CAnimParamType()
    : m_type(kAnimParamTypeInvalid) {}

CAnimParamType::CAnimParamType(const AZStd::string& name)
{
    *this = name;
}

CAnimParamType::CAnimParamType(AnimParamType type)
{
    *this = type;
}

// Convert from old enum or int
void CAnimParamType::operator =(AnimParamType type)
{
    m_type = type;
}

void CAnimParamType::operator =(const AZStd::string& name)
{
    m_type = kAnimParamTypeByString;
    m_name = name;
}

bool CAnimParamType::operator ==(const CAnimParamType& animParamType) const
{
    if (m_type == kAnimParamTypeByString && animParamType.m_type == kAnimParamTypeByString)
    {
        return m_name == animParamType.m_name;
    }

    return m_type == animParamType.m_type;
}

bool CAnimParamType::operator !=(const CAnimParamType& animParamType) const
{
    return !(*this == animParamType);
}

bool CAnimParamType::operator <(const CAnimParamType& animParamType) const
{
    if (m_type == kAnimParamTypeByString && animParamType.m_type == kAnimParamTypeByString)
    {
        return m_name < animParamType.m_name;
    }
    else if (m_type == kAnimParamTypeByString)
    {
        return false; // Always sort named params last
    }
    else if (animParamType.m_type == kAnimParamTypeByString)
    {
        return true; // Always sort named params last
    }
    if (m_type < animParamType.m_type)
    {
        return true;
    }

    return false;
}

void CAnimParamType::SaveToXml(XmlNodeRef& xmlNode) const
{
    IMovieSystem* movieSystem = AZ::Interface<IMovieSystem>::Get();
    if (movieSystem)
    {
        movieSystem->SaveParamTypeToXml(*this, xmlNode);
    }
}

void CAnimParamType::LoadFromXml(const XmlNodeRef& xmlNode, const uint version)
{
    IMovieSystem* movieSystem = AZ::Interface<IMovieSystem>::Get();
    if (movieSystem)
    {
        movieSystem->LoadParamTypeFromXml(*this, xmlNode, version);
    }
}

void CAnimParamType::Serialize(XmlNodeRef& xmlNode, bool bLoading, const uint version)
{
    IMovieSystem* movieSystem = AZ::Interface<IMovieSystem>::Get();
    if (movieSystem)
    {
        movieSystem->SerializeParamType(*this, xmlNode, bLoading, version);
    }
}

// SAnimContext members
SAnimContext::SAnimContext()
{
    singleFrame = false;
    forcePlay = false;
    resetting = false;
    time = 0;
    dt = 0;
    fps = 0;
    sequence = nullptr;
    trackMask = 0;
    startTime = 0;
}


// SCameraParams member functions

SCameraParams::SCameraParams()
{
    fov = 0.0f;
    nearZ = DEFAULT_NEAR;
    justActivated = false;
}


// IAnimTrack member definitions
AZ_TYPE_INFO_WITH_NAME_IMPL(IAnimTrack, "IAnimTrack", "{AA0D5170-FB28-426F-BA13-7EFF6BB3AC67}");
AZ_RTTI_NO_TYPE_INFO_IMPL(IAnimTrack);
AZ_CLASS_ALLOCATOR_IMPL(IAnimTrack, AZ::SystemAllocator);

void IAnimTrack::Reflect(AZ::ReflectContext* context)
{
    if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
    {
        serializeContext->Class<IAnimTrack>();
    }
}


int IAnimTrack::NextKeyByTime(int key) const
{
    if (key + 1 < GetNumKeys())
    {
        return key + 1;
    }
    else
    {
        return -1;
    }
}

// IAnimNode member definitions
AZ_TYPE_INFO_WITH_NAME_IMPL(IAnimNode, "IAnimNode", "{0A096354-7F26-4B18-B8C0-8F10A3E0440A}");
AZ_RTTI_NO_TYPE_INFO_IMPL(IAnimNode);
AZ_CLASS_ALLOCATOR_IMPL(IAnimNode, AZ::SystemAllocator);

void IAnimNode::Reflect(AZ::ReflectContext* context)
{
    if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
    {
        serializeContext->Class<IAnimNode>();
    }
}

IAnimNode::SParamInfo::SParamInfo()
    : name("")
    , valueType(kAnimValueDefault)
    , flags(ESupportedParamFlags(0)) {};
IAnimNode::SParamInfo::SParamInfo(const char* _name, CAnimParamType _paramType, AnimValueType _valueType, ESupportedParamFlags _flags)
    : name(_name)
    , paramType(_paramType)
    , valueType(_valueType)
    , flags(_flags) {};

//! Compute and return the offset which brings the current position to the given position
Vec3 IAnimNode::GetOffsetPosition(const Vec3& position) { return position - GetPos(); }

// IAnimStringTable RTTI member definitions
AZ_TYPE_INFO_WITH_NAME_IMPL(IAnimStringTable, "IAnimStringTable", "{35690309-9D22-41FF-80B7-8AF7C8419945}");
AZ_RTTI_NO_TYPE_INFO_IMPL(IAnimStringTable);

// IAnimSequence RTTI member definitions
AZ_TYPE_INFO_WITH_NAME_IMPL(IAnimSequence, "IAnimSequence",  "{A60F95F5-5A4A-47DB-B3BB-525BBC0BC8DB}");
AZ_RTTI_NO_TYPE_INFO_IMPL(IAnimSequence);
AZ_CLASS_ALLOCATOR_IMPL(IAnimSequence, AZ::SystemAllocator);

void IAnimSequence::Reflect(AZ::ReflectContext* context)
{
    if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
    {
        serializeContext->Class<IAnimSequence>();
    }
}

// IMovieSystem RTTI definitions
AZ_TYPE_INFO_WITH_NAME_IMPL(IMovieSystem, "IMovieSystem", "{D8E6D6E9-830D-40DC-87F3-E9A069FBEB69}");
AZ_RTTI_NO_TYPE_INFO_IMPL(IMovieSystem);
