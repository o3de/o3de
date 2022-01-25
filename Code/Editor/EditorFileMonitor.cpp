/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "EditorFileMonitor.h"

// Editor
#include "CryEdit.h"

#include <AzCore/Utils/Utils.h>

//////////////////////////////////////////////////////////////////////////
CEditorFileMonitor::CEditorFileMonitor()
{
    GetIEditor()->RegisterNotifyListener(this);
}

//////////////////////////////////////////////////////////////////////////
CEditorFileMonitor::~CEditorFileMonitor()
{
    CFileChangeMonitor::DeleteInstance();
}

//////////////////////////////////////////////////////////////////////////
void CEditorFileMonitor::OnEditorNotifyEvent(EEditorNotifyEvent ev)
{
    if (ev == eNotify_OnInit)
    {
        // We don't want the file monitor to be enabled while
        // in console mode...
        if (!GetIEditor()->IsInConsolewMode())
        {
            MonitorDirectories();
        }

        CFileChangeMonitor::Instance()->Subscribe(this);
    }
    else if (ev == eNotify_OnQuit)
    {
        CFileChangeMonitor::Instance()->StopMonitor();
        GetIEditor()->UnregisterNotifyListener(this);
    }
}

//////////////////////////////////////////////////////////////////////////
bool CEditorFileMonitor::RegisterListener(IFileChangeListener* pListener, const char* sMonitorItem)
{
    return RegisterListener(pListener, sMonitorItem, "*");
}


//////////////////////////////////////////////////////////////////////////
static AZStd::string CanonicalizePath(const char* path)
{
    auto canon = QFileInfo(path).canonicalFilePath();
    return canon.isEmpty() ? AZStd::string(path) : AZStd::string(canon.toUtf8());
}

//////////////////////////////////////////////////////////////////////////
bool CEditorFileMonitor::RegisterListener(IFileChangeListener* pListener, const char* sFolderRelativeToGame, const char* sExtension)
{
    bool success = true;

    AZStd::string gameFolder = Path::GetEditingGameDataFolder().c_str();
    AZStd::string naivePath;
    CFileChangeMonitor* fileChangeMonitor = CFileChangeMonitor::Instance();
    AZ_Assert(fileChangeMonitor, "CFileChangeMonitor singleton missing.");

    naivePath += gameFolder;
    // Append slash in preparation for appending the second part.
    naivePath = PathUtil::AddSlash(naivePath);
    naivePath += sFolderRelativeToGame;
    AZ::StringFunc::Replace(naivePath, '/', '\\');

    // Remove the final slash if the given item is a folder so the file change monitor correctly picks up on it.
    naivePath = PathUtil::RemoveSlash(naivePath);

    AZStd::string canonicalizedPath = CanonicalizePath(naivePath.c_str());

    if (fileChangeMonitor->IsDirectory(canonicalizedPath.c_str()) || fileChangeMonitor->IsFile(canonicalizedPath.c_str()))
    {
        if (fileChangeMonitor->MonitorItem(canonicalizedPath.c_str()))
        {
            m_vecFileChangeCallbacks.push_back(SFileChangeCallback(pListener, sFolderRelativeToGame, sExtension));
        }
        else
        {
            CryLogAlways("File Monitor: [%s] not found outside of PAK files. Monitoring disabled for this item", sFolderRelativeToGame);
            success = false;
        }
    }

    return success;
}

bool CEditorFileMonitor::UnregisterListener(IFileChangeListener* pListener)
{
    bool bRet = false;

    // Note that we remove the listener, but we don't currently remove the monitored item
    // from the file monitor. This is fine, but inefficient

    std::vector<SFileChangeCallback>::iterator iter = m_vecFileChangeCallbacks.begin();
    while (iter != m_vecFileChangeCallbacks.end())
    {
        if (iter->pListener == pListener)
        {
            iter = m_vecFileChangeCallbacks.erase(iter);
            bRet = true;
        }
        else
        {
            iter++;
        }
    }

    return bRet;
}

//////////////////////////////////////////////////////////////////////////
void CEditorFileMonitor::MonitorDirectories()
{
    QString primaryCD = Path::AddPathSlash(QString(GetIEditor()->GetPrimaryCDFolder()));

    // NOTE: Instead of monitoring each sub-directory we monitor the whole root
    // folder. This is needed since if the sub-directory does not exist when
    // we register it it will never get monitored properly.
    CFileChangeMonitor::Instance()->MonitorItem(QStringLiteral("%1/%2/").arg(primaryCD).arg(QString::fromLatin1(Path::GetEditingGameDataFolder().c_str())));

    // Add editor directory for scripts
    CFileChangeMonitor::Instance()->MonitorItem(QStringLiteral("%1/Editor/").arg(primaryCD));
}

QString RemoveGameName(const QString &filename)
{
    // Remove first part of path.  File coming in has the game name included
    // eg (AutomatedTesting/Animations/Chicken/anim_chicken_flapping.i_caf)->(Animations/Chicken/anim_chicken_flapping.i_caf)

    int indexOfFirstSlash = filename.indexOf('/');
    int indexOfFirstBackSlash = filename.indexOf('\\');
    if (indexOfFirstSlash >= 0)
    {
        if (indexOfFirstBackSlash >= 0 && indexOfFirstBackSlash < indexOfFirstSlash)
        {
            indexOfFirstSlash = indexOfFirstBackSlash;
        }
    }
    else
    {
        indexOfFirstSlash = indexOfFirstBackSlash;
    }
    return filename.mid(indexOfFirstSlash + 1);
}
///////////////////////////////////////////////////////////////////////////

// Called when file monitor message is received
void CEditorFileMonitor::OnFileMonitorChange(const SFileChangeInfo& rChange)
{
    CCryEditApp* app = CCryEditApp::instance();
    if (app == nullptr || app->IsExiting())
    {
        return;
    }

    // skip folders!
    if (QFileInfo(rChange.filename).isDir())
    {
        return;
    }

    // Process updated file.
    // Make file relative to PrimaryCD folder.
    QString filename = rChange.filename;

    // Make path relative to the the project directory
    AZ::IO::Path projectPath{ AZ::Utils::GetProjectPath() };
    AZ::IO::FixedMaxPath projectRelativeFilePath = AZ::IO::PathView(filename.toUtf8().constData()).LexicallyProximate(
        projectPath);

    if (!projectRelativeFilePath.empty())
    {
        AZ::IO::PathView ext = projectRelativeFilePath.Extension();

        // Check for File Monitor callback
        std::vector<SFileChangeCallback>::iterator iter;
        for (iter = m_vecFileChangeCallbacks.begin(); iter != m_vecFileChangeCallbacks.end(); ++iter)
        {
            SFileChangeCallback& sCallback = *iter;

            // We compare against length of callback string, so we get directory matches as well as full filenames
            if (sCallback.pListener)
            {
                if (sCallback.extension == "*" || AZ::IO::PathView(sCallback.extension.toUtf8().constData()) == ext)
                {
                    if (AZ::IO::PathView(sCallback.item.toUtf8().constData()) == projectRelativeFilePath)
                    {
                        sCallback.pListener->OnFileChange(qPrintable(projectRelativeFilePath.c_str()), IFileChangeListener::EChangeType(rChange.changeType));
                    }
                }
            }
        }

        // Set this flag to make sure that the viewport will update at least once,
        // so that the changes will be shown, even if the app does not have focus.
        CCryEditApp::instance()->ForceNextIdleProcessing();
    }
}
