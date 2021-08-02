/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>
#include <SceneAPI/SceneCore/Components/ExportingComponent.h>
#include <SceneAPI/SceneCore/Events/ExportEventContext.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            class Scene;
            class SceneGraph;
            class SceneManifest;
        }
    }
}

namespace SceneLoggingExample
{
    // The ExportTrackingProcessor class demonstrates how to use the ExportingComponent to listen to scene export events.
    // It also shows how to collect data from a graph by traversing the graph in a hierarchical way.
    class ExportTrackingProcessor
        : public AZ::SceneAPI::SceneCore::ExportingComponent
    {
    public:
        AZ_COMPONENT(ExportTrackingProcessor, "{EAD9C07A-60D5-4E48-8465-72034D326368}", AZ::SceneAPI::SceneCore::ExportingComponent);

        ExportTrackingProcessor();
        ~ExportTrackingProcessor() override = default;

        static void Reflect(AZ::ReflectContext* context);

    protected:
        AZ::SceneAPI::Events::ProcessingResult PrepareForExport(AZ::SceneAPI::Events::PreExportEventContext& context);
        AZ::SceneAPI::Events::ProcessingResult ContextCallback(AZ::SceneAPI::Events::ICallContext& context);

        uint8_t GetPriority() const override;

        void LogGraph(const AZ::SceneAPI::Containers::SceneGraph& graph, const AZStd::string& nodePath) const;

        const AZ::SceneAPI::Containers::SceneManifest* m_manifest = nullptr;
    };
} // namespace SceneLoggingExample
