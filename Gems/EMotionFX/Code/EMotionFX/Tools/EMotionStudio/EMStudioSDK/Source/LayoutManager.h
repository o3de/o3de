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

#include "EMStudioConfig.h"
#include <MCore/Source/MemoryManager.h>
#include <Editor/InputDialogValidatable.h>

namespace EMStudio
{
    // layout file header
    struct LayoutHeader
    {
        char    mFileTypeCode[9];   // "EMSLAYOUT", otherwise no valid layout file
        uint32  mEMFXVersionHigh;   // EMotion FX high version as used in EMStudio
        uint32  mEMFXVersionLow;    // EMotion FX low version as used in EMStudio
        char    mEMFXCompileDate[64];// EMotion FX compile date
        uint32  mLayoutVersionHigh; // layout file type high version
        uint32  mLayoutVersionLow;  // layout file type low version
        char    mCompileDate[64];   // EMStudio compile date
        char    mDescription[256];  // optional description of the layout
        uint32  mNumPlugins;        // the number of plugins

        // followed by:
        //      LayoutPluginHeader[mNumPlugins]
        //      uint32 mainWindowStateSize
        //      int8 mainWindowState[mainWindowStateSize]
    };

    // the plugin data header
    struct LayoutPluginHeader
    {
        uint32  mDataSize;          // data size of the data which the given plugin will store
        char    mPluginName[128];   // the name of the plugin (its ID to create as passed to PluginManager::CreateWindowOfType)
        char    mObjectName[128];
        uint32  mDataVersion;       // the data version, to for backward compatibility of loading individual plugin settings from layout files

        // followed by:
        //      int8    pluginData[mDataSize]
    };

    class EMSTUDIO_API LayoutManager
    {
        MCORE_MEMORYOBJECTCATEGORY(LayoutManager, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK)

    public:
        LayoutManager();
        ~LayoutManager();

        void SaveLayoutAs();
        bool SaveLayout(const char* filename);
        bool LoadLayout(const char* filename);

        void SaveDialogAccepted();
        void SaveDialogRejected();

        InputDialogValidatable* GetSaveLayoutNameDialog();
    private:
        bool mIsSwitching;
        InputDialogValidatable* m_inputDialog = nullptr;
    };
}   // namespace EMStudio
