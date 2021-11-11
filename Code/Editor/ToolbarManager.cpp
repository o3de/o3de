/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include "ToolbarManager.h"

// AzCore
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Interface/Interface.h>

// Qt
#include <QDebug>
#include <QDrag>
#include <QMimeData>
#include <QPainter>
#include <QLayout>

#include <QTextDocumentFragment>

// Editor
#include "MainWindow.h"
#include "ToolBox.h"
#include "PluginManager.h"
#include "ActionManager.h"
#include "Settings.h"


static const char* SUBSTANCE_TOOLBAR_NAME = "Substance";
static const QString TOOLBAR_SETTINGS_KEY("ToolbarSettings");


// Save out the version of the toolbars with it
// Only save a toolbar if it's not a standard or has some changes to it from the standard
// On load, add any actions that are with a newer version to it
// Check if a toolbar is the same as a default version on load

const int TOOLBAR_IDENTIFIER = 0xFFFF; // must be an int, for compatibility

enum AmazonToolbarVersions
{
    ORIGINAL_TOOLBAR_VERSION = 1,
    TOOLBARS_WITH_PLAY_GAME = 2,
    TOOLBARS_WITH_PERSISTENT_VISIBILITY = 3,
    TOOLBARS_WITHOUT_CVAR_MODES = 5,

    //TOOLBAR_VERSION = 1
    TOOLBAR_VERSION = TOOLBARS_WITHOUT_CVAR_MODES
};

struct InternalAmazonToolbarList
{
    int version;
    AmazonToolbar::List toolbars;
};

Q_DECLARE_METATYPE(AmazonToolbar)
Q_DECLARE_METATYPE(AmazonToolbar::List)
Q_DECLARE_METATYPE(InternalAmazonToolbarList)

static bool ObjectIsSeparator(QObject* o)
{
    return o && o->metaObject()->className() == QStringLiteral("QToolBarSeparator");
}

static QDataStream& WriteToolbarDataStream(QDataStream& out, const AmazonToolbar& toolbar)
{
    out << toolbar.GetName();
    out << toolbar.GetTranslatedName();
    out << toolbar.ActionIds();
    out << toolbar.IsShowByDefault();
    out << toolbar.IsShowToggled();

    return out;
}

static QDataStream& ReadToolbarDataStream(QDataStream& in, AmazonToolbar& toolbar, int version)
{
    QString name;
    QString translatedName;
    QVector<int> actionIds;

    bool showByDefault = true;
    bool showToggled = false;

    in >> name;
    in >> translatedName;

    in >> actionIds;

    if (version > 0)
    {
        in >> showByDefault;
        toolbar.SetShowByDefault(showByDefault);
    }

    if (version >= TOOLBARS_WITH_PERSISTENT_VISIBILITY)
    {
        in >> showToggled;
        toolbar.SetShowToggled(showToggled);
    }

    for (int actionId : actionIds)
    {
        toolbar.AddAction(actionId);
    }

    toolbar.SetName(name, translatedName);

    return in;
}

static QDataStream& operator<<(QDataStream& out, const InternalAmazonToolbarList& list)
{
    out << TOOLBAR_IDENTIFIER;
    out << list.version;
    out << list.toolbars.size();
    for (const AmazonToolbar& t : list.toolbars)
    {
        WriteToolbarDataStream(out, t);
    }

    return out;
}

static QDataStream& operator>>(QDataStream& in, InternalAmazonToolbarList& list)
{
    int identifier;

    in >> identifier;

    int size = 0;
    if (identifier == TOOLBAR_IDENTIFIER)
    {
        in >> list.version;
        in >> size;
    }
    else
    {
        // version 0; size is identifier
        list.version = 0;
        size = identifier;
    }

    size = std::min(size, 30); // protect against corrupt data
    list.toolbars.reserve(size);
    for (int i = 0; i < size; ++i)
    {
        AmazonToolbar t;
        ReadToolbarDataStream(in, t, list.version);
        list.toolbars.push_back(t);
    }

    return in;
}

class AmazonToolBarExpanderWatcher
    : public QObject
{
public:
    AmazonToolBarExpanderWatcher(QObject* parent = nullptr)
        : QObject(parent)
    {
    }

    bool eventFilter(QObject* obj, QEvent* event) override
    {
        switch (event->type())
        {
            case QEvent::MouseButtonPress:
            case QEvent::MouseButtonRelease:
            case QEvent::MouseButtonDblClick:
            {
                if (qobject_cast<QToolButton*>(obj))
                {
                    auto mouseEvent = static_cast<QMouseEvent*>(event);
                    auto expansion = qobject_cast<QToolButton*>(obj);

                    expansion->setPopupMode(QToolButton::InstantPopup);
                    auto menu = new QMenu(expansion);

                    auto toolbar = qobject_cast<QToolBar*>(expansion->parentWidget());

                    for (auto toolbarAction : toolbar->actions())
                    {
                        auto actionWidget = toolbar->widgetForAction(toolbarAction);
                        if (actionWidget && !actionWidget->isVisible())
                        {
                            QString plainText = QTextDocumentFragment::fromHtml(actionWidget->toolTip()).toPlainText();
                            toolbarAction->setText(plainText);
                            menu->addAction(toolbarAction);
                        }
                    }

                    menu->exec(mouseEvent->globalPos());
                    return true;
                }

                break;
            }
        }

        return QObject::eventFilter(obj, event);
    }
};

