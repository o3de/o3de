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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_EDITOR_LENSFLAREEDITOR_LENSFLAREMANAGER_H
#define CRYINCLUDE_EDITOR_LENSFLAREEDITOR_LENSFLAREMANAGER_H
#pragma once

#include "BaseLibraryManager.h"
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>

class IOpticsElementBase;
class CLensFlareEditor;

AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
class CRYEDIT_API CLensFlareManager
    : public CBaseLibraryManager
    , private AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler

{
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
public:
    CLensFlareManager();
    virtual ~CLensFlareManager();

    void ClearAll();

    virtual bool LoadFlareItemByName(const QString& fullItemName, IOpticsElementBasePtr pDestOptics);
    void Modified();
    //! Path to libraries in this manager.
    QString GetLibsPath();
    IDataBaseLibrary* LoadLibrary(const QString& filename, bool bReload = false);

    static bool IsLensFlareLibraryXML(const char* fullFilePath);

private:
    CBaseLibraryItem* MakeNewItem();
    CBaseLibrary* MakeNewLibrary();

    //! Root node where this library will be saved.
    QString GetRootNodeName();
    QString m_libsPath;
    
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // AssetBrowser::AssetBrowserInteractionNotificationBus::Handler
    AzToolsFramework::AssetBrowser::SourceFileDetails GetSourceFileDetails(const char* fullSourceFileName) override;
    virtual AZ::s32 GetPriority() const override { return 1; } // get our priority in before others.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////


};

#endif // CRYINCLUDE_EDITOR_LENSFLAREEDITOR_LENSFLAREMANAGER_H
