/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "EditorCommon_precompiled.h"
#include "PropertyRowColorPicker.h"
#include "Serialization/ClassFactory.h"
#include <IEditor.h>
#include <QMenu>
#include <QPainter>
#include <QIcon>
#include <QKeyEvent>

#include <AzQtComponents/Components/Widgets/ColorPicker.h>
#include <AzQtComponents/Utilities/Conversions.h>

bool PropertyRowColorPicker::onActivate(const PropertyActivationEvent& e)
{
    if (e.reason == e.REASON_RELEASE)
    {
        return false;
    }

    // ColorF -> QColor.
    AZ::Color initialColor;
    initialColor.SetR(color_.r);
    initialColor.SetG(color_.g);
    initialColor.SetB(color_.b);
    initialColor.SetA(color_.a);

    const AZ::Color colorFromDialog = AzQtComponents::ColorPicker::getColor(AzQtComponents::ColorPicker::Configuration::RGBA,
            initialColor,
            QObject::tr("Select Color"));

    if (initialColor == colorFromDialog)
    {
        // The user cancelled the dialog box.
        // Nothing more to do.
        return false;
    }

    // QColor -> ColorF.
    ColorF color(colorFromDialog.GetR(),
        colorFromDialog.GetG(),
        colorFromDialog.GetB(),
        colorFromDialog.GetA());

    e.tree->model()->rowAboutToBeChanged(this);
    color_ = color;
    e.tree->model()->rowChanged(this);

    return true;
}
void PropertyRowColorPicker::setValueAndContext(const Serialization::SStruct& ser, [[maybe_unused]] Serialization::IArchive& ar)
{
    ColorPicker* value = (ColorPicker*)ser.pointer();
    color_ = *value->color;
}

bool PropertyRowColorPicker::assignTo(const Serialization::SStruct& ser) const
{
    ((ColorPicker*)ser.pointer())->SetColor(&color_);

    return true;
}

void PropertyRowColorPicker::serializeValue(Serialization::IArchive& ar)
{
    ar(color_, "color");
}

const QIcon& PropertyRowColorPicker::buttonIcon([[maybe_unused]] const QPropertyTree* tree, [[maybe_unused]] int index) const
{
    // Color-chip.

    QColor color((int)(color_.r * 255.0f),
        (int)(color_.g * 255.0f),
        (int)(color_.b * 255.0f));
    QPen pen(color);
    QBrush brush(color);

    QPixmap pixmap(16, 16);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setBrush(brush);
    painter.setPen(pen);
    painter.drawEllipse(0, 0, 15, 15);

    static QIcon icon;
    icon.addPixmap(pixmap);
    return icon;
}

string PropertyRowColorPicker::valueAsString() const
{
    int r = (int)(255.0f * color_.r);
    int g = (int)(255.0f * color_.g);
    int b = (int)(255.0f * color_.b);
    int a = (int)(255.0f * color_.a);
    string value;
    value.Format("#%02x%02x%02x%02x", r, g, b, a);
    return value;
}

void PropertyRowColorPicker::clear()
{
    color_ = Col_White;
}

bool PropertyRowColorPicker::onContextMenu(QMenu& menu, QPropertyTree* tree)
{
    QAction* action = menu.addAction("Clear");
    QObject::connect(action,
        &QAction::triggered,
        tree,
        [ this, tree ]
        {
            tree->model()->rowAboutToBeChanged(this);
            clear();
            tree->model()->rowChanged(this);
        });
    return true;
}

bool PropertyRowColorPicker::processesKey(QPropertyTree* tree, const QKeyEvent* ev)
{
    if (QKeySequence(ev->key()) == QKeySequence(Qt::Key_Delete))
    {
        return true;
    }

    return PropertyRowField::processesKey(tree, ev);
}

bool PropertyRowColorPicker::onKeyDown(QPropertyTree* tree, const QKeyEvent* ev)
{
    if (ev->key() == Qt::Key_Delete)
    {
        tree->model()->rowAboutToBeChanged(this);
        clear();
        tree->model()->rowChanged(this);
        return true;
    }
    return PropertyRowField::onKeyDown(tree, ev);
}

REGISTER_PROPERTY_ROW(ColorPicker, PropertyRowColorPicker);
DECLARE_SEGMENT(PropertyRowColorPicker)

