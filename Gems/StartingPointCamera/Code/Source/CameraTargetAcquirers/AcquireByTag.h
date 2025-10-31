/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <AzCore/RTTI/RTTI.h>
#include <CameraFramework/ICameraTargetAcquirer.h>
#include <AzCore/Component/Component.h>
#include <LmbrCentral/Scripting/TagComponentBus.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ
{
    class ReflectContext;
}

namespace Camera
{
    //////////////////////////////////////////////////////////////////////////
    /// This will request Camera targets from the CameraTarget buses.  It will
    /// then return that target's transform when requested by the Camera Rig
    //////////////////////////////////////////////////////////////////////////
    class AcquireByTag
        : public ICameraTargetAcquirer
        , private LmbrCentral::TagGlobalNotificationBus::Handler
    {
    public:
        ~AcquireByTag() override = default;
        AZ_RTTI(AcquireByTag, "{E76621A5-E5A8-41B0-AC1D-EC87553181F5}", ICameraTargetAcquirer)
        AZ_CLASS_ALLOCATOR(AcquireByTag, AZ::SystemAllocator);
        static void Reflect(AZ::ReflectContext* reflection);

        //////////////////////////////////////////////////////////////////////////
        // ICameraTargetAcquirer
        bool AcquireTarget(AZ::Transform& outTransformInformation) override;
        void Activate(AZ::EntityId) override;
        void Deactivate() override;
        
        //////////////////////////////////////////////////////////////////////////
        // LmbrCentral::TagGlobalNotificationBus
        void OnEntityTagAdded(const AZ::EntityId&) override;
        void OnEntityTagRemoved(const AZ::EntityId&) override;

    private:
        //////////////////////////////////////////////////////////////////////////
        // Reflected Data
        AZStd::string m_targetTag;
        bool m_shouldUseTargetRotation = true;
        bool m_shouldUseTargetPosition = true;
        
        //////////////////////////////////////////////////////////////////////////
        // Private Data
        AZStd::vector<AZ::EntityId> m_targets;
    };
} //namespace Camera
