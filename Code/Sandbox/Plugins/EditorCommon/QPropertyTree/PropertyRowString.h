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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYROWSTRING_H
#define CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYROWSTRING_H
#pragma once

#if !defined(Q_MOC_RUN)
#include "PropertyRowField.h"
#include "QPropertyTree.h"
#include "PropertyTreeModel.h"
#include "Unicode.h"
#include "MathUtils.h"

#include <QLineEdit>
#endif

class PropertyRowString
    : public PropertyRowField
{
public:
    bool isLeaf() const override { return true; }
    bool isStatic() const override { return false; }
    using PropertyRowField::assignTo;
    bool assignTo(string& str) const;
    bool assignTo(wstring& str) const;
    void setValue(const char* str, const void* handle, const Serialization::TypeID& typeId);
    void setValue(const wchar_t* str, const void* handle, const Serialization::TypeID& typeId);
    PropertyRowWidget* createWidget(QPropertyTree* tree);
    string valueAsString() const;
    wstring valueAsWString() const { return value_; }
    WidgetPlacement widgetPlacement() const override { return WIDGET_VALUE; }
    void serializeValue(Serialization::IArchive& ar) override;
    const wstring& value() const{ return value_; }
    bool assignToByPointer(void* instance, const Serialization::TypeID& type) const;
protected:
    wstring value_;
};

class PropertyRowWidgetString
    : public PropertyRowWidget
{
    Q_OBJECT
public:
    PropertyRowWidgetString(PropertyRowString* row, QPropertyTree* tree)
        : PropertyRowWidget(row, tree)
        , entry_(new QLineEdit())
        , tree_(tree)
    {
        initialValue_ = QString(fromWideChar(row->value().c_str()).c_str());
        entry_->setText(initialValue_);
        entry_->selectAll();
        connect(entry_.data(), SIGNAL(editingFinished()), this, SLOT(onEditingFinished()));
        connect(entry_.data(), &QLineEdit::textChanged, this, [this, tree] {
                QFontMetrics fm(entry_->font());
                int contentWidth = min((int)fm.horizontalAdvance(entry_->text()) + 8, tree->width() - entry_->x());
                if (contentWidth > entry_->width())
                {
                    entry_->resize(contentWidth, entry_->height());
                }
            });
    }
    ~PropertyRowWidgetString()
    {
        entry_->hide();
        entry_->setParent(0);
        entry_.take()->deleteLater();
    }

    void commit()
    {
        onEditingFinished();
    }
    QWidget* actualWidget() { return entry_.data(); }

public slots:
    void onEditingFinished()
    {
        PropertyRowString* row = static_cast<PropertyRowString*>(this->row());
        if (initialValue_ != entry_->text() || row_->multiValue())
        {
            model()->rowAboutToBeChanged(row);
            vector<wchar_t> str;
            QString text = entry_->text();
            str.resize(text.size() + 1, L'\0');
            if (!text.isEmpty())
            {
                text.toWCharArray(&str[0]);
            }
            row->setValue(&str[0], row->searchHandle(), row->typeId());
            model()->rowChanged(row);
        }
        else
        {
            tree_->_cancelWidget();
        }
    }
protected:
    QPropertyTree* tree_;
    QScopedPointer<QLineEdit> entry_;
    QString initialValue_;
};

#endif // CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYROWSTRING_H
