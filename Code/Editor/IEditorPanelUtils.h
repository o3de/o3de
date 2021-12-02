/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#ifndef CRYINCLUDE_CRYEDITOR_IPANELEDITORUTILS_H
#define CRYINCLUDE_CRYEDITOR_IPANELEDITORUTILS_H
#pragma once

#include "Cry_Vector3.h"

#include "DisplaySettings.h"
#include "Include/IDisplayViewport.h"
#include "Include/IIconManager.h"
#include <qstringlist.h>
#include <qkeysequence.h>
#include <qevent.h>
#include <QShortcutEvent>

class CBaseObject;
class CViewport;
class IQToolTip;

struct HotKey
{
    HotKey()
        : path("")
        , sequence(QKeySequence())
    {
    }
    void CopyFrom(const HotKey& other)
    {
        path = other.path;
        sequence = other.sequence;
    }
    void SetPath(const char* _path)
    {
        path = QString(_path);
    }
    void SetSequenceFromString(const char* _sequence)
    {
        sequence = QKeySequence::fromString(_sequence);
    }
    void SetSequence(const QKeySequence& other)
    {
        sequence = other;
    }
    bool IsMatch(QString _path)
    {
        return path.compare(_path, Qt::CaseInsensitive) == 0;
    }
    bool IsMatch(QKeySequence _sequence)
    {
        return sequence.matches(_sequence);
    }
    bool operator < (const HotKey& other) const
    {
        //split the paths into lists compare per level
        QStringList m_categories = path.split('.');
        QStringList o_categories = other.path.split('.');
        int m_catSize = m_categories.size();
        int o_catSize = o_categories.size();
        int size = (m_catSize < o_catSize) ? m_catSize : o_catSize;

        //sort categories to keep them together
        for (int i = 0; i < size; i++)
        {
            if (m_categories[i] < o_categories[i])
            {
                return true;
            }
            if (m_categories[i] > o_categories[i])
            {
                return false;
            }
        }
        //if comparing a category and a item in that category the category is < item
        return m_catSize > o_catSize;
    }
    QKeySequence sequence;
    QString path;
};

struct IEditorPanelUtils
{
    virtual ~IEditorPanelUtils() {}
    virtual void SetViewportDragOperation(void(*)(CViewport* viewport, int dragPointX, int dragPointY, void* custom), void* custom) = 0;

    //PREVIEW WINDOW UTILS////////////////////////////////////////////////////
    virtual int PreviewWindow_GetDisplaySettingsDebugFlags(CDisplaySettings* settings) = 0;
    virtual void PreviewWindow_SetDisplaySettingsDebugFlags(CDisplaySettings* settings, int flags) = 0;

    //HOTKEY UTILS////////////////////////////////////////////////////////////
    virtual bool HotKey_Import() = 0;
    virtual void HotKey_Export() = 0;
    virtual QKeySequence HotKey_GetShortcut(const char* path) = 0;
    virtual bool HotKey_IsPressed(const QKeyEvent* event, const char* path) = 0;
    virtual bool HotKey_IsPressed(const QShortcutEvent* event, const char* path) = 0;
    virtual bool HotKey_LoadExisting() = 0;
    virtual void HotKey_SaveCurrent() = 0;
    virtual void HotKey_BuildDefaults() = 0;
    virtual void HotKey_SetKeys(QVector<HotKey> keys) = 0;
    virtual QVector<HotKey> HotKey_GetKeys() = 0;
    virtual QString HotKey_GetPressedHotkey(const QKeyEvent* event) = 0;
    virtual QString HotKey_GetPressedHotkey(const QShortcutEvent* event) = 0;
    virtual void HotKey_SetEnabled(bool val) = 0;
    virtual bool HotKey_IsEnabled() const = 0;

    //TOOLTIP UTILS///////////////////////////////////////////////////////////
    
    //! Loads a table of tooltip configuration data from an xml file.
    virtual void ToolTip_LoadConfigXML(QString filepath) = 0;
    
    //! Initializes a QToolTipWidget from loaded configuration data (see ToolTip_LoadConfigXML())
    //! \param tooltip       Will be initialized using loaded configuration data
    //! \param path          Variable serialization path. Will be used as the key for looking up data in the configuration table. (ex: "Rotation.Rotation_Rate_X")
    //! \param option        Name of a sub-option of the variable specified by "path". (ex: "Emitter_Strength" will look up the tooltip data for "Rotation.Rotation_Rate_X.Emitter_Strength")
    //! \param optionalData  The argument to be used with "special_content" feature. See ToolTip_GetSpecialContentType() and QToolTipWidget::AddSpecialContent().
    //! \param isEnabled     If false, the tooltip will indicate the reason why the widget is disabled.
    virtual void ToolTip_BuildFromConfig(IQToolTip* tooltip, QString path, QString option, QString optionalData = "", bool isEnabled = true) = 0;
    
    virtual QString ToolTip_GetTitle(QString path, QString option = "") = 0;
    virtual QString ToolTip_GetContent(QString path, QString option = "") = 0;
    virtual QString ToolTip_GetSpecialContentType(QString path, QString option = "") = 0;
    virtual QString ToolTip_GetDisabledContent(QString path, QString option = "") = 0;
};


#endif
