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
#include <Atom/RPI.Public/Image/StreamingImagePool.h>

#include <AtomBridge/PerViewportDynamicDrawInterface.h>

#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QPainter>
#include <QSvgRenderer>

#pragma optimize("", off)

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
        AzToolsFramework::EditorViewportIconDisplay::Register(this);

        Bootstrap::NotificationBus::Handler::BusConnect();
    }

    void AtomViewportDisplayIconsSystemComponent::Deactivate()
    {
        Bootstrap::NotificationBus::Handler::BusDisconnect();

        auto perViewportDynamicDrawInterface = AtomBridge::PerViewportDynamicDraw::Get();
        if (!perViewportDynamicDrawInterface)
        {
            return;
        } 
        if (perViewportDynamicDrawInterface)
        {
            perViewportDynamicDrawInterface->UnregisterDynamicDrawContext(m_drawContextName);
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

    QString AtomViewportDisplayIconsSystemComponent::FindAssetPath(const QString& sourceRelativePath) const
    {
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
            if (dir.exists(sourceRelativePath))
            {
                return dir.absoluteFilePath(sourceRelativePath);
            }
        }

        return QString();
    }

    QImage AtomViewportDisplayIconsSystemComponent::RenderSvgToImage(const QString& svgPath) const
    {
        QSvgRenderer renderer(svgPath);
        renderer.setAspectRatioMode(Qt::KeepAspectRatio);
        QSize size = renderer.defaultSize().expandedTo(MinimumRenderedSvgSize);
        QImage image(size, QtImageFormat);
        image.fill(0x00000000);
        QPainter painter(&image);
        renderer.render(&painter);
        return image;
    }

    AZ::Data::Instance<AZ::RPI::Image> AtomViewportDisplayIconsSystemComponent::ConvertToAtomImage(AZ::Uuid assetId, QImage image) const
    {
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
        AZ_Error(
            "AtomViewportDisplayIconsSystemComponent", AzFramework::StringFunc::Path::IsRelative(path.data()),
            "GetOrLoadIconForPath assumes that it will always be given a relative path, but got '%s'", path.data());

        auto existingEntryIt = AZStd::find_if(m_iconData.begin(), m_iconData.end(), [&path](const auto& iconData)
        {
            return iconData.second.m_path == path;
        });
        if (existingEntryIt != m_iconData.end())
        {
            return existingEntryIt->first;
        }

        AZ::Uuid assetId = AZ::Uuid::CreateName(path.data());

        QString assetPath = FindAssetPath(path.data());
        if (assetPath.isEmpty())
        {
            AZ_Error("AtomViewportDisplayIconSystemComponent", false, "Failed to locate icon on disk: \"%s\"", path.data());
            return InvalidIconId;
        }

        QImage loadedImage;

        AZStd::string extension;
        AzFramework::StringFunc::Path::GetExtension(path.data(), extension, false);
        if (extension == "svg")
        {
            loadedImage = RenderSvgToImage(assetPath);
        }
        else
        {
            const bool loaded = loadedImage.load(assetPath);
            if (!loaded)
            {
                AZ_Error("AtomViewportDisplayIconSystemComponent", false, "Failed to load icon: \"%s\"", assetPath.toUtf8().constData());
                return InvalidIconId;
            }
        }

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
        AtomBridge::PerViewportDynamicDraw::Get()->RegisterDynamicDrawContext(m_drawContextName, [](RPI::Ptr<RPI::DynamicDrawContext> drawContext)
        {
            auto shader = RPI::LoadShader(DrawContextShaderPath);
            drawContext->InitShader(shader);
            drawContext->InitVertexFormat(
                {{"POSITION", RHI::Format::R32G32B32_FLOAT},
                 {"COLOR", RHI::Format::R8G8B8A8_UNORM},
                 {"TEXCOORD", RHI::Format::R32G32_FLOAT}});
            drawContext->EndInit();
        });
    }
} // namespace AZ::Render
