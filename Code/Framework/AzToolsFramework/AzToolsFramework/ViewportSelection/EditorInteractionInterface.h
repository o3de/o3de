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

#include <AzCore/Interface/Interface.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AzToolsFramework
{
        /*!
         * EditorInteractionInterface
         * Allows systems to alter the behavior of viewport selection.
         */
        class EditorInteractionInterface
        {
        public:
            AZ_RTTI(EditorInteractionInterface, "{09276E3C-9AA6-40FF-A0B5-3D33A33F0E5A}");

            /*!
             * Allows the entity system to redirect the selection of an entity to another entity.
             * It can be used to select a container when clicking on its content.
             */
            virtual AZ::EntityId RedirectEntitySelection(AZ::EntityId entityId) = 0;
        };

} // namespace AzToolsFramework

