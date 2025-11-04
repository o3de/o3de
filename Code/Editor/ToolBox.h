/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : ToolBox Macro System


#ifndef CRYINCLUDE_EDITOR_TOOLBOX_H
#define CRYINCLUDE_EDITOR_TOOLBOX_H
#pragma once

#define TOOLBOXMACROS_NODE "toolboxmacros"
#define INVALID_TOOLBAR_ID -1

class ActionManager;

class CToolBoxManager;

#include <QAction>

/** Represents a single ToolBox command.
*/
class CToolBoxCommand
{
public:
    enum EType
    {
        eT_INVALID_COMMAND = 0,
        eT_SCRIPT_COMMAND,
        eT_CONSOLE_COMMAND,
    };

    CToolBoxCommand()
        : m_bVariableToggle(false)
        , m_type(eT_INVALID_COMMAND)
    {
    }

    void Save(XmlNodeRef commandNode) const;
    void Load(XmlNodeRef commandNode);

    void Execute() const;

    QString m_text;

    bool m_bVariableToggle;
    EType m_type;
};

Q_DECLARE_METATYPE(CToolBoxCommand*)

/** Represents a sequence of ToolBox commands.
*/
class CToolBoxMacro
{
    friend class CToolBoxManager;
public:
    CToolBoxMacro(const QString& title)
        : m_action(title, 0)
        , m_toolbarId(INVALID_TOOLBAR_ID)
    {
        QObject::connect(&m_action, &QAction::triggered, &m_action, [this]() { Execute(); });
    }
    ~CToolBoxMacro()
    { Clear(); }

    void Save(XmlNodeRef macroNode) const;
    void Load(XmlNodeRef macroNode);

    void AddCommand(CToolBoxCommand::EType type, const QString& command, bool bVariableToggle = false);
    void Clear();
    QString GetTitle() const
    { return m_action.text(); }
    void SetTitle(const QString& title) { m_action.setText(title); }
    int GetCommandCount() const
    { return int(m_commands.size()); }
    const CToolBoxCommand* GetCommandAt(int index) const;
    CToolBoxCommand* GetCommandAt(int index);
    void SwapCommand(int index1, int index2);
    void RemoveCommand(int index);

    QAction* action() { return &m_action;  }

    void Execute() const;

    void SetShortcutName(const QKeySequence& name)
    {   m_action.setShortcut(name);   }
    QKeySequence GetShortcutName()  const
    {   return m_action.shortcut();         }

    void SetIconPath(const char* path)
    {
        m_iconPath = path;
        m_action.setIcon(QPixmap(path));
    }
    const QString& GetIconPath() const
    { return m_iconPath; }

    void SetToolbarId(int nID)
    { m_toolbarId = nID; }
    int GetToolbarId()
    { return m_toolbarId; }

private:
    std::vector<CToolBoxCommand*> m_commands;
    QString m_iconPath;
    QAction m_action;
    int m_toolbarId;
};

Q_DECLARE_METATYPE(CToolBoxMacro*)

/** Manages user defined macros.
*/
class CToolBoxManager
{
public:
    ~CToolBoxManager()
    { Clear(); }

    // Save macros configuration to registry.
    void Save() const;
    // Load macros configuration from registry.
    void Load();

    //! Get the number of managed macros.
    int GetMacroCount(bool bToolbox) const;
    //! Get a macro by index.
    const CToolBoxMacro* GetMacro(int iIndex, bool bToolbox) const;
    CToolBoxMacro* GetMacro(int iIndex, bool bToolbox);
    //! Get the index of a macro from its title.
    int GetMacroIndex(const QString& title, bool bToolbox) const;
    //! Creates a new macro in the manager. If the title is duplicate, this returns nullptr.
    CToolBoxMacro* NewMacro(const QString& title, bool bToolbox, int* newIdx);
    //! Try to change the title of a macro. If the title is duplicate, the change is aborted and this returns false.
    bool SetMacroTitle(int index, const QString& title, bool bToolbox);
    //! Delete all macros.
    void Clear();

    void SwapMacro(int index1, int index2, bool bToolbox);
    void RemoveMacro(int index, bool bToolbox);

    //! Execute the macro with specified id.
    void ExecuteMacro(int iIndex, bool bToolbox) const;
    void ExecuteMacro(const QString& name, bool bToolbox) const;

    void GetSaveFilePath(QString& path) const;

private:
    void Load(QString xmlpath, bool bToolbox);

    std::vector<CToolBoxMacro*> m_macros;
    std::vector<CToolBoxMacro*> m_shelveMacros;
};

#endif // CRYINCLUDE_EDITOR_TOOLBOX_H
