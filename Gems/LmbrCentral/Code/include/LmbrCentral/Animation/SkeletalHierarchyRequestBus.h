/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Transform.h>

namespace LmbrCentral
{
    /*!
     * SkeletonHierarchyRequestBus
     * Messages serviced by components to provide information about skeletal hierarchies.
     */
    class SkeletalHierarchyRequests
        : public AZ::ComponentBus
    {
    public:

        /**
         * \return Number of joints in the skeleton joint hierarchy.
         */
        virtual AZ::u32 GetJointCount() { return 0; }

        /**
        * \param jointIndex Index of joint whose name should be returned.
        * \return Name of the joint at the specified index. Null if joint index is not valid.
        */
        virtual const char* GetJointNameByIndex(AZ::u32 /*jointIndex*/) { return nullptr; }

        /**
        * \param jointName Name of joint whose index should be returned.
        * \return Index of the joint with the specified name. -1 if the joint was not found.
        */
        virtual AZ::s32 GetJointIndexByName(const char* /*jointName*/) { return 0; }

        /**
        * \param jointIndex Index of joint whose local-space transform should be returned.
        * \return Joint's character-space transform. Identify if joint index was not valid.
        */
        virtual AZ::Transform GetJointTransformCharacterRelative(AZ::u32 /*jointIndex*/) { return AZ::Transform::CreateIdentity(); }
    };

    using SkeletalHierarchyRequestBus = AZ::EBus<SkeletalHierarchyRequests>;

} // namespace LmbrCentral
