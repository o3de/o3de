/**
 *  wWidgets - Lightweight UI Toolkit.
 *  Copyright (C) 2009-2011 Evgeny Andreeshchev <eugene.andreeshchev@gmail.com>
 *                          Alexander Kotliar <alexander.kotliar@gmail.com>
 * 
 *  This code is distributed under the MIT License:
 *                          http://www.opensource.org/licenses/MIT
 */
// Modifications copyright Amazon.com, Inc. or its affiliates.

#include "EditorCommon_precompiled.h"
#include "PropertyRowField.h"
#include "PropertyDrawContext.h"
#include "QPropertyTree.h"
#include "QPropertyTreeStyle.h"
#include <QtGui/QIcon>

enum { BUTTON_SIZE = 16 };

QRect PropertyRowField::fieldRect(const QPropertyTree* tree) const
{
        QRect fieldRect = widgetRect(tree);
        fieldRect.setRight(fieldRect.right() - buttonCount() * BUTTON_SIZE);
        return fieldRect;
}

bool PropertyRowField::onActivate(const PropertyActivationEvent& e)
{
    if (e.reason == e.REASON_PRESS) {
        int buttonCount = this->buttonCount();
        QRect buttonsRect = widgetRect(e.tree);
        buttonsRect.setLeft(buttonsRect.right() - buttonCount * BUTTON_SIZE);

        if (buttonsRect.contains(e.clickPoint)) {
            int buttonIndex = buttonCount - (e.clickPoint.x() - buttonsRect.x()) / BUTTON_SIZE - 1;
            if (buttonIndex >= 0 && buttonIndex < buttonCount)
            {
                if (onActivateButton(buttonIndex, e))
                    return true;
            }
        }
    }

    return PropertyRow::onActivate(e);
}

void PropertyRowField::redraw(const PropertyDrawContext& context)
{
    int buttonCount = this->buttonCount();
    int offset = 0;
    for (int i = 0; i < buttonCount; ++i) {
        const QIcon& icon = buttonIcon(context.tree, i);
        QRect iconRect(context.widgetRect.right() - offset - BUTTON_SIZE, context.widgetRect.top(), BUTTON_SIZE, context.widgetRect.height());
        icon.paint(context.painter, iconRect, Qt::AlignCenter, userReadOnly() ? QIcon::Disabled : QIcon::Normal);
        offset += BUTTON_SIZE;
    }

    int iconSpace = offset ? offset + 2 : 0;
    if(multiValue())
        context.drawEntry(L" ... ", false, true, iconSpace);
    else if(userReadOnly())
        context.drawValueText(pulledSelected(), valueAsWString().c_str());
    else
        context.drawEntry(valueAsWString().c_str(), usePathEllipsis(), false, iconSpace);

}

const QIcon& PropertyRowField::buttonIcon([[maybe_unused]] const QPropertyTree* tree, [[maybe_unused]] int index) const
{
    static QIcon defaultIcon;
    return defaultIcon;
}

int PropertyRowField::widgetSizeMin(const QPropertyTree* tree) const
{ 
    if (userWidgetSize() >= 0)
        return userWidgetSize();

    if (userWidgetToContent_)
        return widthCache_.getOrUpdate(tree, this, 0);
    else
        return 40;
}

