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

#include "AtomViewportDisplayIconsSystemComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/std/containers/array.h>

#include <AzFramework/Asset/AssetSystemBus.h>

#include <AzToolsFramework/Viewport/ViewportMessages.h>

#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/DynamicDraw/DynamicDrawContext.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>

namespace AZ::Render
{
    void AtomViewportDisplayIconsSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<AtomViewportDisplayIconsSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<AtomViewportDisplayIconsSystemComponent>("Viewport Display Icons", "Provides an interface for drawing simple icons to the Editor viewport")
                    ->ClassElement(Edit::ClassElements::EditorData, "")
                        ->Attribute(Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                        ->Attribute(Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void AtomViewportDisplayIconsSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("ViewportDisplayIconsService"));
    }

    void AtomViewportDisplayIconsSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("ViewportDisplayIconsService"));
    }

    void AtomViewportDisplayIconsSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("RPISystem", 0xf2add773));
    }

    void AtomViewportDisplayIconsSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    void AtomViewportDisplayIconsSystemComponent::Activate()
    {
        AzToolsFramework::EditorViewportIconDisplay::Register(this);

        Bootstrap::NotificationBus::Handler::BusConnect();
    }

    void AtomViewportDisplayIconsSystemComponent::Deactivate()
    {
        Bootstrap::NotificationBus::Handler::BusDisconnect();

        auto dynamicDrawInterface =
            Interface<RPI::DynamicDrawInterface>::Get();
        if (dynamicDrawInterface)
        {
            dynamicDrawInterface->UnregisterPerViewportDynamicDrawContext(m_drawContextName);
        }

        AzToolsFramework::EditorViewportIconDisplay::Unregister(this);
    }

    void AtomViewportDisplayIconsSystemComponent::DrawIcon(const DrawParameters& drawParameters)
    {
        auto viewportContext = RPI::ViewportContextRequests::Get()->GetViewportContextById(drawParameters.m_viewport);
        if (viewportContext == nullptr)
        {
            return;
        }
        auto view = viewportContext->GetDefaultView();

        auto dynamicDrawInterface =
            Interface<RPI::DynamicDrawInterface>::Get();
        if (!dynamicDrawInterface)
        {
            return;
        } 

        RHI::Ptr<RPI::DynamicDrawContext> dynamicDraw =
            dynamicDrawInterface->GetDynamicDrawContextForViewport(m_drawContextName, drawParameters.m_viewport);
        if (dynamicDraw == nullptr)
        {
            return;
        }

        AZ::Data::Instance<AZ::RPI::ShaderResourceGroup> drawSrg = dynamicDraw->NewDrawSrg();
        if (!m_shaderIndexesInitialized)
        {
            auto layout = drawSrg->GetAsset()->GetLayout();
            m_textureParameterIndex = layout->FindShaderInputImageIndex(AZ::Name("m_texture"));
            m_viewportSizeIndex = layout->FindShaderInputConstantIndex(AZ::Name("m_viewportSize"));

            m_shaderIndexesInitialized =
                m_textureParameterIndex.IsValid() &&
                m_viewportSizeIndex.IsValid();
            if (!m_shaderIndexesInitialized)
            {
                return;
            }
        }

        AZ::Data::Instance<AZ::RPI::Image> image = AZ::RPI::ImageSystemInterface::Get()->GetSystemImage(AZ::RPI::SystemImage::Grey);
        if (auto iconIt = m_iconData.find(drawParameters.m_icon); iconIt != m_iconData.end())
        {
            auto& iconData = iconIt->second;
            if (!iconData.m_image)
            {
                if (iconData.m_asset.IsReady())
                {
                    iconData.m_image = AZ::RPI::StreamingImage::FindOrCreate(iconData.m_asset); 
                }
            }

            if (iconData.m_image)
            {
                image = iconData.m_image;
            }
        }

        auto viewportSize = viewportContext->GetViewportSize();
        drawSrg->SetConstant(m_viewportSizeIndex, AZ::Vector2(aznumeric_cast<float>(viewportSize.m_width), aznumeric_cast<float>(viewportSize.m_height)));
        drawSrg->SetImageView(m_textureParameterIndex, image->GetImageView());
        drawSrg->Compile();

        AzFramework::ScreenPoint screenPosition;
        using ViewportRequestBus = AzToolsFramework::ViewportInteraction::ViewportInteractionRequestBus;
        ViewportRequestBus::EventResult(screenPosition, drawParameters.m_viewport, &ViewportRequestBus::Events::ViewportWorldToScreen, drawParameters.m_position);

        struct Vertex
        {
            float m_position[3];
            AZ::u32 m_color;
            float m_uv[2];
        };
        using Indice = AZ::u16;

        // Create a vertex offset from the position to draw from based on the icon size
        auto createVertex = [&](float offsetX, float offsetY, float u, float v) -> Vertex
        {
            Vertex vertex;
            vertex.m_position[0] = aznumeric_cast<float>(screenPosition.m_x) + offsetX * drawParameters.m_size.GetX();
            vertex.m_position[1] = aznumeric_cast<float>(screenPosition.m_y) + offsetY * drawParameters.m_size.GetY();
            vertex.m_position[2] = 0.f;
            vertex.m_color = drawParameters.m_color.ToU32();
            vertex.m_uv[0] = u;
            vertex.m_uv[1] = v;
            return vertex;
        };

        AZStd::array<Vertex, 4> vertices = {
            createVertex(-0.5f, -0.5f, 0.f, 0.f),
            createVertex(0.5f,  -0.5f, 1.f, 0.f),
            createVertex(0.5f,  0.5f,  1.f, 1.f),
            createVertex(-0.5f, 0.5f,  0.f, 1.f)
        };
        AZStd::array<Indice, 6> indices = {0, 1, 2, 0, 2, 3};
        dynamicDraw->DrawIndexed(&vertices, vertices.size(), &indices, indices.size(), RHI::IndexFormat::Uint16, drawSrg);
    }

    // Check if a file exists. This does not go through the AssetCatalog so that it can identify files that exist but aren't processed yet,
    // and so that it will work before the AssetCatalog has loaded
    bool AtomViewportDisplayIconsSystemComponent::CheckIfFileExists(
        AZStd::string_view sourceRelativePath, AZStd::string_view cacheRelativePath) const
    {
        // If the file exists, it has already been processed and does not need to be modified
        bool fileExists = AZ::IO::FileIOBase::GetInstance()->Exists(cacheRelativePath.data());

        if (!fileExists)
        {
            // If the texture doesn't exist check if it's queued or being compiled.
            AzFramework::AssetSystem::AssetStatus status;
            AzFramework::AssetSystemRequestBus::BroadcastResult(
                status, &AzFramework::AssetSystemRequestBus::Events::GetAssetStatus, sourceRelativePath);

            switch (status)
            {
            case AzFramework::AssetSystem::AssetStatus_Queued:
            case AzFramework::AssetSystem::AssetStatus_Compiling:
            case AzFramework::AssetSystem::AssetStatus_Compiled:
            case AzFramework::AssetSystem::AssetStatus_Failed: {
                // The file is queued, in progress, or finished processing after the initial FileIO check
                fileExists = true;
                break;
            }
            case AzFramework::AssetSystem::AssetStatus_Unknown:
            case AzFramework::AssetSystem::AssetStatus_Missing:
            default: {
                // The file does not exist
                fileExists = false;
                break;
            }
            }
        }

        return fileExists;
    }

    AzToolsFramework::EditorViewportIconDisplayInterface::IconId AtomViewportDisplayIconsSystemComponent::GetOrLoadIconForPath(
        AZStd::string_view path)
    {
        AZ_Error(
            "AtomViewportDisplayIconsSystemComponent", AzFramework::StringFunc::Path::IsRelative(path.data()),
            "GetOrLoadIconForPath assumes that it will always be given a relative path, but got '%s'", path.data());

        AZStd::string sourceRelativePath(path);
        AZStd::string cacheRelativePath = sourceRelativePath + ".streamingimage";

        bool textureExists = false;
        textureExists = CheckIfFileExists(sourceRelativePath, cacheRelativePath);

        if (!textureExists)
        {
            // A lot of cry code uses the .dds extension even when the actual source file is .tif.
            // For the .streamingimage file we need the correct source extension before .streamingimage
            // So if the file doesn't exist and the extension was .dds then try replacing it with .tif
            AZStd::string extension;
            AzFramework::StringFunc::Path::GetExtension(path.data(), extension, false);
            if (extension == "dds")
            {
                sourceRelativePath = path;

                constexpr const char* textureExtensions[] = {"png", "tif", "tiff", "tga", "jpg", "jpeg", "bmp", "gif"};

                for (const char* extensionReplacement : textureExtensions)
                {
                    AzFramework::StringFunc::Path::ReplaceExtension(sourceRelativePath, extensionReplacement);
                    cacheRelativePath = sourceRelativePath + ".streamingimage";

                    textureExists = CheckIfFileExists(sourceRelativePath, cacheRelativePath);
                    if (textureExists)
                    {
                        break;
                    }
                }
            }
        }

        if (!textureExists)
        {
            AZ_Error("AtomViewportDisplayIconsSystemComponent", false, "Attempted to load '%s', but it does not exist.", path.data());
            // Since neither the given extension nor the .dds version exist, we'll default to the given extension for hot-reloading in case
            // the file is added to the source folder later
            sourceRelativePath = path.data();
            cacheRelativePath = sourceRelativePath + ".streamingimage";
        }

        // The file may not be in the AssetCatalog at this point if it is still processing or doesn't exist on disk.
        // Use GenerateAssetIdTEMP instead of GetAssetIdByPath so that it will return a valid AssetId anyways
        Data::AssetId streamingImageAssetId;
        Data::AssetCatalogRequestBus::BroadcastResult(
            streamingImageAssetId, &Data::AssetCatalogRequestBus::Events::GenerateAssetIdTEMP, sourceRelativePath.c_str());
        streamingImageAssetId.m_subId = RPI::StreamingImageAsset::GetImageAssetSubId();

        auto streamingImageAsset = Data::AssetManager::Instance().FindOrCreateAsset<RPI::StreamingImageAsset>(
            streamingImageAssetId, AZ::Data::AssetLoadBehavior::PreLoad);
        streamingImageAsset.QueueLoad();

        IconId id = m_currentId++;
        IconData& iconData = m_iconData[id];
        iconData.m_path = AZStd::move(sourceRelativePath);
        iconData.m_asset = AZStd::move(streamingImageAsset);
        return id;
    }

    AzToolsFramework::EditorViewportIconDisplayInterface::IconLoadStatus AtomViewportDisplayIconsSystemComponent::GetIconLoadStatus(
        IconId icon)
    {
        auto iconIt = m_iconData.find(icon);
        if (iconIt == m_iconData.end())
        {
            return IconLoadStatus::Unloaded;
        }
        if (iconIt->second.m_image)
        {
            return IconLoadStatus::Loaded;
        }
        const auto& asset = iconIt->second.m_asset;
        switch (asset.GetStatus())
        {
        case Data::AssetData::AssetStatus::Ready:
            return IconLoadStatus::Loaded;
        case Data::AssetData::AssetStatus::Error:
            return IconLoadStatus::Error;
        }
        return IconLoadStatus::Loading;
    }

    void AtomViewportDisplayIconsSystemComponent::OnBootstrapSceneReady([[maybe_unused]]AZ::RPI::Scene* bootstrapScene)
    {
        Interface<RPI::DynamicDrawInterface>::Get()->RegisterPerViewportDynamicDrawContext(m_drawContextName, [](RPI::Ptr<RPI::DynamicDrawContext> drawContext)
        {
            auto shader = RPI::LoadShader(s_drawContextShaderPath);
            drawContext->InitShader(shader);
            drawContext->InitVertexFormat(
                {{"POSITION", RHI::Format::R32G32B32_FLOAT},
                 {"COLOR", RHI::Format::R8G8B8A8_UNORM},
                 {"TEXCOORD", RHI::Format::R32G32_FLOAT}});
            drawContext->EndInit();
        });
    }
} // namespace AZ::Render
