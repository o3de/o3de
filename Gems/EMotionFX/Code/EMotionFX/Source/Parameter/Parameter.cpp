/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Parameter.h"

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/Utils.h>
#include <EMotionFX/Source/Allocators.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(Parameter, AnimGraphAllocator)


    void Parameter::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<Parameter>()
            ->Version(1)
            ->Field("name", &Parameter::m_name)
            ->Field("description", &Parameter::m_description)
            ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<Parameter>("Parameter", "")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::Default, &Parameter::m_name, "Name", "Parameter's name")
            ->DataElement(AZ::Edit::UIHandlers::MultiLineEdit, &Parameter::m_description, "Description", "Parameter's description")
            ;
    }

    const char Parameter::s_invalidCharacters[] = {'"', '%', '{', '}'};

    bool Parameter::IsNameValid(const AZStd::string& name, AZStd::string* outInvalidCharacters)
    {
        for (char c : s_invalidCharacters)
        {
            if (name.find(c) != AZStd::string::npos)
            {
                if (outInvalidCharacters)
                {
                    if (!(*outInvalidCharacters).empty())
                    {
                        (*outInvalidCharacters) += ", ";
                    }
                    (*outInvalidCharacters) += c;
                }
                else
                {
                    return false;
                }
            }
        }

        if (outInvalidCharacters)
        {
            return (*outInvalidCharacters).empty();
        }
        else
        {
            return true;
        }
    }

}   // namespace EMotionFX
