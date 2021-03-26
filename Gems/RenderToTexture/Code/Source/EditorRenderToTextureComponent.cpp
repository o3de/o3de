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

#include "RenderToTexture_precompiled.h"

#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED

#include "EditorRenderToTextureComponent.h"
#include "RenderToTextureComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Math/Transform.h>
#include <MathConversion.h>
#include <I3DEngine.h>
#include <IViewSystem.h>
#include <AzFramework/Components/CameraBus.h>
#include <RTTBus.h>
#include <CryName.h>

#include <QApplication>
#include <QMessageBox>

namespace AzRTT
{
    /**
    * AA type
    */
    enum class AAType
    {
        None = 0,
        FXAA,
        SMAA1TX
    };

    AZ::Crc32 RenderContextConfig::GetShadowSettingsVisible()
    {
        return m_shadowsEnabled ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }

    bool RenderContextConfig::ValidateTextureSize(void* newValue, const AZ::Uuid& valueType)
    {
        if (azrtti_typeid<uint32_t>() != valueType)
        {
            AZ_Assert(false, "Unexpected value type");
            return false;
        }

        uint32_t* newTextureSize = static_cast<uint32_t*>(newValue);
        AZ_Assert(newTextureSize, "Texture size parameter is null in ValidateTextureSize");

        if (*newTextureSize > AzRTT::MaxRecommendedRenderTargetSize)
        {
            if (QMessageBox::Cancel == QMessageBox::warning(QApplication::activeWindow(),
                QObject::tr("Large Texture Size"),
                QObject::tr("Large texture sizes can lead to excess memory usage, low performance and instability.  Do you want to continue?"),
                QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel))
            {
                return false;
            }
        }

        return true;
    }
}

