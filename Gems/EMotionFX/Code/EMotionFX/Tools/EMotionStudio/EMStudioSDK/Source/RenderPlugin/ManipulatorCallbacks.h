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
