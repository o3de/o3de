/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Vegetation/Descriptor.h>
#include <SurfaceData/SurfaceTag.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/sort.h>
#include <Vegetation/EmptyInstanceSpawner.h>
#include <Vegetation/PrefabInstanceSpawner.h>

//////////////////////////////////////////////////////////////////////
// #pragma inline_depth(0)

namespace Vegetation
{
    namespace DescriptorUtil
    {
        static bool UpdateVersion(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            if (classElement.GetVersion() < 4)
            {
                AZ::Vector3 positionMin(-0.3f, -0.3f, 0.0f);
                if (classElement.GetChildData(AZ_CRC_CE("PositionMin"), positionMin))
                {
                    classElement.RemoveElementByName(AZ_CRC_CE("PositionMin"));
                    classElement.AddElementWithData(context, "PositionMinX", (float)positionMin.GetX());
                    classElement.AddElementWithData(context, "PositionMinY", (float)positionMin.GetY());
                    classElement.AddElementWithData(context, "PositionMinZ", (float)positionMin.GetZ());
                }

                AZ::Vector3 positionMax(0.3f, 0.3f, 0.0f);
                if (classElement.GetChildData(AZ_CRC_CE("PositionMax"), positionMax))
                {
                    classElement.RemoveElementByName(AZ_CRC_CE("PositionMax"));
                    classElement.AddElementWithData(context, "PositionMaxX", (float)positionMax.GetX());
                    classElement.AddElementWithData(context, "PositionMaxY", (float)positionMax.GetY());
                    classElement.AddElementWithData(context, "PositionMaxZ", (float)positionMax.GetZ());
                }

                AZ::Vector3 rotationMin(0.0f, 0.0f, -180.0f);
                if (classElement.GetChildData(AZ_CRC_CE("RotationMin"), rotationMin))
                {
                    classElement.RemoveElementByName(AZ_CRC_CE("RotationMin"));
                    classElement.AddElementWithData(context, "RotationMinX", (float)rotationMin.GetX());
                    classElement.AddElementWithData(context, "RotationMinY", (float)rotationMin.GetY());
                    classElement.AddElementWithData(context, "RotationMinZ", (float)rotationMin.GetZ());
                }

                AZ::Vector3 rotationMax(0.0f, 0.0f, 180.0f);
                if (classElement.GetChildData(AZ_CRC_CE("RotationMax"), rotationMax))
                {
                    classElement.RemoveElementByName(AZ_CRC_CE("RotationMax"));
                    classElement.AddElementWithData(context, "RotationMaxX", (float)rotationMax.GetX());
                    classElement.AddElementWithData(context, "RotationMaxY", (float)rotationMax.GetY());
                    classElement.AddElementWithData(context, "RotationMaxZ", (float)rotationMax.GetZ());
                }
            }
            if (classElement.GetVersion() < 5)
            {
                classElement.RemoveElementByName(AZ_CRC_CE("RadiusMax"));
            }
            if (classElement.GetVersion() < 7)
            {
                // The only type of spawners supported prior to this version were legacy vegetation spawners so replace with an empty spawner.
                auto baseInstanceSpawner = AZStd::make_shared<EmptyInstanceSpawner>();
                classElement.AddElementWithData(context, "InstanceSpawner", baseInstanceSpawner);
                AZ_Error("Dynamic Vegetation", false, "Replacing legacy vegetation spawner with an empty instance spawner");
            }
            if (classElement.GetVersion() < 8)
            {
                // Spawner type was briefly stored as a display string instead of a TypeId.
                AZStd::string spawnerType;
                if (classElement.GetChildData(AZ_CRC_CE("SpawnerType"), spawnerType))
                {
                    AZ::TypeId newSpawnerType = azrtti_typeid<EmptyInstanceSpawner>();
                    if (spawnerType == "Legacy Vegetation")
                    {
                        AZ_Error("Dynamic Vegetation", false, "Replacing legacy vegetation spawner with an empty instance spawner");
                    }

                    classElement.RemoveElementByName(AZ_CRC_CE("SpawnerType"));
                    classElement.AddElementWithData(context, "SpawnerType", newSpawnerType);
                }
            }
            return true;
        }
    }

    AZStd::fixed_vector<AZStd::pair<AZ::TypeId, AZStd::string_view>, Descriptor::m_maxSpawnerTypesExpected> Descriptor::m_spawnerTypes;

