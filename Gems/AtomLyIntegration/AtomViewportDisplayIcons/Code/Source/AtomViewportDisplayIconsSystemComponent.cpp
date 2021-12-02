/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AtomViewportDisplayIconsSystemComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/std/containers/array.h>

#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/Viewport/ViewportScreen.h>

#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/DynamicDraw/DynamicDrawContext.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAssetCreator.h>
#include <Atom/RPI.Reflect/Image/ImageMipChainAssetCreator.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/RPI.Public/Image/StreamingImagePool.h>

#include <AtomBridge/PerViewportDynamicDrawInterface.h>

#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QPainter>
#include <QSvgRenderer>

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
        required.push_back(AZ_CRC("AtomBridgeService", 0xdb816a99));
    }

    void AtomViewportDisplayIconsSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    void AtomViewportDisplayIconsSystemComponent::Activate()
    {
        m_drawContextRegistered = false;

        AzToolsFramework::EditorViewportIconDisplay::Register(this);

        Bootstrap::NotificationBus::Handler::BusConnect();
    }

    void AtomViewportDisplayIconsSystemComponent::Deactivate()
    {
        Data::AssetBus::Handler::BusDisconnect();
        Bootstrap::NotificationBus::Handler::BusDisconnect();

        auto perViewportDynamicDrawInterface = AtomBridge::PerViewportDynamicDraw::Get();
        if (!perViewportDynamicDrawInterface)
        {
            return;
        } 
        if (perViewportDynamicDrawInterface && m_drawContextRegistered)
        {
            perViewportDynamicDrawInterface->UnregisterDynamicDrawContext(m_drawContextName);
            m_drawContextRegistered = false;
        }

        AzToolsFramework::EditorViewportIconDisplay::Unregister(this);
    }

    void AtomViewportDisplayIconsSystemComponent::DrawIcon(const DrawParameters& drawParameters)
    {
        // Ensure we have a valid viewport context & dynamic draw interface
        auto viewportContext = RPI::ViewportContextRequests::Get()->GetViewportContextById(drawParameters.m_viewport);
        if (viewportContext == nullptr)
        {
            return;
        }

        auto perViewportDynamicDrawInterface =
            AtomBridge::PerViewportDynamicDraw::Get();
        if (!perViewportDynamicDrawInterface)
        {
            return;
        } 

        RHI::Ptr<RPI::DynamicDrawContext> dynamicDraw =
            perViewportDynamicDrawInterface->GetDynamicDrawContextForViewport(m_drawContextName, drawParameters.m_viewport);
        if (dynamicDraw == nullptr)
        {
            return;
        }

        // Find our icon, falling back on a grey placeholder if its image is unavailable
        AZ::Data::Instance<AZ::RPI::Image> image = AZ::RPI::ImageSystemInterface::Get()->GetSystemImage(AZ::RPI::SystemImage::Grey);
        if (auto iconIt = m_iconData.find(drawParameters.m_icon); iconIt != m_iconData.end())
        {
            auto& iconData = iconIt->second;
            if (iconData.m_image)
            {
                image = iconData.m_image;
            }
        }
        else
        {
            return;
        }

        // Initialize our shader
        AZ::Vector2 viewportSize;
        {
            AzFramework::WindowSize viewportWindowSize = viewportContext->GetViewportSize();
            viewportSize = AZ::Vector2{aznumeric_cast<float>(viewportWindowSize.m_width), aznumeric_cast<float>(viewportWindowSize.m_height)};
        }
        AZ::Data::Instance<AZ::RPI::ShaderResourceGroup> drawSrg = dynamicDraw->NewDrawSrg();
        drawSrg->SetConstant(m_viewportSizeIndex,viewportSize);
        drawSrg->SetImageView(m_textureParameterIndex, image->GetImageView());
        drawSrg->Compile();

        // Scale icons by screen DPI
        float scalingFactor = 1.0f;
        {
            using ViewportRequestBus = AzToolsFramework::ViewportInteraction::ViewportInteractionRequestBus;
            ViewportRequestBus::EventResult(
                scalingFactor, drawParameters.m_viewport, &ViewportRequestBus::Events::DeviceScalingFactor);
        }

        AZ::Vector3 screenPosition;
        if (drawParameters.m_positionSpace == CoordinateSpace::ScreenSpace)
        {
            screenPosition = drawParameters.m_position;
        }
        else if (drawParameters.m_positionSpace == CoordinateSpace::WorldSpace)
        {
            // Calculate our screen space position using the viewport size
            // We want this instead of RenderViewportWidget::WorldToScreen which works in QWidget virtual coordinate space
            AzFramework::ScreenPoint position = AzFramework::WorldToScreen(
                drawParameters.m_position, viewportContext->GetCameraViewMatrix(), viewportContext->GetCameraProjectionMatrix(),
                viewportSize);
            screenPosition.SetX(aznumeric_cast<float>(position.m_x));
            screenPosition.SetY(aznumeric_cast<float>(position.m_y));
        }

        struct Vertex
        {
            float m_position[3];
            AZ::u32 m_color;
            float m_uv[2];
        };
        using Indice = AZ::u16;

        // Create a vertex offset from the position to draw from based on the icon size
        // Vertex positions are in screen space coordinates
        auto createVertex = [&](float offsetX, float offsetY, float u, float v) -> Vertex
        {
            Vertex vertex;
            screenPosition.StoreToFloat3(vertex.m_position);
            vertex.m_position[0] += offsetX * drawParameters.m_size.GetX() * scalingFactor;
            vertex.m_position[1] += offsetY * drawParameters.m_size.GetY() * scalingFactor;
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
        dynamicDraw->DrawIndexed(&vertices, static_cast<uint32_t>(vertices.size()), &indices, static_cast<uint32_t>(indices.size()), RHI::IndexFormat::Uint16, drawSrg);
    }

    QString AtomViewportDisplayIconsSystemComponent::FindAssetPath(const QString& path) const
    {
        // If we get an absolute path, just use it.
        QFileInfo pathInfo(path);
        if (pathInfo.isAbsolute())
        {
            return path;
        }

        bool found = false;
        AZStd::vector<AZStd::string> scanFolders;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
            found, &AzToolsFramework::AssetSystemRequestBus::Events::GetScanFolders, scanFolders);
        if (!found)
        {
            AZ_Error("AtomViewportDisplayIconSystemComponent", false, "Failed to load asset scan folders");
            return QString();
        }

        for (const auto& folder : scanFolders)
        {
            QDir dir(folder.data());
            if (dir.exists(path))
            {
                return dir.absoluteFilePath(path);
            }
        }

        return QString();
    }

    QImage AtomViewportDisplayIconsSystemComponent::RenderSvgToImage(const QString& svgPath) const
    {
        // Set up our SVG renderer
        QSvgRenderer renderer(svgPath);
        renderer.setAspectRatioMode(Qt::KeepAspectRatio);

        // Set up our target image
        QSize size = renderer.defaultSize().expandedTo(MinimumRenderedSvgSize);
        QImage image(size, QtImageFormat);
        image.fill(0x00000000);

        // Render the SVG
        QPainter painter(&image);
        renderer.render(&painter);
        return image;
    }

    AZ::Data::Instance<AZ::RPI::Image> AtomViewportDisplayIconsSystemComponent::ConvertToAtomImage(AZ::Uuid assetId, QImage image) const
    {
        // Ensure our image is in the correct pixel format so we can memcpy it to our renderer image
        image.convertTo(QtImageFormat);
        Data::Instance<RPI::StreamingImagePool> streamingImagePool = RPI::ImageSystemInterface::Get()->GetSystemStreamingPool();
        return RPI::StreamingImage::CreateFromCpuData(
            *streamingImagePool.get(),
            RHI::ImageDimension::Image2D,
            RHI::Size(image.width(), image.height(), 1),
            RHI::Format::R8G8B8A8_UNORM_SRGB,
            image.bits(),
            image.sizeInBytes(),
            assetId);
    }

    AzToolsFramework::EditorViewportIconDisplayInterface::IconId AtomViewportDisplayIconsSystemComponent::GetOrLoadIconForPath(
        AZStd::string_view path)
    {
        // Check our cache to see if the image is already loaded
        auto existingEntryIt = AZStd::find_if(m_iconData.begin(), m_iconData.end(), [&path](const auto& iconData)
        {
            return iconData.second.m_path == path;
        });
        if (existingEntryIt != m_iconData.end())
        {
            return existingEntryIt->first;
        }

        AZ::Uuid assetId = AZ::Uuid::CreateName(path.data());

        // Find the asset to load on disk
        QString assetPath = FindAssetPath(path.data());
        if (assetPath.isEmpty())
        {
            AZ_Error("AtomViewportDisplayIconSystemComponent", false, "Failed to locate icon on disk: \"%s\"", path.data());
            return InvalidIconId;
        }

        QImage loadedImage;

        AZStd::string extension;
        AzFramework::StringFunc::Path::GetExtension(path.data(), extension, false);
        // For SVGs, we need to actually rasterize to an image
        if (extension == "svg")
        {
            loadedImage = RenderSvgToImage(assetPath);
        }
        // For everything else, we can just load it through QImage via its image plugins
        else
        {
            const bool loaded = loadedImage.load(assetPath);
            if (!loaded)
            {
                AZ_Error("AtomViewportDisplayIconSystemComponent", false, "Failed to load icon: \"%s\"", assetPath.toUtf8().constData());
                return InvalidIconId;
            }
        }

        // Cache our loaded icon
        IconId id = m_currentId++;
        IconData& iconData = m_iconData[id];
        iconData.m_path = path;
        iconData.m_image = ConvertToAtomImage(assetId, loadedImage);
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
        return IconLoadStatus::Error;
    }

    void AtomViewportDisplayIconsSystemComponent::OnBootstrapSceneReady([[maybe_unused]]AZ::RPI::Scene* bootstrapScene)
    {
        // Queue a load for the draw context shader, and wait for it to load
        Data::Asset<RPI::ShaderAsset> shaderAsset = RPI::AssetUtils::GetAssetByProductPath<RPI::ShaderAsset>(DrawContextShaderPath, RPI::AssetUtils::TraceLevel::Assert);
        shaderAsset.QueueLoad();
        Data::AssetBus::Handler::BusConnect(shaderAsset.GetId());
    }

    void AtomViewportDisplayIconsSystemComponent::OnAssetReady(Data::Asset<Data::AssetData> asset)
    {
        // Once the shader is loaded, register it with the dynamic draw context
        Data::Asset<RPI::ShaderAsset> shaderAsset = asset;
        AtomBridge::PerViewportDynamicDraw::Get()->RegisterDynamicDrawContext(m_drawContextName, [shaderAsset](RPI::Ptr<RPI::DynamicDrawContext> drawContext)
            {
                AZ_Assert(shaderAsset->IsReady(), "Attempting to register the AtomViewportDisplayIconsSystemComponent"
                    " dynamic draw context before the shader asset is loaded. The shader should be loaded first"
                    " to avoid a blocking asset load and potential deadlock, since the DynamicDrawContext lambda"
                    " will be executed during scene processing and there may be multiple scenes executing in parallel.");

                Data::Instance<RPI::Shader> shader = RPI::Shader::FindOrCreate(shaderAsset);
                drawContext->InitShader(shader);
                drawContext->InitVertexFormat(
                    { {"POSITION", RHI::Format::R32G32B32_FLOAT},
                     {"COLOR", RHI::Format::R8G8B8A8_UNORM},
                     {"TEXCOORD", RHI::Format::R32G32_FLOAT} });
                drawContext->EndInit();
            });

        m_drawContextRegistered = true;

        Data::AssetBus::Handler::BusDisconnect();
    }
} // namespace AZ::Render
