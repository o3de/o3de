/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzFramework/Components/EditorEntityEvents.h>

#include <StartingPointInput/InputEventRequestBus.h>

#include <InputEventBindings.h>

namespace AZ
{
    class SerializeContext;
}

namespace StartingPointInput
{
    class InputConfigurationComponent
        : public AZ::Component
        , private AZ::Data::AssetBus::Handler
        , private InputConfigurationComponentRequestBus::Handler
        , public AzFramework::EditorEntityEvents
    {
    public:

        AZ_COMPONENT(InputConfigurationComponent, "{3106EE2A-4816-433E-B855-D17A6484D5EC}", AzFramework::EditorEntityEvents);
        virtual ~InputConfigurationComponent();
        InputConfigurationComponent();
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void Reflect(AZ::ReflectContext* reflection);

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        /// AzFramework::EditorEntityEvents
        void EditorSetPrimaryAsset(const AZ::Data::AssetId& assetId) override;
        //////////////////////////////////////////////////////////////////////////
    private:
        InputConfigurationComponent(const InputConfigurationComponent&) = delete;

        //////////////////////////////////////////////////////////////////////////
        // AZ::InputConfigurationComponentRequestBus::Handler
        void SetLocalUserId(AzFramework::LocalUserId localUserId) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        /// AZ::Data::AssetBus::Handler
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        //////////////////////////////////////////////////////////////////////////

        void ActivateBindingsIfAppropriate();

        //////////////////////////////////////////////////////////////////////////
        // Reflected Data
        InputEventBindings m_inputEventBindings;
        AZStd::vector<AZStd::string> m_inputContexts;
        AZ::Data::Asset<InputEventBindingsAsset> m_inputEventBindingsAsset;
        AZ::s32 m_localPlayerIndex = -1;

        AzFramework::LocalUserId m_localUserId = AzFramework::LocalUserIdAny;
        bool m_isContextActive = false;

        // Unlike the definition of most assets, the input asset requires additional preparation after its loaded
        // in order to actually be prepared to be used.
        bool m_isAssetPrepared = false;

        AZ::Uuid m_uuid;
    };
} // namespace Input
