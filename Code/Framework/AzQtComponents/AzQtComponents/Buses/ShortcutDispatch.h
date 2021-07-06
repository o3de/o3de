/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Crc.h>
#include <AzCore/EBus/EBus.h>

#include <QKeySequence>

AZ_PUSH_DISABLE_WARNING(4251 4800 4244, "-Wunknown-warning-option")
#include <QVariant>
#include <QWidget>
AZ_POP_DISABLE_WARNING

namespace AzQtComponents
{
    static const QKeySequence RedoKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Z);

    static const char* SHORTCUT_DISPATCHER_CONTEXT_BREAK_PROPERTY = "ShortcutDispatcherContextBreak";

    // Call this method to mark a widget as a shortcut break; any widgets marked this way
    // will not be searched for shortcuts, and shortcuts under them will only be triggered
    // if the focus widget is within that hierarchy.
    //
    // Most specifically used for view panes.
    //
    // Note that this method is inlined and static so that it can be called from anything that includes this header file. Don't move it into the cpp.
    inline void MarkAsShortcutSearchBreak(QWidget* widget)
    {
        widget->setProperty(SHORTCUT_DISPATCHER_CONTEXT_BREAK_PROPERTY, true);
    }

    /**
     * The ShortcutDispatcBus is intended to allow systems to hook in and override providing a valid keyboard shortcut
     * scopeRoot. GraphCanvas uses sub-systems of Qt which don't play well with the regular
     * Qt widget hierarchy and as a result, some widgets which should have parents don't.
     *
     */
    class ShortcutDispatchTraits
        : public AZ::EBusTraits
    {
    public:
        using Bus = AZ::EBus<ShortcutDispatchTraits>;

        ///////////////////////////////////////////////////////////////////////
        using BusIdType = QWidget*;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        ///////////////////////////////////////////////////////////////////////

        /**
        * Sent when the Editor's shortcut dispatcher can't find a parent for the given focus widget.
        * Specifically used for GraphCanvas widgets, as they don't have parents and aren't part
        * of the regular Qt hierarchy.
        */
        virtual QWidget* GetShortcutDispatchScopeRoot(QWidget* /*focus*/) { return nullptr; }
    };
    using ShortcutDispatchBus = AZ::EBus<ShortcutDispatchTraits>;

} // namespace AzToolsFramework
