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

#include "EditorDefs.h"

#include "LensFlareManager.h"

// AzCore
#include <AzCore/XML/rapidxml.h>
#include <AzCore/std/string/wildcard.h>

// Editor
#include "LensFlareEditor.h"
#include "LensFlareItem.h"
#include "LensFlareLibrary.h"
#include "LensFlareUtil.h"


//////////////////////////////////////////////////////////////////////////
// CLensFlareManager implementation.
//////////////////////////////////////////////////////////////////////////
CLensFlareManager::CLensFlareManager()
    : CBaseLibraryManager()
{
    m_bUniqNameMap = true;
    m_pLevelLibrary = (CBaseLibrary*)AddLibrary("Level", true);
    AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusConnect();
}

//////////////////////////////////////////////////////////////////////////
CLensFlareManager::~CLensFlareManager()
{
    AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusDisconnect();
}

//////////////////////////////////////////////////////////////////////////
void CLensFlareManager::ClearAll()
{
    CBaseLibraryManager::ClearAll();

    m_pLevelLibrary = (CBaseLibrary*)AddLibrary("Level", true);
}

//////////////////////////////////////////////////////////////////////////
CBaseLibraryItem* CLensFlareManager::MakeNewItem()
{
    return new CLensFlareItem;
}

//////////////////////////////////////////////////////////////////////////
CBaseLibrary* CLensFlareManager::MakeNewLibrary()
{
    return new CLensFlareLibrary(this);
}

//////////////////////////////////////////////////////////////////////////
QString CLensFlareManager::GetRootNodeName()
{
    return "FlareLibs";
}

//////////////////////////////////////////////////////////////////////////
QString CLensFlareManager::GetLibsPath()
{
    if (m_libsPath.isEmpty())
    {
        m_libsPath += FLARE_LIBS_PATH;
    }

    return m_libsPath;
}

//////////////////////////////////////////////////////////////////////////
bool CLensFlareManager::LoadFlareItemByName(const QString& fullItemName, IOpticsElementBasePtr pDestOptics)
{
    if (pDestOptics == NULL)
    {
        return false;
    }

    CLensFlareItem* pLensFlareItem = (CLensFlareItem*)LoadItemByName(fullItemName);
    if (pLensFlareItem == NULL)
    {
        return false;
    }

    LensFlareUtil::CopyOptics(pLensFlareItem->GetOptics(), pDestOptics, true);
    return true;
}

//////////////////////////////////////////////////////////////////////////
void CLensFlareManager::Modified()
{
    CLensFlareEditor* pEditor = CLensFlareEditor::GetLensFlareEditor();
    if (pEditor == NULL)
    {
        return;
    }
    if (pEditor->GetCurrentLibrary())
    {
        pEditor->GetCurrentLibrary()->SetModified(true);
    }
}

//////////////////////////////////////////////////////////////////////////
IDataBaseLibrary* CLensFlareManager::LoadLibrary(const QString& filename, [[maybe_unused]] bool bReload)
{
    CLensFlareEditor* pEditor = CLensFlareEditor::GetLensFlareEditor();

    QString fileNameWithGameFolder(filename);

    fileNameWithGameFolder.replace('\\', '/');

    int nGamePathLength = static_cast<int>(Path::GetEditingGameDataFolder().size());
    QString gamePathName;
    if (nGamePathLength < filename.length())
    {
        gamePathName = filename.left(nGamePathLength);
    }
    if (gamePathName != Path::GetEditingGameDataFolder().c_str())
    {
        fileNameWithGameFolder.insert(0, "/");
        fileNameWithGameFolder.insert(0, Path::GetEditingGameDataFolder().c_str());
    }

    int nLibraryIndex(-1);
    bool bSameAsCurrentLibrary(false);

    for (int i = 0; i < m_libs.size(); i++)
    {
        if (QString::compare(fileNameWithGameFolder, m_libs[i]->GetFilename(), Qt::CaseInsensitive) == 0)
        {
            IDataBaseLibrary* pExistingLib = m_libs[i];
            for (int j = 0; j < pExistingLib->GetItemCount(); j++)
            {
                UnregisterItem((CBaseLibraryItem*)pExistingLib->GetItem(j));
            }
            pExistingLib->RemoveAllItems();
            nLibraryIndex = i;
            if (pEditor)
            {
                bSameAsCurrentLibrary = pEditor->GetCurrentLibrary() == pExistingLib;
            }
            break;
        }
    }

    TSmartPtr<CBaseLibrary> pLib = MakeNewLibrary();
    if (!pLib->Load(filename))
    {
        Error(QObject::tr("Failed to Load Item Library: %1").arg(filename).toUtf8().data());
        return NULL;
    }

    if (nLibraryIndex != -1)
    {
        m_libs[nLibraryIndex] = pLib;
        if (bSameAsCurrentLibrary && pEditor)
        {
            pEditor->ResetElementTreeControl();
            pEditor->SelectLibrary(pLib, true);
        }
    }
    else
    {
        m_libs.push_back(pLib);
    }

    pLib->SetFilename(filename);

    return pLib;
}


bool CLensFlareManager::IsLensFlareLibraryXML(const char* fileSourceFilePath)
{
    if ((!fileSourceFilePath) || (!AZStd::wildcard_match("*.xml", fileSourceFilePath)))
    {
        return false;
    }

    using namespace AZ::IO;

    bool isLensFlareLibrary = false;

    // we are forced to read the asset to discover its type since "xml" was the chosen extension.
    SystemFile::SizeType fileSize = SystemFile::Length(fileSourceFilePath);
    if (fileSize > 0)
    {
        AZStd::vector<char> buffer(fileSize + 1);
        buffer[fileSize] = 0;
        if (!AZ::IO::SystemFile::Read(fileSourceFilePath, buffer.data(), fileSize))
        {
            return false;
        }

        AZ::rapidxml::xml_document<char>* xmlDoc = azcreate(AZ::rapidxml::xml_document<char>, (), AZ::SystemAllocator, "LensFlareLibrary Temp XML Reader");
        if (xmlDoc->parse<AZ::rapidxml::parse_no_data_nodes>(buffer.data()))
        {
            AZ::rapidxml::xml_node<char>* xmlRootNode = xmlDoc->first_node();
            if (xmlRootNode)
            {
                if (azstricmp(xmlRootNode->name(), "LensFlareLibrary") == 0)
                {
                    isLensFlareLibrary = true;
                }
            }
        }

        azdestroy(xmlDoc, AZ::SystemAllocator, AZ::rapidxml::xml_document<char>);
    }

    return isLensFlareLibrary;
}


AzToolsFramework::AssetBrowser::SourceFileDetails CLensFlareManager::GetSourceFileDetails(const char* fullSourceFileName)
{
    if (IsLensFlareLibraryXML(fullSourceFileName))
    {
        return AzToolsFramework::AssetBrowser::SourceFileDetails("Icons/AssetBrowser/LensFlare_16.png");
    }
    return AzToolsFramework::AssetBrowser::SourceFileDetails();
}
