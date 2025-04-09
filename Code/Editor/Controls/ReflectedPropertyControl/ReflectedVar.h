/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef CRYINCLUDE_EDITOR_UTILS_REFLECTEDVAR_H
#define CRYINCLUDE_EDITOR_UTILS_REFLECTEDVAR_H
#pragma once

#include <algorithm>
#include <limits>
#include "Util/VariablePropertyType.h"
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>


//Base class for generic reflected variables
class CReflectedVar
{
public:
    AZ_RTTI(CReflectedVar, "{9CF461B5-4093-4F7E-9A28-75531F0D046C}")

    CReflectedVar() = default;
    CReflectedVar(const AZStd::string& name)
        : m_varName(name){}
    virtual ~CReflectedVar(){}

    AZStd::string m_varName;
    AZStd::string m_description;
};


// Reflected container of reflected values.  Also holds ePropertyTable data
class CPropertyContainer
    : public CReflectedVar
{
public:
    AZ_RTTI(CPropertyContainer, "{99500790-241A-4274-BAD8-C4510E869FC6}", CReflectedVar)
    CPropertyContainer(const AZStd::string& name)
        : CReflectedVar(name) {}
    CPropertyContainer() = default;

    AZStd::string varName() const { return m_varName; }
    AZStd::string description() const { return m_description; }

    void AddProperty(CReflectedVar* property);

    void Clear();

    //If we're an unnamed container, just show our children in flat list.  Otherwise show the container name with children underneath
    AZ::u32 GetVisibility() const
    {
        return m_varName.empty() ? AZ_CRC_CE("PropertyVisibility_ShowChildrenOnly") : AZ_CRC_CE("PropertyVisibility_Show");
    }

    void SetAutoExpand(bool autoExpand) { m_autoExpand = autoExpand; }
    bool AutoExpand() const { return m_autoExpand; }

    AZStd::vector<CReflectedVar*> GetProperties() const { return m_properties; }

    void SetValueText(const AZStd::string& valueText) { m_valueText = valueText; }

    friend class ReflectedVarInit;

private:
    AZStd::vector<CReflectedVar*> m_properties;
    bool m_autoExpand = false;
    AZStd::string m_valueText;
};


template<class T>
class CReflectedVarAny
    : public CReflectedVar
{
public:
    AZ_RTTI((CReflectedVarAny, "{EE8293C3-9B1E-470B-9922-2CBB8DA13D78}", T), CReflectedVar);

    CReflectedVarAny(const AZStd::string& name, const T& val = T())
        : CReflectedVar(name)
        , m_value(val) {}
    CReflectedVarAny() = default;

    AZStd::string varName() const { return m_varName; }
    AZStd::string description() const { return m_description; }

    static void reflect(AZ::SerializeContext* serializeContext);

    T m_value;
};

// Class to hold values that have min/max
// T = data type held in this variable
// R = data type of the range
template<class T, class R>
class CReflectedVarRanged
    : public CReflectedVar
{
public:
    AZ_RTTI((CReflectedVarRanged, "{6AB4EC29-E17B-4B3B-A153-BFDAA48B8CF8}", T, R), CReflectedVar);

    CReflectedVarRanged(const AZStd::string& name, const T& val = T())
        : CReflectedVar(name)
        , m_value(val)
        , m_minVal(std::numeric_limits<R>::lowest())
        , m_maxVal(std::numeric_limits<R>::max())
        , m_stepSize(1)
        , m_softMinVal(std::numeric_limits<R>::lowest())
        , m_softMaxVal(std::numeric_limits<R>::max())
    {}
    CReflectedVarRanged()
        : CReflectedVarRanged(AZStd::string(), T()){}

    AZStd::string varName() const { return m_varName; }
    AZStd::string description() const { return m_description; }

    R minValue() const { return m_minVal; }
    R maxValue() const { return m_maxVal; }
    R stepSize() const { return m_stepSize; }
    R softMinVal() const { return m_softMinVal; }
    R softMaxVal() const { return m_softMaxVal; }

    static void reflect(AZ::SerializeContext* serializeContext);

    T m_value;
    R m_minVal;
    R m_maxVal;
    R m_stepSize;
    R m_softMinVal;
    R m_softMaxVal;
};

//name some commonly-used variable types
template <class T>
using  CReflectedVarNumeric = CReflectedVarRanged<T, T>;

