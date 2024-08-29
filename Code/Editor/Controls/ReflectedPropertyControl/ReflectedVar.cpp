/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include "ReflectedVar.h"

// AzCore
#include <AzCore/Serialization/EditContext.h>


bool ReflectedVarInit::s_reflectionDone = false;

void ReflectedVarInit::setupReflection(AZ::SerializeContext* serializeContext)
{
    if (!serializeContext)
        return;

    if (s_reflectionDone)
        return;

    s_reflectionDone = true;

    serializeContext->Class< CReflectedVar>()
        ->Version(1)
        ->Field("description", &CReflectedVar::m_description)
        ->Field("varName", &CReflectedVar::m_varName);

    serializeContext->Class <CReflectedVarResource, CReflectedVar >()
        ->Version(1)
        ->Field("path", &CReflectedVarResource::m_path)
        ->Field("propertyType", &CReflectedVarResource::m_propertyType)
        ;

    serializeContext->Class< CReflectedVarColor, CReflectedVar>()
        ->Version(1)
        ->Field("color", &CReflectedVarColor::m_color);

    serializeContext->Class< CReflectedVarUser, CReflectedVar>()
        ->Version(1)
        ->Field("value", &CReflectedVarUser::m_value)
        ->Field("enableEdit", &CReflectedVarUser::m_enableEdit)
        ->Field("title", &CReflectedVarUser::m_dialogTitle)
        ->Field("useTree", &CReflectedVarUser::m_useTree)
        ->Field("treeSeparator", &CReflectedVarUser::m_treeSeparator)
        ->Field("itemNames", &CReflectedVarUser::m_itemNames)
        ->Field("itemDescriptions", &CReflectedVarUser::m_itemDescriptions);

    serializeContext->Class <CReflectedVarSpline, CReflectedVar >()
        ->Version(1)
        ->Field("spline", &CReflectedVarSpline::m_spline)
        ->Field("propertyType", &CReflectedVarSpline::m_propertyType)
        ;

    serializeContext->Class< CPropertyContainer, CReflectedVar>()
        ->Version(1)
        ->Field("properties", &CPropertyContainer::m_properties);

    serializeContext->Class <CReflectedVarMotion, CReflectedVar >()
        ->Version(1)
        ->Field("motion", &CReflectedVarMotion::m_motion)
        ->Field("assetId", &CReflectedVarMotion::m_assetId)
        ;

    AZ::EditContext* ec = serializeContext->GetEditContext();
    if (ec)
    {
        ec->Class< CReflectedVarUser >("VarUser", "")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::NameLabelOverride, &CReflectedVarUser::varName)
            ->Attribute(AZ::Edit::Attributes::Handler, AZ_CRC_CE("ePropertyUser"))
            ;

        ec->Class< CReflectedVarColor >("VarColor", "")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC_CE("PropertyVisibility_ShowChildrenOnly"))
            ->DataElement(AZ::Edit::UIHandlers::Color, &CReflectedVarColor::m_color, "Color", "")
            ->Attribute(AZ::Edit::Attributes::NameLabelOverride, &CReflectedVarColor::varName)
            ->Attribute(AZ::Edit::Attributes::DescriptionTextOverride, &CReflectedVarColor::description)
            ;

        ec->Class< CReflectedVarSpline >("VarSpline", "")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::NameLabelOverride, &CReflectedVarSpline::varName)
            ->Attribute(AZ::Edit::Attributes::Handler, &CReflectedVarSpline::handler)
            ;

        ec->Class< CPropertyContainer >("PropertyContainer", "")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC_CE("PropertyVisibility_ShowChildrenOnly"))
            ->DataElement(AZ::Edit::UIHandlers::Default, &CPropertyContainer::m_properties, "Properties", "")
            ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
            ->Attribute(AZ::Edit::Attributes::NameLabelOverride, &CPropertyContainer::varName)
            ->Attribute(AZ::Edit::Attributes::DescriptionTextOverride, &CPropertyContainer::description)
            ->Attribute(AZ::Edit::Attributes::Visibility, &CPropertyContainer::GetVisibility)
            ->Attribute(AZ::Edit::Attributes::AutoExpand, &CPropertyContainer::m_autoExpand)
            ->Attribute(AZ::Edit::Attributes::ValueText, &CPropertyContainer::m_valueText) //will be ignored if blank
            ;

        ec->Class< CReflectedVarMotion >("VarMotion", "Motion")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::NameLabelOverride, &CReflectedVarMotion::varName)
            ->Attribute(AZ::Edit::Attributes::DescriptionTextOverride, &CReflectedVarMotion::description)
            ;
    }
    CReflectedVarString::reflect(serializeContext);
    CReflectedVarBool::reflect(serializeContext);
    CReflectedVarFloat::reflect(serializeContext);
    CReflectedVarInt::reflect(serializeContext);
    CReflectedVarVector2::reflect(serializeContext);
    CReflectedVarVector3::reflect(serializeContext);
    CReflectedVarVector4::reflect(serializeContext);
    CReflectedVarAny<AZStd::vector<AZStd::string>>::reflect(serializeContext);
    CReflectedVarEnum<int>::reflect(serializeContext);
    CReflectedVarEnum<AZStd::string>::reflect(serializeContext);
    CReflectedVarGenericProperty::reflect(serializeContext);
}


