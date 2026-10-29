/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if !defined(AZ_MONOLITHIC_BUILD)

#include <AzCore/Component/Component.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Module/Environment.h>
#include <SceneAPI/SceneBuilder/SceneImportRequestHandler.h>

#include <SceneAPI/SceneBuilder/SceneImporter.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneBuilder
        {
            static AZStd::vector<AZ::ComponentDescriptor*> g_componentDescriptors;

            void Reflect(AZ::SerializeContext* /*context*/)
            {
                using namespace AZ::SceneAPI;
                using namespace AZ::SceneAPI::SceneBuilder;

                if (g_componentDescriptors.empty())
                {
                    // Global importer and behavior
                    g_componentDescriptors.push_back(SceneBuilder::SceneImporter::CreateDescriptor());
                    g_componentDescriptors.push_back(SceneImportRequestHandler::CreateDescriptor());

                    for (AZ::ComponentDescriptor* descriptor : g_componentDescriptors)
                    {
                        AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Handler::RegisterComponentDescriptor, descriptor);
                    }
                }
            }

            void ReflectBehavior([[maybe_unused]] AZ::BehaviorContext* context)
            {
                // stub in until LYN-1284 is done
            }

            void Activate()
            {
            }

            void Deactivate()
            {
            }

            void Uninitialize()
            {
                if (!g_componentDescriptors.empty())
                {
                    for (AZ::ComponentDescriptor* descriptor : g_componentDescriptors)
                    {
                        descriptor->ReleaseDescriptor();
                    }
                    g_componentDescriptors.clear();
                    g_componentDescriptors.shrink_to_fit();
                }
            }
        } // namespace SceneBuilder
    } // namespace SceneAPI
} // namespace AZ

extern "C" AZ_DLL_EXPORT void InitializeDynamicModule()
{
}
extern "C" AZ_DLL_EXPORT void Reflect(AZ::SerializeContext* context)
{
    AZ::SceneAPI::SceneBuilder::Reflect(context);
}
extern "C" AZ_DLL_EXPORT void ReflectBehavior(AZ::BehaviorContext* context)
{
    AZ::SceneAPI::SceneBuilder::ReflectBehavior(context);
}
extern "C" AZ_DLL_EXPORT void UninitializeDynamicModule()
{
    AZ::SceneAPI::SceneBuilder::Uninitialize();
}

#endif // !defined(AZ_MONOLITHIC_BUILD)
