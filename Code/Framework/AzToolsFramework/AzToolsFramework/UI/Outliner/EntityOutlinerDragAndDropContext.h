/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/EntityId.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>

#include <AzQtComponents/Buses/DragAndDrop.h>

class QMimeData;

#pragma once

namespace AzQtComponents
{
    // Note that this is intentionally in the AzQtComponents namespace, since all the other
    // Context IDs are also in that namespace.
    namespace DragAndDropContexts
    {
        //! Represents a context where someone is dropping data onto something in the gui that represents
        //! an outliner item (Could be a prefab root, instance, or even the empty space between things).
        //! Contrast this with for example dropping data into 3d space in a 3d viewport.
        static const AZ::u32 EntityOutliner = AZ_CRC_CE("EntityOutliner");
    }
}

namespace AzToolsFramework
{
    //! This context is used for the above DragAndDropContext EntityOutliner id.
    //! Note that the Outliner is an Item View, and thus will only invoke the DoItemViewDrop / CanItemViewDrop
    //! functions on the Drag and Drop Bus.
    class EntityOutlinerDragAndDropContext : public AzQtComponents::DragAndDropContextBase
    {
    public:
        AZ_CLASS_ALLOCATOR(EntityOutlinerDragAndDropContext, AZ::SystemAllocator);
        AZ_RTTI(EntityOutlinerDragAndDropContext, "{5CF08C17-C5C0-48C5-873F-702E9402D162}", AzQtComponents::DragAndDropContextBase);

        //! If m_parentEntity is the Invalid Entity Id, it should be considered
        //! to be dragging over empty space and not over between any particular element.
        AZ::EntityId m_parentEntity;

        //! The mimedata is a pointer to the mimedata being dropped.
        //! It survives only for this call.
        const QMimeData* m_dataBeingDropped = nullptr;
    };

} // namespace AzQtComponents