//ePropertyFloat
using CReflectedVarFloat = CReflectedVarNumeric<float>;

//ePropertyInt
using CReflectedVarInt = CReflectedVarNumeric<int>;

//ePropertyString
using CReflectedVarString = CReflectedVarAny<AZStd::string>;

//ePropertyBool
using CReflectedVarBool = CReflectedVarAny<bool>;

//ePropertyVector2
using CReflectedVarVector2 = CReflectedVarRanged<AZ::Vector2, float>;

//ePropertyVector
using CReflectedVarVector3 = CReflectedVarRanged<AZ::Vector3, float>;

//ePropertyVector4
using CReflectedVarVector4 = CReflectedVarRanged<AZ::Vector4, float>;

// Class for holding enumerated values, ePropertySelection
// Keeps a key-value pair values (int, string, float, etc) and names corresponding to each value
// The names are displayed to user when editing, the values are used by underlying code.

template<class T>
class CReflectedVarEnum
    : public CReflectedVar
{
public:
    AZ_RTTI((CReflectedVarEnum, "{40AE7D74-7E3A-41A9-8F71-2BBC3067118B}", T), CReflectedVar);

    CReflectedVarEnum(const AZStd::string& name)
        : CReflectedVar(name) {}
    CReflectedVarEnum() = default;

    void setEnums(const AZStd::vector<AZStd::pair<T, AZStd::string> >& enums)
    {
        m_enums = enums;
        if (m_enums.size() > 0)
        {
            m_value = m_enums.at(0).first;
            m_selectedEnumName = m_enums.at(0).second;
        }
        else
        {
            m_value = T();
            m_selectedEnumName.clear();
        }
    }

    void addEnum(const T& value, const AZStd::string& name)
    {
        m_enums.push_back(AZStd::pair<T, AZStd::string>(value, name));
        if (m_enums.size() == 1)
        {
            m_selectedEnumName = name;
            m_value = value;
        }
    }

    void setEnumValue(const T& value)
    {
        auto it = std::find_if(m_enums.cbegin(), m_enums.cend(), [value](const AZStd::pair<T, AZStd::string>& item) -> bool { return item.first == value; });
        if (it != m_enums.end())
        {
            m_value = it->first;
            m_selectedEnumName = it->second;
        }
    }

    void setEnumByName(const AZStd::string& name)
    {
        auto it = std::find_if(m_enums.cbegin(), m_enums.cend(), [name](const AZStd::pair<T, AZStd::string>& item) -> bool { return item.second == name; });
        if (it != m_enums.end())
        {
            m_value = it->first;
            m_selectedEnumName = it->second;
        }
    }

    void OnEnumChanged()
    {
        setEnumByName(m_selectedEnumName);
    }

    AZStd::vector < AZStd::string> GetEnums() const
    {
        AZStd::vector < AZStd::string> returnVal;
        for (const auto& i : m_enums)
        {
            returnVal.push_back(i.second);
        }

        return returnVal;
    }


    AZStd::string varName() const { return m_varName; }
    AZStd::string description() const { return m_description; }

    static void reflect(AZ::SerializeContext* serializeContext);

    T m_value;
    AZStd::string m_selectedEnumName;
    AZStd::vector<AZStd::pair<T, AZStd::string> > m_enums;
};

//Class to hold ePropertyColor (IVariable::DT_COLOR)
class CReflectedVarColor
    : public CReflectedVar
{
public:
    AZ_RTTI(CReflectedVarColor, "{CC69E773-B4FA-4B6D-8A46-0B580097B6D2}", CReflectedVar)

    CReflectedVarColor(const AZStd::string& name, AZ::Vector3 color = AZ::Vector3())
        : CReflectedVar(name)
        , m_color(color) {}
    CReflectedVarColor() {}

    AZStd::string varName() const { return m_varName; }
    AZStd::string description() const { return m_description; }

    AZ::Vector3 m_color;
};

//Class to hold:
// ePropertyTexture          (IVariable::DT_TEXTURE)
// ePropertyAudioTrigger        (IVariable::DT_AUDIO_TRIGGER)
// ePropertyAudioSwitch         (IVariable::DT_AUDIO_SWITCH )
// ePropertyAudioSwitchState    (IVariable::DT_AUDIO_SWITCH_STATE)
// ePropertyAudioRTPC           (IVariable::DT_AUDIO_RTPC)
// ePropertyAudioEnvironment    (IVariable::DT_AUDIO_ENVIRONMENT)
// ePropertyAudioPreloadRequest (IVariable::DT_AUDIO_PRELOAD_REQUEST)

