#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/set.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/SceneCoreConfiguration.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            class Scene;
        }
        namespace DataTypes
        {
            class IGraphObject;
        }
        namespace Events
        {
#if defined(AZ_PLATFORM_LINUX)
            class SCENE_CORE_API GraphMetaInfo
#else
            class GraphMetaInfo
#endif
                : public AZ::EBusTraits
            {
            public:
                static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

                using VirtualTypesSet = AZStd::unordered_set<Crc32>;

                inline static Crc32 GetIgnoreVirtualType()
                {
                    static Crc32 s_ignoreVirtualType = AZ_CRC_CE("Ignore");
                    return s_ignoreVirtualType;
                }

                SCENE_CORE_API GraphMetaInfo() = default;

                virtual ~GraphMetaInfo() = default;

                // Gets the path to the icon associated with the given object.
                SCENE_CORE_API virtual void GetIconPath([[maybe_unused]] AZStd::string& iconPath, [[maybe_unused]] const DataTypes::IGraphObject* target) {}

                // Provides a short description of the type.
                SCENE_CORE_API virtual void GetToolTip([[maybe_unused]] AZStd::string& toolTip, [[maybe_unused]] const DataTypes::IGraphObject* target) {}

                // Provides a list of string CRCs that indicate the virtual type the given node can act as.
                //      Virtual types are none custom types that are different interpretations of existing types based on
                //      their name or attributes.
                SCENE_CORE_API virtual void GetVirtualTypes([[maybe_unused]] VirtualTypesSet& types, 
                                                            [[maybe_unused]] const Containers::Scene& scene,
                                                            [[maybe_unused]] Containers::SceneGraph::NodeIndex node) {}

                // Provides a list of string CRCs that indicate all available virtual types.
                SCENE_CORE_API virtual void GetAllVirtualTypes([[maybe_unused]] VirtualTypesSet& types) {}

                // Converts the virtual type hashed name into a readable name.
                SCENE_CORE_API virtual void GetVirtualTypeName([[maybe_unused]] AZStd::string& name, [[maybe_unused]] Crc32 type) {}

                // Provides the policies that will be applied to the scene from the asset builders.
                SCENE_CORE_API virtual void GetAppliedPolicyNames([[maybe_unused]] AZStd::set<AZStd::string>& appliedPolicies,
                                                                  [[maybe_unused]] const Containers::Scene& scene) const {}
            };

            using GraphMetaInfoBus = AZ::EBus<GraphMetaInfo>;
        } // Events
    } // SceneAPI
} // AZ
