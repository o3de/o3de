/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "SettingsManager.h"

// Qt
#include <QDateTime>

// Editor
#include "Settings.h"
#include "MainWindow.h"
#include "ToolBox.h"
#include "QtViewPaneManager.h"


#define TOOLBOX_FILE "ToolBox.xml"
#define TOOLBOX_NODE "ToolBox"

CSettingsManager::CSettingsManager(EditorSettingsManagerType managerType)
{
    if (managerType < eSettingsManagerLast)
    {
        m_managerType = managerType;
    }
    else
    {
        managerType = eSettingsManagerMemoryStorage;
    }

    if (m_managerType == eSettingsManagerMemoryStorage)
    {
        m_pSettingsManagerMemoryNode = XmlHelpers::CreateXmlNode(EDITOR_SETTINGS_ROOT_NODE);
    }

    // Main frame layouts must be processed as first
    AddToolName(QStringLiteral(MAINFRM_LAYOUT_NORMAL), QStringLiteral("Sandbox Layout"));
    AddToolName(QStringLiteral(MAINFRM_LAYOUT_PREVIEW), QStringLiteral("Sandbox Preview Layout"));

    CreateDefaultLayoutSettingsFile();

    QDateTime theTime = QDateTime::currentDateTimeUtc();
    SaveLogEventSetting("EditorStart", "time", theTime.toString(LOG_DATETIME_FORMAT));
}

CSettingsManager::~CSettingsManager()
{
}

void CSettingsManager::SaveLayoutSettings(const QByteArray& layout, const QString& toolName)
{
    if (m_managerType == eSettingsManagerMemoryStorage)
    {
        XmlNodeRef rootLayoutNode = m_pSettingsManagerMemoryNode->findChild(EDITOR_LAYOUT_ROOT_NODE);
        if (rootLayoutNode)
        {
            XmlNodeRef xmlDockingLayoutNode = rootLayoutNode->findChild(EDITOR_LAYOUT_NODE);

            if (!xmlDockingLayoutNode)
            {
                return;
            }

            XmlNodeRef toolNode = xmlDockingLayoutNode->findChild(toolName.toUtf8().data());

            if (toolNode)
            {
                xmlDockingLayoutNode->removeChild(toolNode);
            }

            toolNode = XmlHelpers::CreateXmlNode(toolName.toUtf8().data());
            xmlDockingLayoutNode->addChild(toolNode);

            XmlNodeRef windowStateNode = XmlHelpers::CreateXmlNode("WindowState");
            toolNode->addChild(windowStateNode);

            windowStateNode->setContent(layout.toHex().data());
        }
    }
}

bool CSettingsManager::CreateDefaultLayoutSettingsFile()
{
    XmlNodeRef pLayoutRootNode = XmlHelpers::CreateXmlNode(EDITOR_LAYOUT_ROOT_NODE);
    XmlNodeRef pEditorLayoutNode = XmlHelpers::CreateXmlNode(EDITOR_LAYOUT_NODE);

    pLayoutRootNode->addChild(pEditorLayoutNode);

    if (m_pSettingsManagerMemoryNode->findChild(EDITOR_LAYOUT_ROOT_NODE))
    {
        m_pSettingsManagerMemoryNode->removeChild(m_pSettingsManagerMemoryNode->findChild(EDITOR_LAYOUT_ROOT_NODE));
    }

    m_pSettingsManagerMemoryNode->addChild(pLayoutRootNode);

    return true;
}

AZStd::vector<AZStd::string> CSettingsManager::BuildSettingsList()
{
    XmlNodeRef root = nullptr;

    root = m_pSettingsManagerMemoryNode;

    XmlNodeRef tmpNode = root->findChild(EDITOR_SETTINGS_CONTENT_NODE);
    if (!tmpNode)
    {
        root->addChild(root->createNode(EDITOR_SETTINGS_CONTENT_NODE));
        tmpNode = root->findChild(EDITOR_SETTINGS_CONTENT_NODE);

        if (!tmpNode)
        {
            AZ_Warning("Settings Manager", false, "Failed to build the settings list - could not find the root xml node.");
            return AZStd::vector<AZStd::string>();
        }
    }

    AZStd::vector<AZStd::string> result;
    BuildSettingsList_Helper(tmpNode, "", result);

    return result;
}

