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

#include <EMotionFX/Source/AutoRegisteredActor.h>
#include <SceneAPI/SceneCore/Components/ExportingComponent.h>
#include <Integration/System/SystemCommon.h>


namespace EMotionFX
{
    class Actor;

    namespace Pipeline
    {
        struct ActorGroupExportContext;

        class ActorGroupExporter
            : public AZ::SceneAPI::SceneCore::ExportingComponent
        {
        public:
            AZ_COMPONENT(ActorGroupExporter, "{9E21DF50-202B-44CB-A9E7-F429D3B5E5BE}", AZ::SceneAPI::SceneCore::ExportingComponent);

            ActorGroupExporter();

            Actor* GetActor() const;

            static void Reflect(AZ::ReflectContext* context);

            AZ::SceneAPI::Events::ProcessingResult ProcessContext(ActorGroupExportContext& context);

        private:
            AZ::SceneAPI::Events::ProcessingResult SaveActor(ActorGroupExportContext& context);
            AutoRegisteredActor m_actor;
            AZStd::vector<AZStd::string> m_actorMaterialReferences;
        };
    } // namespace Pipeline
} // namespace EMotionFX
