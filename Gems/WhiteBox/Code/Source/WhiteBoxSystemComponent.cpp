/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "WhiteBox_precompiled.h"

#include "Asset/WhiteBoxMeshAssetHandler.h"
#include "Rendering/Atom/WhiteBoxAtomRenderMesh.h"
#include "WhiteBoxSystemComponent.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace WhiteBox
{
    void WhiteBoxSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<WhiteBoxSystemComponent, AZ::Component>()->Version(0);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<WhiteBoxSystemComponent>(
                      "WhiteBox", "[Description of functionality provided by this System Component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);
            }
        }
    }

    WhiteBoxSystemComponent::WhiteBoxSystemComponent() = default;
    WhiteBoxSystemComponent::~WhiteBoxSystemComponent() = default;

    void WhiteBoxSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("WhiteBoxService", 0x2f2f42b8));
    }

    void WhiteBoxSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("WhiteBoxService", 0x2f2f42b8));
    }

    void WhiteBoxSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& /*required*/) {}

    void WhiteBoxSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& /*dependent*/) {}

    void WhiteBoxSystemComponent::Activate()
    {
        // default builder
        SetRenderMeshInterfaceBuilder(
            []() -> AZStd::unique_ptr<RenderMeshInterface>
            {
                return AZStd::make_unique<AtomRenderMesh>();
            });

        WhiteBoxRequestBus::Handler::BusConnect();
    }

    void WhiteBoxSystemComponent::Deactivate()
    {
        WhiteBoxRequestBus::Handler::BusDisconnect();
        m_assetHandlers.clear();
    }

    AZStd::unique_ptr<RenderMeshInterface> WhiteBoxSystemComponent::CreateRenderMeshInterface()
    {
        return m_renderMeshInterfaceBuilder();
    }

    void WhiteBoxSystemComponent::SetRenderMeshInterfaceBuilder(RenderMeshInterfaceBuilderFn builder)
    {
        m_renderMeshInterfaceBuilder = AZStd::move(builder);
    }
} // namespace WhiteBox
