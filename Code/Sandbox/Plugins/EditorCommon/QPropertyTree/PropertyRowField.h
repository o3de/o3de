/**
 *  wWidgets - Lightweight UI Toolkit.
 *  Copyright (C) 2009-2011 Evgeny Andreeshchev <eugene.andreeshchev@gmail.com>
 *                          Alexander Kotliar <alexander.kotliar@gmail.com>
 * 
 *  This code is distributed under the MIT License:
 *                          http://www.opensource.org/licenses/MIT
 */
// Modifications copyright Amazon.com, Inc. or its affiliates.

#ifndef CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYROWFIELD_H
#define CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYROWFIELD_H
#pragma once

#include "PropertyRow.h"
class QIcon;

class PropertyRowField : public PropertyRow
{
public:
    WidgetPlacement widgetPlacement() const override{ return WIDGET_VALUE; }
    int widgetSizeMin(const QPropertyTree* tree) const override;

    virtual int buttonCount() const{ return 0; }
    virtual const QIcon& buttonIcon(const QPropertyTree* tree, int index) const;
    virtual bool usePathEllipsis() const { return false; }
    virtual bool onActivateButton([[maybe_unused]] int buttonIndex, [[maybe_unused]] const PropertyActivationEvent& e) { return false; }

    void redraw(const PropertyDrawContext& context) override;
    bool onActivate(const PropertyActivationEvent& e) override;
protected:
    QRect fieldRect(const QPropertyTree* tree) const;
    void drawButtons(int* offset);

    mutable RowWidthCache widthCache_;
};


#endif // CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYROWFIELD_H
