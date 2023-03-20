/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        char    m_fileTypeCode[9];   // "EMSLAYOUT", otherwise no valid layout file
        uint32  m_emfxVersionHigh;   // EMotion FX high version as used in EMStudio
        uint32  m_emfxVersionLow;    // EMotion FX low version as used in EMStudio
        char    m_emfxCompileDate[64];// EMotion FX compile date
        uint32  m_layoutVersionHigh; // layout file type high version
        uint32  m_layoutVersionLow;  // layout file type low version
        char    m_compileDate[64];   // EMStudio compile date
        char    m_description[256];  // optional description of the layout
        uint32  m_numPlugins;        // the number of plugins

        // followed by:
        //      LayoutPluginHeader[m_numPlugins]
        //      uint32 mainWindowStateSize
        //      int8 mainWindowState[mainWindowStateSize]
    };

    // the plugin data header
    struct LayoutPluginHeader
    {
        uint32  m_dataSize;          // data size of the data which the given plugin will store
        char    m_pluginName[128];   // the name of the plugin (its ID to create as passed to PluginManager::CreateWindowOfType)
        char    m_objectName[128];
        uint32  m_dataVersion;       // the data version, to for backward compatibility of loading individual plugin settings from layout files

        // followed by:
        //      int8    pluginData[m_dataSize]
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
        bool m_isSwitching;
        InputDialogValidatable* m_inputDialog = nullptr;
    };
}   // namespace EMStudio
