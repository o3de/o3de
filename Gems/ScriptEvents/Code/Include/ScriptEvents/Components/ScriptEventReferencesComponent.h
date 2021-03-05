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

#include <ScriptEvents/ScriptEventsAssetRef.h>
#include <AzCore/Asset/AssetCommon.h>

namespace ScriptEvents
{
    namespace Components
    {
        class ScriptEventReferencesComponent 
            : public AZ::Component
            , private AZ::Data::AssetBus::MultiHandler

        {
        public:

            AZ_COMPONENT(ScriptEventReferencesComponent, "{D0F440AC-32D4-49EC-8B93-860B188266A6}", AZ::Component);

            //////////////////////////////////////////////////////////////////////////
            // AZ::Component
            void Init() override {}
            void Activate() override;
            void Deactivate() override;
            //////////////////////////////////////////////////////////////////////////

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

            void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
            static void Reflect(AZ::ReflectContext* reflection);

            AZStd::vector<ScriptEvents::ScriptEventsAssetRef> m_scriptEventAssets;
        };
    }
}
