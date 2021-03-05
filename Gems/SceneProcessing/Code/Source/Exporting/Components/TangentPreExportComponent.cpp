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

#include <Exporting/Components/TangentPreExportComponent.h>
#include <Exporting/Components/TangentGenerateComponent.h>


namespace AZ
{
    namespace SceneExportingComponents
    {
        namespace SceneEvents = AZ::SceneAPI::Events;
        //namespace SceneUtil = AZ::SceneAPI::Utilities;
        namespace SceneContainers = AZ::SceneAPI::Containers;
        namespace SceneViews = AZ::SceneAPI::Containers::Views;
        namespace SceneDataTypes = AZ::SceneAPI::DataTypes;

        TangentPreExportComponent::TangentPreExportComponent()
        {
            BindToCall(&TangentPreExportComponent::Register);
        }


        void TangentPreExportComponent::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<TangentPreExportComponent, AZ::SceneAPI::SceneCore::ExportingComponent>()->Version(1);
            }
        }


        AZ::SceneAPI::Events::ProcessingResult TangentPreExportComponent::Register(AZ::SceneAPI::Events::PreExportEventContext& context)
        {
            SceneEvents::ProcessingResultCombiner result;
            TangentGenerateContext tangentGenerateContext(const_cast<AZ::SceneAPI::Containers::Scene&>(context.GetScene()));
            result += SceneEvents::Process<TangentGenerateContext>(tangentGenerateContext);
            return SceneEvents::ProcessingResult::Success;
        }
    } // namespace SceneExportingComponents
} // namespace AZ
