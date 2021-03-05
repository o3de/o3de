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

// include required headers
#include "EMotionFXConfig.h"
#include "LayerPass.h"
#include <MCore/Source/Array.h>


namespace EMotionFX
{
    // forward declarations
    class MotionLayerSystem;
    class Node;
    class MotionInstance;
    class Transform;

    /**
     * The motion based actor repositioning layer pass.
     * This layer pass is responsible to apply relative movements from the actor's repositioning node to the actor itself,
     * instead of to moving the repositioning node.
     */
    class EMFX_API RepositioningLayerPass
        : public LayerPass
    {
        AZ_CLASS_ALLOCATOR_DECL
    public:
        // the unique type ID of this layer pass type
        enum
        {
            TYPE_ID = 0x00000002
        };

        /**
         * The create method.
         * @param motionLayerSystem The motion layer system where this pass will be added to.
         */
        static RepositioningLayerPass* Create(MotionLayerSystem* motionLayerSystem);

        /**
         * Get the unique type ID of the layer pass class.
         * @result The unique type ID number of this layer pass type.
         */
        uint32 GetType() const override;

        /**
         * Process the layer pass.
         * This will move the actor based on the relative movements of the specified motion root node of the actor.
         * Also the movements are blend together accordingly to the weights and arrangement of the motion layers.
         */
        void Process() override;


    private:
        MCore::Array<uint32>    mHierarchyPath; /**< The path of node indices to the repositioning node. */
        uint32                  mLastReposNode; /**< The last repositioning node index that was used. When this changes, the hierarchy path has to be updated. */

        /**
         * The constructor.
         * @param motionLayerSystem The motion layer system where this pass will be added to.
         */
        RepositioningLayerPass(MotionLayerSystem* motionLayerSystem);

        /**
         * The destructor.
         */
        ~RepositioningLayerPass();
    };
} // namespace EMotionFX
