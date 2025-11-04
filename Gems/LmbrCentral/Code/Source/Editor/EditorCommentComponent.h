/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
            provided.push_back(AZ_CRC_CE("EditorCommentingService"));
        }

        static void Reflect(AZ::ReflectContext* context);

    private:
    
        // A user editable comment for this entity
        AZStd::string m_comment;
    };
} // namespace LmbrCentral
