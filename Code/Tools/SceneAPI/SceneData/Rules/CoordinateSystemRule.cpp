/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <SceneAPI/SceneCore/Containers/RuleContainer.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneData/Rules/CoordinateSystemRule.h>

namespace AZ::SceneAPI::SceneData
{
    AZ_CLASS_ALLOCATOR_IMPL(CoordinateSystemRule, AZ::SystemAllocator)

    CoordinateSystemRule::CoordinateSystemRule()
        : m_targetCoordinateSystem(ZUpPositiveYForward)
    {
    }

    void CoordinateSystemRule::UpdateCoordinateSystemConverter()
    {
        if (m_useAdvancedData)
        {
            m_coordinateSystemConverter = {};
        }
        else
        {
            switch (m_targetCoordinateSystem)
            {
                case ZUpPositiveYForward:
                {
                    // Source coordinate system, use identity for now, which will currently just assume LY's coordinate system.
                    const AZ::Vector3 sourceBasisVectors[3] = { AZ::Vector3( 1.0f, 0.0f, 0.0f), 
                                                                AZ::Vector3( 0.0f, 1.0f, 0.0f), 
                                                                AZ::Vector3( 0.0f, 0.0f, 1.0f) };

                    // The target coordinate system, with X and Y inverted (rotate 180 degrees over Z)
                    const AZ::Vector3 targetBasisVectors[3] = { AZ::Vector3(-1.0f, 0.0f, 0.0f), 
                                                                AZ::Vector3( 0.0f,-1.0f, 0.0f), 
                                                                AZ::Vector3( 0.0f, 0.0f, 1.0f) };

                    // X, Y and Z are all at the same indices inside the target coordinate system, compared to the source coordinate system.
                    const AZ::u32 targetBasisIndices[3] = { 0, 1, 2 };

                    m_coordinateSystemConverter = CoordinateSystemConverter::CreateFromBasisVectors(sourceBasisVectors, targetBasisVectors, targetBasisIndices);                                             
                }
                break;

                case ZUpNegativeYForward:
                {
                    // Source coordinate system, use identity for now, which will currently just assume LY's coordinate system.
                    const AZ::Vector3 sourceBasisVectors[3] = { AZ::Vector3( 1.0f, 0.0f, 0.0f), 
                                                                AZ::Vector3( 0.0f, 1.0f, 0.0f), 
                                                                AZ::Vector3( 0.0f, 0.0f, 1.0f) };

                    // The target coordinate system, which is the same as the source, so basically we won't do anything here.
                    const AZ::Vector3 targetBasisVectors[3] = { AZ::Vector3( 1.0f, 0.0f, 0.0f), 
                                                                AZ::Vector3( 0.0f, 1.0f, 0.0f), 
                                                                AZ::Vector3( 0.0f, 0.0f, 1.0f) };

                    // X, Y and Z are all at the same indices inside the target coordinate system, compared to the source coordinate system.
                    const AZ::u32 targetBasisIndices[3] = { 0, 1, 2 };

                    m_coordinateSystemConverter = CoordinateSystemConverter::CreateFromBasisVectors(sourceBasisVectors, targetBasisVectors, targetBasisIndices);                                             
                }
                break;

                default:
                    AZ_Assert(false, "Unsupported coordinate system conversion");
            };
        }
    }

    void CoordinateSystemRule::SetTargetCoordinateSystem(CoordinateSystem targetCoordinateSystem)
    {
        m_targetCoordinateSystem = targetCoordinateSystem;
        UpdateCoordinateSystemConverter();
    }

    CoordinateSystemRule::CoordinateSystem CoordinateSystemRule::GetTargetCoordinateSystem() const
    {
        return m_targetCoordinateSystem;
    }

