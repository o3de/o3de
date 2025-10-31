/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AtomViewportDisplayIconsSystemComponent.h"

#include <AzCore/Math/VectorConversions.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/std/containers/array.h>

#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/Viewport/ViewportScreen.h>

#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

#include <AtomCore/Instance/Instance.h>

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
                        ->Attribute(Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void AtomViewportDisplayIconsSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("ViewportDisplayIconsService"));
    }

    void AtomViewportDisplayIconsSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("ViewportDisplayIconsService"));
    }

    void AtomViewportDisplayIconsSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("RPISystem"));
        required.push_back(AZ_CRC_CE("AtomBridgeService"));
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

        AZ::AtomBridge::PerViewportDynamicDrawInterface* perViewportDynamicDrawInterface = AtomBridge::PerViewportDynamicDraw::Get();
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

    void AtomViewportDisplayIconsSystemComponent::AddIcon(const DrawParameters& drawParameters)
    {
        if (drawParameters.m_viewport == AzFramework::InvalidViewportId)
        {
            AZ_WarningOnce("AtomViewportDisplayIconsSystemComponent", false, "Invalid viewport ID provided for icon draw request, discarded.");
            return;
        }

        if (m_drawRequestViewportId == AzFramework::InvalidViewportId)
        {
            // this is the first request, initialize m_drawRequestViewportId.
            m_drawRequestViewportId = drawParameters.m_viewport;
        }
        else if (m_drawRequestViewportId != drawParameters.m_viewport)
        {
            AZ_WarningOnce("AtomViewportDisplayIconsSystemComponent", false, "Multiple viewports provided for a single icon draw batch, discarded.");
            return;
        }
        m_drawRequests[drawParameters.m_icon].emplace_back(drawParameters);

        // the maximum we can batch at a time would be the largest index that can fit into a u16, (65535), and it eats 4 index values per icon
        // since the indices go (0, 1, 2, 0, 2, 3), ie, 2 triangles making up 6 indices per quad but only using four actual index numbers (0,1,2,3) per.
        // So we can only batch (max_uint16 / 4) icons at a time before the u16 would overflow (about 16k icons).
        constexpr AZStd::vector<IconIndexData>::size_type maxQuads = (AZStd::numeric_limits<IconIndexData>::max() / 4) - 1;
        if (m_drawRequests[drawParameters.m_icon].size() >= maxQuads)
        {
            DrawIcons(); // flush all buffers immediately.
        }
    }

    // create a SRG specific to the viewport dimensions and icon.
    AZ::Data::Instance<AZ::RPI::ShaderResourceGroup> AtomViewportDisplayIconsSystemComponent::CreateIconSRG(AzFramework::ViewportId viewportId, AZ::Data::Instance<AZ::RPI::Image> image)
    {
        using namespace AZ;

        RHI::Ptr<RPI::DynamicDrawContext> dynamicDraw = GetDynamicDrawContextForViewport(m_drawRequestViewportId);

        AZ::RPI::ViewportContextPtr viewportContext = RPI::ViewportContextRequests::Get()->GetViewportContextById(viewportId);
        if (viewportContext == nullptr)
        {
            return {};
        }

        const auto [viewportWidth, viewportHeight] = viewportContext->GetViewportSize();
        const auto viewportSize = AzFramework::ScreenSize(viewportWidth, viewportHeight);

        AZ::Data::Instance<AZ::RPI::ShaderResourceGroup> drawSrg = dynamicDraw->NewDrawSrg();
        drawSrg->SetConstant(m_viewportSizeIndex, AzFramework::Vector2FromScreenSize(viewportSize));
        drawSrg->SetImageView(m_textureParameterIndex, image->GetImageView());
        drawSrg->Compile();
        return drawSrg;
    }

    RHI::Ptr<RPI::DynamicDrawContext> AtomViewportDisplayIconsSystemComponent::GetDynamicDrawContextForViewport(AzFramework::ViewportId viewportId)
    {
        AZ::AtomBridge::PerViewportDynamicDrawInterface* perViewportDynamicDrawInterface = AtomBridge::PerViewportDynamicDraw::Get();
        if (!perViewportDynamicDrawInterface)
        {
            return {};
        }
        return perViewportDynamicDrawInterface->GetDynamicDrawContextForViewport(m_drawContextName, viewportId);
    }

    AZ::Data::Instance<AZ::RPI::Image> AtomViewportDisplayIconsSystemComponent::GetImageForIconId(IconId iconId)
    {
        if (auto iconIt = m_iconData.find(iconId); iconIt != m_iconData.end())
        {
            auto& iconData = iconIt->second;
            if (iconData.m_image)
            {
                return iconData.m_image;
            }
        }

        return AZ::RPI::ImageSystemInterface::Get()->GetSystemImage(AZ::RPI::SystemImage::Grey);
    }

    void AtomViewportDisplayIconsSystemComponent::DrawIcons()
    {
        // the strategy for drawing icons here is to do the expensive stuff once, and then draw all of the icons using the same texture
        // in one go.

        // To achieve this, we initialize all the variables that are per-viewport just one time in this function,
        // then we initialize the variables that are per-texture just once per texture,
        // then we build the vertex list once per texture by accumulating all the quads.
        // Note that the index cache is a special case - becuase the indexes for quads are always 0,1,2, 0,2,3, etc, we don't need to update
        // them every frame, just make sure that the index cache has ENOUGH initialized data for the amount of quads we intend to render.
        // This allows us to re-use the index cache even between viewports and textures, the only rapidly changing data is the vertex data,
        // and we store that in a vector so that its memory stays stable.

        using ViewportRequestBus = AzToolsFramework::ViewportInteraction::ViewportInteractionRequestBus;
        
        if (m_drawRequestViewportId == AzFramework::InvalidViewportId)
        {
            // its possible for the hash map to have entries in it (since they represent texture slots) with no currently rendering quads.
            return;
        }

        if (m_drawRequests.empty())
        {
            return;
        }

        AZ_Assert(m_drawRequestViewportId != AzFramework::InvalidViewportId, "Viewport ID is somehow invalid despite icons being in the list.");

        RHI::Ptr<RPI::DynamicDrawContext> dynamicDraw = GetDynamicDrawContextForViewport(m_drawRequestViewportId);
        AZ::RPI::ViewportContextPtr viewportContext = RPI::ViewportContextRequests::Get()->GetViewportContextById(m_drawRequestViewportId);
        if ((viewportContext == nullptr) || (dynamicDraw == nullptr))
        {
            // this is not an error or assert as we might be running headlessly.
            m_drawRequests.clear();
            m_drawRequestViewportId = AzFramework::InvalidViewportId;
            return;
        }
        // Scale icons by screen DPI
        float scalingFactor = 1.0f;
        ViewportRequestBus::EventResult(scalingFactor, m_drawRequestViewportId, &ViewportRequestBus::Events::DeviceScalingFactor);

        const auto [viewportWidth, viewportHeight] = viewportContext->GetViewportSize();
        const auto viewportSize = AzFramework::ScreenSize(viewportWidth, viewportHeight);

        for (auto &[iconId, drawIconRequests] : m_drawRequests)
        {
            // Find our icon, falling back on a gray placeholder if its image is unavailable
            if (drawIconRequests.empty())
            {
                continue;
            }

            AZ::Data::Instance<AZ::RPI::Image> image = GetImageForIconId(iconId);
            AZ::Data::Instance<AZ::RPI::ShaderResourceGroup> drawSrg = CreateIconSRG(m_drawRequestViewportId, image);

            // add all of the icons to draw buffers.
            m_vertexCache.clear();
            m_vertexCache.reserve(drawIconRequests.size() * 4);
            
            float minZ = aznumeric_cast<float>(AZStd::numeric_limits<int64_t>::max());
            float maxZ = aznumeric_cast<float>(AZStd::numeric_limits<int64_t>::min());

            for (const DrawParameters& drawParameters : drawIconRequests)
            {
                AZ::Vector3 screenPosition;
                if (drawParameters.m_positionSpace == CoordinateSpace::ScreenSpace)
                {
                    screenPosition = drawParameters.m_position;
                }
                else if (drawParameters.m_positionSpace == CoordinateSpace::WorldSpace)
                {
                    // Calculate the ndc point (0.0-1.0 range) including depth
                    const AZ::Vector3 ndcPoint = AzFramework::WorldToScreenNdc(
                        drawParameters.m_position, viewportContext->GetCameraViewMatrixAsMatrix3x4(),
                        viewportContext->GetCameraProjectionMatrix());

                    // Calculate our screen space position using the viewport size
                    // We want this instead of RenderViewportWidget::WorldToScreen which works in QWidget virtual coordinate space
                    const AzFramework::ScreenPoint screenPoint = AzFramework::ScreenPointFromNdc(AZ::Vector3ToVector2(ndcPoint), viewportSize);
                    screenPosition = AzFramework::Vector3FromScreenPoint(screenPoint, ndcPoint.GetZ());
                }
                minZ = AZStd::GetMin(minZ, screenPosition.GetZ());
                maxZ = AZStd::GetMin(maxZ, screenPosition.GetZ());

                // Create a vertex offset from the position to draw from based on the icon size
                // Vertex positions are in screen space coordinates
                auto createVertex = [&](float offsetX, float offsetY, float u, float v) -> IconVertexData
                {
                    IconVertexData vertex;
                    screenPosition.StoreToFloat3(vertex.m_position);
                    vertex.m_position[0] += offsetX * drawParameters.m_size.GetX() * scalingFactor;
                    vertex.m_position[1] += offsetY * drawParameters.m_size.GetY() * scalingFactor;
                    vertex.m_color = drawParameters.m_color.ToU32();
                    vertex.m_uv[0] = u;
                    vertex.m_uv[1] = v;
                    return vertex;
                };

                m_vertexCache.emplace_back(createVertex(-0.5f, -0.5f, 0.f, 0.f));
                m_vertexCache.emplace_back(createVertex(0.5f,  -0.5f, 1.f, 0.f));
                m_vertexCache.emplace_back(createVertex(0.5f,  0.5f,  1.f, 1.f));
                m_vertexCache.emplace_back(createVertex(-0.5f, 0.5f,  0.f, 1.f));
            }

            if (!m_vertexCache.empty())
            {
                // the indexes are always the same (0,1,2,0,2,3, 4,5,6,4,6,7, etc) and thus don't need to be updated unless more quads are added
                using IndexCacheSize = AZStd::vector<IconIndexData>::size_type;

                IndexCacheSize numQuadsInVertexBuffer = m_vertexCache.size() / 4;
                IndexCacheSize numIndicesRequired = numQuadsInVertexBuffer * 6;

                IndexCacheSize currentIndexCacheSize = m_indexCache.size();
                if (currentIndexCacheSize < numIndicesRequired)
                {
                    m_indexCache.resize_no_construct(numIndicesRequired);
                    IconIndexData baseIndex = aznumeric_cast<IconIndexData>(currentIndexCacheSize / 6);
                    while (currentIndexCacheSize < numIndicesRequired)
                    {
                        m_indexCache[currentIndexCacheSize++] = (baseIndex * 4) + 0;
                        m_indexCache[currentIndexCacheSize++] = (baseIndex * 4) + 1;
                        m_indexCache[currentIndexCacheSize++] = (baseIndex * 4) + 2;
                        m_indexCache[currentIndexCacheSize++] = (baseIndex * 4) + 0;
                        m_indexCache[currentIndexCacheSize++] = (baseIndex * 4) + 2;
                        m_indexCache[currentIndexCacheSize++] = (baseIndex * 4) + 3;
                        ++baseIndex;
                    }
                }

                dynamicDraw->SetSortKey((maxZ - minZ) * 0.5f  * aznumeric_cast<float>(AZStd::numeric_limits<int64_t>::max()));
                dynamicDraw->DrawIndexed( m_vertexCache.data(), static_cast<uint32_t>(m_vertexCache.size()),
                                         m_indexCache.data(), static_cast<uint32_t>(numIndicesRequired), RHI::IndexFormat::Uint16,
                    drawSrg);
            }
            drawIconRequests.clear(); // note - we don't remove the key, we just clear the vector and keep the memory allocated.
        }
        m_drawRequestViewportId = AzFramework::InvalidViewportId;
    }


    void AtomViewportDisplayIconsSystemComponent::DrawIcon(const DrawParameters& drawParameters)
    {
        AddIcon(drawParameters);
        // Be careful when using this method as it does not support batching.
        // Prefer using AddIcon, AddIcon, AddIcon, ..., DrawIcons() to render them in a batch.
        DrawIcons();
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
        AtomBridge::PerViewportDynamicDraw::Get()->RegisterDynamicDrawContext(m_drawContextName, [shaderAsset](RPI::Ptr<RPI::DynamicDrawContext> dynamicDraw)
            {
                AZ_Assert(shaderAsset->IsReady(), "Attempting to register the AtomViewportDisplayIconsSystemComponent"
                    " dynamic draw context before the shader asset is loaded. The shader should be loaded first"
                    " to avoid a blocking asset load and potential deadlock, since the DynamicDrawContext lambda"
                    " will be executed during scene processing and there may be multiple scenes executing in parallel.");

                Data::Instance<RPI::Shader> shader = RPI::Shader::FindOrCreate(shaderAsset);
                dynamicDraw->InitShader(shader);
                dynamicDraw->InitVertexFormat({ { "POSITION", RHI::Format::R32G32B32_FLOAT },
                                                { "COLOR", RHI::Format::R8G8B8A8_UNORM },
                                                { "TEXCOORD", RHI::Format::R32G32_FLOAT } });
                dynamicDraw->EndInit();
            });

        m_drawContextRegistered = true;

        Data::AssetBus::Handler::BusDisconnect();
    }
} // namespace AZ::Render
