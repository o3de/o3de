/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <SceneAPI/SceneCore/Events/CallProcessorBus.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>

namespace AZ
{
    namespace SceneAPI::DataTypes
    {
        class IMeshData;
        class IMeshVertexUVData;
    }

    namespace SceneData::GraphData
    {
        class MeshVertexUVData;
    }

    class ComponentDescriptor;
} // namespace AZ

namespace AZ::SceneGenerationComponents
{
    struct UVsGenerateContext
        : public AZ::SceneAPI::Events::ICallContext
    {
        AZ_RTTI(UVsGenerateContext, "{CC7301AB-A7EC-41FB-8BEE-DCC8C8C32BF4}", AZ::SceneAPI::Events::ICallContext)

        UVsGenerateContext(AZ::SceneAPI::Containers::Scene& scene)
            : m_scene(scene) {}
        UVsGenerateContext& operator=(const UVsGenerateContext& other) = delete;

        AZ::SceneAPI::Containers::Scene& GetScene() { return m_scene; }
        const AZ::SceneAPI::Containers::Scene& GetScene() const { return m_scene; }

    private:
        AZ::SceneAPI::Containers::Scene& m_scene;
    };

    inline constexpr const char* s_UVsGenerateComponentTypeId = "{49121BDD-C7E5-4D39-89BC-28789C90057F}";

    //! This function will be called by the module class to get the descriptor.  Doing it this way saves
    //! it from having to actually see the entire component declaration here, it can all be in the implementation file.
    AZ::ComponentDescriptor* CreateUVsGenerateComponentDescriptor();
} // namespace AZ::SceneGenerationComponents