template<class T>
void CReflectedVarAny<T>::reflect(AZ::SerializeContext* serializeContext)
{
    static bool reflected = false;
    if (reflected)
        return;
    reflected = true;

    serializeContext->Class< CReflectedVarAny<T>, CReflectedVar>()
        ->Version(1)
        ->Field("value", &CReflectedVarAny<T>::m_value);

    AZ::EditContext* ec = serializeContext->GetEditContext();
    if (ec)
    {
        ec->Class< CReflectedVarAny<T> >("VarAny", "")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC_CE("PropertyVisibility_ShowChildrenOnly"))
            ->DataElement(AZ::Edit::UIHandlers::Default, &CReflectedVarAny<T>::m_value, "Value", "")
            ->Attribute(AZ::Edit::Attributes::NameLabelOverride, &CReflectedVarAny<T>::varName)
            ->Attribute(AZ::Edit::Attributes::DescriptionTextOverride, &CReflectedVarAny<T>::description)
            ;
    }
}


template<class T, class R>
void CReflectedVarRanged<T, R>::reflect(AZ::SerializeContext* serializeContext)
{
    static bool reflected = false;
    if (reflected)
        return;
    reflected = true;

    serializeContext->Class< CReflectedVarRanged<T, R>, CReflectedVar>()
        ->Version(1)
        ->Field("value", &CReflectedVarRanged<T, R>::m_value)
        ->Field("min", &CReflectedVarRanged<T, R>::m_minVal)
        ->Field("max", &CReflectedVarRanged<T, R>::m_maxVal)
        ->Field("step", &CReflectedVarRanged<T, R>::m_stepSize)
        ->Field("softMin", &CReflectedVarRanged<T, R>::m_softMinVal)
        ->Field("softMax", &CReflectedVarRanged<T, R>::m_softMaxVal)
        ;

    AZ::EditContext* ec = serializeContext->GetEditContext();
    if (ec)
    {
        ec->Class< CReflectedVarRanged<T, R> >("VarAny", "")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC_CE("PropertyVisibility_ShowChildrenOnly"))
            ->DataElement(AZ::Edit::UIHandlers::Slider, &CReflectedVarRanged<T, R>::m_value, "Value", "")
            ->Attribute(AZ::Edit::Attributes::NameLabelOverride, &CReflectedVarRanged<T, R>::varName)
            ->Attribute(AZ::Edit::Attributes::DescriptionTextOverride, &CReflectedVarRanged<T, R>::description)
            ->Attribute(AZ::Edit::Attributes::Min, &CReflectedVarRanged<T, R>::minValue)
            ->Attribute(AZ::Edit::Attributes::Max, &CReflectedVarRanged<T, R>::maxValue)
            ->Attribute(AZ::Edit::Attributes::Step, &CReflectedVarRanged<T, R>::stepSize)
            ->Attribute(AZ::Edit::Attributes::SoftMin, &CReflectedVarRanged<T, R>::softMinVal)
            ->Attribute(AZ::Edit::Attributes::SoftMax, &CReflectedVarRanged<T, R>::softMaxVal)
            ;
    }

}


