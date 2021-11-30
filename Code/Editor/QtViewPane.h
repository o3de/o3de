/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITORCOMMON_QTVIEWPANE_H
#define CRYINCLUDE_EDITORCOMMON_QTVIEWPANE_H
#pragma once

#include "IEditor.h"
#include "Include/IEditorClassFactory.h"
#include "Include/IViewPane.h"
#include "Include/ObjectEvent.h"
#include "Objects/ClassDesc.h"

#include <QRect>
#include <QWidget>

namespace Serialization {
    class IArchive;
}
using Serialization::IArchive;

// ---------------------------------------------------------------------------

template<class TObject>
class CTemplateObjectClassDesc
    : public CObjectClassDesc
{
public:
    const char* m_className;
    const char* m_category;
    const char* m_textureIcon;
    ObjectType m_objectType;
    int m_order;
    const char* m_fileSpec;
    const char* m_toolClassName;

    CTemplateObjectClassDesc(const char* className, const char* category, const char* textureIcon, ObjectType objectType, int order = 100, const char* fileSpec = "", const char* toolClassName = nullptr)
        : m_className(className)
        , m_category(category)
        , m_textureIcon(textureIcon)
        , m_objectType(objectType)
        , m_order(order)
        , m_fileSpec(fileSpec)
        , m_toolClassName(toolClassName)
    {
    }

    REFGUID ClassID() override
    {
        return TObject::GetClassID();
    }

    QString GetFileSpec() override
    {
        return m_fileSpec;
    }

    ESystemClassID SystemClassID() override
    {
        return ESYSTEM_CLASS_OBJECT;
    };

    ObjectType GetObjectType() override
    {
        return m_objectType;
    }

    QString ClassName() override
    {
        return m_className;
    }

    QString Category() override
    {
        return m_category;
    }

    QString GetTextureIcon() override
    {
        return m_textureIcon;
    }

    QObject* CreateQObject() const override
    {
        return new TObject;
    }

    int GameCreationOrder() override
    {
        return m_order;
    };

    bool IsEnabled() const override
    {
        return TObject::IsEnabled();
    }

    QString GetToolClassName() override
    {
        return m_toolClassName ? m_toolClassName : CObjectClassDesc::GetToolClassName();
    }
};

template<class TWidget>
class CQtViewClass
    : public IViewPaneClass
{
public:
    const char* m_name;
    const char* m_category;
    ESystemClassID m_classId;

    CQtViewClass(const char* name, const char* category, ESystemClassID classId = ESYSTEM_CLASS_VIEWPANE)
        : m_name(name)
        , m_category(category)
        , m_classId(classId)
    {
    }

    ESystemClassID SystemClassID() override { return m_classId; };
    static const GUID& GetClassID()
    {
        return TWidget::GetClassID();
    }

    const GUID& ClassID() override
    {
        return GetClassID();
    }
    QString ClassName() override { return m_name; };
    QString Category() override { return m_category; };

    QObject* CreateQObject() const override { return new TWidget(); };
    QString GetPaneTitle() override { return m_name; };
    unsigned int GetPaneTitleID() const override { return 0; };
    EDockingDirection GetDockingDirection() override { return DOCK_FLOAT; };
    QRect GetPaneRect() override { return {}; /* KDAB_TODO: ;m_sizeOptions.m_paneRect; */};
    bool SinglePane() override { return false; };
    bool WantIdleUpdate() override { return true; };
    QSize GetMinSize() override { return {}; /*return m_sizeOptions.m_minSize;*/ }
};

#endif // CRYINCLUDE_EDITORCOMMON_QTVIEWPANE_H