namespace RenderToTexture
{
    void EditorRenderToTextureConfig::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<EditorRenderToTextureConfig, RenderToTextureConfig>()
                ->Version(1)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<AzRTT::RenderContextConfig>("RenderContext", "RenderContext")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->Attribute(AZ::Edit::Attributes::Category, "Rendering")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                    // Required settings
                    ->DataElement(0, &AzRTT::RenderContextConfig::m_width, "Width", "Texture width")
                        ->Attribute(AZ::Edit::Attributes::ChangeValidate, &AzRTT::RenderContextConfig::ValidateTextureSize)
                        ->Attribute(AZ::Edit::Attributes::Min, AzRTT::MinRenderTargetWidth)
                    ->DataElement(0, &AzRTT::RenderContextConfig::m_height, "Height", "Texture height")
                        ->Attribute(AZ::Edit::Attributes::ChangeValidate, &AzRTT::RenderContextConfig::ValidateTextureSize)
                        ->Attribute(AZ::Edit::Attributes::Min, AzRTT::MinRenderTargetHeight)
                    ->DataElement(0, &AzRTT::RenderContextConfig::m_sRGBWrite, "Apply Gamma", "")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &AzRTT::RenderContextConfig::m_alphaMode, "Alpha Mode", "")
                        ->EnumAttribute(AzRTT::AlphaMode::ALPHA_OPAQUE, "Opaque")
                        ->EnumAttribute(AzRTT::AlphaMode::ALPHA_DEPTH_BASED, "Depth Based")

                    // Scene Settings
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Scene Settings")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ->DataElement(0, &AzRTT::RenderContextConfig::m_oceanEnabled, "Enable Ocean", "")
                    ->DataElement(0, &AzRTT::RenderContextConfig::m_terrainEnabled, "Enable Terrain", "")
                    ->DataElement(0, &AzRTT::RenderContextConfig::m_vegetationEnabled, "Enable Vegetation", "")
                    ->DataElement(0, &AzRTT::RenderContextConfig::m_shadowsEnabled, "Enable Shadows", "")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                    ->DataElement(0, &AzRTT::RenderContextConfig::m_shadowsNumCascades, "GSM LODs", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &AzRTT::RenderContextConfig::GetShadowSettingsVisible)
                        ->Attribute(AZ::Edit::Attributes::Min, -1)
                    ->DataElement(0, &AzRTT::RenderContextConfig::m_shadowsGSMRange, "GSM range", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &AzRTT::RenderContextConfig::GetShadowSettingsVisible)
                        ->Attribute(AZ::Edit::Attributes::Min, -1)
                    ->DataElement(0, &AzRTT::RenderContextConfig::m_shadowsGSMRangeStep, "GSM range step", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &AzRTT::RenderContextConfig::GetShadowSettingsVisible)
                        ->Attribute(AZ::Edit::Attributes::Min, -1)

                    // Post Effects
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Post Effects")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &AzRTT::RenderContextConfig::m_aaMode, "Antialiasing Mode", "")
                        ->EnumAttribute(AzRTT::AAType::None, "None")
                        ->EnumAttribute(AzRTT::AAType::FXAA, "FXAA")
                    ;

                editContext->Class<RenderToTextureConfig>("EditorRenderToTextureConfig", "Editor Config for RenderToTextureConfig")
                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::Category, "Rendering")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                // Required settings
                ->DataElement(0, &RenderToTextureConfig::m_camera, "Camera", "Optional camera to use")
                ->DataElement(0, &RenderToTextureConfig::m_textureName, "Texture name", "Name of texture to render to")
                ->DataElement(0, &RenderToTextureConfig::m_maxFPS, "Max FPS", "Maximum FPS limit, or 0 for no limit.")
                ->DataElement(0, &RenderToTextureConfig::m_renderContextConfig, "Render Context Config", "Render Context Config")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                ->ClassElement(AZ::Edit::ClassElements::Group, "Debug")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                ->DataElement(0, &RenderToTextureConfig::m_displayDebugImage, "Display Debug Image", "Display an image of the render target in the main viewport.");
            }
        }
    }

    void EditorRenderToTextureComponent::Reflect(AZ::ReflectContext* reflection)
    {
        EditorRenderToTextureConfig::Reflect(reflection);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<EditorRenderToTextureComponent, AzToolsFramework::Components::EditorComponentBase>()
                ->Version(1)
                ->Field("Config", &EditorRenderToTextureComponent::m_config)
                ->Field("Update In Editor", &EditorRenderToTextureComponent::m_updateInEditor)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorRenderToTextureComponent>("Render to Texture", "Render the world to a texture")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Rendering")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/RenderToTexture.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/RenderToTexture.png")
                        ->Attribute(AZ::Edit::Attributes::PreferNoViewportIcon, true)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "http://docs.aws.amazon.com/console/lumberyard/userguide/render-to-texture-component")
                    ->DataElement(0, &EditorRenderToTextureComponent::m_config, "Config", "Render To Texture Configuration")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorRenderToTextureComponent::ConfigurationChanged)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(0, &EditorRenderToTextureComponent::m_updateInEditor, "Update in editor", "If enabled, the render texture will update every frame while in editor mode.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorRenderToTextureComponent::ConfigurationChanged)
                    ;
            }
        }
    }

    void EditorRenderToTextureComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        auto component = gameEntity->CreateComponent<RenderToTextureComponent>();
        if (component)
        {
            component->SetConfiguration(m_config);
        }
    }

    void EditorRenderToTextureComponent::Activate()
    {
        // Default to using the entity this component is assigned to.
        // This makes it obvious to the user where the camera data is coming from.
        if (!m_config.m_camera.IsValid())
        {
            m_config.m_camera = GetEntityId();
        }

        AzRTT::RTTRequestBus::BroadcastResult(m_config.m_renderContextId, &AzRTT::RTTRequestBus::Events::CreateContext, m_config.m_renderContextConfig);
        if (m_config.m_renderContextId.IsNull())
        {
            AZ_Printf("EditorRenderToTextureComponent", "$2Failed to create render context.");
        }
        else
        {
            m_nextRefreshTime = 0.0;

            ConfigurationChanged();

            ValidateCVars();
        }

        if (!m_bRenderDebugDrawRegistered)
        {
            gEnv->pRenderer->AddRenderDebugListener(this);
            m_bRenderDebugDrawRegistered = true;
        }
    }

    int EditorRenderToTextureComponent::GetTickOrder()
    {
        return AZ::TICK_LAST;
    }

    void EditorRenderToTextureComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        // we can do this in the editor because it is single threaded
        Render(m_renderTargetHandle, m_config, GetEntityId());
    }

    void EditorRenderToTextureComponent::UpdateRenderTarget(const AZStd::string& textureName, int width, int height)
    {
        if (textureName.empty() || width < AzRTT::MinRenderTargetWidth || height < AzRTT::MinRenderTargetHeight )
        {
            return;
        }

        // verify this name isn't already in use
        ITexture* texture = gEnv->pRenderer->EF_GetTextureByName(m_config.m_textureName.c_str());
        if (texture != nullptr)
        {
            // make sure we're not already using this target
            if (texture->GetTextureID() != m_renderTargetHandle)
            {
                // we can re-use an existing render target
                if (texture->GetFlags() & FT_USAGE_RENDERTARGET)
                {
                    ReleaseCurrentRenderTarget();
                    m_renderTargetHandle = texture->GetTextureID();

                    // increment the reference count because we will call DestroyRenderTarget
                    // which will attempt to release this CTexture
                    texture->AddRef();
                }
                else if (!gEnv->pRenderer->IsTextureExist(texture))
                {
                    // it is possible that a CTexture was created because it was part of a material
                    // but no file exists for it.  Calling CreateRenderTarget will attempt to
                    // convert it to a render target and increment the CTexture reference count
                    ReleaseCurrentRenderTarget();
                    m_renderTargetHandle = gEnv->pRenderer->CreateRenderTarget(textureName.c_str(), width, height, Clr_Unknown, eTF_R8G8B8A8);
                    AZ_Warning("EditorRenderToTextureComponent", m_renderTargetHandle != INVALID_RENDER_TARGET , "$4Failed to create render target %s.", textureName.c_str());
                }
                else
                {
                    AZ_Warning("EditorRenderToTextureComponent", false, "$2The name %s is already in use by a texture that is not a valid render target.", textureName.c_str());
                }
            }
            else if (texture->GetWidth() != width || texture->GetHeight() != height)
            {
                gEnv->pRenderer->ResizeRenderTarget(m_renderTargetHandle, width, height);
            }
        }
        else
        {
            ReleaseCurrentRenderTarget();
            m_renderTargetHandle = gEnv->pRenderer->CreateRenderTarget(textureName.c_str(), width, height, Clr_Unknown, eTF_R8G8B8A8);
            AZ_Warning("EditorRenderToTextureComponent", m_renderTargetHandle != INVALID_RENDER_TARGET , "$4Failed to create render target %s.", textureName.c_str());
        }
    }

    void EditorRenderToTextureComponent::ReleaseCurrentRenderTarget()
    {
        if (m_renderTargetHandle != INVALID_RENDER_TARGET)
        {
            gEnv->pRenderer->DestroyRenderTarget(m_renderTargetHandle);
            m_renderTargetHandle = INVALID_RENDER_TARGET;
        }
    }

    void EditorRenderToTextureComponent::ConfigurationChanged()
    {
        UpdateRenderTarget(m_config.m_textureName, m_config.m_renderContextConfig.m_width, m_config.m_renderContextConfig.m_height);

        if (m_renderTargetHandle != INVALID_RENDER_TARGET)
        {
            if (m_updateInEditor != AZ::TickBus::Handler::BusIsConnected())
            {
                if (m_updateInEditor)
                {
                    AZ::TickBus::Handler::BusConnect();
                }
                else
                {
                    AZ::TickBus::Handler::BusDisconnect();
                }
            }
        }

        // push the changes from the component's RenderContextConfig to our RenderContext
        if (!m_config.m_renderContextId.IsNull())
        {
            AzRTT::RTTRequestBus::Broadcast(&AzRTT::RTTRequestBus::Events::SetContextConfig, m_config.m_renderContextId, m_config.m_renderContextConfig);
        }
    }

    void EditorRenderToTextureComponent::Deactivate()
    {
        if (!m_config.m_renderContextId.IsNull())
        {
            AzRTT::RTTRequestBus::Broadcast(&AzRTT::RTTRequestBus::Events::DestroyContext, m_config.m_renderContextId);
        }

        ReleaseCurrentRenderTarget();

        if (AZ::TickBus::Handler::BusIsConnected())
        {
            AZ::TickBus::Handler::BusDisconnect();
        }

        if (m_bRenderDebugDrawRegistered)
        {
            gEnv->pRenderer->RemoveRenderDebugListener(this);
            m_bRenderDebugDrawRegistered = false;
        }
    }

    bool EditorRenderToTextureComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const RenderToTextureConfig*>(baseConfig))
        {
            static_cast<RenderToTextureConfig&>(m_config) = *config;
            return true;
        }
        return false;
    }

    bool EditorRenderToTextureComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto outConfig = azrtti_cast<RenderToTextureConfig*>(outBaseConfig))
        {
            *outConfig = m_config;
            return true;
        }
        return false;
    }

    void EditorRenderToTextureComponent::OnDebugDraw()
    {
        if (m_config.m_displayDebugImage)
        {
            DisplayDebugImage(m_config);
        }
    }
}
#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
