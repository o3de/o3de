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

#include "EMotionFXConfig.h"
#include "Constraint.h"
#include "Transform.h"
#include <Source/Integration/System/SystemCommon.h>


namespace EMotionFX
{

    /**
     * The transformation constraint base class.
     * This applies a given constraint on a given transformation.
     */     
    class EMFX_API ConstraintTransform : public Constraint
    {
        public:
            AZ_RTTI(ConstraintTransform, "{8C821457-C3C2-4DAD-B552-A7318B420A9C}", Constraint)
            AZ_CLASS_ALLOCATOR(ConstraintTransform, EMotionFX::Integration::EMotionFXAllocator, 0)

            ConstraintTransform() : Constraint()                    { mTransform.Identity(); }
            ~ConstraintTransform() override                         { }

            void SetTransform(const Transform& transform)           { mTransform = transform; }
            MCORE_INLINE const Transform& GetTransform() const      { return mTransform; }
            MCORE_INLINE Transform& GetTransform()                  { return mTransform; }

        protected:
            Transform mTransform = Transform::CreateIdentity();
    };


}   // namespace EMotionFX