template<class T>
void CReflectedVarEnum<T>::reflect(AZ::SerializeContext* serializeContext)
{
    static bool reflected = false;
    if (reflected)
        return;
    reflected = true;

    serializeContext->Class< CReflectedVarEnum<T>, CReflectedVar>()
        ->Version(1)
        ->Field("value", &CReflectedVarEnum<T>::m_value)
        ->Field("selectedName", &CReflectedVarEnum<T>::m_selectedEnumName)
        ->Field("availableValues", &CReflectedVarEnum<T>::m_enums)
        ;

    AZ::EditContext* ec = serializeContext->GetEditContext();
    if (ec)
    {
        ec->Class< CReflectedVarEnum<T> >("Enum Variable", "")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC_CE("PropertyVisibility_ShowChildrenOnly"))
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &CReflectedVarEnum<T>::m_selectedEnumName, "Value", "")
            ->Attribute(AZ::Edit::Attributes::StringList, &CReflectedVarEnum<T>::GetEnums)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &CReflectedVarEnum<T>::OnEnumChanged)
            ->Attribute(AZ::Edit::Attributes::NameLabelOverride, &CReflectedVarEnum<T>::varName)
            ->Attribute(AZ::Edit::Attributes::DescriptionTextOverride, &CReflectedVarEnum<T>::description)
            ;
    }
}


void CReflectedVarGenericProperty::reflect(AZ::SerializeContext* serializeContext)
{
    static bool reflected = false;
    if (reflected)
        return;
    reflected = true;

    serializeContext->Class< CReflectedVarGenericProperty, CReflectedVar>()
        ->Version(1)
        ->Field("value", &CReflectedVarGenericProperty::m_value)
        ->Field("propertyType", &CReflectedVarGenericProperty::m_propertyType)
        ;

    AZ::EditContext* ec = serializeContext->GetEditContext();
    if (ec)
    {
        ec->Class< CReflectedVarGenericProperty >("GenericProperty", "GenericProperty")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::NameLabelOverride, &CReflectedVarGenericProperty::varName)
            ->Attribute(AZ::Edit::Attributes::DescriptionTextOverride, &CReflectedVarGenericProperty::description)
            ->Attribute(AZ::Edit::Attributes::Handler, &CReflectedVarGenericProperty::handler)
            ;
    }

}

AZ::u32 CReflectedVarSpline::handler()
{
    switch (m_propertyType)
    {
    case ePropertyFloatCurve:
        return AZ_CRC_CE("ePropertyFloatCurve");
    case ePropertyColorCurve:
        return AZ_CRC_CE("ePropertyColorCurve");
    default:
        AZ_Assert(false, "CReflectedVarSpline property type must be ePropertyFloatCurve or ePropertyColorCurve");
        return AZ::Edit::UIHandlers::Default;
    }
}


AZ::u32 CReflectedVarGenericProperty::handler()
{
    switch (m_propertyType)
    {
    case ePropertyShader:
        return AZ_CRC_CE("ePropertyShader");
    case ePropertyEquip:
        return AZ_CRC_CE("ePropertyEquip");
    case ePropertyDeprecated0:
        return AZ_CRC_CE("ePropertyCustomAction");
    case ePropertyGameToken:
        return AZ_CRC_CE("ePropertyGameToken");
    case ePropertyMissionObj:
        return AZ_CRC_CE("ePropertyMissionObj");
    case ePropertySequence:
        return AZ_CRC_CE("ePropertySequence");
    case ePropertySequenceId:
        return AZ_CRC_CE("ePropertySequenceId");
    case ePropertyLocalString:
        return AZ_CRC_CE("ePropertyLocalString");
    case ePropertyLightAnimation:
        return AZ_CRC_CE("ePropertyLightAnimation");
    case ePropertyParticleName:
        return AZ_CRC_CE("ePropertyParticleName");
    default:
        AZ_Assert(false, "No property handlers defined for the property type");
        return AZ_CRC_CE("Default");
    }
}


void CPropertyContainer::AddProperty(CReflectedVar *property)
{
    if (property)
        m_properties.push_back(property);
}

void CPropertyContainer::Clear()
{
    m_properties.clear();
}
