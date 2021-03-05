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

#if !defined(Q_MOC_RUN)
#include "ActionHistoryCallback.h"
#include "../../../../EMStudioSDK/Source/DockWidgetPlugin.h"
#endif


namespace EMStudio
{
    class ActionHistoryPlugin
        : public EMStudio::DockWidgetPlugin
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(ActionHistoryPlugin, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        enum
        {
            CLASS_ID = 0x00000003
        };

        ActionHistoryPlugin();
        ~ActionHistoryPlugin();

        // overloaded
        const char* GetCompileDate() const override;
        const char* GetName() const override;
        uint32 GetClassID() const override;
        const char* GetCreatorName() const override;
        float GetVersion() const override;
        bool GetIsClosable() const override             { return true; }
        bool GetIsFloatable() const override            { return true; }
        bool GetIsVertical() const override             { return false; }

        // overloaded main init function
        bool Init() override;
        EMStudioPlugin* Clone() override;

        void ReInit();

    public slots:
        void OnSelectedItemChanged();

    private:
        QListWidget*            mList;
        ActionHistoryCallback*  mCallback;
    };
}   // namespace EMStudio