/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <EMotionFX/Source/TransformSpace.h>


namespace EMotionFX
{
    void TransformSpace::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Enum<ETransformSpace>("Transform Space", "Transform space, which controls what space we are operating on.")
            ->Value("Local", ETransformSpace::TRANSFORM_SPACE_LOCAL)
            ->Value("World", ETransformSpace::TRANSFORM_SPACE_WORLD)
            ->Value("Model", ETransformSpace::TRANSFORM_SPACE_MODEL)
            ;
    }

} // namespace EMotionFX