ToolbarManager::ToolbarManager(ActionManager* actionManager, MainWindow* mainWindow)
    : m_mainWindow(mainWindow)
    , m_actionManager(actionManager)
    , m_settings("O3DE", "O3DE")
    , m_expanderWatcher(new AmazonToolBarExpanderWatcher())
{
    // Note that we don't actually save/load from AmazonToolbar::List
    // The data saved for existing users had that name, and it can't be changed now without ignoring user's data.
    // We need to know the version stored, so we need to save/load into a different structure (InternalAmazonToolbarList)
    qRegisterMetaType<InternalAmazonToolbarList>("AmazonToolbar::List");
    qRegisterMetaTypeStreamOperators<InternalAmazonToolbarList>("AmazonToolbar::List");
}

ToolbarManager::~ToolbarManager()
{
    SaveToolbars();

    if (m_expanderWatcher)
    {
        delete m_expanderWatcher;
    }
}

EditableQToolBar* ToolbarManager::ToolbarParent(QObject* o) const
{
    if (!o)
    {
        return nullptr;
    }

    if (auto t = qobject_cast<EditableQToolBar*>(o))
    {
        return t;
    }

    return ToolbarParent(o->parent());
}

void ToolbarManager::LoadToolbars()
{
    InitializeStandardToolbars();

    m_settings.beginGroup(TOOLBAR_SETTINGS_KEY);
    InternalAmazonToolbarList loadedToolbarList = m_settings.value("toolbars").value<InternalAmazonToolbarList>();
    m_toolbars = loadedToolbarList.toolbars;
    m_loadedVersion = loadedToolbarList.version;
    qDebug() << "Loaded toolbars:" << m_toolbars.size();

    // Load the defaults which were saved by the previous version
    // If no defaults are found, the effect is to re-add all commands in the current standard toolbars which are no
    // longer present in the edited version (i.e. result = the set union of edited and current default)
    QMap<QString, AmazonToolbar> previousStandardToolbars;
    if (m_loadedVersion < TOOLBAR_VERSION)
    {
        m_settings.beginGroup("Defaults");
        QVariant defaultsValue = m_settings.value(QString::number(m_loadedVersion));
        if (defaultsValue.isValid())
        {
            InternalAmazonToolbarList oldDefaults = defaultsValue.value<InternalAmazonToolbarList>();

            for (AmazonToolbar& oldDefault : oldDefaults.toolbars)
            {
                previousStandardToolbars[oldDefault.GetName()] = oldDefault;
            }
        }
        m_settings.endGroup();
    }

    m_settings.endGroup();

    SanitizeToolbars(previousStandardToolbars);
    InstantiateToolbars();
}

static bool operator==(const AmazonToolbar& t1, const AmazonToolbar& t2)
{
    return t1.GetName() == t2.GetName();
}