    void Descriptor::Reflect(AZ::ReflectContext* context)
    {
        // Don't reflect again if we're already reflected to the passed in context
        if (context->IsTypeReflected(VegetationDescriptorTypeId))
        {
            return;
        }

        SurfaceTagDistance::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->RegisterGenericType<AZStd::vector<Descriptor>>();
            serialize->RegisterGenericType<AZStd::shared_ptr<Descriptor>>();

            serialize->Class<Descriptor>()
                ->Version(8, &DescriptorUtil::UpdateVersion)
                ->Field("SpawnerType", &Descriptor::m_spawnerType)
                ->Field("InstanceSpawner", &Descriptor::m_instanceSpawner)
                ->Field("Weight", &Descriptor::m_weight)
                ->Field("Advanced", &Descriptor::m_advanced)
                ->Field("PositionOverrideEnabled", &Descriptor::m_positionOverrideEnabled)
                ->Field("PositionMinX", &Descriptor::m_positionMinX)
                ->Field("PositionMaxX", &Descriptor::m_positionMaxX)
                ->Field("PositionMinY", &Descriptor::m_positionMinY)
                ->Field("PositionMaxY", &Descriptor::m_positionMaxY)
                ->Field("PositionMinZ", &Descriptor::m_positionMinZ)
                ->Field("PositionMaxZ", &Descriptor::m_positionMaxZ)
                ->Field("RotationOverrideEnabled", &Descriptor::m_rotationOverrideEnabled)
                ->Field("RotationMinX", &Descriptor::m_rotationMinX)
                ->Field("RotationMaxX", &Descriptor::m_rotationMaxX)
                ->Field("RotationMinY", &Descriptor::m_rotationMinY)
                ->Field("RotationMaxY", &Descriptor::m_rotationMaxY)
                ->Field("RotationMinZ", &Descriptor::m_rotationMinZ)
                ->Field("RotationMaxZ", &Descriptor::m_rotationMaxZ)
                ->Field("ScaleOverrideEnabled", &Descriptor::m_scaleOverrideEnabled)
                ->Field("ScaleMin", &Descriptor::m_scaleMin)
                ->Field("ScaleMax", &Descriptor::m_scaleMax)
                ->Field("AltitudeFilterOverrideEnabled", &Descriptor::m_altitudeFilterOverrideEnabled)
                ->Field("AltitudeFilterMin", &Descriptor::m_altitudeFilterMin)
                ->Field("AltitudeFilterMax", &Descriptor::m_altitudeFilterMax)
                ->Field("RadiusOverrideEnabled", &Descriptor::m_radiusOverrideEnabled)
                ->Field("BoundMode", &Descriptor::m_boundMode)
                ->Field("RadiusMin", &Descriptor::m_radiusMin)
                ->Field("SurfaceAlignmentOverrideEnabled", &Descriptor::m_surfaceAlignmentOverrideEnabled)
                ->Field("SurfaceAlignmentMin", &Descriptor::m_surfaceAlignmentMin)
                ->Field("SurfaceAlignmentMax", &Descriptor::m_surfaceAlignmentMax)
                ->Field("SlopeFilterOverrideEnabled", &Descriptor::m_slopeFilterOverrideEnabled)
                ->Field("SlopeFilterMin", &Descriptor::m_slopeFilterMin)
                ->Field("SlopeFilterMax", &Descriptor::m_slopeFilterMax)
                ->Field("SurfaceFilterOverrideMode", &Descriptor::m_surfaceFilterOverrideMode)
                ->Field("InclusiveSurfaceFilterTags", &Descriptor::m_inclusiveSurfaceFilterTags)
                ->Field("ExclusiveSurfaceFilterTags", &Descriptor::m_exclusiveSurfaceFilterTags)
                ->Field("SurfaceTagDistance", &Descriptor::m_surfaceTagDistance)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<Descriptor>(
                    "Vegetation Descriptor", "Details used to create vegetation instances")
                    // For this ComboBox to actually work, there is a PropertyHandler registration in EditorVegetationSystemComponent.cpp
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &Descriptor::m_spawnerType, "Instance Spawner", "The type of instances to spawn")
                        ->Attribute(AZ::Edit::Attributes::GenericValueList, &Descriptor::GetSpawnerTypeList)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &Descriptor::SpawnerTypeChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Descriptor::m_instanceSpawner, "Instance", "Instance data")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(0, &Descriptor::m_weight, "Weight", "Weight counted against the total density of the placed vegetation sector")
                    ->DataElement(0, &Descriptor::m_advanced, "Display Per-Item Overrides", "Display the per-item override settings that can be used with filter and modifier components when those components have 'Allow Per-Item Overrides' enabled.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Position Modifier")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                        ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                        ->DataElement(0, &Descriptor::m_positionOverrideEnabled, "Override Enabled", "Enable per-item override settings for this item when the Position Modifier has 'Allow Per-Item Overrides' enabled.")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &Descriptor::m_positionMinX, "Min X", "Minimum position offset on X axis.")
                            ->Attribute(AZ::Edit::Attributes::Min, std::numeric_limits<float>::lowest())
                            ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                            ->Attribute(AZ::Edit::Attributes::SoftMin, -2.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 2.0f)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &Descriptor::IsPositionFilterReadOnly)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &Descriptor::m_positionMaxX, "Max X", "Maximum position offset on X axis.")
                            ->Attribute(AZ::Edit::Attributes::Min, std::numeric_limits<float>::lowest())
                            ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                            ->Attribute(AZ::Edit::Attributes::SoftMin, -2.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 2.0f)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &Descriptor::IsPositionFilterReadOnly)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &Descriptor::m_positionMinY, "Min Y", "Minimum position offset on Y axis.")
                            ->Attribute(AZ::Edit::Attributes::Min, std::numeric_limits<float>::lowest())
                            ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                            ->Attribute(AZ::Edit::Attributes::SoftMin, -2.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 2.0f)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &Descriptor::IsPositionFilterReadOnly)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &Descriptor::m_positionMaxY, "Max Y", "Maximum position offset on Y axis.")
                            ->Attribute(AZ::Edit::Attributes::Min, std::numeric_limits<float>::lowest())
                            ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                            ->Attribute(AZ::Edit::Attributes::SoftMin, -2.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 2.0f)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &Descriptor::IsPositionFilterReadOnly)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &Descriptor::m_positionMinZ, "Min Z", "Minimum position offset on Z axis.")
                            ->Attribute(AZ::Edit::Attributes::Min, std::numeric_limits<float>::lowest())
                            ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                            ->Attribute(AZ::Edit::Attributes::SoftMin, -2.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 2.0f)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &Descriptor::IsPositionFilterReadOnly)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &Descriptor::m_positionMaxZ, "Max Z", "Maximum position offset on Z axis.")
                            ->Attribute(AZ::Edit::Attributes::Min, std::numeric_limits<float>::lowest())
                            ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                            ->Attribute(AZ::Edit::Attributes::SoftMin, -2.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 2.0f)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &Descriptor::IsPositionFilterReadOnly)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Rotation Modifier")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                        ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                        ->DataElement(0, &Descriptor::m_rotationOverrideEnabled, "Override Enabled", "Enable per-item override settings for this item when the Rotation Modifier has 'Allow Per-Item Overrides' enabled.")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &Descriptor::m_rotationMinX, "Min X", "Minimum rotation offset on X axis.")
                            ->Attribute(AZ::Edit::Attributes::Min, std::numeric_limits<float>::lowest())
                            ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                            ->Attribute(AZ::Edit::Attributes::SoftMin, -180.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 180.0f)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &Descriptor::IsRotationFilterReadOnly)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &Descriptor::m_rotationMaxX, "Max X", "Maximum rotation offset on X axis.")
                            ->Attribute(AZ::Edit::Attributes::Min, std::numeric_limits<float>::lowest())
                            ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                            ->Attribute(AZ::Edit::Attributes::SoftMin, -180.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 180.0f)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &Descriptor::IsRotationFilterReadOnly)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &Descriptor::m_rotationMinY, "Min Y", "Minimum rotation offset on Y axis.")
                            ->Attribute(AZ::Edit::Attributes::Min, std::numeric_limits<float>::lowest())
                            ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                            ->Attribute(AZ::Edit::Attributes::SoftMin, -180.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 180.0f)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &Descriptor::IsRotationFilterReadOnly)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &Descriptor::m_rotationMaxY, "Max Y", "Maximum rotation offset on Y axis.")
                            ->Attribute(AZ::Edit::Attributes::Min, std::numeric_limits<float>::lowest())
                            ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                            ->Attribute(AZ::Edit::Attributes::SoftMin, -180.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 180.0f)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &Descriptor::IsRotationFilterReadOnly)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &Descriptor::m_rotationMinZ, "Min Z", "Minimum rotation offset on Z axis.")
                            ->Attribute(AZ::Edit::Attributes::Min, std::numeric_limits<float>::lowest())
                            ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                            ->Attribute(AZ::Edit::Attributes::SoftMin, -180.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 180.0f)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &Descriptor::IsRotationFilterReadOnly)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &Descriptor::m_rotationMaxZ, "Max Z", "Maximum rotation offset on Z axis.")
                            ->Attribute(AZ::Edit::Attributes::Min, std::numeric_limits<float>::lowest())
                            ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                            ->Attribute(AZ::Edit::Attributes::SoftMin, -180.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 180.0f)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &Descriptor::IsRotationFilterReadOnly)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Scale Modifier")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                        ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                        ->DataElement(0, &Descriptor::m_scaleOverrideEnabled, "Override Enabled", "Enable per-item override settings for this item when the Scale Modifier has 'Allow Per-Item Overrides' enabled.")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &Descriptor::m_scaleMin, "Min", "")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 10.0f)
                            ->Attribute(AZ::Edit::Attributes::Step, 0.125f)
                            ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &Descriptor::IsScaleFilterReadOnly)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &Descriptor::m_scaleMax, "Max", "")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 10.0f)
                            ->Attribute(AZ::Edit::Attributes::Step, 0.125f)
                            ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &Descriptor::IsScaleFilterReadOnly)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Altitude Filter")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                        ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                        ->DataElement(0, &Descriptor::m_altitudeFilterOverrideEnabled, "Override Enabled", "Enable per-item override settings for this item when the Altitude Filter has 'Allow Per-Item Overrides' enabled.")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                        ->DataElement(0, &Descriptor::m_altitudeFilterMin, "Min", "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &Descriptor::IsAltitudeFilterReadOnly)
                        ->DataElement(0, &Descriptor::m_altitudeFilterMax, "Max", "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &Descriptor::IsAltitudeFilterReadOnly)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Distance Between Filter (Radius)")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                        ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                        ->DataElement(0, &Descriptor::m_radiusOverrideEnabled, "Override Enabled", "Enable per-item override settings for this item when the Distance Between Filter has 'Allow Per-Item Overrides' enabled.")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &Descriptor::m_boundMode, "Bound Mode", "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetBoundModeVisibility)
                             ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                            ->EnumAttribute(BoundMode::Radius, "Radius")
                            ->EnumAttribute(BoundMode::MeshRadius, "MeshRadius")
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &Descriptor::IsDistanceBetweenFilterReadOnly)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &Descriptor::m_radiusMin, "Radius Min", "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &Descriptor::IsRadiusReadOnly)
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 16.0f) // match current default sector size in meters.

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Surface Slope Alignment")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                        ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                        ->DataElement(0, &Descriptor::m_surfaceAlignmentOverrideEnabled, "Override Enabled", "Enable per-item override settings for this item when the Surface Slope Alignment Modifier has 'Allow Per-Item Overrides' enabled.")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &Descriptor::m_surfaceAlignmentMin, "Min", "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &Descriptor::IsSurfaceAlignmentFilterReadOnly)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &Descriptor::m_surfaceAlignmentMax, "Max", "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &Descriptor::IsSurfaceAlignmentFilterReadOnly)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Slope Filter")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                        ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                        ->DataElement(0, &Descriptor::m_slopeFilterOverrideEnabled, "Override Enabled", "Enable per-item override settings for this item when the Slope Filter has 'Allow Per-Item Overrides' enabled.")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &Descriptor::m_slopeFilterMin, "Min", "")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 180.0f)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &Descriptor::IsSlopeFilterReadOnly)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &Descriptor::m_slopeFilterMax, "Max", "")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 180.0f)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &Descriptor::IsSlopeFilterReadOnly)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Surface Mask Filter")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                        ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &Descriptor::m_surfaceFilterOverrideMode, "Override Mode", "Enable per-item override settings for this item when the Surface Mask Filter has 'Allow Per-Item Overrides' enabled.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                            ->EnumAttribute(OverrideMode::Disable, "Disable")
                            ->EnumAttribute(OverrideMode::Replace, "Replace")
                            ->EnumAttribute(OverrideMode::Extend, "Extend")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                        ->DataElement(0, &Descriptor::m_inclusiveSurfaceFilterTags, "Inclusion Tags", "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &Descriptor::IsSurfaceTagFilterReadOnly)
                        ->DataElement(0, &Descriptor::m_exclusiveSurfaceFilterTags, "Exclusion Tags", "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &Descriptor::IsSurfaceTagFilterReadOnly)
                    ->EndGroup()

                    ->DataElement(0, &Descriptor::m_surfaceTagDistance, "Surface Mask Depth Filter", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &Descriptor::GetAdvancedGroupVisibility)

                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<Descriptor>()
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Attribute(AZ::Script::Attributes::Module, "vegetation")
                ->Constructor()
                ->Property("spawnerType", &Descriptor::GetSpawnerType, &Descriptor::SetSpawnerType)
                ->Property("spawner", &Descriptor::GetSpawner, &Descriptor::SetSpawner)
                ->Property("weight", BehaviorValueProperty(&Descriptor::m_weight))
                ->Property("surfaceTagDistance", BehaviorValueProperty(&Descriptor::m_surfaceTagDistance))
                ->Property("surfaceFilterOverrideMode", 
                    [](Descriptor* descriptor) { return (AZ::u8)(descriptor->m_surfaceFilterOverrideMode); },
                    [](Descriptor* descriptor, const AZ::u8& i) { descriptor->m_surfaceFilterOverrideMode = (OverrideMode)i; })
                ->Property("radiusOverrideEnabled", BehaviorValueProperty(&Descriptor::m_radiusOverrideEnabled))
                ->Property("radiusMin", BehaviorValueProperty(&Descriptor::m_radiusMin))
                ->Property("boundMode",
                    [](Descriptor* descriptor) { return (AZ::u8)(descriptor->m_boundMode); },
                    [](Descriptor* descriptor, const AZ::u8& i) { descriptor->m_boundMode = (BoundMode)i; })
                ->Property("surfaceAlignmentOverrideEnabled", BehaviorValueProperty(&Descriptor::m_surfaceAlignmentOverrideEnabled))
                ->Property("surfaceAlignmentMin", BehaviorValueProperty(&Descriptor::m_surfaceAlignmentMin))
                ->Property("surfaceAlignmentMax", BehaviorValueProperty(&Descriptor::m_surfaceAlignmentMax))
                ->Property("rotationOverrideEnabled", BehaviorValueProperty(&Descriptor::m_rotationOverrideEnabled))
                ->Property("rotationMinX", BehaviorValueProperty(&Descriptor::m_rotationMinX))
                ->Property("rotationMaxX", BehaviorValueProperty(&Descriptor::m_rotationMaxX))
                ->Property("rotationMinY", BehaviorValueProperty(&Descriptor::m_rotationMinY))
                ->Property("rotationMaxY", BehaviorValueProperty(&Descriptor::m_rotationMaxY))
                ->Property("rotationMinZ", BehaviorValueProperty(&Descriptor::m_rotationMinZ))
                ->Property("rotationMaxZ", BehaviorValueProperty(&Descriptor::m_rotationMaxZ))
                ->Property("positionOverrideEnabled", BehaviorValueProperty(&Descriptor::m_positionOverrideEnabled))
                ->Property("positionMinX", BehaviorValueProperty(&Descriptor::m_positionMinX))
                ->Property("positionMaxX", BehaviorValueProperty(&Descriptor::m_positionMaxX))
                ->Property("positionMinY", BehaviorValueProperty(&Descriptor::m_positionMinY))
                ->Property("positionMaxY", BehaviorValueProperty(&Descriptor::m_positionMaxY))
                ->Property("positionMinZ", BehaviorValueProperty(&Descriptor::m_positionMinZ))
                ->Property("positionMaxZ", BehaviorValueProperty(&Descriptor::m_positionMaxZ))
                ->Property("scaleOverrideEnabled", BehaviorValueProperty(&Descriptor::m_scaleOverrideEnabled))
                ->Property("scaleMin", BehaviorValueProperty(&Descriptor::m_scaleMin))
                ->Property("scaleMax", BehaviorValueProperty(&Descriptor::m_scaleMax))
                ->Property("altitudeFilterOverrideEnabled", BehaviorValueProperty(&Descriptor::m_altitudeFilterOverrideEnabled))
                ->Property("altitudeFilterMin", BehaviorValueProperty(&Descriptor::m_altitudeFilterMin))
                ->Property("altitudeFilterMax", BehaviorValueProperty(&Descriptor::m_altitudeFilterMax))
                ->Property("slopeFilterOverrideEnabled", BehaviorValueProperty(&Descriptor::m_slopeFilterOverrideEnabled))
                ->Property("slopeFilterMin", BehaviorValueProperty(&Descriptor::m_slopeFilterMin))
                ->Property("slopeFilterMax", BehaviorValueProperty(&Descriptor::m_slopeFilterMax))
                ->Method("GetNumInclusiveSurfaceFilterTags", &Descriptor::GetNumInclusiveSurfaceFilterTags)
                ->Method("GetInclusiveSurfaceFilterTag", &Descriptor::GetInclusiveSurfaceFilterTag)
                ->Method("RemoveInclusiveSurfaceFilterTag", &Descriptor::RemoveInclusiveSurfaceFilterTag)
                ->Method("AddInclusiveSurfaceFilterTag", &Descriptor::AddInclusiveSurfaceFilterTag)
                ->Method("GetNumExclusiveSurfaceFilterTags", &Descriptor::GetNumExclusiveSurfaceFilterTags)
                ->Method("GetExclusiveSurfaceFilterTag", &Descriptor::GetExclusiveSurfaceFilterTag)
                ->Method("RemoveExclusiveSurfaceFilterTag", &Descriptor::RemoveExclusiveSurfaceFilterTag)
                ->Method("AddExclusiveSurfaceFilterTag", &Descriptor::AddExclusiveSurfaceFilterTag)
                ;
        }
    }

    void SurfaceTagDistance::Reflect(AZ::ReflectContext * context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<SurfaceTagDistance>()
                ->Version(0)
                ->Field("SurfaceTag", &SurfaceTagDistance::m_tags)
                ->Field("UpperDistanceInMeters", &SurfaceTagDistance::m_upperDistanceInMeters)
                ->Field("LowerDistanceInMeters", &SurfaceTagDistance::m_lowerDistanceInMeters)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<SurfaceTagDistance>(
                    "VegetationSurfaceTagDepth", "Describes depth information for a vegetation object based on a tag to match with a surface mask")
                    ->DataElement(0, &SurfaceTagDistance::m_tags, "Surface Tags", "The surface tags to compare the distance from the planting tag to.")
                    ->DataElement(0, &SurfaceTagDistance::m_upperDistanceInMeters, "Upper Distance Range (m)", "Upper Distance in meters from comparison surface, negative for below")
                    ->DataElement(0, &SurfaceTagDistance::m_lowerDistanceInMeters, "Lower Distance Range (m)", "Lower Distance in meters from comparison surface, negative for below")
                    ;
            }
        }
        
        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<SurfaceTagDistance>()
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Constructor()
                ->Property("upperDistanceInMeters", BehaviorValueProperty(&SurfaceTagDistance::m_upperDistanceInMeters))
                ->Property("lowerDistanceInMeters", BehaviorValueProperty(&SurfaceTagDistance::m_lowerDistanceInMeters))
                ->Method("GetNumTags", &SurfaceTagDistance::GetNumTags)
                ->Method("GetTag", &SurfaceTagDistance::GetTag)
                ->Method("RemoveTag", &SurfaceTagDistance::RemoveTag)
                ->Method("AddTag", &SurfaceTagDistance::AddTag)
                ;
        }
    }

    size_t SurfaceTagDistance::GetNumTags() const
    {
        return m_tags.size();
    }

    AZ::Crc32 SurfaceTagDistance::GetTag(int tagIndex) const
    {
        if (tagIndex < m_tags.size() && tagIndex >= 0)
        {
            return m_tags[tagIndex];
        }

        return AZ::Crc32();
    }

    void SurfaceTagDistance::RemoveTag(int tagIndex)
    {
        if (tagIndex < m_tags.size() && tagIndex >= 0)
        {
            m_tags.erase(m_tags.begin() + tagIndex);
        }
    }

    void SurfaceTagDistance::AddTag(const AZStd::string& tag)
    {
        m_tags.push_back(SurfaceData::SurfaceTag(tag));
    }

    bool SurfaceTagDistance::operator==(const SurfaceTagDistance& rhs) const
    {
        return m_tags == rhs.m_tags && m_lowerDistanceInMeters == rhs.m_lowerDistanceInMeters && m_upperDistanceInMeters == rhs.m_upperDistanceInMeters;
    }

    Descriptor::Descriptor()
    {
        m_instanceSpawner = AZStd::make_shared<PrefabInstanceSpawner>();
        m_spawnerType = azrtti_typeid(*m_instanceSpawner);
    }

    Descriptor::~Descriptor()
    {
    }

    bool Descriptor::HasEquivalentInstanceSpawners(const Descriptor& rhs) const
    {
        bool instanceSpawnersMatch = false;

        if (m_spawnerType == rhs.m_spawnerType)
        {
            if (m_instanceSpawner == rhs.m_instanceSpawner)
            {
                // This will match if they're both null, or both the same pointer.
                instanceSpawnersMatch = true;
            }
            else
            {
                // Only match if they're both not null and have equivalent data.
                if (m_instanceSpawner && rhs.m_instanceSpawner)
                {
                    instanceSpawnersMatch = (*m_instanceSpawner) == (*rhs.m_instanceSpawner);
                }
            }
        }

        return instanceSpawnersMatch;
    }

    bool Descriptor::operator==(const Descriptor& rhs) const
    {
        return
            HasEquivalentInstanceSpawners(rhs) &&
            m_weight == rhs.m_weight &&
            m_surfaceTagDistance == rhs.m_surfaceTagDistance &&
            m_surfaceFilterOverrideMode == rhs.m_surfaceFilterOverrideMode &&
            m_inclusiveSurfaceFilterTags == rhs.m_inclusiveSurfaceFilterTags &&
            m_exclusiveSurfaceFilterTags == rhs.m_exclusiveSurfaceFilterTags &&
            m_radiusOverrideEnabled == rhs.m_radiusOverrideEnabled &&
            m_radiusMin == rhs.m_radiusMin &&
            m_boundMode == rhs.m_boundMode &&
            m_surfaceAlignmentOverrideEnabled == rhs.m_surfaceAlignmentOverrideEnabled &&
            m_surfaceAlignmentMin == rhs.m_surfaceAlignmentMin &&
            m_surfaceAlignmentMax == rhs.m_surfaceAlignmentMax &&
            m_rotationOverrideEnabled == rhs.m_rotationOverrideEnabled &&
            m_rotationMinX == rhs.m_rotationMinX &&
            m_rotationMaxX == rhs.m_rotationMaxX &&
            m_rotationMinY == rhs.m_rotationMinY &&
            m_rotationMaxY == rhs.m_rotationMaxY &&
            m_rotationMinZ == rhs.m_rotationMinZ &&
            m_rotationMaxZ == rhs.m_rotationMaxZ &&
            m_positionOverrideEnabled == rhs.m_positionOverrideEnabled &&
            m_positionMinX == rhs.m_positionMinX &&
            m_positionMaxX == rhs.m_positionMaxX &&
            m_positionMinY == rhs.m_positionMinY &&
            m_positionMaxY == rhs.m_positionMaxY &&
            m_positionMinZ == rhs.m_positionMinZ &&
            m_positionMaxZ == rhs.m_positionMaxZ &&
            m_scaleOverrideEnabled == rhs.m_scaleOverrideEnabled &&
            m_scaleMin == rhs.m_scaleMin &&
            m_scaleMax == rhs.m_scaleMax &&
            m_altitudeFilterOverrideEnabled == rhs.m_altitudeFilterOverrideEnabled &&
            m_altitudeFilterMin == rhs.m_altitudeFilterMin &&
            m_altitudeFilterMax == rhs.m_altitudeFilterMax &&
            m_slopeFilterOverrideEnabled == rhs.m_slopeFilterOverrideEnabled &&
            m_slopeFilterMin == rhs.m_slopeFilterMin &&
            m_slopeFilterMax == rhs.m_slopeFilterMax
            ;
    }

    size_t Descriptor::GetNumInclusiveSurfaceFilterTags() const
    {
        return m_inclusiveSurfaceFilterTags.size();
    }

    AZ::Crc32 Descriptor::GetInclusiveSurfaceFilterTag(int tagIndex) const
    {
        if (tagIndex < m_inclusiveSurfaceFilterTags.size() && tagIndex >= 0)
        {
            return m_inclusiveSurfaceFilterTags[tagIndex];
        }

        return AZ::Crc32();
    }

    void Descriptor::RemoveInclusiveSurfaceFilterTag(int tagIndex)
    {
        if (tagIndex < m_inclusiveSurfaceFilterTags.size() && tagIndex >= 0)
        {
            m_inclusiveSurfaceFilterTags.erase(m_inclusiveSurfaceFilterTags.begin() + tagIndex);
        }
    }

    void Descriptor::AddInclusiveSurfaceFilterTag(const AZStd::string& tag)
    {
        m_inclusiveSurfaceFilterTags.push_back(SurfaceData::SurfaceTag(tag));
    }

    size_t Descriptor::GetNumExclusiveSurfaceFilterTags() const
    {
        return m_exclusiveSurfaceFilterTags.size();
    }

    AZ::Crc32 Descriptor::GetExclusiveSurfaceFilterTag(int tagIndex) const
    {
        if (tagIndex < m_exclusiveSurfaceFilterTags.size() && tagIndex >= 0)
        {
            return m_exclusiveSurfaceFilterTags[tagIndex];
        }

        return AZ::Crc32();
    }

    void Descriptor::RemoveExclusiveSurfaceFilterTag(int tagIndex)
    {
        if (tagIndex < m_exclusiveSurfaceFilterTags.size() && tagIndex >= 0)
        {
            m_exclusiveSurfaceFilterTags.erase(m_exclusiveSurfaceFilterTags.begin() + tagIndex);
        }
    }

    void Descriptor::AddExclusiveSurfaceFilterTag(const AZStd::string& tag)
    {
        m_exclusiveSurfaceFilterTags.push_back(SurfaceData::SurfaceTag(tag));
    }

    AZ::u32 Descriptor::GetAdvancedGroupVisibility() const
    {
        return m_advanced ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }

    AZ::u32 Descriptor::GetBoundModeVisibility() const
    {
        // Only show Bound Mode if we're showing advanced settings *and* this type of instance spawner
        // can provide radius data.  If not, the "MeshRadius" setting is meaningless, so don't allow it
        // to be set.
        return (m_advanced && m_instanceSpawner && m_instanceSpawner->HasRadiusData())
            ? AZ::Edit::PropertyVisibility::Show
            : AZ::Edit::PropertyVisibility::Hide;
    }

    void Descriptor::RefreshSpawnerTypeList() const
    {
        m_spawnerTypes.clear();

        // Find all registered types that are derived from InstanceSpawner, and get their display names.
        // (To change the display name for a class, go to its EditContext and change the name passed in
        // to the EditContext Class constructor)
        AZ::SerializeContext* serializeContext{};
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        AZ_Assert(serializeContext, "No SerializeContext found.");
        if (serializeContext)
        {
            serializeContext->EnumerateDerived<InstanceSpawner>(
            [&](const AZ::SerializeContext::ClassData* classData, [[maybe_unused]] const AZ::Uuid& classUuid) -> bool
            {
                auto spawnerDisplayName = classData->m_editData ? classData->m_editData->m_name : classData->m_name;
                m_spawnerTypes.push_back(AZStd::make_pair(classData->m_typeId, spawnerDisplayName));
                return true;
            });
        }

        // Alphabetically sort the list so that it has a well-defined order, regardless of what order we find the entries.
        AZStd::sort(m_spawnerTypes.begin(), m_spawnerTypes.end(), [](const auto& lhs, const auto& rhs) {return lhs.second < rhs.second; });

        AZ_Assert(!m_spawnerTypes.empty(), "No serialized InstanceSpawner types were found.");
    }
    
    AZStd::vector<AZStd::pair<AZ::TypeId, AZStd::string>> Descriptor::GetSpawnerTypeList() const
    {
        if (m_spawnerTypes.empty())
        {
            RefreshSpawnerTypeList();
        }

        AZ_Assert(!m_spawnerTypes.empty(), "No serialized InstanceSpawner types were found.");

        // Copy our static list into a new list with the proper types.
        // This is necessary because the PropertyEditor doesn't always recognize alternate forms of string types,
        // such as string_view or const char*.
        AZStd::vector<AZStd::pair<AZ::TypeId, AZStd::string>> returnList;
        returnList.reserve(m_spawnerTypes.size());
        returnList.assign(m_spawnerTypes.begin(), m_spawnerTypes.end());

        return returnList;
    }

    bool Descriptor::CreateInstanceSpawner(AZ::TypeId spawnerType, InstanceSpawner* spawnerToClone)
    {
        // Locate the registered Behavior class for the requested type.
        const AZ::BehaviorClass* sourceClass = AZ::BehaviorContextHelper::GetClass(spawnerType);
        if (!sourceClass)
        {
            // Can't find the new spawner type, so set back to the previous one.
            AZ_Error("Vegetation", false, "Unrecognized spawner type: %s", spawnerType.ToString<AZStd::string>().c_str());
            return false;
        }

        // Create (or clone) a new instance of the type, and verify that it's the type we expected.
        AZ::BehaviorObject newInstance;
        if (spawnerToClone)
        {
            AZ_Assert(spawnerType == azrtti_typeid(spawnerToClone), "Mismatched InstanceSpawner types");

            AZ::BehaviorObject source(spawnerToClone, spawnerType);
            newInstance = sourceClass->Clone(source);
        }
        else
        {
            newInstance = sourceClass->Create();
        }

        AZ_Assert(newInstance.m_address, "Failed to create requested spawner type: %s", spawnerType.ToString<AZStd::string>().c_str());
        AZ_Assert(newInstance.m_typeId == spawnerType, "Unrecognized spawner type: %s", newInstance.m_typeId.ToString<AZStd::string>().c_str());
        m_instanceSpawner = AZStd::shared_ptr<InstanceSpawner>(reinterpret_cast<InstanceSpawner*>(newInstance.m_address));
        AZ_Assert(spawnerType == azrtti_typeid(*m_instanceSpawner), "Unrecognized spawner type: %s", m_instanceSpawner->RTTI_GetTypeName());

        // Force the bound mode to use Radius if this type of spawner can't provide MeshRadius information.
        if (!m_instanceSpawner->HasRadiusData())
        {
            m_boundMode = BoundMode::Radius;
        }

        // Make sure the spawner type stays in sync with the actual spawner type
        m_spawnerType = spawnerType;

        return true;
    }

    AZ::TypeId Descriptor::GetSpawnerType() const
    {
        return m_spawnerType;
    }

    void Descriptor::SetSpawnerType(const AZ::TypeId& spawnerType)
    {
        m_spawnerType = spawnerType;
        SpawnerTypeChanged();
    }

    AZStd::any Descriptor::GetSpawner() const
    {
        // Note that this is bypassing our shared pointer, which has the potential to cause pointer lifetime
        // issues, since scripts won't affect the shared pointer lifetime.
        AZStd::any::type_info valueInfo;
        valueInfo.m_id = azrtti_typeid(*m_instanceSpawner);
        valueInfo.m_isPointer = false;
        valueInfo.m_useHeap = true;
        valueInfo.m_handler = [](AZStd::any::Action action, AZStd::any* dest, const AZStd::any* source)
        {
            switch (action)
            {
                case AZStd::any::Action::Reserve:
                {
                    // No-op
                    break;
                }
                case AZStd::any::Action::Copy:
                case AZStd::any::Action::Move:
                {
                    *reinterpret_cast<void**>(dest) = AZStd::any_cast<void>(const_cast<AZStd::any*>(source));
                    break;
                }
                case AZStd::any::Action::Destroy:
                {
                    *reinterpret_cast<void**>(dest) = nullptr;
                    break;
                }
            }
        };

        return AZStd::any(&(*m_instanceSpawner), valueInfo);
    }

    void Descriptor::SetSpawner(const AZStd::any& spawnerContainer)
    {
        bool success = false;

        // Convert our AZStd::any container back to an InstanceSpawner pointer.
        void* anyToVoidPtr = AZStd::any_cast<void>(&const_cast<AZStd::any&>(spawnerContainer));
        InstanceSpawner* spawner = reinterpret_cast<InstanceSpawner*>(anyToVoidPtr);

        if (spawner)
        {
            success = CreateInstanceSpawner(spawnerContainer.type(), spawner);
        }

        if (!success)
        {
            AZ_Error("Vegetation", false, "Error setting spawner to type: %s", spawnerContainer.type().ToString<AZStd::string>().c_str());
        }
    }


    AZ::u32 Descriptor::SpawnerTypeChanged()
    {
        // Create a new InstanceSpawner if we changed the spawner type.
        if (m_spawnerType != azrtti_typeid(*m_instanceSpawner))
        {
            bool success = CreateInstanceSpawner(m_spawnerType);

            // If something went wrong creating the new one, still make sure our spawner type
            // stays in sync with whatever existing spawner type we have.
            if (!success)
            {
                m_spawnerType = azrtti_typeid(*m_instanceSpawner);
            }

            // If we change our instance spawner, refresh the entire tree.  The set of editable properties
            // will change based on the new spawner type.
            return AZ::Edit::PropertyRefreshLevels::EntireTree;
        }

        // Nothing changed, so nothing to refresh.
        return AZ::Edit::PropertyRefreshLevels::None;
    }
}

