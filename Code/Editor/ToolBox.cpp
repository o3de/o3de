/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : ToolBox Macro System


#include "EditorDefs.h"

#include "ToolBox.h"

#include <AzCore/Utils/Utils.h>

// AzToolsFramework
#include <AzToolsFramework/API/EditorPythonRunnerRequestsBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

// Editor
#include "Settings.h"
#include "ActionManager.h"


//////////////////////////////////////////////////////////////////////////
// CToolBoxCommand
//////////////////////////////////////////////////////////////////////////
void CToolBoxCommand::Save(XmlNodeRef commandNode) const
{
    commandNode->setAttr("type", (int)m_type);
    commandNode->setAttr("text", m_text.toUtf8().data());
    commandNode->setAttr("bVariableToggle", m_bVariableToggle);
}

//////////////////////////////////////////////////////////////////////////
void CToolBoxCommand::Load(XmlNodeRef commandNode)
{
    int type = 0;
    commandNode->getAttr("type", type);
    m_type = CToolBoxCommand::EType(type);
    m_text = commandNode->getAttr("text");
    commandNode->getAttr("bVariableToggle", m_bVariableToggle);
}

//////////////////////////////////////////////////////////////////////////
void CToolBoxCommand::Execute() const
{
    if (m_type == CToolBoxCommand::eT_SCRIPT_COMMAND)
    {
        using namespace AzToolsFramework;
        EditorPythonRunnerRequestBus::Broadcast(&EditorPythonRunnerRequestBus::Events::ExecuteByString, m_text.toUtf8().data(), false);
    }
    else if (m_type == CToolBoxCommand::eT_CONSOLE_COMMAND)
    {
        if (m_bVariableToggle)
        {
            // Toggle the variable.
            float val = GetIEditor()->GetConsoleVar(m_text.toUtf8().data());
            bool bOn = val != 0;
            GetIEditor()->SetConsoleVar(m_text.toUtf8().data(), (bOn) ? 0 : 1);
        }
        else
        {
            // does the command required an active Python interpreter?
            if (m_text.contains("pyRunFile") && !AzToolsFramework::EditorPythonRunnerRequestBus::HasHandlers())
            {
                AZ_Warning("toolbar", false, "The command '%s' requires an embedded Python interpreter. "
                                             "The gem named EditorPythonBindings offers this service. "
                                             "Please enable this gem for the project.", m_text.toUtf8().data());
                return;
            }
            GetIEditor()->GetSystem()->GetIConsole()->ExecuteString(m_text.toUtf8().data());
        }
    }
}

//////////////////////////////////////////////////////////////////////////
// CToolBoxMacro
//////////////////////////////////////////////////////////////////////////
void CToolBoxMacro::Save(XmlNodeRef macroNode) const
{
    for (size_t i = 0; i < m_commands.size(); ++i)
    {
        XmlNodeRef commandNode = macroNode->newChild("command");
        m_commands[i]->Save(commandNode);
    }
}

//////////////////////////////////////////////////////////////////////////
void CToolBoxMacro::Load(XmlNodeRef macroNode)
{
    for (int i = 0; i < macroNode->getChildCount(); ++i)
    {
        XmlNodeRef commandNode = macroNode->getChild(i);
        m_commands.push_back(new CToolBoxCommand);
        m_commands[i]->Load(commandNode);
    }
}

//////////////////////////////////////////////////////////////////////////
void CToolBoxMacro::AddCommand(CToolBoxCommand::EType type, const QString& command, bool bVariableToggle)
{
    CToolBoxCommand* pNewCommand = new CToolBoxCommand;
    pNewCommand->m_type = type;
    pNewCommand->m_text = command;
    pNewCommand->m_bVariableToggle = bVariableToggle;
    m_commands.push_back(pNewCommand);
}

//////////////////////////////////////////////////////////////////////////
void CToolBoxMacro::Clear()
{
    for (size_t i = 0; i < m_commands.size(); ++i)
    {
        delete m_commands[i];
    }

    m_commands.clear();
}

//////////////////////////////////////////////////////////////////////////
const CToolBoxCommand* CToolBoxMacro::GetCommandAt(int index) const
{
    assert(0 <= index && index < m_commands.size());

    return m_commands[index];
}

//////////////////////////////////////////////////////////////////////////
CToolBoxCommand* CToolBoxMacro::GetCommandAt(int index)
{
    assert(0 <= index && index < m_commands.size());

    return m_commands[index];
}

//////////////////////////////////////////////////////////////////////////
void CToolBoxMacro::SwapCommand(int index1, int index2)
{
    assert(0 <= index1 && index1 < m_commands.size());
    assert(0 <= index2 && index2 < m_commands.size());
    std::swap(m_commands[index1], m_commands[index2]);
}