void ToolbarManager::SanitizeToolbars(const QMap<QString, AmazonToolbar>& oldStandard)
{
    // All standard toolbars must be present
    auto stdToolbars = m_standardToolbars;

    if (m_loadedVersion < TOOLBARS_WITHOUT_CVAR_MODES)
    {
        // Check if any standard toolbars have been deprecated and no longer exist
        for (const auto& oldStandardToolbar : oldStandard)
        {
            if (!stdToolbars.contains(oldStandardToolbar.GetName()))
            {
                // Add an empty standard toolbar as placeholder for the deprecated standard
                // toolbar so that it is kept around with only the user added actions or
                // removed from the toolbar list if it does not contain any custom actions
                stdToolbars.append(AmazonToolbar(oldStandardToolbar.GetName(), oldStandardToolbar.GetTranslatedName()));
            }
        }
    }

    // make a set of the loaded toolbars
    QMap<QString, AmazonToolbar> toolbarSet;
    for (const AmazonToolbar& toolbar : m_toolbars)
    {
        toolbarSet[toolbar.GetName()] = toolbar;
    }

    // the order is important because IsCustomToolbar() checks based on the order (which it shouldn't...)
    // so we go through the loaded toolbars and make sure that our standard ones are all in there
    // in the right order. We also remove them from the set, so that we know what's left later
    AmazonToolbar::List newToolbars;
    for (AmazonToolbar& stdToolbar : stdToolbars)
    {
        auto customToolbarReference = toolbarSet.find(stdToolbar.GetName());
        if (customToolbarReference == toolbarSet.end())
        {
            // An untouched standard toolbar or a user-created one
            newToolbars.push_back(stdToolbar);
        }
        else if (customToolbarReference.value().IsSame(stdToolbar))
        {
            // Edge case of previous versions where all toolbars were saved regardless of dirtiness
            // If we're replacing the Toolbar and haven't changed whether or not it's hidden by default since
            // last load, ensure we respect whether or not the user had previously toggled it
            if (stdToolbar.IsShowByDefault() == customToolbarReference.value().IsShowByDefault())
            {
                stdToolbar.SetShowToggled(customToolbarReference.value().IsShowToggled());
            }
            newToolbars.push_back(stdToolbar);
        }
        else
        {
            AmazonToolbar& newToolbar = customToolbarReference.value();

            // make sure to add any actions added since the last time the user saved this toolbar
            if (oldStandard.contains(stdToolbar.GetName()))
            {
                QVector<int> newCommands = stdToolbar.ActionIds();
                QVector<int> previous = oldStandard[stdToolbar.GetName()].ActionIds();
                QVector<int> custom = newToolbar.ActionIds();

                // If the new layout removed some, we want to remove those if present in the edited layout
                for (int previousCommand : previous)
                {
                    if (!newCommands.contains(previousCommand))
                    {
                        custom.removeOne(previousCommand);
                    }
                }

                // We only want commands that weren't in the old default version and which aren't already in the
                // customized toolbar.
                // We just append them here, but it might be possible to attempt to preserve the ordering...
                for (int command : newCommands)
                {
                    if (!previous.contains(command) && !custom.contains(command))
                    {
                        custom.push_back(command);
                    }
                }

                newToolbar.Clear();
                for (int actionId : custom)
                {
                    newToolbar.AddAction(actionId);
                }
            }

            newToolbars.push_back(newToolbar);

            toolbarSet.remove(stdToolbar.GetName());
        }
    }

    // go through and add in all of the left over toolbars, in the same order now
    for (const AmazonToolbar& existingToolbar : m_toolbars)
    {
        if (!newToolbars.contains(existingToolbar.GetName()))
        {
            newToolbars.push_back(existingToolbar);
        }
    }

    // it isn't an older version of the std toolbar, but it needs to have all of the actions
    // that the newest one has, so add anything newer than what it was saved with
    // WORKS FOR THIS, BUT WHAT ABOUT FOR PLUGIN CREATOR TOOLBARS? HOW DO THEY ADD NEW BUTTONS?

    // keep the new list now
    m_toolbars = newToolbars;

    // TODO: Remove this once gems are able to control toolbars
    bool removeSubstanceToolbar = true;

    if (AZ::SettingsRegistryInterface* settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
    {
        // Query the /Amazon/Gems/Substance key to determine if the substance gem is available
        AZ::SettingsRegistryInterface::Type keyType = settingsRegistry->GetType(AZ::SettingsRegistryInterface::FixedValueString::format("%s/Gems/%s",
            AZ::SettingsRegistryMergeUtils::OrganizationRootKey, SUBSTANCE_TOOLBAR_NAME));
        removeSubstanceToolbar = keyType == AZ::SettingsRegistryInterface::Type::NoType;
    }

    // Remove toolbars with invalid names (corrupted)
    m_toolbars.erase(std::remove_if(m_toolbars.begin(), m_toolbars.end(), [removeSubstanceToolbar](const AmazonToolbar& t)
    {
        return t.GetName().isEmpty() || (removeSubstanceToolbar && t.GetName() == SUBSTANCE_TOOLBAR_NAME);
    }), m_toolbars.end());

    if (m_loadedVersion < TOOLBARS_WITHOUT_CVAR_MODES)
    {
        // Remove any empty deprecated toolbars that haven't been customized, or
        // rename and keep the toolbar if a custom action has been added
        m_toolbars.erase(std::remove_if(m_toolbars.begin(), m_toolbars.end(), [](AmazonToolbar& toolbar)
            {
                const QString& toolbarName = toolbar.GetName();
                if (toolbarName == "debugViewsToolbar"
                    || toolbarName == "environmentModesToolbar"
                    || toolbarName == "viewModesToolbar")
                {
                    if (toolbar.ActionIds().isEmpty())
                    {
                        return true;
                    }
                    else
                    {
                        QString newToolbarName = QString("%1 (Deprecated)").arg(toolbar.GetTranslatedName());
                        toolbar.SetName(newToolbarName, newToolbarName);                        
                    }
                }
                return false;
            }), m_toolbars.end());
    }
}

void ToolbarManager::SaveToolbar(EditableQToolBar* toolbar)
{
    for (AmazonToolbar& at : m_toolbars)
    {
        if (at.Toolbar() == toolbar)
        {
            at.Clear();
            for (QAction* action : toolbar->actions())
            {
                const int actionId = action->data().toInt();
                if (actionId >= 0)
                {
                    at.AddAction(actionId);
                }
                else
                {
                    qWarning() << Q_FUNC_INFO << "Invalid action id";
                }
            }

            AmazonToolbar::UpdateAllowedAreas(toolbar);
            SaveToolbars();
            return;
        }
    }

    qWarning() << Q_FUNC_INFO << "Couldn't find toolbar";
}

void ToolbarManager::SaveToolbars()
{
    // Determine if the user has manually shown or hidden any toolbars and flag that so we remember on startup
    for (auto& toolbar : m_toolbars)
    {
        QToolBar* toolbarWidget = toolbar.Toolbar();
        // If we're not explicitly hidden and we're not shown by default, or the converse, record that the user toggled our visibility
        if (toolbarWidget && (!toolbarWidget->isHidden()) != toolbar.IsShowByDefault())
        {
            toolbar.SetShowToggled(true);
        }
        else
        {
            toolbar.SetShowToggled(false);
        }
    }

    m_settings.beginGroup(TOOLBAR_SETTINGS_KEY);

    QVector<AmazonToolbar> toBeSaved = m_toolbars;
    // We only save toolbars that differ from their default or are user-created
    toBeSaved.erase(std::remove_if(toBeSaved.begin(), toBeSaved.end(), [this](const AmazonToolbar& t) { return !IsDirty(t); }), toBeSaved.end());
    InternalAmazonToolbarList savedToolbars = { TOOLBAR_VERSION, toBeSaved };
    m_settings.setValue("toolbars", QVariant::fromValue<InternalAmazonToolbarList>(savedToolbars));
    m_settings.endGroup();
}

void ToolbarManager::InitializeStandardToolbars()
{
    if (m_standardToolbars.isEmpty())
    {
        auto macroToolbars = GetIEditor()->GetToolBoxManager()->GetToolbars();

        m_standardToolbars.reserve(static_cast<int>(5 + macroToolbars.size()));
        m_standardToolbars.push_back(GetEditModeToolbar());
        m_standardToolbars.push_back(GetObjectToolbar());
        m_standardToolbars.push_back(GetPlayConsoleToolbar());

        IPlugin* pGamePlugin = GetIEditor()->GetPluginManager()->GetPluginByGUID("{71CED8AB-54E2-4739-AA78-7590A5DC5AEB}");
        IPlugin* pDescriptionEditorPlugin = GetIEditor()->GetPluginManager()->GetPluginByGUID("{4B9B7074-2D58-4AFD-BBE1-BE469D48456A}");
        if (pGamePlugin && pDescriptionEditorPlugin)
        {
            m_standardToolbars.push_back(GetMiscToolbar());
        }

        std::copy(std::begin(macroToolbars), std::end(macroToolbars), std::back_inserter(m_standardToolbars));

        // Save that default state to future versions can reason about updating modified standard toolbars
        m_settings.beginGroup(TOOLBAR_SETTINGS_KEY);
        m_settings.beginGroup("Defaults");

        InternalAmazonToolbarList savedToolbars = { TOOLBAR_VERSION, m_standardToolbars };
        m_settings.setValue(QString::number(TOOLBAR_VERSION), QVariant::fromValue<InternalAmazonToolbarList>(savedToolbars));

        m_settings.endGroup();
        m_settings.endGroup();
    }
}

void AmazonToolbar::UpdateAllowedAreas()
{
    UpdateAllowedAreas(m_toolbar);
}

void AmazonToolbar::UpdateAllowedAreas(QToolBar* toolbar)
{
    bool horizontalOnly = false;

    for (QAction* action : toolbar->actions())
    {
        if (qobject_cast<QWidgetAction*>(action))
        {
            // if it's a widget action, we assume it won't fit in the vertical toolbars
            horizontalOnly = true;
            break;
        }
    }

    if (horizontalOnly)
    {
        toolbar->setAllowedAreas(Qt::BottomToolBarArea | Qt::TopToolBarArea);
    }
    else
    {
        toolbar->setAllowedAreas(Qt::AllToolBarAreas);
    }
}

bool ToolbarManager::IsDirty(const AmazonToolbar& toolbar) const
{
    const AmazonToolbar* defaultStandardToolbar = FindDefaultToolbar(toolbar.GetName());

    // custom toolbars are always considered dirty
    bool isDirty = true;

    if (defaultStandardToolbar != nullptr)
    {
        isDirty = !defaultStandardToolbar->IsSame(toolbar);
    }

    return isDirty;
}


AmazonToolbar ToolbarManager::GetEditModeToolbar() const
{
    AmazonToolbar t = AmazonToolbar("EditMode", QObject::tr("Edit Mode Toolbar"));
    t.SetMainToolbar(true);
    return t;
}

AmazonToolbar ToolbarManager::GetObjectToolbar() const
{
    AmazonToolbar t = AmazonToolbar("Object", QObject::tr("Object Toolbar"));
    t.SetMainToolbar(true);
    t.AddAction(ID_GOTO_SELECTED, ORIGINAL_TOOLBAR_VERSION);

    return t;
}

QMenu* ToolbarManager::CreatePlayButtonMenu() const
{
    QMenu* playButtonMenu = new QMenu("Play Game");

    playButtonMenu->addAction(m_actionManager->GetAction(ID_VIEW_SWITCHTOGAME_VIEWPORT));
    playButtonMenu->addAction(m_actionManager->GetAction(ID_VIEW_SWITCHTOGAME_FULLSCREEN));

    return playButtonMenu;
}

AmazonToolbar ToolbarManager::GetPlayConsoleToolbar() const
{
    AmazonToolbar t = AmazonToolbar("PlayConsole", QObject::tr("Play Controls"));
    t.SetMainToolbar(true);

    t.AddAction(ID_TOOLBAR_WIDGET_SPACER_RIGHT, ORIGINAL_TOOLBAR_VERSION); 
    t.AddAction(ID_TOOLBAR_SEPARATOR, ORIGINAL_TOOLBAR_VERSION);
    t.AddAction(ID_TOOLBAR_WIDGET_PLAYCONSOLE_LABEL, ORIGINAL_TOOLBAR_VERSION);

    QAction* playAction = m_actionManager->GetAction(ID_VIEW_SWITCHTOGAME);
    QToolButton* playButton = new QToolButton(t.Toolbar());

    QMenu* menu = CreatePlayButtonMenu();
    menu->setParent(t.Toolbar());
    playAction->setMenu(menu);

    playButton->setDefaultAction(playAction);
    t.AddWidget(playButton, ID_VIEW_SWITCHTOGAME, ORIGINAL_TOOLBAR_VERSION);

    t.AddAction(ID_TOOLBAR_SEPARATOR, ORIGINAL_TOOLBAR_VERSION);
    t.AddAction(ID_SWITCH_PHYSICS, TOOLBARS_WITH_PLAY_GAME);    
    return t;
}

AmazonToolbar ToolbarManager::GetEditorsToolbar() const
{
    AmazonToolbar t = AmazonToolbar("Editors", QObject::tr("Editors Toolbar"));

    t.AddAction(ID_OPEN_AUDIO_CONTROLS_BROWSER, ORIGINAL_TOOLBAR_VERSION);

    return t;
}

AmazonToolbar ToolbarManager::GetMiscToolbar() const
{
    AmazonToolbar t = AmazonToolbar("Misc", QObject::tr("Misc Toolbar"));
    return t;
}

void ToolbarManager::AddButtonToEditToolbar(QAction* action)
{
    QString toolbarName = "EditMode";
    const AmazonToolbar* toolbar = FindToolbar(toolbarName);

    if (toolbar)
    {
        if (toolbar->Toolbar())
        {
            toolbar->Toolbar()->addAction(action);
        }
    }
}

const AmazonToolbar* ToolbarManager::FindDefaultToolbar(const QString& toolbarName) const
{
    for (const AmazonToolbar& toolbar : m_standardToolbars)
    {
        if (toolbar.GetName() == toolbarName)
        {
            return &toolbar;
        }
    }

    return nullptr;
}

AmazonToolbar* ToolbarManager::FindToolbar(const QString& toolbarName)
{
    for (AmazonToolbar& toolbar : m_toolbars)
    {
        if (toolbar.GetName() == toolbarName)
        {
            return &toolbar;
        }
    }

    return nullptr;
}

void ToolbarManager::RestoreToolbarDefaults(const QString& toolbarName)
{
    if (!IsCustomToolbar(toolbarName))
    {
        const AmazonToolbar* defaultToolbar = FindDefaultToolbar(toolbarName);
        AmazonToolbar* existingToolbar = FindToolbar(toolbarName);
        Q_ASSERT(existingToolbar != nullptr);

        if (existingToolbar != nullptr)
        {
            const bool isInstantiated = existingToolbar->IsInstantiated();

            if (isInstantiated)
            {
                // We have a QToolBar instance, updated it too
                for (QAction* action : existingToolbar->Toolbar()->actions())
                {
                    existingToolbar->Toolbar()->removeAction(action);
                }
            }

            Q_ASSERT(defaultToolbar != nullptr);
            if (defaultToolbar != nullptr)
            {
                existingToolbar->CopyActions(*defaultToolbar);
            }

            if (isInstantiated)
            {
                existingToolbar->SetActionsOnInternalToolbar(m_actionManager);
                existingToolbar->UpdateAllowedAreas();
            }
            SaveToolbars();
        }
    }
    else
    {
        qWarning() << Q_FUNC_INFO << "Can only reset standard toolbars";
    }
}

void ToolbarManager::SetEnabled(bool enabled)
{
    for (AmazonToolbar& amazonToolbar : m_toolbars)
    {
        QToolBar* toolbar = amazonToolbar.Toolbar();
        if (toolbar)
        {
            toolbar->setEnabled(enabled);
        }
    }
}

void AmazonToolbar::SetActionsOnInternalToolbar(ActionManager* actionManager)
{
    for (auto actionData : m_actions)
    {
        int actionId = actionData.actionId;

        if (actionId == ID_TOOLBAR_SEPARATOR)
        {
            QAction* action = m_toolbar->addSeparator();
            action->setData(ID_TOOLBAR_SEPARATOR);
        }
        else
        {
            if (actionManager->HasAction(actionId))
            {
                if (actionData.widget != nullptr)
                {
                    m_toolbar->addWidget(actionData.widget);
                }
                else
                {
                    m_toolbar->addAction(actionManager->GetAction(actionId));
                }
            }
        }
    }
}

void ToolbarManager::InstantiateToolbars()
{
    const int numToolbars = m_toolbars.size();
    for (int i = 0; i < numToolbars; ++i)
    {
        InstantiateToolbar(i);
        if (i == 2)
        {
            // Hack. Just copying how it was
            m_mainWindow->addToolBarBreak();
        }
    }
}

void ToolbarManager::InstantiateToolbar(int index)
{
    AmazonToolbar& t = m_toolbars[index];
    t.InstantiateToolbar(m_mainWindow, this); // Create the QToolbar
}

AmazonToolbar::List ToolbarManager::GetToolbars() const
{
    return m_toolbars;
}

AmazonToolbar ToolbarManager::GetToolbar(int index)
{
    if (index < 0 || index >= m_toolbars.size())
    {
        return AmazonToolbar();
    }

    return m_toolbars.at(index);
}

bool ToolbarManager::Delete(int index)
{
    if (!IsCustomToolbar(index))
    {
        qWarning() << Q_FUNC_INFO << "Won't try to delete invalid or standard toolbar" << index << m_toolbars.size();
        return false;
    }

    AmazonToolbar t = m_toolbars.takeAt(index);
    delete t.Toolbar();

    SaveToolbars();

    return true;
}


bool ToolbarManager::Rename(int index, const QString& newName)
{
    if (newName.isEmpty())
    {
        return false;
    }

    if (!IsCustomToolbar(index))
    {
        qWarning() << Q_FUNC_INFO << "Won't try to rename invalid or standard toolbar" << index << m_toolbars.size();
        return false;
    }

    AmazonToolbar& t = m_toolbars[index];
    if (t.GetName() == newName)
    {
        qWarning() << Q_FUNC_INFO << "Won't try to rename to the same name" << newName;
        return false;
    }
    t.SetName(newName, newName); // No translation for custom bars
    SaveToolbars();

    return true;
}

int ToolbarManager::Add(const QString& name)
{
    if (name.isEmpty())
    {
        return -1;
    }

    AmazonToolbar t(name, name);
    t.InstantiateToolbar(m_mainWindow, this);

    MainWindow::instance()->AdjustToolBarIconSize(static_cast<AzQtComponents::ToolBar::ToolBarIconSize>(gSettings.gui.nToolbarIconSize));

    m_toolbars.push_back(t);
    SaveToolbars();
    return m_toolbars.size() - 1;
}

bool ToolbarManager::IsCustomToolbar(int index) const
{
    return IsCustomToolbar(m_toolbars[index].GetName());
}

bool ToolbarManager::IsCustomToolbar(const QString& toolbarName) const
{
    for (auto toolbar : m_standardToolbars)
    {
        if (toolbar.GetName() == toolbarName)
        {
            return false;
        }
    }

    return true;
}

ActionManager* ToolbarManager::GetActionManager() const
{
    return m_actionManager;
}

AmazonToolBarExpanderWatcher* ToolbarManager::GetExpanderWatcher() const
{
    return m_expanderWatcher;
}

bool ToolbarManager::DeleteAction(QAction* action, EditableQToolBar* toolbar)
{
    if (!action)
    {
        // Doesn't happen
        qWarning() << Q_FUNC_INFO << "Null action!";
        return false;
    }

    const int actionId = action->data().toInt();
    if (actionId <= 0)
    {
        qWarning() << Q_FUNC_INFO << "Action has null id";
        return false;
    }

    if (toolbar->actions().contains(action))
    {
        toolbar->removeAction(action);
    }
    else
    {
        qWarning() << Q_FUNC_INFO << "Couldnt find action to remove";
        return false;
    }

    SaveToolbar(toolbar);
    return true;
}

void ToolbarManager::SetIsEditingToolBars(bool is)
{
    m_isEditingToolBars = is;
}

bool ToolbarManager::IsEditingToolBars() const
{
    return m_isEditingToolBars;
}

void ToolbarManager::InsertAction(QAction* action, QWidget* beforeWidget, QAction* beforeAction, EditableQToolBar* toolbar)
{
    if (!action)
    {
        qWarning() << Q_FUNC_INFO << "Invalid action for id" << action;
        return;
    }

    const int actionId = action->data().toInt();
    if (actionId <= 0)
    {
        qWarning() << Q_FUNC_INFO << "Invalid action id";
        return;
    }

    const int beforeActionId = beforeAction ? beforeAction->data().toInt() : -1;
    const bool beforeIsSeparator = beforeActionId == ID_TOOLBAR_SEPARATOR;

    if (beforeIsSeparator)
    {
        beforeAction = beforeWidget->actions().first();
    }

    if (beforeAction && !toolbar->actions().contains(beforeAction))
    {
        qWarning() << Q_FUNC_INFO << "Invalid before action" << beforeAction << beforeActionId << beforeWidget->actions();
        return;
    }

    toolbar->insertAction(beforeAction, action);

    SaveToolbar(toolbar);
}

class DnDIndicator
    : public QWidget
{
public:
    DnDIndicator(EditableQToolBar* parent)
        : QWidget(parent)
        , m_toolbar(parent)
    {
        setVisible(false);
    }

    void paintEvent([[maybe_unused]] QPaintEvent* ev) override
    {
        QPainter painter(this);
        painter.fillRect(QRect(0, 0, width(), height()), QBrush(QColor(217, 130, 46)));
    }

    void setLastDragPos(QPoint lastDragPos)
    {
        if (lastDragPos != m_lastDragPos)
        {
            m_lastDragPos = lastDragPos;
            if (lastDragPos.isNull())
            {
                m_dragSourceWidget = nullptr;
                setVisible(false);
            }
            else
            {
                setVisible(true);
                updatePosition();
            }
            update();
        }
    }

    void setDragSourceWidget(QWidget* w)
    {
        m_dragSourceWidget = w;
    }

    void updatePosition()
    {
        QWidget* beforeWidget = m_toolbar->insertPositionForDrop(m_lastDragPos);
        const auto widgets = m_toolbar->childWidgetsWithActions();
        QWidget* lastWidget = widgets.isEmpty() ? nullptr : widgets.last();

        if (beforeWidget && beforeWidget == m_dragSourceWidget)
        {
            // Nothing to do, user is dragging to the same place, don't indicate it as a possibility
            setVisible(false);
            return;
        }

        if (!beforeWidget && m_dragSourceWidget == lastWidget)
        {
            // Nothing to do. Don't show indicator. The widget is already at the end.
            setVisible(false);
            return;
        }

        int x = 0;
        if (beforeWidget)
        {
            x = beforeWidget->pos().x();
        }
        else
        {
            if (lastWidget)
            {
                x = lastWidget->pos().x() + lastWidget->width();
            }
            else
            {
                x = style()->pixelMetric(QStyle::PM_ToolBarHandleExtent) + style()->pixelMetric(QStyle::PM_ToolBarItemSpacing);
            }
        }

        const int w = 2;
        const int y = 5;
        const int h = m_toolbar->height() - (y * 2);
        setGeometry(x, y, w, h);
        raise();
    }

    QPoint lastDragPos() const
    {
        return m_lastDragPos;
    }

private:
    QPoint m_lastDragPos;
    QPointer<QWidget> m_dragSourceWidget = nullptr;
    EditableQToolBar* const m_toolbar;
};

EditableQToolBar::EditableQToolBar(const QString& title, ToolbarManager* manager)
    : QToolBar(title)
    , m_toolbarManager(manager)
    , m_actionManager(manager->GetActionManager())
    , m_dndIndicator(new DnDIndicator(this))
{
    setAcceptDrops(true);

    connect(this, &QToolBar::orientationChanged, this, [this](Qt::Orientation orientation)
    {
        for (const auto& widget : findChildren<QWidget*>())
            layout()->setAlignment(widget, orientation == Qt::Horizontal ? Qt::AlignVCenter : Qt::AlignHCenter);
    });
}

QWidget* EditableQToolBar::insertPositionForDrop(QPoint mousePos)
{
    // QToolBar::actionAt() is no good here, since it sometimes returns nullptr between widgets

    const QList<QWidget*> widgets = childWidgetsWithActions();
    // Find the closest button
    QWidget* beforeWidget = nullptr;
    for (auto w : widgets)
    {
        if (w && w->pos().x() + w->width() / 2 > mousePos.x())
        {
            beforeWidget = w;
            break;
        }
    }

    return beforeWidget;
}

void EditableQToolBar::childEvent(QChildEvent* ev)
{
    QObject* child = ev->child();
    if (ev->type() == QEvent::ChildAdded && child->isWidgetType()) // we can't qobject_cast to QToolButton yet, since it's not fully constructed
    {
        child->installEventFilter(this);
    }

    QToolBar::childEvent(ev);
}

QList<QWidget*> EditableQToolBar::childWidgetsWithActions() const
{
    QList<QWidget*> widgets;
    widgets.reserve(actions().size());
    for (QAction* action : actions())
    {
        if (QWidget* w = widgetForAction(action))
        {
            if (w->actions().isEmpty() && ObjectIsSeparator(w))
            {
                // Hack around the fact that QToolBarSeparator doesn't have an action associated
                w->addAction(action);
                action->setData(ID_TOOLBAR_SEPARATOR);
            }
            widgets.push_back(w);
        }
    }

    return widgets;
}

bool EditableQToolBar::eventFilter(QObject* obj, QEvent* ev)
{
    auto type = ev->type();
    const bool isMouseEvent = type == QEvent::MouseButtonPress || type == QEvent::MouseButtonRelease ||
        type == QEvent::MouseButtonDblClick || type == QEvent::MouseMove;

    auto* sourceWidget = qobject_cast<QWidget*>(obj);
    if (!m_toolbarManager->IsEditingToolBars() || !isMouseEvent || !sourceWidget)
    {
        return QToolBar::eventFilter(obj, ev);
    }

    QAction* sourceAction = ActionForWidget(sourceWidget);
    if (!sourceAction)
    {
        qWarning() << Q_FUNC_INFO << "Source widget" << sourceWidget << "doesn't have actions";
        return QToolBar::eventFilter(obj, ev);
    }

    if (ev->type() == QEvent::MouseButtonPress)
    {
        QAction* action = ActionForWidget(sourceWidget);
        const int actionId = action ? action->data().toInt() : 0;
        if (actionId <= 0)
        {
            // Doesn't happen
            qWarning() << Q_FUNC_INFO << "Invalid action id for widget" << sourceWidget << action << actionId;
            return false;
        }

        QDrag* drag = new QDrag(sourceWidget);

        { // Nested scope so painter gets deleted before we enter nested event-loop of QDrag::exec().
          // Otherwise QPainter dereferences invalid pointer because QWidget was deleted already
            QPixmap iconPixmap(sourceWidget->size());
            QPainter painter(&iconPixmap);
            sourceWidget->render(&painter);
            drag->setPixmap(iconPixmap);
        }

        QMimeData* mimeData = new QMimeData();
        mimeData->setText(action->text());
        drag->setMimeData(mimeData);

        drag->exec();
        m_dndIndicator->setLastDragPos(QPoint());
        return true;
    }
    else if (ev->type() == QEvent::MouseButtonPress)
    {
        if (QWidget* w = qobject_cast<QWidget*>(obj))
        {
            qDebug() << w->actions();
        }
    }
    else if (isMouseEvent)
    {
        return true;
    }

    return QToolBar::eventFilter(obj, ev);
}

QAction* EditableQToolBar::actionFromDrop(QDropEvent* ev) const
{
    if (ev->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist"))
    {
        // The drag originated in ToolbarCustomizationDialog's list view of commands, decode it
        QByteArray encoded = ev->mimeData()->data("application/x-qabstractitemmodeldatalist");
        QDataStream stream(&encoded, QIODevice::ReadOnly);

        if (!stream.atEnd())
        {
            int row, col;
            QMap<int,  QVariant> roleDataMap;
            stream >> row >> col >> roleDataMap;
            const int actionId = roleDataMap.value(ActionRole).toInt();
            if (actionId > 0)
            {
                return m_actionManager->GetAction(actionId);
            }
        }
    }
    else if (auto w = qobject_cast<QWidget*>(ev->source()))
    {
        return ActionForWidget(w);
    }

    return nullptr;
}

QAction* EditableQToolBar::ActionForWidget(QWidget* w) const
{
    EditableQToolBar* toolbar = m_toolbarManager->ToolbarParent(w);
    if (!toolbar)
    {
        qWarning() << Q_FUNC_INFO << "Couldn't find parent toolbar for widget" << w;
        return nullptr;
    }

    // Does the reverse of QToolBar::widgetForAction()
    // Useful because only QToolButtons have actions, separators and custom widgets
    // return an empty actions list.

    for (QAction* action : toolbar->actions())
    {
        if (w == toolbar->widgetForAction(action))
        {
            return action;
        }
    }

    return nullptr;
}

void EditableQToolBar::dropEvent(QDropEvent* ev)
{
    auto srcWidget = qobject_cast<QWidget*>(ev->source());
    QAction* action = actionFromDrop(ev);
    if (!action || !srcWidget)
    { // doesn't happen
        qDebug() << Q_FUNC_INFO << "null action or widget" << action << srcWidget;
        return;
    }

    QWidget* beforeWidget = insertPositionForDrop(ev->pos());
    QAction* beforeAction = beforeWidget ? ActionForWidget(beforeWidget) : nullptr;

    if (beforeAction == action)
    {// Same place, nothing to do
        m_dndIndicator->setLastDragPos(QPoint());
        return;
    }

    if (EditableQToolBar* sourceToolbar = m_toolbarManager->ToolbarParent(srcWidget)) // If we're dragging from a QToolBar (instead of the customization dialog)
    {
        if (!m_toolbarManager->DeleteAction(sourceToolbar->ActionForWidget(srcWidget), sourceToolbar))
        {
            qWarning() << Q_FUNC_INFO << "Failed to delete source widget" << srcWidget;
            return;
        }
    }
    m_toolbarManager->InsertAction(action, beforeWidget, beforeAction, this);
    m_dndIndicator->setLastDragPos(QPoint());
}

void EditableQToolBar::dragEnterEvent(QDragEnterEvent* ev)
{
    dragMoveEvent(ev); // Same code to run
}

void EditableQToolBar::dragMoveEvent(QDragMoveEvent* ev)
{
    // FIXME: D&D into & within vertical toolbars is broken
    if (!m_toolbarManager->IsEditingToolBars() || orientation() == Qt::Orientation::Vertical)
    {
        return;
    }

    // We support dragging from a QToolBar but also from ToolbarCustomizationDialog's list view of commands
    auto sourceWidget = qobject_cast<QWidget*>(ev->source());
    if (!sourceWidget)
    {
        qWarning() << Q_FUNC_INFO << "Ignoring drag, widget is null";
        return;
    }

    const bool valid = ev->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist") || ActionForWidget(sourceWidget) != nullptr;
    if (valid)
    {
        m_dndIndicator->setDragSourceWidget(sourceWidget);
        m_dndIndicator->setLastDragPos(ev->pos());
        ev->accept();
        update();
    }
    else
    {
        qWarning() << Q_FUNC_INFO << "Ignoring drag. Widget=" << ev->source();
        m_dndIndicator->setLastDragPos(QPoint());
        ev->ignore();
    }
}

void EditableQToolBar::dragLeaveEvent(QDragLeaveEvent* ev)
{
    if (!m_toolbarManager->IsEditingToolBars())
    {
        return;
    }

    if (!m_dndIndicator->lastDragPos().isNull())
    {
        m_dndIndicator->setLastDragPos(QPoint());
        ev->accept();
        update();
    }
    else
    {
        ev->ignore();
    }
}

AmazonToolbar::AmazonToolbar(const QString& name, const QString& translatedName)
    : m_name(name)
    , m_translatedName(translatedName)
{
}

const bool AmazonToolbar::IsSame(const AmazonToolbar& other) const
{
    if (m_actions.size() != other.m_actions.size())
    {
        return false;
    }

    if (m_showByDefault != other.m_showByDefault)
    {
        return false;
    }

    if (m_showToggled != other.m_showToggled)
    {
        return false;
    }

    bool actionListsSame = AZStd::equal(m_actions.cbegin(), m_actions.cend(), other.m_actions.cbegin());
    if (!actionListsSame)
    {
        return false;
    }

    return true;
}

void AmazonToolbar::InstantiateToolbar(QMainWindow* mainWindow, ToolbarManager* manager)
{
    Q_ASSERT(!m_toolbar);
    m_toolbar = new EditableQToolBar(m_translatedName, manager);
    m_toolbar->setObjectName(m_name);
    if (IsMainToolbar())
    {
        AzQtComponents::ToolBar::addMainToolBarStyle(m_toolbar);
    }
    if (QToolButton* expansion = AzQtComponents::ToolBar::getToolBarExpansionButton(m_toolbar))
    {
        expansion->installEventFilter(manager->GetExpanderWatcher());
    }
    mainWindow->addToolBar(m_toolbar);

    // Hide custom toolbars if they've been flagged that way.
    // We now store whether or not the user has toggled away the default visibility
    // and use that restore in lieu of QMainWindow's restoreState
    // So hide if we're hidden by default XOR we've toggled the default visibility
    if ((!m_showByDefault) ^ m_showToggled)
    {
#ifdef AZ_PLATFORM_MAC
        // on macOS, initially hidden tool bars result in a white rectangle when
        // attaching a previously detached toolbar LY-66320
        m_toolbar->show();
        QMetaObject::invokeMethod(m_toolbar, "hide", Qt::QueuedConnection);
#else
        m_toolbar->hide();
#endif
    }

    ActionManager* actionManager = manager->GetActionManager();
    actionManager->AddToolBar(m_toolbar);

    SetActionsOnInternalToolbar(actionManager);

    UpdateAllowedAreas();
}

void AmazonToolbar::AddAction(int actionId, int toolbarVersionAdded)
{
    AddWidget(nullptr, actionId, toolbarVersionAdded);
}

void AmazonToolbar::AddWidget(QWidget* widget, int actionId, int toolbarVersionAdded)
{
    m_actions.push_back({ actionId, toolbarVersionAdded, widget });
}

void AmazonToolbar::Clear()
{
    m_actions.clear();
}

QVector<int> AmazonToolbar::ActionIds() const
{
    QVector<int> ret;
    ret.reserve(m_actions.size());

    for (auto actionData : m_actions)
    {
        ret.push_back(actionData.actionId);
    }

    return ret;
}

void AmazonToolbar::SetName(const QString& name, const QString& translatedName)
{
    m_name = name;
    m_translatedName = translatedName;
    if (m_toolbar)
    {
        m_toolbar->setWindowTitle(translatedName);
    }
}

#include <moc_ToolbarManager.cpp>
