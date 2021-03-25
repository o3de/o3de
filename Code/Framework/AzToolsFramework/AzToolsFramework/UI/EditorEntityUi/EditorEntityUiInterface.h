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

#include <AzCore/Component/Entity.h>
#include <AzCore/RTTI/RTTI.h>

namespace AzToolsFramework
{
    using EditorEntityUiHandlerId = AZ::u64;
    class EditorEntityUiHandlerBase;

    /*!
    * EditorEntityUiInterface
    * Interface serviced by the Editor Entity Ui System Component.
    */
    class EditorEntityUiInterface
    {
    public:
        AZ_RTTI(EditorEntityUiInterface, "{E9966C1E-EC61-40B4-805D-47A1F0CFF6B0}");

        EditorEntityUiInterface() = default;
        virtual ~EditorEntityUiInterface() = default;

        virtual EditorEntityUiHandlerId RegisterHandler(EditorEntityUiHandlerBase* handler) = 0;
        virtual void UnregisterHandler(EditorEntityUiHandlerBase* handler) = 0;

        virtual bool RegisterEntity(AZ::EntityId entityId, EditorEntityUiHandlerId handlerId) = 0;
        virtual bool UnregisterEntity(AZ::EntityId entityId) = 0;

        virtual EditorEntityUiHandlerBase* GetHandler(AZ::EntityId entityId) = 0;
    };
} // namespace AzToolsFramework
