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