    void CoordinateSystemRule::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<CoordinateSystemRule, IRule>()->Version(2) // LYN-2442
            ->Field("targetCoordinateSystem", &CoordinateSystemRule::m_targetCoordinateSystem)
            ->Field("useAdvancedData", &CoordinateSystemRule::m_useAdvancedData)
            ->Field("originNodeName", &CoordinateSystemRule::m_originNodeName)
            ->Field("rotation", &CoordinateSystemRule::m_rotation)
            ->Field("translation", &CoordinateSystemRule::m_translation)
            ->Field("scale", &CoordinateSystemRule::m_scale);

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (editContext)
        {
            editContext->Class<CoordinateSystemRule>("Coordinate system change",
                "Modify the target coordinate system, applying a transformation to all data (transforms and vertex data if it exists).")
                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                ->DataElement(AZ::Edit::UIHandlers::Default, &CoordinateSystemRule::m_useAdvancedData,
                    "Use Advanced Settings",
                    "Toggles on the advanced settings for transforming the mesh group.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                ->DataElement(AZ::Edit::UIHandlers::ComboBox, &CoordinateSystemRule::m_targetCoordinateSystem,
                    "Facing direction",
                    "Change the direction the actor/motion will face by applying a post transformation to the data.")
                    ->EnumAttribute(CoordinateSystem::ZUpNegativeYForward, "Do nothing")
                    ->EnumAttribute(CoordinateSystem::ZUpPositiveYForward, "Rotate 180 degrees around the up axis")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &CoordinateSystemRule::GetBasicVisibility)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                ->DataElement("NodeListSelection", &CoordinateSystemRule::m_originNodeName,
                    "Relative Origin Node",
                    "Select a Node from the scene as the origin for this export.")
                    ->Attribute("DisabledOption", "")
                    ->Attribute("DefaultToDisabled", false)
                    ->Attribute("ExcludeEndPoints", true)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &CoordinateSystemRule::GetAdvancedVisibility)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                ->DataElement(AZ::Edit::UIHandlers::Default, &CoordinateSystemRule::m_translation,
                    "Translation",
                    "Moves the group along the given vector.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &CoordinateSystemRule::GetAdvancedVisibility)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                ->DataElement(AZ::Edit::UIHandlers::Default, &CoordinateSystemRule::m_rotation,
                    "Rotation",
                    "Sets the orientation offset of the processed mesh in degrees. Rotates the group after translation.")
                    ->Attribute(Edit::Attributes::LabelForX, "P")
                    ->Attribute(Edit::Attributes::LabelForY, "R")
                    ->Attribute(Edit::Attributes::LabelForZ, "Y")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &CoordinateSystemRule::GetAdvancedVisibility)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                ->DataElement(AZ::Edit::UIHandlers::Default, &CoordinateSystemRule::m_scale,
                    "Scale",
                    "Sets the scale offset of the processed mesh.")
                    ->Attribute(Edit::Attributes::Min, 0.0001)
                    ->Attribute(Edit::Attributes::Max, 1000.0)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &CoordinateSystemRule::GetAdvancedVisibility)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues);
        }
    }

    AZ::Crc32 CoordinateSystemRule::GetBasicVisibility() const
    {
        return (m_useAdvancedData) ? AZ::Edit::PropertyVisibility::Hide : AZ::Edit::PropertyVisibility::Show;
    }

    AZ::Crc32 CoordinateSystemRule::GetAdvancedVisibility() const
    {
        return (m_useAdvancedData) ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }

    bool CoordinateSystemRule::ConvertLegacyCoordinateSystemRule(AZ::SerializeContext& serializeContext,
        AZ::SerializeContext::DataElementNode& classElement)
    {
        AZ::SerializeContext::DataElementNode* ruleContainerNode = classElement.FindSubElement(AZ_CRC_CE("rules"));
        if (!ruleContainerNode)
        {
            AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow, "Can't find rule container.\n");
            return false;
        }

        AZ::SerializeContext::DataElementNode* rulesNode = ruleContainerNode->FindSubElement(AZ_CRC_CE("rules"));
        if (!rulesNode)
        {
            AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow, "Can't find rules within rule container.\n");
            return false;
        }

        const AZ::Uuid oldCoordSysRuleId("{603207E2-4F55-4C33-9AAB-98CA75C1E351}");
        const int numRules = rulesNode->GetNumSubElements();
        for (int i = 0; i < numRules; ++i)
        {
            AZ::SerializeContext::DataElementNode& sharedPointerNode = rulesNode->GetSubElement(i);
            if (sharedPointerNode.GetNumSubElements() == 1)
            {
                AZ::SerializeContext::DataElementNode& currentRuleNode = sharedPointerNode.GetSubElement(0);

                // Convert the coordinate system rule
                if (currentRuleNode.GetId() == oldCoordSysRuleId)
                {
                    int targetCoordinateSystem = 0;
                    currentRuleNode.FindSubElementAndGetData(AZ_CRC_CE("targetCoordinateSystem"), targetCoordinateSystem);

                    AZStd::shared_ptr<CoordinateSystemRule> coordSysRule = AZStd::make_shared<CoordinateSystemRule>();
                    coordSysRule->SetTargetCoordinateSystem(static_cast<AZ::SceneAPI::DataTypes::ICoordinateSystemRule::CoordinateSystem>(targetCoordinateSystem));

                    rulesNode->RemoveElement(i);
                    rulesNode->AddElementWithData<AZStd::shared_ptr<AZ::SceneAPI::DataTypes::IRule>>(serializeContext, "element", coordSysRule);

                    return true;
                }
            }
        }

        return true;
    }

    bool CoordinateSystemRule::GetUseAdvancedData() const
    {
        return m_useAdvancedData;
    }

    void CoordinateSystemRule::SetUseAdvancedData(bool useAdvancedData)
    {
        m_useAdvancedData = useAdvancedData;
    }

    const AZStd::string& CoordinateSystemRule::GetOriginNodeName() const
    {
        return m_originNodeName;
    }

    void CoordinateSystemRule::SetOriginNodeName(const AZStd::string& originNodeName)
    {
        m_originNodeName = originNodeName;
    }

    const Quaternion& CoordinateSystemRule::GetRotation() const
    {
        return m_rotation;
    }

    void CoordinateSystemRule::SetRotation(const Quaternion& rotation)
    {
        m_rotation = rotation;
    }

    const Vector3& CoordinateSystemRule::GetTranslation() const
    {
        return m_translation;
    }

    void CoordinateSystemRule::SetTranslation(const Vector3& translation)
    {
        m_translation = translation;
    }

    float CoordinateSystemRule::GetScale() const
    {
        return m_scale;
    }

    void CoordinateSystemRule::SetScale(float scale)
    {
        m_scale = scale;
    }
} // namespace AZ::SceneAPI::SceneData