void CToolBoxMacro::RemoveCommand(int index)
{
    assert(0 <= index && index < m_commands.size());

    m_commands.erase(m_commands.begin() + index);
}

//////////////////////////////////////////////////////////////////////////
void CToolBoxMacro::Execute() const
{
    for (size_t i = 0; i < m_commands.size(); ++i)
    {
        m_commands[i]->Execute();
    }
}

//////////////////////////////////////////////////////////////////////////
// CToolBoxManager
//////////////////////////////////////////////////////////////////////////
int CToolBoxManager::GetMacroCount(bool bToolbox) const
{
    if (bToolbox)
    {
        return int(m_macros.size());
    }
    return int(m_shelveMacros.size());
}

//////////////////////////////////////////////////////////////////////////
const CToolBoxMacro* CToolBoxManager::GetMacro(int iIndex, bool bToolbox) const
{
    if (bToolbox)
    {
        assert(0 <= iIndex && iIndex < m_macros.size());
        return m_macros[iIndex];
    }
    else
    {
        assert(0 <= iIndex && iIndex < m_shelveMacros.size());
        return m_shelveMacros[iIndex];
    }
    return NULL;
}

//////////////////////////////////////////////////////////////////////////
CToolBoxMacro* CToolBoxManager::GetMacro(int iIndex, bool bToolbox)
{
    if (bToolbox)
    {
        assert(0 <= iIndex && iIndex < m_macros.size());
        return m_macros[iIndex];
    }
    else
    {
        assert(0 <= iIndex && iIndex < m_shelveMacros.size());
        return m_shelveMacros[iIndex];
    }
    return NULL;
}

//////////////////////////////////////////////////////////////////////////
int CToolBoxManager::GetMacroIndex(const QString& title, bool bToolbox) const
{
    if (bToolbox)
    {
        for (size_t i = 0; i < m_macros.size(); ++i)
        {
            if (QString::compare(m_macros[i]->GetTitle(), title, Qt::CaseInsensitive) == 0)
            {
                return int(i);
            }
        }
    }
    else
    {
        for (size_t i = 0; i < m_shelveMacros.size(); ++i)
        {
            if (QString::compare(m_shelveMacros[i]->GetTitle(), title, Qt::CaseInsensitive) == 0)
            {
                return int(i);
            }
        }
    }

    return -1;
}

//////////////////////////////////////////////////////////////////////////
CToolBoxMacro* CToolBoxManager::NewMacro(const QString& title, bool bToolbox, int* newIdx)
{
    if (bToolbox)
    {
        const int macroCount = m_macros.size();
        if (macroCount > ID_TOOL_LAST - ID_TOOL_FIRST + 1)
        {
            return NULL;
        }

        for (size_t i = 0; i < macroCount; ++i)
        {
            if (QString::compare(m_macros[i]->GetTitle(), title, Qt::CaseInsensitive) == 0)
            {
                return NULL;
            }
        }

        CToolBoxMacro* pNewTool = new CToolBoxMacro(title);
        if (newIdx)
        {
            *newIdx = macroCount;
        }
        m_macros.push_back(pNewTool);
        return pNewTool;
    }
    else
    {
        const int shelveMacroCount = m_shelveMacros.size();
        if (shelveMacroCount > ID_TOOL_SHELVE_LAST - ID_TOOL_SHELVE_FIRST + 1)
        {
            return NULL;
        }

        CToolBoxMacro* pNewTool = new CToolBoxMacro(title);
        if (newIdx)
        {
            *newIdx = shelveMacroCount;
        }
        m_shelveMacros.push_back(pNewTool);
        return pNewTool;
    }
    return NULL;
}