class CReflectedVarResource
    : public CReflectedVar
{
public:
    AZ_RTTI(CReflectedVarResource, "{162864C2-0C3E-4B6A-84D3-BBAD975B4FD2}", CReflectedVar)

    CReflectedVarResource(const AZStd::string& name)
        : CReflectedVar(name)
        , m_propertyType(ePropertyInvalid)
    {}
    CReflectedVarResource()
        : m_propertyType(ePropertyInvalid){}

    AZStd::string varName() const { return m_varName; }
    AZStd::string description() const { return m_description; }

    AZStd::string m_path;
    PropertyType m_propertyType;
};

//Class to hold ePropertyUser (IVariable::DT_USERITEMCB)
class CReflectedVarUser 
    : public CReflectedVar
{
public:
    AZ_RTTI(CReflectedVarUser, "{A901DA91-3893-4848-9AE8-62C0ED074970}", CReflectedVar)

        CReflectedVarUser(const AZStd::string &name)
        : CReflectedVar(name)
        , m_enableEdit(false)
        , m_useTree(false)
    {}
    CReflectedVarUser() : m_enableEdit(false), m_useTree(false) {}

    AZStd::string varName() const { return m_varName; }

    AZStd::string m_value;

    bool m_enableEdit;
    bool m_useTree;
    AZStd::string m_dialogTitle;
    AZStd::string m_treeSeparator;
    AZStd::vector<AZStd::string> m_itemNames;
    AZStd::vector<AZStd::string> m_itemDescriptions;
};

class CReflectedVarSpline
    : public CReflectedVar
{
public:
    AZ_RTTI(CReflectedVarSpline, "{9A928683-7C84-48BF-8A2E-F7BEC423EE4E}", CReflectedVar)

        CReflectedVarSpline(PropertyType propertyType, const AZStd::string &name)
        : CReflectedVar(name)
        , m_spline(0)
        , m_propertyType(propertyType)
    {}

    CReflectedVarSpline()
        : m_spline(0)
        , m_propertyType(ePropertyInvalid) 
    {}

    AZStd::string varName() const { return m_varName; }
    AZ::u32 handler();

    uint64_t m_spline;
    PropertyType m_propertyType;
};

//Class to wrap all the many properties that can be represented by a string and edited via a popup
class CReflectedVarGenericProperty
    : public CReflectedVar
{
public:
    AZ_RTTI(CReflectedVarGenericProperty, "{C4A34C95-3D71-40CE-86D2-DDE314B33CC5}", CReflectedVar)

    CReflectedVarGenericProperty(PropertyType pType, const AZStd::string& name = AZStd::string(), const AZStd::string& val = AZStd::string())
        : CReflectedVar(name)
        , m_propertyType(pType)
        , m_value(val) {}
    CReflectedVarGenericProperty()
        : CReflectedVar()
        , m_propertyType(ePropertyInvalid){}

    AZStd::string varName() const { return m_varName; }
    AZStd::string description() const { return m_description; }
    PropertyType propertyType() const { return m_propertyType; }

    AZ::u32 handler();

    static void reflect(AZ::SerializeContext* serializeContext);

    PropertyType m_propertyType;
    AZStd::string m_value;
};


class EDITOR_CORE_API ReflectedVarInit
{
public:
    static void setupReflection(AZ::SerializeContext* serializeContext);
private:
    static bool s_reflectionDone;
};

//Class to hold ePropertyMotion (IVariable::DT_MOTION )
class CReflectedVarMotion
    : public CReflectedVar
{
public:
    AZ_RTTI(CReflectedVarMotion, "{66397EFB-620A-40B8-8C66-D6AECF690DF5}", CReflectedVar)

    CReflectedVarMotion(const AZStd::string& name)
        : CReflectedVar(name) {}

    CReflectedVarMotion() = default;

    AZStd::string varName() const { return m_varName; }
    AZStd::string description() const { return m_description; }

    AZStd::string m_motion;
    AZ::Data::AssetId m_assetId;
};


#endif // CRYINCLUDE_EDITOR_UTILS_REFLECTEDVAR_H