void CSettingsManager::BuildSettingsList_Helper(const XmlNodeRef& node, const AZStd::string_view& pathToNode, AZStd::vector<AZStd::string>& result)
{
    for (int i = 0; i < node->getNumAttributes(); ++i)
    {
        const char* key = nullptr;
        const char* value = nullptr;
        node->getAttributeByIndex(i, &key, &value);
        if (!pathToNode.empty())
        {
            result.push_back(AZStd::string(pathToNode));
        }
    }

    if (node->getChildCount() > 0)
    {
        // Explore Children
        int numChildren = node->getChildCount();
        for (int i = 0; i < numChildren; ++i)
        {
            if (pathToNode.empty())
            {
                BuildSettingsList_Helper(
                    node->getChild(i),
                    node->getChild(i)->getTag(),
                    result
                );
            }
            else
            {
                BuildSettingsList_Helper(
                    node->getChild(i),
                    AZStd::string(pathToNode) + "|" + node->getChild(i)->getTag(),
                    result
                );
            }

        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CSettingsManager::SaveSetting(const QString& path, const QString& attr, const QString& val)
{
    QStringList strNodes;

    QString fixedPath = path;

    // Make sure it ends with "\" before extraction of tokens
    QString slashChar = fixedPath.right(fixedPath.length() - 1);

    if (slashChar != "\\")
    {
        fixedPath += "\\";
    }

    Path::GetDirectoryQueue(fixedPath, strNodes);

    QString writeAttr = attr;

    // Spaces in node names not allowed
    writeAttr.replace(" ", "");

    XmlNodeRef root = nullptr;

    root = m_pSettingsManagerMemoryNode;

    XmlNodeRef tmpNode = root->findChild(EDITOR_SETTINGS_CONTENT_NODE);
    if (!tmpNode)
    {
        root->addChild(root->createNode(EDITOR_SETTINGS_CONTENT_NODE));
        tmpNode = root->findChild(EDITOR_SETTINGS_CONTENT_NODE);

        if (!tmpNode)
        {
            return;
        }
    }

    for (int i = 0; i < strNodes.size(); ++i)
    {
        if (!tmpNode->findChild(strNodes[i].toUtf8().data()))
        {
            tmpNode->addChild(tmpNode->createNode(strNodes[i].toUtf8().data()));
            tmpNode = tmpNode->findChild(strNodes[i].toUtf8().data());
        }
        else
        {
            tmpNode = tmpNode->findChild(strNodes[i].toUtf8().data());
        }
    }

    if (!tmpNode->findChild(writeAttr.toUtf8().data()))
    {
        tmpNode->addChild(tmpNode->createNode(writeAttr.toUtf8().data()));
        tmpNode = tmpNode->findChild(writeAttr.toUtf8().data());
    }
    else
    {
        tmpNode = tmpNode->findChild(writeAttr.toUtf8().data());
    }

    if (!tmpNode)
    {
        return;
    }

    tmpNode->setAttr(EDITOR_SETTINGS_ATTRIB_NAME, val.toUtf8().data());
}

void CSettingsManager::SaveSetting(const QString& path, const QString& attr, bool bVal)
{
    SaveSetting(path, attr, QString ::number(bVal));
}

void CSettingsManager::SaveSetting(const QString& path, const QString& attr, float fVal)
{
    SaveSetting(path, attr, QString::number(fVal));
}

void CSettingsManager::SaveSetting(const QString& path, const QString& attr, int iVal)
{
    SaveSetting(path, attr, QString::number(iVal));
}

void CSettingsManager::SaveSetting(const QString& path, const QString& attr, QColor color)
{
    SaveSetting(path, attr, color.name());
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CSettingsManager::LoadSetting(const QString& path, const QString& attr, QString& val)
{
    // Make sure it ends with "\" before extraction of tokens
    QString fixedPath = path;
    QString slashChar = fixedPath.right(fixedPath.length() - 1);

    if (slashChar != "\\")
    {
        fixedPath += "\\";
    }

    QStringList strNodes;
    Path::GetDirectoryQueue(fixedPath, strNodes);
    QString readAttr = attr;

    // Spaces in node names not allowed
    readAttr.replace(" ", "");

    XmlNodeRef root = nullptr;

    root = m_pSettingsManagerMemoryNode;

    XmlNodeRef tmpNode = nullptr;

    if (NeedSettingsNode(path))
    {
        tmpNode = root->findChild(EDITOR_SETTINGS_CONTENT_NODE);
    }
    else
    {
        tmpNode = root;
    }

    if (!tmpNode)
    {
        return nullptr;
    }

    for (int i = 0; i < strNodes.size(); ++i)
    {
        if (tmpNode->findChild(strNodes[i].toUtf8().data()))
        {
            tmpNode = tmpNode->findChild(strNodes[i].toUtf8().data());
        }
        else
        {
            return nullptr;
        }
    }

    if (!tmpNode->findChild(readAttr.toUtf8().data()))
    {
        return nullptr;
    }
    else
    {
        tmpNode = tmpNode->findChild(readAttr.toUtf8().data());
    }

    if (tmpNode->haveAttr(EDITOR_SETTINGS_ATTRIB_NAME))
    {
        tmpNode->getAttr(EDITOR_SETTINGS_ATTRIB_NAME, val);
    }

    return tmpNode;
}

void CSettingsManager::LoadSetting(const QString& path, const QString& attr, bool& bVal)
{
    QString defaultVal = QString::number(bVal);
    LoadSetting(path, attr, defaultVal);
    bVal = defaultVal.toInt() != 0;
}

void CSettingsManager::LoadSetting(const QString& path, const QString& attr, int& iVal)
{
    QString defaultVal = QString::number(iVal);
    LoadSetting(path, attr, defaultVal);
    iVal = defaultVal.toInt();
}

void CSettingsManager::LoadSetting(const QString& path, const QString& attr, float& fVal)
{
    QString defaultVal = QString::number(fVal, 'g');
    LoadSetting(path, attr, defaultVal);
    fVal = defaultVal.toFloat();
}

void CSettingsManager::LoadSetting(const QString& path, const QString& attr, QColor &val)
{
    QString defaultVal = val.name();
    LoadSetting(path, attr, defaultVal);
    val = QColor(defaultVal);
}

void CSettingsManager::AddToolVersion(const QString& toolName, const QString& toolVersion)
{
    if (toolName.isEmpty())
    {
        return;
    }

    if (stl::find_in_map(m_toolNames, toolName, nullptr) == "")
    {
        if (!toolVersion.isEmpty())
        {
            m_toolVersions[toolName] = toolVersion;
        }
        else
        {
            m_toolVersions[toolName] = "";
        }
    }
};

void CSettingsManager::AddToolName(const QString& toolName, const QString& humanReadableName)
{
    if (toolName == "")
    {
        return;
    }

    if (stl::find_in_map(m_toolNames, toolName, nullptr) == "")
    {
        if (!humanReadableName.isEmpty())
        {
            m_toolNames[toolName] = humanReadableName;
        }
        else
        {
            m_toolNames[toolName] = toolName;
        }
    }
};

void CSettingsManager::AddSettingsNode(XmlNodeRef newNode)
{
    QString nodeStr = newNode->getTag();

    XmlNodeRef oldNode = m_pSettingsManagerMemoryNode->findChild(nodeStr.toUtf8().data());

    if (oldNode)
    {
        m_pSettingsManagerMemoryNode->removeChild(oldNode);
    }

    m_pSettingsManagerMemoryNode->addChild(newNode);
}

void CSettingsManager::ExportSettings(XmlNodeRef exportNode, QString fileName)
{
    exportNode->saveToFile(fileName.toUtf8().data());
}

void CSettingsManager::Export()
{
    // Feed in-memory node of CSettingsManager with global Sandbox settings
    gSettings.Load();

    if (m_ExportFilePath.isEmpty())
    {
        return;
    }

    // Update to the latest layout
    UpdateLayoutNode();

    // Save CVars
    SerializeCVars(m_pSettingsManagerMemoryNode, false);

    m_pSettingsManagerMemoryNode->saveToFile(m_ExportFilePath.toUtf8().data());

    GetIEditor()->SetStatusText("Export Successful");
}

void CSettingsManager::UpdateLayoutNode()
{
    QtViewPaneManager::instance()->SaveLayout();

    XmlNodeRef rootLayoutNode = m_pSettingsManagerMemoryNode->findChild(EDITOR_LAYOUT_ROOT_NODE);
    if (!rootLayoutNode)
    {
        return;
    }

    XmlNodeRef xmlDockingLayoutNode = rootLayoutNode->findChild(EDITOR_LAYOUT_NODE);
    if (!xmlDockingLayoutNode)
    {
        return;
    }

    xmlDockingLayoutNode->removeAllChilds();

    // serialize layout for main window

    XmlNodeRef xmlMainFrameLayoutNode = XmlHelpers::CreateXmlNode(MAINFRM_LAYOUT_NORMAL);
    xmlDockingLayoutNode->addChild(xmlMainFrameLayoutNode);

    QtViewPaneManager::instance()->SerializeLayout(xmlMainFrameLayoutNode);

    // serialize layout for panes

    TToolNamesMap::const_iterator tIt;
    for (tIt = m_toolNames.begin(); tIt != m_toolNames.end(); ++tIt)
    {
        const QString& toolName = tIt->first;

        QMainWindow* pane = FindViewPane<QMainWindow>(tIt->second);
        if (pane)
        {
            SaveLayoutSettings(pane->saveState(), toolName);
        }
    }
}

void CSettingsManager::GetMatchingLayoutNames(TToolNamesMap& foundTools, XmlNodeRef& resultNode, QString file)
{
    // Need to populate in-memory node with available layouts.
    UpdateLayoutNode();

    XmlNodeRef root = XmlHelpers::LoadXmlFromFile(file.toUtf8().data());

    if (!root)
    {
        return;
    }

    root = root->findChild(EDITOR_LAYOUT_ROOT_NODE);

    if (!root)
    {
        return;
    }

    root = root->findChild(EDITOR_LAYOUT_NODE);

    if (!root)
    {
        return;
    }

    TToolNamesMap* toolNames = nullptr;

    if (!foundTools.empty())
    {
        toolNames = &foundTools;
    }
    else
    {
        toolNames = GetToolNames();
    }

    if (!toolNames)
    {
        return;
    }

    if (toolNames->empty())
    {
        return;
    }

    // toolIT->first is tool layout node name in XML file
    TToolNamesMap::const_iterator toolIT = toolNames->begin();

    for (; toolIT != toolNames->end(); ++toolIT)
    {
        if (root->findChild(toolIT->first.toUtf8().data()))
        {
            foundTools[toolIT->first] = toolIT->second;

            if (resultNode)
            {
                resultNode->addChild(root->findChild(toolIT->first.toUtf8().data()));
            }
        }
    }
}

void CSettingsManager::ImportSettings(QString file)
{
    XmlNodeRef root = XmlHelpers::LoadXmlFromFile(file.toUtf8().data());

    if (!root)
    {
        return;
    }

    XmlNodeRef importedSettingsContentNode = root->findChild(EDITOR_SETTINGS_CONTENT_NODE);

    if (!importedSettingsContentNode)
    {
        return;
    }

    // Remove old settings node
    XmlNodeRef oldSettingsContentNode = m_pSettingsManagerMemoryNode->findChild(EDITOR_SETTINGS_CONTENT_NODE);
    m_pSettingsManagerMemoryNode->removeChild(oldSettingsContentNode);

    // Add new, imported settings node
    m_pSettingsManagerMemoryNode->addChild(importedSettingsContentNode);

    // Force global settings to reload from memory node instead of registry
    gSettings.bSettingsManagerMode = true;
    gSettings.Load();
    gSettings.bSettingsManagerMode = false;

    // Dump ToolBox node on disk, replacing the old one
    XmlNodeRef toolBoxNode =  root->findChild(TOOLBOX_NODE);
    if (toolBoxNode)
    {
        toolBoxNode->saveToFile(TOOLBOX_FILE);
    }

    // Dump UserTools node on disk, replacing the old one
    XmlNodeRef userToolsNode =  root->findChild(TOOLBOXMACROS_NODE);

    if (userToolsNode)
    {
        QString macroFilePath;
        GetIEditor()->GetToolBoxManager()->GetSaveFilePath(macroFilePath);
        userToolsNode->saveToFile(macroFilePath.toUtf8().data());
        GetIEditor()->GetToolBoxManager()->Load();
    }

    // Get and update CVars
    SerializeCVars(root, true);

    GetIEditor()->SetStatusText("Import Successful");
}

bool CSettingsManager::NeedSettingsNode(const QString& path)
{
    if ((path != EDITOR_LAYOUT_ROOT_NODE) && (path != TOOLBOX_NODE) && (path != TOOLBOXMACROS_NODE))
    {
        return true;
    }
    else
    {
        return false;
    }
}

void CSettingsManager::SerializeCVars(XmlNodeRef& node, bool bLoad)
{
    int nNumberOfVariables(0);
    int nCurrentVariable(0);
    IConsole* piConsole(nullptr);
    ICVar* piVariable(nullptr);
    AZStd::vector<AZStd::string_view>  cszVariableNames;

    char* szKey(nullptr);
    char* szValue(nullptr);
    ICVar* piCVar(nullptr);

    piConsole = gEnv->pConsole;

    if (!piConsole)
    {
        return;
    }

    if (bLoad)
    {
        XmlNodeRef readNode = nullptr;
        XmlNodeRef inputCVarsNode = node->findChild(CVARS_NODE);

        if (!inputCVarsNode)
        {
            return;
        }

        // Remove Entities with ID`s from the list.
        for (int childNo = 0; childNo < inputCVarsNode->getChildCount(); ++childNo)
        {
            readNode = inputCVarsNode->getChild(childNo);

            for (int i = 0; i < readNode->getNumAttributes(); ++i)
            {
                readNode->getAttributeByIndex(i, (const char**)&szKey, (const char**)&szValue);
                piCVar = piConsole->GetCVar(szKey);
                if (!piCVar)
                {
                    continue;
                }
                piCVar->Set(szValue);
            }
        }
    }
    else
    {
        XmlNodeRef newCVarNode = nullptr;
        XmlNodeRef oldCVarsNode = node->findChild(CVARS_NODE);

        if (oldCVarsNode)
        {
            node->removeChild(oldCVarsNode);
        }

        XmlNodeRef cvarsNode = XmlHelpers::CreateXmlNode(CVARS_NODE);

        nNumberOfVariables = piConsole->GetNumVisibleVars();
        cszVariableNames.resize(nNumberOfVariables);

        if (piConsole->GetSortedVars(cszVariableNames) != nNumberOfVariables)
        {
            assert(false);
            return;
        }

        for (nCurrentVariable = 0; nCurrentVariable < cszVariableNames.size(); ++nCurrentVariable)
        {
            if (_stricmp(cszVariableNames[nCurrentVariable].data(), "_TestFormatMessage") == 0)
            {
                continue;
            }

            piVariable = piConsole->GetCVar(cszVariableNames[nCurrentVariable].data());
            if (!piVariable)
            {
                assert(false);
                continue;
            }

            newCVarNode = XmlHelpers::CreateXmlNode(CVAR_NODE);
            newCVarNode->setAttr(cszVariableNames[nCurrentVariable].data(), piVariable->GetString());
            cvarsNode->addChild(newCVarNode);
        }

        node->addChild(cvarsNode);
    }
}

void CSettingsManager::ReadValueStr(XmlNodeRef& sourceNode, const QString& path, const QString& attr, QString& val)
{
    // Make sure the path has "\" at the end
    QString fixedPath = path;

    // Make sure it ends with "\" before extraction of tokens
    QString slashChar = fixedPath.right(fixedPath.length() - 1);

    if (slashChar != "\\")
    {
        fixedPath += "\\";
    }

    QStringList strNodes;
    Path::GetDirectoryQueue(fixedPath, strNodes);
    QString readAttr = attr;

    // Spaces in node names not allowed
    readAttr.replace(" ", "");

    XmlNodeRef root = nullptr;
    XmlNodeRef tmpNode = nullptr;

    if (NeedSettingsNode(path))
    {
        tmpNode = sourceNode->findChild(EDITOR_SETTINGS_CONTENT_NODE);
    }
    else
    {
        tmpNode = sourceNode;
    }

    if (!tmpNode)
    {
        return;
    }

    for (int i = 0; i < strNodes.size(); ++i)
    {
        if (tmpNode->findChild(strNodes[i].toUtf8().data()))
        {
            tmpNode = tmpNode->findChild(strNodes[i].toUtf8().data());
        }
        else
        {
            return;
        }
    }

    if (!tmpNode->findChild(readAttr.toUtf8().data()))
    {
        return;
    }
    else
    {
        tmpNode = tmpNode->findChild(readAttr.toUtf8().data());
    }

    if (tmpNode->haveAttr(EDITOR_SETTINGS_ATTRIB_NAME))
    {
        tmpNode->getAttr(EDITOR_SETTINGS_ATTRIB_NAME, val);
    }

    return;
}

void CSettingsManager::RegisterEvent(const SEventLog& event)
{
    if (event.m_eventName.isEmpty())
    {
        return;
    }

    QString eventName = event.m_eventName;

    QString path(eventName);
    path.replace(" ", "");
    SaveLogEventSetting(path, EVENT_LOG_EVENT_NAME, event.m_eventName);
    QString subPath = path + "\\" + EVENT_LOG_EVENT_NAME;

    SaveLogEventSetting(subPath, EVENT_LOG_CALLER_VERSION, event.m_callerVersion);
    SaveLogEventSetting(subPath, "state", event.m_eventState);

    QDateTime theTime = QDateTime::currentDateTimeUtc();
    SaveLogEventSetting(subPath, "time", theTime.toString(LOG_DATETIME_FORMAT));
}

void CSettingsManager::UnregisterEvent(const SEventLog& event)
{
    XmlNodeRef root = XmlHelpers::LoadXmlFromFile(EDITOR_EVENT_LOG_FILE_PATH);

    if (!root)
    {
        return;
    }

    QString eventName = event.m_eventName;

    QString path(eventName);
    path.replace(" ", "");
    QString subPath = path + "\\" + EVENT_LOG_EVENT_NAME;

    XmlNodeRef resNode = LoadLogEventSetting(subPath, EDITOR_EVENT_LOG_ATTRIB_NAME, eventName, root);
    if (!resNode)
    {
        return;
    }

    root->removeChild(resNode->getParent());
    root->saveToFile(EDITOR_EVENT_LOG_FILE_PATH);
}

bool CSettingsManager::IsEventSafe(const SEventLog& event)
{
    XmlNodeRef root = XmlHelpers::LoadXmlFromFile(EDITOR_EVENT_LOG_FILE_PATH);

    if (!root)
    {
        return true;
    }

    QString eventName = event.m_eventName;

    QString path(eventName);
    path += "\\";
    path += EVENT_LOG_EVENT_NAME;
    path.replace(" ", "");
    XmlNodeRef resNode = LoadLogEventSetting(path, EDITOR_EVENT_LOG_ATTRIB_NAME, eventName, root);

    // Log entry not found, so it is safe to start
    if (!resNode)
    {
        return true;
    }

    XmlNodeRef callerVersion = resNode->findChild(EVENT_LOG_CALLER_VERSION);

    if (callerVersion)
    {
        QString callerVersionStr;

        if (callerVersion->haveAttr(EDITOR_EVENT_LOG_ATTRIB_NAME))
        {
            callerVersionStr = callerVersion->getAttr(EDITOR_EVENT_LOG_ATTRIB_NAME);
        }

        if (!callerVersionStr.isEmpty())
        {
            if (callerVersionStr != GetToolVersion(eventName))
            {
                return true;
            }
        }

        // The same version of tool/level found
        return false;
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
void CSettingsManager::SaveLogEventSetting(const QString& path, const QString& attr, const QString& val)
{
    QStringList strNodes;

    QString fixedPath = path;

    // Make sure it ends with "\" before extraction of tokens
    QString slashChar = fixedPath.right(fixedPath.length() - 1);

    if (slashChar != "\\")
    {
        fixedPath += "\\";
    }

    Path::GetDirectoryQueue(fixedPath, strNodes);

    QString writeAttr = attr;

    // Simple cleanup of node names - remove all characters except letters, digits,
    // underscores, colons, periods, and hyphens.
    // If this ever needs to be 100% compliant, the following rules would need to
    // be added:
    // - name must only *start* with a letter or underscore (not a digit, colon, period, or hyphen)
    // - name must not start with "xml" in any case combination
    const QRegularExpression xmlNodeCleanupRegex("[^a-zA-Z0-9_:.\\-]*");
    writeAttr.remove(xmlNodeCleanupRegex);
    for (auto& node : strNodes)
    {
        node.remove(xmlNodeCleanupRegex);
    }

    XmlNodeRef root = XmlHelpers::LoadXmlFromFile(EDITOR_EVENT_LOG_FILE_PATH);

    if (!root)
    {
        root = XmlHelpers::CreateXmlNode(EDITOR_EVENT_LOG_ROOT_NODE);
        root->saveToFile(EDITOR_LAYOUT_FILE_PATH);
    }

    XmlNodeRef tmpNode = root;

    for (int i = 0; i < strNodes.size(); ++i)
    {
        if (!tmpNode->findChild(strNodes[i].toUtf8().data()))
        {
            tmpNode->addChild(tmpNode->createNode(strNodes[i].toUtf8().data()));
            tmpNode = tmpNode->findChild(strNodes[i].toUtf8().data());
        }
        else
        {
            tmpNode = tmpNode->findChild(strNodes[i].toUtf8().data());
        }
    }

    if (!tmpNode->findChild(writeAttr.toUtf8().data()))
    {
        tmpNode->addChild(tmpNode->createNode(writeAttr.toUtf8().data()));
        tmpNode = tmpNode->findChild(writeAttr.toUtf8().data());
    }
    else
    {
        tmpNode = tmpNode->findChild(writeAttr.toUtf8().data());
    }

    if (!tmpNode)
    {
        return;
    }

    tmpNode->setAttr(EDITOR_EVENT_LOG_ATTRIB_NAME, val.toUtf8().data());

    root->saveToFile(EDITOR_EVENT_LOG_FILE_PATH);
}

XmlNodeRef CSettingsManager::LoadLogEventSetting(const QString& path, const QString& attr, [[maybe_unused]] QString& val, XmlNodeRef& root)
{
    // Make sure it ends with "\" before extraction of tokens
    QString fixedPath = path;
    QString slashChar = fixedPath.right(fixedPath.length() - 1);

    if (slashChar != "\\")
    {
        fixedPath += "\\";
    }

    QStringList strNodes;
    Path::GetDirectoryQueue(fixedPath, strNodes);
    QString readAttr = attr;

    // Spaces in node names not allowed
    readAttr.replace(" ", "");

    if (!root)
    {
        return nullptr;
    }

    XmlNodeRef tmpNode = nullptr;
    tmpNode = root;

    if (!tmpNode)
    {
        return nullptr;
    }

    for (int i = 0; i < strNodes.size(); ++i)
    {
        if (tmpNode->findChild(strNodes[i].toUtf8().data()))
        {
            tmpNode = tmpNode->findChild(strNodes[i].toUtf8().data());
        }
        else
        {
            return nullptr;
        }
    }

    if (tmpNode->haveAttr(readAttr.toUtf8().data()))
    {
        return tmpNode;
    }

    return nullptr;
}

QString CSettingsManager::GenerateContentHash(XmlNodeRef& node, QString sourceName)
{
    QString hashStr("");

    if (node->getChildCount() == 0)
    {
        return sourceName;
    }

    uint32 hash = AZ::Crc32(node->getXML(0));
    hashStr = QString::number(hash);

    return hashStr;
}
