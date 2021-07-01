/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */



#include "EditorDefs.h"

#include "EditorPanelUtils.h"

#include <AzCore/Utils/Utils.h>

// Qt
#include <QInputDialog>
#include <QFileDialog>
#include <QXmlStreamReader>

// Editor
#include "IEditorPanelUtils.h"
#include "Objects/EntityObject.h"
#include "CryEditDoc.h"
#include "ViewManager.h"
#include "Controls/QToolTipWidget.h"
#include "Objects/SelectionGroup.h"



#ifndef PI
#define PI 3.14159265358979323f
#endif


struct ToolTip
{
    bool isValid;
    QString title;
    QString content;
    QString specialContent;
    QString disabledContent;
};

// internal implementation for better compile times - should also never be used externally, use IParticleEditorUtils interface for that.
class CEditorPanelUtils_Impl
    : public IEditorPanelUtils
{
    #pragma region Drag & Drop
public:
    virtual void SetViewportDragOperation(void(* dropCallback)(CViewport* viewport, int dragPointX, int dragPointY, void* custom), void* custom) override
    {
        for (int i = 0; i < GetIEditor()->GetViewManager()->GetViewCount(); i++)
        {
            GetIEditor()->GetViewManager()->GetView(i)->SetGlobalDropCallback(dropCallback, custom);
        }
    }
    #pragma endregion
    #pragma region Preview Window
public:

    virtual int PreviewWindow_GetDisplaySettingsDebugFlags(CDisplaySettings* settings)
    {
        CRY_ASSERT(settings);
        return settings->GetDebugFlags();
    }

    virtual void PreviewWindow_SetDisplaySettingsDebugFlags(CDisplaySettings* settings, int flags)
    {
        CRY_ASSERT(settings);
        settings->SetDebugFlags(flags);
    }

    #pragma endregion
    #pragma region Shortcuts
protected:
    QVector<HotKey> hotkeys;
    bool m_hotkeysAreEnabled;
public:

    virtual bool HotKey_Import() override
    {
        QVector<QPair<QString, QString> > keys;
        QString filepath = QFileDialog::getOpenFileName(nullptr, "Select shortcut configuration to load",
                QString(), "HotKey Config Files (*.hkxml)");
        QFile file(filepath);
        if (!file.open(QIODevice::ReadOnly))
        {
            return false;
        }
        QXmlStreamReader stream(&file);
        bool result = true;

        while (!stream.isEndDocument())
        {
            if (stream.isStartElement())
            {
                if (stream.name() == "HotKey")
                {
                    QPair<QString, QString> key;
                    QXmlStreamAttributes att = stream.attributes();
                    for (QXmlStreamAttribute attr : att)
                    {
                        if (attr.name().compare(QLatin1String("path"), Qt::CaseInsensitive) == 0)
                        {
                            key.first = attr.value().toString();
                        }
                        if (attr.name().compare(QLatin1String("sequence"), Qt::CaseInsensitive) == 0)
                        {
                            key.second = attr.value().toString();
                        }
                    }
                    if (!key.first.isEmpty())
                    {
                        keys.push_back(key); // we allow blank key sequences for unassigned shortcuts
                    }
                    else
                    {
                        result = false; //but not blank paths!
                    }
                }
            }
            stream.readNext();
        }
        file.close();

        if (result)
        {
            HotKey_BuildDefaults();
            for (QPair<QString, QString> key : keys)
            {
                for (unsigned int j = 0; j < hotkeys.count(); j++)
                {
                    if (hotkeys[j].path.compare(key.first, Qt::CaseInsensitive) == 0)
                    {
                        hotkeys[j].SetPath(key.first.toStdString().c_str());
                        hotkeys[j].SetSequenceFromString(key.second.toStdString().c_str());
                    }
                }
            }
        }
        return result;
    }

    virtual void HotKey_Export() override
    {
        auto settingDir = AZ::IO::FixedMaxPath(AZ::Utils::GetEnginePath()) / "Editor" / "Plugins" / "ParticleEditorPlugin" / "settings";
        QString filepath = QFileDialog::getSaveFileName(nullptr, "Select shortcut configuration to load", settingDir.c_str(), "HotKey Config Files (*.hkxml)");
        QFile file(filepath);
        if (!file.open(QIODevice::WriteOnly))
        {
            return;
        }

        QXmlStreamWriter stream(&file);
        stream.setAutoFormatting(true);
        stream.writeStartDocument();
        stream.writeStartElement("HotKeys");

        for (HotKey key : hotkeys)
        {
            stream.writeStartElement("HotKey");
            stream.writeAttribute("path", key.path);
            stream.writeAttribute("sequence", key.sequence.toString());
            stream.writeEndElement();
        }
        stream.writeEndElement();
        stream.writeEndDocument();
        file.close();
    }

    virtual QKeySequence HotKey_GetShortcut(const char* path) override
    {
        for (HotKey combo : hotkeys)
        {
            if (combo.IsMatch(path))
            {
                return combo.sequence;
            }
        }
        return QKeySequence();
    }

    virtual bool HotKey_IsPressed(const QKeyEvent* event, const char* path) override
    {
        if (!m_hotkeysAreEnabled)
        {
            return false;
        }
        unsigned int keyInt = 0;
        //Capture any modifiers
        Qt::KeyboardModifiers modifiers = QApplication::keyboardModifiers();
        if (modifiers & Qt::ShiftModifier)
        {
            keyInt += Qt::SHIFT;
        }
        if (modifiers & Qt::ControlModifier)
        {
            keyInt += Qt::CTRL;
        }
        if (modifiers & Qt::AltModifier)
        {
            keyInt += Qt::ALT;
        }
        if (modifiers & Qt::MetaModifier)
        {
            keyInt += Qt::META;
        }
        //Capture any key
        keyInt += event->key();

        QString t0 = QKeySequence(keyInt).toString();
        QString t1 = HotKey_GetShortcut(path).toString();

        //if strings match then shortcut is pressed
        if (t1.compare(t0, Qt::CaseInsensitive) == 0)
        {
            return true;
        }
        return false;
    }

    virtual bool HotKey_IsPressed(const QShortcutEvent* event, const char* path) override
    {
        if (!m_hotkeysAreEnabled)
        {
            return false;
        }

        QString t0 = event->key().toString();
        QString t1 = HotKey_GetShortcut(path).toString();

        //if strings match then shortcut is pressed
        if (t1.compare(t0, Qt::CaseInsensitive) == 0)
        {
            return true;
        }
        return false;
    }

    virtual bool HotKey_LoadExisting() override
    {
        QSettings settings("O3DE", "O3DE");
        QString group = "Hotkeys/";

        HotKey_BuildDefaults();

        int size = settings.beginReadArray(group);

        for (int i = 0; i < size; i++)
        {
            settings.setArrayIndex(i);
            QPair<QString, QString> hotkey;
            hotkey.first = settings.value("name").toString();
            hotkey.second = settings.value("keySequence").toString();
            if (!hotkey.first.isEmpty())
            {
                for (unsigned int j = 0; j < hotkeys.count(); j++)
                {
                    if (hotkeys[j].path.compare(hotkey.first, Qt::CaseInsensitive) == 0)
                    {
                        hotkeys[j].SetPath(hotkey.first.toStdString().c_str());
                        hotkeys[j].SetSequenceFromString(hotkey.second.toStdString().c_str());
                    }
                }
            }
        }

        settings.endArray();
        if (hotkeys.isEmpty())
        {
            return false;
        }
        return true;
    }

    virtual void HotKey_SaveCurrent() override
    {
        QSettings settings("O3DE", "O3DE");
        QString group = "Hotkeys/";
        settings.remove("Hotkeys/");
        settings.sync();
        settings.beginWriteArray(group);
        int saveIndex = 0;
        for (HotKey key : hotkeys)
        {
            if (!key.path.isEmpty())
            {
                settings.setArrayIndex(saveIndex++);
                settings.setValue("name", key.path);
                settings.setValue("keySequence", key.sequence.toString());
            }
        }
        settings.endArray();
        settings.sync();
    }

    virtual void HotKey_BuildDefaults() override
    {
        m_hotkeysAreEnabled = true;
        QVector<QPair<QString, QString> > keys;
        while (hotkeys.count() > 0)
        {
            hotkeys.takeAt(0);
        }

        //MENU SELECTION SHORTCUTS////////////////////////////////////////////////
        keys.push_back(QPair<QString, QString>("Menus.File Menu", "Alt+F"));
        keys.push_back(QPair<QString, QString>("Menus.Edit Menu", "Alt+E"));
        keys.push_back(QPair<QString, QString>("Menus.View Menu", "Alt+V"));
        //FILE MENU SHORTCUTS/////////////////////////////////////////////////////
        keys.push_back(QPair<QString, QString>("File Menu.Create new emitter", "Ctrl+N"));
        keys.push_back(QPair<QString, QString>("File Menu.Create new library", "Ctrl+Shift+N"));
        keys.push_back(QPair<QString, QString>("File Menu.Create new folder", ""));
        keys.push_back(QPair<QString, QString>("File Menu.Import", "Ctrl+I"));
        keys.push_back(QPair<QString, QString>("File Menu.Import level library", "Ctrl+Shift+I"));
        keys.push_back(QPair<QString, QString>("File Menu.Save", "Ctrl+S"));
        keys.push_back(QPair<QString, QString>("File Menu.Close", "Ctrl+Q"));
        //EDIT MENU SHORTCUTS/////////////////////////////////////////////////////
        keys.push_back(QPair<QString, QString>("Edit Menu.Copy", "Ctrl+C"));
        keys.push_back(QPair<QString, QString>("Edit Menu.Paste", "Ctrl+V"));
        keys.push_back(QPair<QString, QString>("Edit Menu.Duplicate", "Ctrl+D"));
        keys.push_back(QPair<QString, QString>("Edit Menu.Undo", "Ctrl+Z"));
        keys.push_back(QPair<QString, QString>("Edit Menu.Redo", "Ctrl+Shift+Z"));
        keys.push_back(QPair<QString, QString>("Edit Menu.Group", "Ctrl+G"));
        keys.push_back(QPair<QString, QString>("Edit Menu.Ungroup", "Ctrl+Shift+G"));
        keys.push_back(QPair<QString, QString>("Edit Menu.Rename", "Ctrl+R"));
        keys.push_back(QPair<QString, QString>("Edit Menu.Reset", ""));
        keys.push_back(QPair<QString, QString>("Edit Menu.Edit Hotkeys", ""));
        keys.push_back(QPair<QString, QString>("Edit Menu.Assign to selected", "Ctrl+Space"));
        keys.push_back(QPair<QString, QString>("Edit Menu.Insert Comment", "Ctrl+Alt+M"));
        keys.push_back(QPair<QString, QString>("Edit Menu.Enable/Disable Emitter", "Ctrl+E"));
        keys.push_back(QPair<QString, QString>("File Menu.Enable All", ""));
        keys.push_back(QPair<QString, QString>("File Menu.Disable All", ""));
        keys.push_back(QPair<QString, QString>("Edit Menu.Delete", "Del"));
        //VIEW MENU SHORTCUTS/////////////////////////////////////////////////////
        keys.push_back(QPair<QString, QString>("View Menu.Reset Layout", ""));
        //PLAYBACK CONTROL////////////////////////////////////////////////////////
        keys.push_back(QPair<QString, QString>("Previewer.Play/Pause Toggle", "Space"));
        keys.push_back(QPair<QString, QString>("Previewer.Step forward through time", "c"));
        keys.push_back(QPair<QString, QString>("Previewer.Loop Toggle", "z"));
        keys.push_back(QPair<QString, QString>("Previewer.Reset Playback", "x"));
        keys.push_back(QPair<QString, QString>("Previewer.Focus", "Ctrl+F"));
        keys.push_back(QPair<QString, QString>("Previewer.Zoom In", "w"));
        keys.push_back(QPair<QString, QString>("Previewer.Zoom Out", "s"));
        keys.push_back(QPair<QString, QString>("Previewer.Pan Left", "a"));
        keys.push_back(QPair<QString, QString>("Previewer.Pan Right", "d"));

        for (QPair<QString, QString> key : keys)
        {
            unsigned int index = hotkeys.count();
            hotkeys.push_back(HotKey());
            hotkeys[index].SetPath(key.first.toStdString().c_str());
            hotkeys[index].SetSequenceFromString(key.second.toStdString().c_str());
        }
    }

    virtual void HotKey_SetKeys(QVector<HotKey> keys) override
    {
        hotkeys = keys;
    }

    virtual QVector<HotKey> HotKey_GetKeys() override
    {
        return hotkeys;
    }

    virtual QString HotKey_GetPressedHotkey(const QKeyEvent* event) override
    {
        if (!m_hotkeysAreEnabled)
        {
            return "";
        }
        for (HotKey key : hotkeys)
        {
            if (HotKey_IsPressed(event, key.path.toUtf8()))
            {
                return key.path;
            }
        }
        return "";
    }
    virtual QString HotKey_GetPressedHotkey(const QShortcutEvent* event) override
    {
        if (!m_hotkeysAreEnabled)
        {
            return "";
        }
        for (HotKey key : hotkeys)
        {
            if (HotKey_IsPressed(event, key.path.toUtf8()))
            {
                return key.path;
            }
        }
        return "";
    }
    //building the default hotkey list re-enables hotkeys
    //do not use this when rebuilding the default list is a possibility.
    virtual void HotKey_SetEnabled(bool val) override
    {
        m_hotkeysAreEnabled = val;
    }

    virtual bool HotKey_IsEnabled() const override
    {
        return m_hotkeysAreEnabled;
    }

    #pragma  endregion
    #pragma region ToolTip
protected:
    QMap<QString, ToolTip> m_tooltips;

    void ToolTip_ParseNode(XmlNodeRef node)
    {
        if (QString(node->getTag()).compare("tooltip", Qt::CaseInsensitive) != 0)
        {
            unsigned int childCount = node->getChildCount();

            for (unsigned int i = 0; i < childCount; i++)
            {
                ToolTip_ParseNode(node->getChild(i));
            }
        }

        QString title = node->getAttr("title");
        QString content = node->getAttr("content");
        QString specialContent = node->getAttr("special_content");
        QString disabledContent = node->getAttr("disabled_content");

        QMap<QString, ToolTip>::iterator itr = m_tooltips.insert(node->getAttr("path"), ToolTip());
        itr->isValid = true;
        itr->title = title;
        itr->content = content;
        itr->specialContent = specialContent;
        itr->disabledContent = disabledContent;

        unsigned int childCount = node->getChildCount();

        for (unsigned int  i = 0; i < childCount; i++)
        {
            ToolTip_ParseNode(node->getChild(i));
        }
    }

    ToolTip GetToolTip(QString path)
    {
        if (m_tooltips.contains(path))
        {
            return m_tooltips[path];
        }
        ToolTip temp;
        temp.isValid = false;
        return temp;
    }

public:
    virtual void ToolTip_LoadConfigXML(QString filepath) override
    {
        XmlNodeRef node = GetIEditor()->GetSystem()->LoadXmlFromFile(filepath.toStdString().c_str());
        ToolTip_ParseNode(node);
    }

    virtual void ToolTip_BuildFromConfig(IQToolTip* tooltip, QString path, QString option, QString optionalData = "", bool isEnabled = true)
    {
        AZ_Assert(tooltip, "tooltip cannot be null");

        QString title = ToolTip_GetTitle(path, option);
        QString content = ToolTip_GetContent(path, option);
        QString specialContent = ToolTip_GetSpecialContentType(path, option);
        QString disabledContent = ToolTip_GetDisabledContent(path, option);

        // Even if these items are empty, we set them anyway to clear out any data that was left over from when the tooltip was used for a different object.
        tooltip->SetTitle(title);
        tooltip->SetContent(content);

        //this only handles simple creation...if you need complex call this then add specials separate
        if (!specialContent.contains("::"))
        {
            tooltip->AddSpecialContent(specialContent, optionalData);
        }

        if (!isEnabled) // If disabled, add disabled value
        {
            tooltip->AppendContent(disabledContent);
        }
    }

    virtual QString ToolTip_GetTitle(QString path, QString option) override
    {
        if (!option.isEmpty() && GetToolTip(path + "." + option).isValid)
        {
            return GetToolTip(path + "." + option).title;
        }
        if (!option.isEmpty() && GetToolTip("Options." + option).isValid)
        {
            return GetToolTip("Options." + option).title;
        }
        return GetToolTip(path).title;
    }

    virtual QString ToolTip_GetContent(QString path, QString option) override
    {
        if (!option.isEmpty() && GetToolTip(path + "." + option).isValid)
        {
            return GetToolTip(path + "." + option).content;
        }
        if (!option.isEmpty() && GetToolTip("Options." + option).isValid)
        {
            return GetToolTip("Options." + option).content;
        }
        return GetToolTip(path).content;
    }

    virtual QString ToolTip_GetSpecialContentType(QString path, QString option) override
    {
        if (!option.isEmpty() && GetToolTip(path + "." + option).isValid)
        {
            return GetToolTip(path + "." + option).specialContent;
        }
        if (!option.isEmpty() && GetToolTip("Options." + option).isValid)
        {
            return GetToolTip("Options." + option).specialContent;
        }
        return GetToolTip(path).specialContent;
    }

    virtual QString ToolTip_GetDisabledContent(QString path, QString option) override
    {
        if (!option.isEmpty() && GetToolTip(path + "." + option).isValid)
        {
            return GetToolTip(path + "." + option).disabledContent;
        }
        if (!option.isEmpty() && GetToolTip("Options." + option).isValid)
        {
            return GetToolTip("Options." + option).disabledContent;
        }
        return GetToolTip(path).disabledContent;
    }
    #pragma endregion ToolTip
};

IEditorPanelUtils* CreateEditorPanelUtils()
{
    return new CEditorPanelUtils_Impl();
}

