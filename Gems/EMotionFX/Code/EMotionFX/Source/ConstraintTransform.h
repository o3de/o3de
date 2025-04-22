/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
            AZ_CLASS_ALLOCATOR(ConstraintTransform, EMotionFX::Integration::EMotionFXAllocator)

            ConstraintTransform() : Constraint()                    { m_transform.Identity(); }
            ~ConstraintTransform() override                         { }

            void SetTransform(const Transform& transform)           { m_transform = transform; }
            MCORE_INLINE const Transform& GetTransform() const      { return m_transform; }
            MCORE_INLINE Transform& GetTransform()                  { return m_transform; }

        protected:
            Transform m_transform = Transform::CreateIdentity();
    };


}   // namespace EMotionFX
