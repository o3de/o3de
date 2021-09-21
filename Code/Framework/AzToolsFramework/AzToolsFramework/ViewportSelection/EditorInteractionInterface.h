/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Interface/Interface.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AzToolsFramework
{
    //! Allows systems to alter the behavior of viewport selection.
    class EditorInteractionInterface
    {
    public:
        AZ_RTTI(EditorInteractionInterface, "{9A150939-6203-4C7D-BD79-2959855CF49B}");

         //! Allows the entity system to redirect the selection of an entity to another entity.
         //! It can be used to select a container when clicking on its content.
        virtual AZ::EntityId RedirectEntitySelection(AZ::EntityId entityId) = 0;
    };

} // namespace AzToolsFramework
