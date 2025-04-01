/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzCore/Math/Vector3.h>

namespace LmbrCentral
{
    /**!
     * EditorAudioListenerComponent
     */
    class EditorAudioListenerComponent
        : public AzToolsFramework::Components::EditorComponentBase
    {
    public:
        AZ_EDITOR_COMPONENT(EditorAudioListenerComponent, "{62D0ED59-F638-4444-96BE-F98504DF5852}",
            AzToolsFramework::Components::EditorComponentBase);

        void Activate() override;
        void Deactivate() override;

        void BuildGameEntity(AZ::Entity* gameEntity) override;

    protected:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("AudioListenerService"));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("TransformService"));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("AudioListenerService"));
        }

        static void Reflect(AZ::ReflectContext* context);

    private:
        //! Serialized data
        bool m_defaultListenerState = true;
        AZ::EntityId m_rotationEntity;
        AZ::EntityId m_positionEntity;
        AZ::Vector3 m_fixedOffset = AZ::Vector3::CreateZero();
    };

} // namespace LmbrCentral
