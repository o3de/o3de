/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_SETTINGSMANAGER_H
#define CRYINCLUDE_EDITOR_SETTINGSMANAGER_H
#pragma once

#include <QString>

class QByteArray;

#define EDITOR_LAYOUT_FILE_PATH "@user@/Editor/EditorLayout.xml"
#define EDITOR_SETTINGS_FILE_PATH "@user@/Editor/EditorSettings.xml"
#define EDITOR_LAYOUT_ROOT_NODE "EditorLayout"
#define EDITOR_SETTINGS_ROOT_NODE "EditorSettings"
#define EDITOR_SETTINGS_CONTENT_NODE "EditorSettingsContent"
#define NEED_SETTINGS_VALID_LOOKUP_PATH "Settings"
#define EDITOR_LAYOUT_NODE "DockingPaneLayouts"
#define EDITOR_SETTINGS_ATTRIB_NAME "value"
#define CVARS_NODE "CVars"
#define CVAR_NODE "CVar"

#define EDITOR_EVENT_LOG_FILE_PATH "@user@/Editor/EditorEventLog.xml"
#define EDITOR_EVENT_LOG_ROOT_NODE "EventRecorder"
#define EVENT_LOG_EVENT_NAME "eventName"
#define EDITOR_EVENT_LOG_ATTRIB_NAME "value"
#define EVENT_LOG_START "start"
#define EVENT_LOG_END "end"

#define LOG_DATETIME_FORMAT "MM/dd/yyyy HH:mm:ss"

#define EVENT_LOG_CALLER_VERSION "callerVersion"

enum EditorSettingsManagerType
{
    eSettingsManagerMemoryStorage = 0,
    eSettingsManagerLast
};

enum EditorSettingsExportType
{
    eSettingsManagerExportSettings = 0,
    eSettingsManagerExportLayout
};

typedef std::map<QString, QString> TToolNamesMap;

struct SEventLog;

struct SEventLog
{
    SEventLog(QString eventName, QString eventState, QString callerVersion = "")
    {
        m_eventName = eventName;
        m_eventState = eventState;
        m_callerVersion = callerVersion;
    }

    QString m_eventName;
    QString m_callerVersion;
    QString m_eventState;
};

class CSettingsManager
{
public:
    // eMemoryStorage=0
    CSettingsManager(EditorSettingsManagerType managerType);
    ~CSettingsManager();

    // Sandbox Editor events
    void RegisterEvent(const SEventLog& event);
    void UnregisterEvent(const SEventLog& event);
    bool IsEventSafe(const SEventLog& event);
    QString GenerateContentHash(XmlNodeRef& node, QString sourceName);

    XmlNodeRef LoadLogEventSetting(const QString& path, const QString& attr, QString& val, XmlNodeRef& root);
    void SaveLogEventSetting(const QString& path, const QString& attr, const QString& val);

    bool CreateDefaultLayoutSettingsFile();

    void SaveLayoutSettings(const QByteArray& layout, const QString& toolName);

    AZStd::vector<AZStd::string> BuildSettingsList();
    void BuildSettingsList_Helper(const XmlNodeRef& nodeList, const AZStd::string_view& previousPath, AZStd::vector<AZStd::string>& result);

    void LoadSetting(const QString& path, const QString& attr, bool& val);
    void LoadSetting(const QString& path, const QString& attr, int& iVal);
    void LoadSetting(const QString& path, const QString& attr, float& fVal);
    void LoadSetting(const QString& path, const QString& attr, QColor& color);
    XmlNodeRef LoadSetting(const QString& path, const QString& attr, QString& val);

    void SaveSetting(const QString& path, const QString& attr, bool val);
    void SaveSetting(const QString& path, const QString& attr, float fVal);
    void SaveSetting(const QString& path, const QString& attr, int iVal);
    void SaveSetting(const QString& path, const QString& attr, QColor color);
    void SaveSetting(const QString& path, const QString& attr, const QString& val);

    void AddSettingsNode(XmlNodeRef newNode);

    void AddToolVersion(const QString& toolName, const QString& toolVersion);
    QString& GetToolVersion(const QString& sPaneClassName){ return m_toolVersions[sPaneClassName]; };

    void AddToolName(const QString& toolName, const QString& humanReadableName = QString());
    TToolNamesMap* GetToolNames() { return &m_toolNames; };

    // Test if all tools can be safely opened
    bool IsToolsOpenSafe();

    void ClearToolNames(){ m_toolNames.clear(); m_toolVersions.clear(); };

    void UpdateLayoutNode();

    void ImportSettings(QString file);

    void ExportSettings(XmlNodeRef exportNode, QString fileName);
    void Export();

    void GetMatchingLayoutNames(TToolNamesMap& foundTools, XmlNodeRef& resultNode, QString file);
    bool NeedSettingsNode(const QString& path);

    void SerializeCVars(XmlNodeRef& node, bool bLoad);

    void ReadValueStr(XmlNodeRef& sourceNode, const QString& path, const QString& attr, QString& val);

    void SetExportFileName(QString exportFilePath) { m_ExportFilePath = exportFilePath; };

private:
    // Save settings to memory or file
    EditorSettingsManagerType m_managerType;

    // Full path of exported file
    QString m_ExportFilePath;

    // Node created in memory to be used for exporting editor settings, console-set cvars and layout
    XmlNodeRef m_pSettingsManagerMemoryNode;

    // Registered Tool Names
    TToolNamesMap m_toolNames;
    TToolNamesMap m_toolVersions;
};

#endif // CRYINCLUDE_EDITOR_SETTINGSMANAGER_H
