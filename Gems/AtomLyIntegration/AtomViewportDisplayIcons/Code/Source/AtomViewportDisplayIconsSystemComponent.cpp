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
            dynamicDrawInterface->UnregisterNamedDynamicDrawContext(m_drawContextName);
        }

        AzToolsFramework::EditorViewportIconDisplay::Unregister(this);
    }

    void AtomViewportDisplayIconsSystemComponent::DrawIcon(const DrawParameters& drawParameters)
    {
        struct Vertex
        {
            float m_position[3];
            AZ::u32 m_color;
            float m_uv[2];
        };
        using Indice = AZ::u16;

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
            dynamicDrawInterface->GetNamedDynamicDrawContext(m_drawContextName, drawParameters.m_viewport);
        if (dynamicDraw == nullptr)
        {
            return;
        }

        AZ::Data::Instance<AZ::RPI::ShaderResourceGroup> drawSrg = dynamicDraw->NewDrawSrg();
        if (!m_shaderIndexesInitialized)
        {
            auto layout = drawSrg->GetAsset()->GetLayout();
            m_textureParameterIndex = layout->FindShaderInputImageIndex(AZ::Name("m_texture"));
            m_worldToProjParameterIndex = layout->FindShaderInputConstantIndex(AZ::Name("m_worldToProj"));

            m_shaderIndexesInitialized =
                m_textureParameterIndex.IsValid() &&
                m_worldToProjParameterIndex.IsValid();
            if (!m_shaderIndexesInitialized)
            {
                return;
            }
        }

        auto image = AZ::RPI::ImageSystemInterface::Get()->GetSystemImage(AZ::RPI::SystemImage::White);
        drawSrg->SetConstant(m_worldToProjParameterIndex, view->GetWorldToClipMatrix());
        drawSrg->SetImageView(m_textureParameterIndex, image->GetImageView());
        drawSrg->Compile();

        AzFramework::ScreenPoint screenPosition;
        using ViewportRequestBus = AzToolsFramework::ViewportInteraction::ViewportInteractionRequestBus;
        ViewportRequestBus::EventResult(screenPosition, drawParameters.m_viewport, &ViewportRequestBus::Events::ViewportWorldToScreen, drawParameters.m_position);
        auto offsetToVertex = [&](float offsetX, float offsetY, float u, float v) -> Vertex
        {
            int dx = aznumeric_cast<int>(offsetX * drawParameters.m_size.GetX());
            int dy = aznumeric_cast<int>(offsetY * drawParameters.m_size.GetY());
            AzFramework::ScreenPoint offsetScreenPosition(screenPosition.m_x + dx, screenPosition.m_y + dy);
            AZStd::optional<AZ::Vector3> position;
            ViewportRequestBus::EventResult(position, drawParameters.m_viewport, &ViewportRequestBus::Events::ViewportScreenToWorld, offsetScreenPosition, 0.f);

            Vertex vertex;
            position.value().StoreToFloat3(vertex.m_position);
            vertex.m_color = drawParameters.m_color.ToU32();
            vertex.m_uv[0] = u;
            vertex.m_uv[1] = v;
            return vertex;
        };

        Vertex vertices[4] = {
            offsetToVertex(-0.5f, -0.5f, 0.f, 0.f),
            offsetToVertex(0.5f,  -0.5f, 1.f, 0.f),
            offsetToVertex(0.5f,  0.5f,  1.f, 1.f),
            offsetToVertex(-0.5f, 0.5f,  0.f, 1.f)
        };
        Indice indices[6] = {0, 1, 2, 0, 2, 3};
        dynamicDraw->DrawIndexed(&vertices, 4, &indices, 6, RHI::IndexFormat::Uint16, drawSrg);
    }

    AzToolsFramework::EditorViewportIconDisplayInterface::IconId AtomViewportDisplayIconsSystemComponent::GetOrLoadIconForPath(
        AZStd::string_view path)
    {
        (void)path;
        return IconId();
    }

    AzToolsFramework::EditorViewportIconDisplayInterface::IconLoadStatus AtomViewportDisplayIconsSystemComponent::GetIconLoadStatus(
        IconId icon)
    {
        (void)icon;
        return IconLoadStatus();
    }

    void AtomViewportDisplayIconsSystemComponent::OnBootstrapSceneReady([[maybe_unused]]AZ::RPI::Scene* bootstrapScene)
    {
        Interface<RPI::DynamicDrawInterface>::Get()->RegisterNamedDynamicDrawContext(m_drawContextName, [](RPI::Ptr<RPI::DynamicDrawContext> drawContext)
        {
            Data::Instance<RPI::Shader> shader = RPI::LoadShader(s_drawContextShaderPath);
            RPI::ShaderOptionList shaderOptions;
            shaderOptions.push_back(AZ::RPI::ShaderOption(AZ::Name("o_useColorChannels"), AZ::Name("true")));
            shaderOptions.push_back(AZ::RPI::ShaderOption(AZ::Name("o_clamp"), AZ::Name("true")));
            drawContext->InitShaderWithVariant(shader, &shaderOptions);
            drawContext->InitVertexFormat(
                {{"POSITION", RHI::Format::R32G32B32_FLOAT},
                 {"COLOR", RHI::Format::R8G8B8A8_UNORM},
                 {"TEXCOORD", RHI::Format::R32G32_FLOAT}});
            drawContext->EndInit();
        });
    }

    RHI::Ptr<RPI::DynamicDrawContext> AtomViewportDisplayIconsSystemComponent::GetDrawContext(AzFramework::ViewportId id) const
    {
        (void)id;
        return nullptr;
    }
} // namespace AZ::Render