//////////////////////////////////////////////////////////////////////////
bool CToolBoxManager::SetMacroTitle(int index, const QString& title, bool bToolbox)
{
    if (bToolbox)
    {
        assert(0 <= index && index < m_macros.size());
        for (size_t i = 0; i < m_macros.size(); ++i)
        {
            if (i == index)
            {
                continue;
            }

            if (QString::compare(m_macros[i]->GetTitle(), title, Qt::CaseInsensitive) == 0)
            {
                return false;
            }
        }

        m_macros[index]->SetTitle(title);
    }
    else
    {
        assert(0 <= index && index < m_shelveMacros.size());
        for (size_t i = 0; i < m_shelveMacros.size(); ++i)
        {
            if (i == index)
            {
                continue;
            }

            if (QString::compare(m_shelveMacros[i]->GetTitle(), title, Qt::CaseInsensitive) == 0)
            {
                return false;
            }
        }

        m_shelveMacros[index]->SetTitle(title);
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
void CToolBoxManager::Load(ActionManager* actionManager)
{
    Clear();

    QString path;
    GetSaveFilePath(path);
    Load(path, nullptr, true, nullptr);

    if (actionManager)
    {
        auto engineSourceAssetPath = AZ::IO::FixedMaxPath(AZ::Utils::GetEnginePath()) / "Assets";
        LoadShelves((engineSourceAssetPath / "Editor" / "Scripts").c_str(),
            (engineSourceAssetPath / "Editor" / "Scripts" / "Shelves").c_str(), actionManager);
    }
}

void CToolBoxManager::LoadShelves(QString scriptPath, QString shelvesPath, ActionManager* actionManager)
{
    IFileUtil::FileArray files;
    CFileUtil::ScanDirectory(shelvesPath, "*.xml", files);

    const int shelfCount = files.size();
    for (int idx = 0; idx < shelfCount; ++idx)
    {
        if (Path::GetExt(files[idx].filename) != "xml")
        {
            continue;
        }

        QString shelfName(PathUtil::GetFileName(files[idx].filename.toUtf8().data()));

        AmazonToolbar toolbar(shelfName, shelfName);
        Load(shelvesPath + QString("/") + files[idx].filename, &toolbar, false, actionManager);

        m_toolbars.push_back(toolbar);
    }
}

void CToolBoxManager::Load(QString xmlpath, AmazonToolbar* pToolbar, bool bToolbox, ActionManager* actionManager)
{
    XmlNodeRef toolBoxNode = XmlHelpers::LoadXmlFromFile(xmlpath.toUtf8().data());
    if (toolBoxNode == NULL)
    {
        return;
    }

    if (!pToolbar)
    {
        GetIEditor()->GetSettingsManager()->AddSettingsNode(toolBoxNode);
    }
    else
    {
        const char* PRETTY_NAME_ATTR = "prettyName";
        const char* SHOW_BY_DEFAULT_ATTR = "showByDefault";
        const char* SHELF_NAME_ATTR = "shelfName";

        QString shelfName = pToolbar->GetName();

        if (toolBoxNode->haveAttr(SHELF_NAME_ATTR))
        {
            shelfName = toolBoxNode->getAttr(SHELF_NAME_ATTR);
        }

        QString prettyName = shelfName;

        if (toolBoxNode->haveAttr(PRETTY_NAME_ATTR))
        {
            prettyName = toolBoxNode->getAttr(PRETTY_NAME_ATTR);
        }

        pToolbar->SetName(shelfName, prettyName);

        if (toolBoxNode->haveAttr(SHOW_BY_DEFAULT_ATTR))
        {
            const char* showByDefaultAttrString = toolBoxNode->getAttr(SHOW_BY_DEFAULT_ATTR);

            QString showByDefaultString = QString(showByDefaultAttrString);
            showByDefaultString = showByDefaultString.trimmed();

            bool hideByDefault = (QString::compare(showByDefaultString, "false", Qt::CaseInsensitive) == 0) || (QString::compare(showByDefaultString, "0") == 0);
            pToolbar->SetShowByDefault(!hideByDefault);
        }
    }

    AZ::IO::FixedMaxPathString engineRoot = AZ::Utils::GetEnginePath();
    QDir engineDir = !engineRoot.empty() ? QDir(QString(engineRoot.c_str())) : QDir::current();

    string enginePath = PathUtil::AddSlash(engineDir.absolutePath().toUtf8().data());

    for (int i = 0; i < toolBoxNode->getChildCount(); ++i)
    {
        XmlNodeRef macroNode = toolBoxNode->getChild(i);
        QString title = macroNode->getAttr("title");
        QString shortcutName = macroNode->getAttr("shortcut");
        QString iconPath = macroNode->getAttr("icon");

        int idx = -1;
        CToolBoxMacro* pMacro = NewMacro(title, bToolbox, &idx);
        if (!pMacro || idx == -1)
        {
            continue;
        }

        pMacro->Load(macroNode);
        pMacro->SetShortcutName(shortcutName);
        pMacro->SetIconPath(iconPath.toUtf8().data());
        pMacro->SetToolbarId(-1);

        if (!pToolbar)
        {
            continue;
        }

        string shelfPath = PathUtil::GetParentDirectory(xmlpath.toUtf8().data());
        string fullIconPath = enginePath + PathUtil::AddSlash(shelfPath.c_str());
        fullIconPath.append(iconPath.toUtf8().data());

        pMacro->SetIconPath(fullIconPath);

        QString toolTip(macroNode->getAttr("tooltip"));
        pMacro->action()->setToolTip(toolTip);

        int actionId;

        if (pMacro->GetCommandCount() == 0 || pMacro->GetCommandAt(0)->m_type == CToolBoxCommand::eT_INVALID_COMMAND)
        {
            actionId = ID_TOOLBAR_SEPARATOR;
        }
        else
        {
            actionId = bToolbox ? ID_TOOL_FIRST + idx : ID_TOOL_SHELVE_FIRST + idx;

            // ActionManager uses a QSignalMapper internally. We need to disconnect any existing
            // connections here otherwise the macro will be called more than once.
            QObject::disconnect(pMacro->action(), &QAction::triggered, nullptr, nullptr);

            actionManager->AddAction(actionId, pMacro->action());
        }

        pToolbar->AddAction(actionId);
    }
}

//////////////////////////////////////////////////////////////////////////
void CToolBoxManager::Save() const
{
    XmlNodeRef toolBoxNode = XmlHelpers::CreateXmlNode(TOOLBOXMACROS_NODE);
    for (size_t i = 0; i < m_macros.size(); ++i)
    {
        if (m_macros[i]->GetToolbarId() != -1)
        {
            continue;
        }

        XmlNodeRef macroNode = toolBoxNode->newChild("macro");
        macroNode->setAttr("title", m_macros[i]->GetTitle().toUtf8().data());
        macroNode->setAttr("shortcut", m_macros[i]->GetShortcutName().toString().toUtf8().data());
        macroNode->setAttr("icon", m_macros[i]->GetIconPath().toUtf8().data());
        m_macros[i]->Save(macroNode);
    }
    QString path;
    GetSaveFilePath(path);
    XmlHelpers::SaveXmlNode(GetIEditor()->GetFileUtil(), toolBoxNode, path.toUtf8().data());
}

//////////////////////////////////////////////////////////////////////////
void CToolBoxManager::Clear()
{
    for (size_t i = 0; i < m_macros.size(); ++i)
    {
        delete m_macros[i];
    }
    m_macros.clear();
}

//////////////////////////////////////////////////////////////////////////
void CToolBoxManager::ExecuteMacro(int iIndex, bool bToolbox) const
{
    if (iIndex >= 0 && iIndex < GetMacroCount(bToolbox) && GetMacro(iIndex, bToolbox))
    {
        GetMacro(iIndex, bToolbox)->Execute();
    }
}

//////////////////////////////////////////////////////////////////////////
void CToolBoxManager::ExecuteMacro(const QString& name, bool bToolbox) const
{
    if (bToolbox)
    {
        // Find tool with this name.
        for (size_t i = 0; i < m_macros.size(); ++i)
        {
            if (QString::compare(m_macros[i]->GetTitle(), name, Qt::CaseInsensitive) == 0)
            {
                ExecuteMacro(int(i), bToolbox);
                break;
            }
        }
    }
    else
    {
        // Find tool with this name.
        for (size_t i = 0; i < m_shelveMacros.size(); ++i)
        {
            if (QString::compare(m_shelveMacros[i]->GetTitle(), name, Qt::CaseInsensitive) == 0)
            {
                ExecuteMacro(int(i), bToolbox);
                break;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CToolBoxManager::SwapMacro(int index1, int index2, bool bToolbox)
{
    assert(0 <= index1 && index1 < GetMacroCount(bToolbox));
    assert(0 <= index2 && index2 < GetMacroCount(bToolbox));
    if (bToolbox)
    {
        std::swap(m_macros[index1], m_macros[index2]);
    }
    else
    {
        std::swap(m_shelveMacros[index1], m_shelveMacros[index2]);
    }
}

//////////////////////////////////////////////////////////////////////////
void CToolBoxManager::RemoveMacro(int index, bool bToolbox)
{
    assert(0 <= index && index < GetMacroCount(bToolbox));

    if (bToolbox)
    {
        delete m_macros[index];
        m_macros[index] = nullptr;
        m_macros.erase(m_macros.begin() + index);
    }
    else
    {
        delete m_shelveMacros[index];
        m_shelveMacros[index] = nullptr;
        m_shelveMacros.erase(m_shelveMacros.begin() + index);
    }
}

//////////////////////////////////////////////////////////////////////////
void CToolBoxManager::GetSaveFilePath(QString& outPath) const
{
    outPath = Path::GetResolvedUserSandboxFolder();
    outPath += "Macros.xml";
}

//////////////////////////////////////////////////////////////////////////

const std::vector<AmazonToolbar>& CToolBoxManager::GetToolbars() const
{
    return m_toolbars;
}
