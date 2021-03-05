//-------------------------------------------------------------------------------
// Copyright (C) Amazon.com, Inc. or its affiliates.
// All Rights Reserved.
//
// Licensed under the terms set out in the LICENSE.HTML file included at the
// root of the distribution; you may not use this file except in compliance
// with the License.
//
// Do not remove or modify this notice or the LICENSE.HTML file.  This file
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
// either express or implied. See the License for the specific language
// governing permissions and limitations under the License.
//-------------------------------------------------------------------------------

// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYROWSPRITE_H
#define CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYROWSPRITE_H
#pragma once

#if !defined(Q_MOC_RUN)
#include "PropertyRowField.h"
#endif

class PropertyRowSprite
: public PropertyRowField
{
public:
    void clear();

    bool isLeaf() const override{ return true; }
    bool isStatic() const override{ return false; }

    void setValueAndContext(const Serialization::SStruct& ser, Serialization::IArchive& ar) override;
    bool assignTo(const Serialization::SStruct& ser) const override;
    bool onActivateButton(int buttonIndex, const PropertyActivationEvent& ev) override;
    bool onActivate(const PropertyActivationEvent& ev) override;

    int buttonCount() const override;
    virtual const QIcon& buttonIcon(const QPropertyTree* tree, int index) const override;
    virtual bool usePathEllipsis() const override { return true; }
    string valueAsString() const override;
    void serializeValue(Serialization::IArchive& ar);

    bool onContextMenu(QMenu& menu, QPropertyTree* tree);

    bool processesKey(QPropertyTree* tree, const QKeyEvent* ev) override;
    bool onKeyDown(QPropertyTree* tree, const QKeyEvent* ev) override;

private:

    enum class Show
    {
        kFilePicker,
        kSpriteBorderEditor
    };

    void Clear(QPropertyTree* tree);
    bool showFilePicker(const PropertyActivationEvent& ev);
    bool showSpriteBorderEditor(const PropertyActivationEvent& ev);
    bool CanBeEdited() const;
    Show FromButtonIndexToShow(int index) const;

    string m_path;
    string m_filter;
    string m_startFolder;
    int m_flags;
};

#endif // CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYROWSPRITE_H
