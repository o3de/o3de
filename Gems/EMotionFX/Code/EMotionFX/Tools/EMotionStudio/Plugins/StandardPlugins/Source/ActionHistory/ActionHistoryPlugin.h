/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include "ActionHistoryCallback.h"
#include "../../../../EMStudioSDK/Source/DockWidgetPlugin.h"
#endif


namespace EMStudio
{
    class ActionHistoryPlugin
        : public EMStudio::DockWidgetPlugin
    {
        Q_OBJECT // AUTOMOC
        MCORE_MEMORYOBJECTCATEGORY(ActionHistoryPlugin, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        enum
        {
            CLASS_ID = 0x00000003
        };

        ActionHistoryPlugin();
        ~ActionHistoryPlugin();

        // overloaded
        const char* GetName() const override;
        uint32 GetClassID() const override;
        bool GetIsClosable() const override             { return true; }
        bool GetIsFloatable() const override            { return true; }
        bool GetIsVertical() const override             { return false; }

        // overloaded main init function
        bool Init() override;
        EMStudioPlugin* Clone() const override { return new ActionHistoryPlugin(); }

        void ReInit();

    public slots:
        void OnSelectedItemChanged();

    private:
        QListWidget*            m_list;
        ActionHistoryCallback*  m_callback;
    };
}   // namespace EMStudio
