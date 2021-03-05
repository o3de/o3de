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

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

namespace LmbrCentral
{
    class EditorCommentComponent
        : public AzToolsFramework::Components::EditorComponentBase
    {
    public:

        AZ_COMPONENT(EditorCommentComponent, "{5181117D-CD69-4C05-8804-C1FBE5F0C00F}",
                     AzToolsFramework::Components::EditorComponentBase);

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override {};
        void Deactivate() override {};
        //////////////////////////////////////////////////////////////////////////

    protected:

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("EditorCommentingService", 0xdd5ab934));
        }

        static void Reflect(AZ::ReflectContext* context);

    private:
    
        // A user editable comment for this entity
        AZStd::string m_comment;
    };
} // namespace LmbrCentral
