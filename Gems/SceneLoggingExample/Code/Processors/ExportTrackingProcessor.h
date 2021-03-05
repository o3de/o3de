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