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

#include "ValueParameterEditor.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/Parameter/ValueParameter.h>
#include <EMotionStudio/EMStudioSDK/Source/Allocators.h>


namespace EMStudio
{
    AZ_CLASS_ALLOCATOR_IMPL(ValueParameterEditor, EMStudio::UIAllocator, 0)

    void ValueParameterEditor::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<ValueParameterEditor>()
            ->Version(1)
        ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<ValueParameterEditor>("Value parameter editor", "")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
        ;
    }

    void ValueParameterEditor::SetAttributes(const AZStd::vector<MCore::Attribute*>& attributes)
    {
        m_attributes = attributes;
    }

    AZStd::string ValueParameterEditor::GetDescription() const
    {
        AZ_Assert(m_valueParameter, "Expected non-null value parameter");
        return m_valueParameter->GetDescription();
    }
} // namespace EMotionFX