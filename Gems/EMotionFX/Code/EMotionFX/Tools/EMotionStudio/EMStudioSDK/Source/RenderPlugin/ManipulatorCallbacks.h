/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef __MCOMMON_MANIPULATORCALLBACKS_H
#define __MCOMMON_MANIPULATORCALLBACKS_H

// include the Core system
#include "../EMStudioConfig.h"
#include <EMotionFX/Rendering/Common/TranslateManipulator.h>
#include "../EMStudioManager.h"


namespace EMStudio
{
    /**
     * callback base class used to update actorinstances.
     */
    class TranslateManipulatorCallback
        : public MCommon::ManipulatorCallback
    {
        MCORE_MEMORYOBJECTCATEGORY(TranslateManipulatorCallback, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK_RENDERPLUGINBASE);

    public:
        /**
         * the constructor.
         */
        TranslateManipulatorCallback(EMotionFX::ActorInstance* actorInstance, const AZ::Vector3& oldValue)
            : ManipulatorCallback(actorInstance, oldValue)
        {
        }

        /**
         * the destructor.
         */
        virtual ~TranslateManipulatorCallback() {}

        /**
         * update the actor instance.
         */
        using MCommon::ManipulatorCallback::Update;
        void Update(const AZ::Vector3& value) override;

        /**
         * update old transformation values of the callback
         */
        void UpdateOldValues() override;

        /**
         * apply the transformation.
         */
        void ApplyTransformation() override;

        bool GetResetFollowMode() const override                { return true; }
    };


    class RotateManipulatorCallback
        : public MCommon::ManipulatorCallback
    {
        MCORE_MEMORYOBJECTCATEGORY(RotateManipulatorCallback, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK_RENDERPLUGINBASE);

    public:
        /**
         * the constructor.
         */
        RotateManipulatorCallback(EMotionFX::ActorInstance* actorInstance, const AZ::Vector3& oldValue)
            : ManipulatorCallback(actorInstance, oldValue)
        {
        }

        RotateManipulatorCallback(EMotionFX::ActorInstance* actorInstance, const AZ::Quaternion& oldValue)
            : ManipulatorCallback(actorInstance, oldValue)
        {
        }

        /**
         * the destructor.
         */
        virtual ~RotateManipulatorCallback() {}

        /**
         * update the actor instance.
         */
        using MCommon::ManipulatorCallback::Update;
        void Update(const AZ::Quaternion& value) override;

        /**
         * update old transformation values of the callback
         */
        void UpdateOldValues() override;

        /**
         * apply the transformation.
         */
        void ApplyTransformation() override;
    };

    class ScaleManipulatorCallback
        : public MCommon::ManipulatorCallback
    {
        MCORE_MEMORYOBJECTCATEGORY(ScaleManipulatorCallback, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK_RENDERPLUGINBASE);

    public:
        /**
         * the constructor.
         */
        ScaleManipulatorCallback(EMotionFX::ActorInstance* actorInstance, const AZ::Vector3& oldValue)
            : ManipulatorCallback(actorInstance, oldValue)
        {
        }

        /**
         * the destructor.
         */
        virtual ~ScaleManipulatorCallback() {}

        /**
         * function to get the current value.
         */
        AZ::Vector3 GetCurrValueVec() override;

        /**
         * update the actor instance.
         */
        using MCommon::ManipulatorCallback::Update;
        void Update(const AZ::Vector3& value) override;

        /**
         * update old transformation values of the callback
         */
        void UpdateOldValues() override;

        /**
         * apply the transformation.
         */
        void ApplyTransformation() override;

        bool GetResetFollowMode() const override                { return true; }
    };
} // namespace MCommon


#endif
