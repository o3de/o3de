/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzToolsFramework/Entity/EditorEntityContextComponent.h>

namespace AzToolsFramework
{
    // This class is an inelegant workaround for use by the Slice Converter to selectively disable entity add/remove logic
    // during slice conversion in the EditorEntityContextComponent.  Specifically, the standard versions of these methods will
    // attempt to activate the entities as they're added.  This is both unnecessary and undesirable during slice conversion, since
    // entity activation requires a lot of subsystems to be active and valid.
    // Instead, by selectively disabling this logic, the entities can remain in an initialized state, which is sufficient for conversion,
    // without requiring those extra subsystems.

    // This problem also could have been solved by adding APIs to the EditorEntityContextComponent or the EntityContext, but there aren't
    // any other known valid use cases for disabling this logic, so the extra APIs would simply encourage "bad behavior" by using them
    // when they likely aren't necessary or desired.

    class SliceConverterEditorEntityContextComponent
        : public EditorEntityContextComponent
    {
    public:

        AZ_COMPONENT(SliceConverterEditorEntityContextComponent, "{1CB0C38F-8E85-4422-91C6-E1F3B9B4B853}");

        SliceConverterEditorEntityContextComponent() : EditorEntityContextComponent() {}

        // Simple API to selectively disable this logic *only* when performing slice to prefab conversion.
        static inline void DisableOnContextEntityLogic()
        {
            m_enableOnContextEntityLogic = false;
        }

    protected:

        void OnContextEntitiesAdded([[maybe_unused]] const EntityList& entities) override
        {
            if (m_enableOnContextEntityLogic)
            {
                EditorEntityContextComponent::OnContextEntitiesAdded(entities);
            }
        }

        void OnContextEntityRemoved([[maybe_unused]] const AZ::EntityId& id) override
        {
            if (m_enableOnContextEntityLogic)
            {
                EditorEntityContextComponent::OnContextEntityRemoved(id);
            }
        }

        // By default, act just like the EditorEntityContextComponent
        static inline bool m_enableOnContextEntityLogic = true;
    };
} // namespace AzToolsFramework
