/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <SceneAPI/SceneCore/Components/SceneSystemComponent.h>

namespace AZ::SceneAPI::SceneBuilder
{
    struct ImportContextProvider;
}

namespace AZ::AssImp
{
    //! SceneSystemComponent that registers the AssImp ImportContextProvider with the SceneAPI
    //! ImportContextRegistry. It is instantiated automatically wherever SceneAPI scene system
    //! components are gathered (the builders' "Scene System" entity and the editor's
    //! "AssetImportersEntity") when the AssImp gem is enabled.
    class AssImpImporterComponent
        : public AZ::SceneAPI::SceneCore::SceneSystemComponent
    {
    public:
        AZ_COMPONENT(AssImpImporterComponent, "{F6977756-A853-4390-A2D3-41189D3155A1}", SceneAPI::SceneCore::SceneSystemComponent);

        ~AssImpImporterComponent() override = default;

        void Activate() override;
        void Deactivate() override;

        static void GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided);
        static void GetDependentServices(ComponentDescriptor::DependencyArrayType& dependent);
        static void Reflect(ReflectContext* context);

    private:
        AZ::SceneAPI::SceneBuilder::ImportContextProvider* m_provider = nullptr;
    };
} // namespace AZ::AssImp
