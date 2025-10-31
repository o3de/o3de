/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <AzCore/Preprocessor/EnumReflectUtils.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzQtComponents/Components/Widgets/FileDialog.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntry.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyFilePathCtrl.h>
#include <Components/TerrainLayerSpawnerComponent.h>
#include <GradientSignal/Editor/EditorGradientImageCreatorUtils.h>
#include <TerrainRenderer/EditorComponents/EditorTerrainMacroMaterialComponent.h>

namespace Terrain
{
    // Implements EditorTerrainMacroMaterialComponent RTTI functions
    AZ_RTTI_NO_TYPE_INFO_IMPL(EditorTerrainMacroMaterialComponent, AzToolsFramework::Components::EditorComponentBase);

    void EditorTerrainMacroMaterialComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorTerrainMacroMaterialComponent, AzToolsFramework::Components::EditorComponentBase>()
                ->Version(3)
                ->Field("Configuration", &EditorTerrainMacroMaterialComponent::m_configuration)
                ->Field("PaintableImageAssetHelper", &EditorTerrainMacroMaterialComponent::m_paintableMacroColorAssetHelper)
                ;

            if (auto* editContext = serializeContext->GetEditContext(); editContext)
            {
                editContext
                    ->Class<TerrainMacroMaterialConfig>(
                        "Terrain Macro Material Component", "Provide a terrain macro material for a region of the world")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &TerrainMacroMaterialConfig::m_macroColorAsset, "Color Texture",
                        "Terrain macro color texture for use by any terrain inside the bounding box on this entity.")
                    ->Attribute(AZ::Edit::Attributes::NameLabelOverride, &TerrainMacroMaterialConfig::GetMacroColorAssetPropertyName)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &TerrainMacroMaterialConfig::m_macroNormalAsset, "Normal Texture",
                        "Texture for defining surface normal direction. These will override normals generated from the geometry.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &TerrainMacroMaterialConfig::m_normalFlipX, "Normal Flip X",
                        "Flip tangent direction for this normal map.")
                    ->Attribute(AZ::Edit::Attributes::ReadOnly, &TerrainMacroMaterialConfig::NormalMapAttributesAreReadOnly)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &TerrainMacroMaterialConfig::m_normalFlipY, "Normal Flip Y",
                        "Flip bitangent direction for this normal map.")
                    ->Attribute(AZ::Edit::Attributes::ReadOnly, &TerrainMacroMaterialConfig::NormalMapAttributesAreReadOnly)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider, &TerrainMacroMaterialConfig::m_normalFactor, "Normal Factor",
                        "Strength factor for scaling the normal map values.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 10.0f)
                        ->Attribute(AZ::Edit::Attributes::SoftMin, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::SoftMax, 2.0f)
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &TerrainMacroMaterialConfig::NormalMapAttributesAreReadOnly)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider, &TerrainMacroMaterialConfig::m_priority, "Priority",
                        "Defines order macro materials are applied.  Larger numbers = higher priority")
                        ->Attribute(AZ::Edit::Attributes::Min, AreaConstants::s_priorityMin)
                        ->Attribute(AZ::Edit::Attributes::Max, AreaConstants::s_priorityMax)
                        ->Attribute(AZ::Edit::Attributes::SoftMin, AreaConstants::s_prioritySoftMin)
                        ->Attribute(AZ::Edit::Attributes::SoftMax, AreaConstants::s_prioritySoftMax)
                    ;

                editContext
                    ->Class<EditorTerrainMacroMaterialComponent>(
                        EditorTerrainMacroMaterialComponent::s_componentName, EditorTerrainMacroMaterialComponent::s_componentDescription)
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Icon, EditorTerrainMacroMaterialComponent::s_icon)
                    ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://docs.o3de.org/docs/user-guide/components/reference/terrain/terrain-macro-material/")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, EditorTerrainMacroMaterialComponent::s_viewportIcon)
                    ->Attribute(AZ::Edit::Attributes::HelpPageURL, EditorTerrainMacroMaterialComponent::s_helpUrl)
                    ->Attribute(AZ::Edit::Attributes::Category, EditorTerrainMacroMaterialComponent::s_categoryName)
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    // Configuration for the Terrain Macro Material
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorTerrainMacroMaterialComponent::m_configuration, "Configuration", "")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorTerrainMacroMaterialComponent::ConfigurationChanged)

                    // Create/edit controls for the macro color image
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &EditorTerrainMacroMaterialComponent::m_paintableMacroColorAssetHelper,
                        "Edit Macro Color Image",
                        "Edit the macro color image asset")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
        }
    }

    // The following methods pass through to the runtime component so that the Editor component shares the same requirements.

    void EditorTerrainMacroMaterialComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        TerrainMacroMaterialComponent::GetRequiredServices(services);
    }

    void EditorTerrainMacroMaterialComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        TerrainMacroMaterialComponent::GetIncompatibleServices(services);
    }

    void EditorTerrainMacroMaterialComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        TerrainMacroMaterialComponent::GetProvidedServices(services);
    }

    void EditorTerrainMacroMaterialComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        // The TerrainMacroMaterialComponent doesn't currently have any dependent services, so there's nothing to pass the call through.
        // TerrainMacroMaterialComponent::GetDependentServices(services);
    }

    void EditorTerrainMacroMaterialComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        // When building the game entity, use the copy of the runtime configuration on the Editor component to create
        // a new runtime component that's configured correctly.
        gameEntity->AddComponent(aznew TerrainMacroMaterialComponent(m_configuration));
    }

    void EditorTerrainMacroMaterialComponent::Init()
    {
        AzToolsFramework::Components::EditorComponentBase::Init();

        // Initialize the copy of the runtime component.
        m_runtimeComponentActive = false;
        m_component.ReadInConfig(&m_configuration);
        m_component.Init();
    }

    void EditorTerrainMacroMaterialComponent::Activate()
    {
        // This block of code is aligned with EditorWrappedComponentBase
        {
            AzToolsFramework::Components::EditorComponentBase::Activate();

            // Use the visibility bus to control whether or not the runtime gradient is active and processing in the Editor.
            AzToolsFramework::EditorVisibilityNotificationBus::Handler::BusConnect(GetEntityId());
            AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
                m_visible, GetEntityId(), &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsVisible);

            // Synchronize the runtime component with the Editor component.
            m_component.ReadInConfig(&m_configuration);
            m_component.SetEntity(GetEntity());

            if (m_visible)
            {
                m_component.Activate();
                m_runtimeComponentActive = true;
            }
        }

        LmbrCentral::DependencyNotificationBus::Handler::BusConnect(GetEntityId());
        AzFramework::PaintBrushNotificationBus::Handler::BusConnect({ GetEntityId(), GetId() });
        TerrainMacroMaterialNotificationBus::Handler::BusConnect();

        // Initialize the paintable image asset helper.
        m_paintableMacroColorAssetHelper.Activate(
            AZ::EntityComponentIdPair(GetEntityId(), GetId()),
            GradientSignal::OutputFormat::R8G8B8A8,
            "Color Texture",
            // Default Save Name callback:
            [this]()
            {
                // Get a default image filename and path that either uses the source asset filename (if the source asset exists)
                // or creates a new name by taking the entity name and adding ".png".
                return AZ::IO::Path(GradientSignal::ImageCreatorUtils::GetDefaultImageSourcePath(
                    m_component.GetMacroColorAsset().GetId(), GetEntity()->GetName() + AZStd::string(".png")));
            },
            // On Asset Created callback:
            [this](AZ::Data::Asset<AZ::Data::AssetData> createdAsset)
            {
                // Set the active image to the created one.
                m_component.SetMacroColorAsset(createdAsset);

                OnCompositionChanged();
            });

        AZStd::string assetLabel = m_paintableMacroColorAssetHelper.Refresh(m_component.GetMacroColorAsset());
        m_configuration.SetMacroColorAssetPropertyName(assetLabel);
    }

    void EditorTerrainMacroMaterialComponent::Deactivate()
    {
        m_paintableMacroColorAssetHelper.Deactivate();

        TerrainMacroMaterialNotificationBus::Handler::BusDisconnect();
        AzFramework::PaintBrushNotificationBus::Handler::BusDisconnect();
        LmbrCentral::DependencyNotificationBus::Handler::BusDisconnect();

        // This block of code is aligned with EditorWrappedComponentBase
        {
            AzToolsFramework::EditorVisibilityNotificationBus::Handler::BusDisconnect();
            AzToolsFramework::Components::EditorComponentBase::Deactivate();

            m_runtimeComponentActive = false;
            m_component.Deactivate();
            // remove the entity association, in case the parent component is being removed, otherwise the component will be reactivated
            m_component.SetEntity(nullptr);
        }
    }

    void EditorTerrainMacroMaterialComponent::OnEntityVisibilityChanged(bool visibility)
    {
        if (m_visible != visibility)
        {
            m_visible = visibility;
            ConfigurationChanged();
        }
    }

    void EditorTerrainMacroMaterialComponent::OnTerrainMacroMaterialCreated(
        AZ::EntityId macroMaterialEntity, [[maybe_unused]] const MacroMaterialData& macroMaterial)
    {
        // This notification gets broadcast to *all* entities, so make sure it belongs to this one before refreshing.
        if (macroMaterialEntity == m_component.GetEntityId())
        {
            RefreshPaintableAssetStatus();
        }
    }

    void EditorTerrainMacroMaterialComponent::OnTerrainMacroMaterialChanged(
        AZ::EntityId macroMaterialEntity, [[maybe_unused]] const MacroMaterialData& macroMaterial)
    {
        // This notification gets broadcast to *all* entities, so make sure it belongs to this one before refreshing.
        if (macroMaterialEntity == m_component.GetEntityId())
        {
            RefreshPaintableAssetStatus();
        }
    }

    void EditorTerrainMacroMaterialComponent::OnTerrainMacroMaterialDestroyed(AZ::EntityId macroMaterialEntity)
    {
        // This notification gets broadcast to *all* entities, so make sure it belongs to this one before refreshing.
        if (macroMaterialEntity == m_component.GetEntityId())
        {
            RefreshPaintableAssetStatus();
        }
    }

    void EditorTerrainMacroMaterialComponent::RefreshPaintableAssetStatus()
    {
        auto imageAssetPropertyName = m_paintableMacroColorAssetHelper.Refresh(m_component.GetMacroColorAsset());

        if (imageAssetPropertyName != m_configuration.GetMacroColorAssetPropertyName())
        {
            m_configuration.SetMacroColorAssetPropertyName(imageAssetPropertyName);

            // If the asset status changed and the image asset property is visible, refresh the entire tree so
            // that the label change is picked up.
            InvalidatePropertyDisplay(AzToolsFramework::Refresh_EntireTree);
        }
        else
        {
            InvalidatePropertyDisplay(AzToolsFramework::Refresh_AttributesAndValues);
        }
    }


    void EditorTerrainMacroMaterialComponent::OnCompositionRegionChanged([[maybe_unused]] const AZ::Aabb& dirtyRegion)
    {
        // If only a region of the entity changed, we don't need to refresh anything.
        // We still need to override this callback though or else region notifications will get passed to OnCompositionChanged().
    }

    void EditorTerrainMacroMaterialComponent::OnCompositionChanged()
    {
        // On configuration changes, make sure to preserve the current asset property name status.
        auto previousMacroColorAssetPropertyName = m_configuration.GetMacroColorAssetPropertyName();
        m_component.WriteOutConfig(&m_configuration);
        m_configuration.SetMacroColorAssetPropertyName(previousMacroColorAssetPropertyName);

        SetDirty();

        RefreshPaintableAssetStatus();
    }

    AZ::u32 EditorTerrainMacroMaterialComponent::ConfigurationChanged()
    {
        // This block of code aligns with EditorWrappedComponentBase
        {
            if (m_runtimeComponentActive)
            {
                m_runtimeComponentActive = false;
                m_component.Deactivate();
            }

            m_component.ReadInConfig(&m_configuration);

            if (m_visible && !m_runtimeComponentActive)
            {
                m_component.Activate();
                m_runtimeComponentActive = true;
            }
        }

        // This OnCompositionChanged notification will refresh our own preview so we don't need to call RefreshPreview explicitly
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);

        return AZ::Edit::PropertyRefreshLevels::None;
    }

    void EditorTerrainMacroMaterialComponent::OnPaintModeBegin()
    {
        // Forward the paint brush notification to the runtime component.
        AzFramework::PaintBrushNotificationBus::Event(
            { m_component.GetEntityId(), m_component.GetId() }, &AzFramework::PaintBrushNotificationBus::Events::OnPaintModeBegin);
    }

    void EditorTerrainMacroMaterialComponent::OnPaintModeEnd()
    {
        // Forward the paint brush notification to the runtime component.
        AzFramework::PaintBrushNotificationBus::Event(
            { m_component.GetEntityId(), m_component.GetId() }, &AzFramework::PaintBrushNotificationBus::Events::OnPaintModeEnd);

        // It's possible that we're leaving component mode as the result of an "undo" action.
        // If that's the case, don't prompt the user to save the changes.
        if (!AzToolsFramework::UndoRedoOperationInProgress() && m_component.MacroColorImageIsModified())
        {
            SavePaintedData();
        }
    }

    void EditorTerrainMacroMaterialComponent::OnBrushStrokeBegin(const AZ::Color& color)
    {
        // Forward the paint brush notification to the runtime component.
        AzFramework::PaintBrushNotificationBus::Event(
            { m_component.GetEntityId(), m_component.GetId() }, &AzFramework::PaintBrushNotificationBus::Events::OnBrushStrokeBegin, color);
    }

    void EditorTerrainMacroMaterialComponent::OnBrushStrokeEnd()
    {
        // Forward the paint brush notification to the runtime component.
        AzFramework::PaintBrushNotificationBus::Event(
            { m_component.GetEntityId(), m_component.GetId() }, &AzFramework::PaintBrushNotificationBus::Events::OnBrushStrokeEnd);
    }

    void EditorTerrainMacroMaterialComponent::OnPaint(
        const AZ::Color& color, const AZ::Aabb& dirtyArea, ValueLookupFn& valueLookupFn, BlendFn& blendFn)
    {
        // Forward the paint brush notification to the runtime component.
        AzFramework::PaintBrushNotificationBus::Event(
            { m_component.GetEntityId(), m_component.GetId() },
            &AzFramework::PaintBrushNotificationBus::Events::OnPaint,
            color,
            dirtyArea,
            valueLookupFn,
            blendFn);
    }

    void EditorTerrainMacroMaterialComponent::OnSmooth(
        const AZ::Color& color,
        const AZ::Aabb& dirtyArea,
        ValueLookupFn& valueLookupFn,
        AZStd::span<const AZ::Vector3> valuePointOffsets,
        SmoothFn& smoothFn)
    {
        // Forward the paint brush notification to the runtime component.
        AzFramework::PaintBrushNotificationBus::Event(
            { m_component.GetEntityId(), m_component.GetId() },
            &AzFramework::PaintBrushNotificationBus::Events::OnSmooth,
            color,
            dirtyArea,
            valueLookupFn,
            valuePointOffsets,
            smoothFn);
    }

    AZ::Color EditorTerrainMacroMaterialComponent::OnGetColor(const AZ::Vector3& brushCenter) const
    {
        AZ::Color result;

        // Forward the paint brush notification to the runtime component.
        AzFramework::PaintBrushNotificationBus::EventResult(
            result,
            { m_component.GetEntityId(), m_component.GetId() },
            &AzFramework::PaintBrushNotificationBus::Events::OnGetColor,
            brushCenter);

        return result;
    }

    bool EditorTerrainMacroMaterialComponent::SavePaintedData()
    {
        const GradientSignal::OutputFormat format = GradientSignal::OutputFormat::R8G8B8A8;

        // Get the resolution of our modified image.
        const auto imageResolution = m_component.GetMacroColorImageSize();

        // Get the image modification buffer
        auto pixelBuffer = m_component.GetMacroColorImageModificationBuffer();

        // The image is stored in memory in linear color space, but the source asset that we write out needs to be in SRGB color space.
        AZStd::vector<uint8_t> rawPixelData = ConvertLinearToSrgbGamma(pixelBuffer);

        auto createdAsset = m_paintableMacroColorAssetHelper.SaveImage(
            imageResolution.m_width, imageResolution.m_height, format, rawPixelData);

        if (createdAsset)
        {
            // Set the active image to the created one.
            m_component.SetMacroColorAsset(createdAsset.value());

            OnCompositionChanged();
        }

        return createdAsset.has_value();
    }

    AZStd::vector<uint8_t> EditorTerrainMacroMaterialComponent::ConvertLinearToSrgbGamma(AZStd::span<const uint32_t> pixelBuffer) const
    {
        AZStd::array<uint8_t, 256> linearToSrgbGamma;
        const float uint8Max = static_cast<float>(AZStd::numeric_limits<uint8_t>::max());

        // Build up a color conversion array.
        for (size_t i = 0; i < linearToSrgbGamma.array_size; ++i)
        {
            float linearValue = i / uint8Max;
            linearToSrgbGamma[i] = aznumeric_cast<uint8_t>(AZ::Color::ConvertSrgbLinearToGamma(linearValue) * uint8Max);
        }

        AZStd::vector<uint8_t> convertedPixels;
        convertedPixels.reserve(pixelBuffer.size() * sizeof(uint32_t));

        union ColorBytes
        {
            struct
            {
                uint8_t m_red;
                uint8_t m_green;
                uint8_t m_blue;
                uint8_t m_alpha;
            };
            uint32_t m_rgba;
        };

        // The pixel buffer consists of R8G8B8A8 values. We'll take each byte individually and convert it from linear to gamma,
        // with the exception of the alpha byte, which should remain as-is.
        for (auto pixel : pixelBuffer)
        {
            const ColorBytes *pixelBytes = reinterpret_cast<const ColorBytes*>(&pixel);
            convertedPixels.push_back(linearToSrgbGamma[pixelBytes->m_red]);
            convertedPixels.push_back(linearToSrgbGamma[pixelBytes->m_green]);
            convertedPixels.push_back(linearToSrgbGamma[pixelBytes->m_blue]);
            convertedPixels.push_back(pixelBytes->m_alpha);
        }

        return convertedPixels;
    }
} // namespace Terrain
