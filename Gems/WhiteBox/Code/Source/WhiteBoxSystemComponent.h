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

#pragma once

#include <AzCore/Component/Component.h>
#include <WhiteBox/WhiteBoxBus.h>

namespace AZ::Data
{
    class AssetHandler;
}

namespace WhiteBox
{
    class RenderMeshInterface;

    //! System component for the White Box Tool.
    class WhiteBoxSystemComponent
        : public AZ::Component
        , private WhiteBoxRequestBus::Handler
    {
    public:
        AZ_COMPONENT(WhiteBoxSystemComponent, "{BD393FD9-CF47-433D-B171-C44FE2F7069F}");
        static void Reflect(AZ::ReflectContext* context);

        WhiteBoxSystemComponent();
        WhiteBoxSystemComponent(const WhiteBoxSystemComponent&) = delete;
        WhiteBoxSystemComponent& operator=(const WhiteBoxSystemComponent&) = delete;
        ~WhiteBoxSystemComponent();

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // WhiteBoxRequestBus ...
        AZStd::unique_ptr<RenderMeshInterface> CreateRenderMeshInterface() override;
        void SetRenderMeshInterfaceBuilder(RenderMeshInterfaceBuilderFn builder) override;

    protected:
        // AZ::Component ...
        void Activate() override;
        AZStd::vector<AZStd::unique_ptr<AZ::Data::AssetHandler>> m_assetHandlers;

    private:
        // AZ::Component ...
        void Deactivate() override;

        //! The builder invoked by CreateRenderMeshInterface.
        RenderMeshInterfaceBuilderFn m_renderMeshInterfaceBuilder;
    };
} // namespace WhiteBox
