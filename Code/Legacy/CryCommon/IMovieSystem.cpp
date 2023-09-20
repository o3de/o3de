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
    if (gEnv->pMovieSystem)
    {
        gEnv->pMovieSystem->SaveParamTypeToXml(*this, xmlNode);
    }
}

void CAnimParamType::LoadFromXml(const XmlNodeRef& xmlNode, const uint version)
{
    if (gEnv->pMovieSystem)
    {
        gEnv->pMovieSystem->LoadParamTypeFromXml(*this, xmlNode, version);
    }
}

void CAnimParamType::Serialize(XmlNodeRef& xmlNode, bool bLoading, const uint version)
{
    if (gEnv->pMovieSystem)
    {
        gEnv->pMovieSystem->SerializeParamType(*this, xmlNode, bLoading, version);
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

// support for AZ:: vector types - re-route to legacy types
void IAnimTrack::GetValue(float time, AZ::Vector3& value, bool applyMultiplier)
{
    Vec3 vec3;
    GetValue(time, vec3, applyMultiplier);
    value.Set(vec3.x, vec3.y, vec3.z);
}
void IAnimTrack::GetValue(float time, AZ::Vector4& value, bool applyMultiplier)
{
    Vec4 vec4;
    GetValue(time, vec4, applyMultiplier);
    value.Set(vec4.x, vec4.y, vec4.z, vec4.w);
}
void IAnimTrack::GetValue(float time, AZ::Quaternion& value)
{
    Quat quat;
    GetValue(time, quat);
    value.Set(quat.v.x, quat.v.y, quat.v.z, quat.w);
}

// support for AZ:: vector types - re-route to legacy types
void IAnimTrack::SetValue(float time, AZ::Vector4& value, bool bDefault, bool applyMultiplier)
{
    Vec4 vec4(value.GetX(), value.GetY(), value.GetZ(), value.GetW());
    SetValue(time, vec4, bDefault, applyMultiplier);
}
void IAnimTrack::SetValue(float time, AZ::Vector3& value, bool bDefault, bool applyMultiplier)
{
    Vec3 vec3(value.GetX(), value.GetY(), value.GetZ());
    SetValue(time, vec3, bDefault, applyMultiplier);
}
void IAnimTrack::SetValue(float time, AZ::Quaternion& value, bool bDefault)
{
    Quat quat(value.GetW(), value.GetX(), value.GetY(), value.GetZ());
    SetValue(time, quat, bDefault);
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

/**
 * O3DE_DEPRECATION_NOTICE(GHI-9326)
 * use equivalent SetPos that accepts AZ::Vector3
 **/
void IAnimNode::SetPos(float time, const AZ::Vector3& pos)
{
    Vec3 vec3(pos.GetX(), pos.GetY(), pos.GetZ());
    SetPos(time, vec3);
}
/**
 * O3DE_DEPRECATION_NOTICE(GHI-9326)
 * use equivalent SetRotate that accepts AZ::Quaternion
 **/
void IAnimNode::SetRotate(float time, const AZ::Quaternion& rot)
{
    Quat quat(rot.GetX(), rot.GetY(), rot.GetZ(), rot.GetW());
    SetRotate(time, quat);
}
/**
 * O3DE_DEPRECATION_NOTICE(GHI-9326)
 * use equivalent SetScale that accepts AZ::Vector3
 **/
void IAnimNode::SetScale(float time, const AZ::Vector3& scale)
{
    Vec3 vec3(scale.GetX(), scale.GetY(), scale.GetZ());
    SetScale(time, vec3);
}

//! Compute and return the offset which brings the current position to the given position
Vec3 IAnimNode::GetOffsetPosition(const Vec3& position) { return position - GetPos(); }

/**
 * O3DE_DEPRECATION_NOTICE(GHI-9326)
 * use equivalent SetParamValue that accepts AZ::Vector3
 **/
bool IAnimNode::SetParamValue(float time, CAnimParamType param, const AZ::Vector3& value)
{
    Vec3 vec3(value.GetX(), value.GetY(), value.GetZ());
    return SetParamValue(time, param, vec3);
}

/**
 * O3DE_DEPRECATION_NOTICE(GHI-9326)
 * use equivalent SetParamValue that accepts AZ::Vector4
 **/
bool IAnimNode::SetParamValue(float time, CAnimParamType param, const AZ::Vector4& value)
{
    Vec4 vec4(value.GetX(), value.GetY(), value.GetZ(), value.GetW());
    return SetParamValue(time, param, vec4);
}

/**
 * O3DE_DEPRECATION_NOTICE(GHI-9326)
 * use equivalent GetParamValue that accepts AZ::Vector4
 **/
bool IAnimNode::GetParamValue(float time, CAnimParamType param, AZ::Vector3& value)
{
    Vec3 vec3;
    const bool result = GetParamValue(time, param, vec3);
    value.Set(vec3.x, vec3.y, vec3.z);
    return result;
}

/**
 * O3DE_DEPRECATION_NOTICE(GHI-9326)
 * use equivalent GetParamValue that accepts AZ::Vector4
 **/
bool IAnimNode::GetParamValue(float time, CAnimParamType param, AZ::Vector4& value)
{
    Vec4 vec4;
    const bool result = GetParamValue(time, param, vec4);
    value.Set(vec4.x, vec4.y, vec4.z, vec4.w);
    return result;
}

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
