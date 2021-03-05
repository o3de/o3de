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

#ifndef CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYROWSTRINGLISTVALUE_H
#define CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYROWSTRINGLISTVALUE_H
#pragma once
#if !defined(Q_MOC_RUN)
#include "PropertyRowImpl.h"
#include "PropertyTreeModel.h"
#include "PropertyDrawContext.h"
#include "QPropertyTree.h"
#include <QComboBox>
#include <QMouseEvent>
#include <QApplication>
#include <QStyle>
#include <QtGui/QPainter>
#endif

using Serialization::StringListValue;
class PropertyRowStringListValue
    : public PropertyRow
{
public:
    PropertyRowStringListValue()
        : handle_() {}
    PropertyRowWidget* createWidget(QPropertyTree* tree) override;
    string valueAsString() const override { return value_.c_str(); }
    bool assignTo(const Serialization::SStruct& ser) const override
    {
        *((StringListValue*)ser.pointer()) = value_.c_str();
        return true;
    }
    void setValueAndContext(const Serialization::SStruct& ser, [[maybe_unused]] Serialization::IArchive& ar) override
    {
        YASLI_ESCAPE(ser.size() == sizeof(StringListValue), return );
        const StringListValue& stringListValue = *((StringListValue*)(ser.pointer()));
        stringList_ = stringListValue.stringList();
        value_ = stringListValue.c_str();
        handle_ = stringListValue.handle();
        type_ = stringListValue.type();
    }

    bool isLeaf() const override { return true; }
    bool isStatic() const override { return false; }
    int widgetSizeMin(const QPropertyTree* tree) const override
    {
        if (userWidgetToContent())
        {
            return widthCache_.getOrUpdate(tree, this, tree->_defaultRowHeight());
        }
        else
        {
            return 80;
        }
    }
    WidgetPlacement widgetPlacement() const override { return WIDGET_VALUE; }
    const void* searchHandle() const override { return handle_; }
    Serialization::TypeID typeId() const override { return type_; }

    void redraw(const PropertyDrawContext& context) override
    {
        if (multiValue())
        {
            context.drawEntry(L" ... ", false, true, 0);
        }
        else if (userReadOnly())
        {
            context.drawValueText(pulledSelected(), valueAsWString().c_str());
        }
        else
        {
            QStyleOptionComboBox option;
            option.editable = false;
            option.frame = true;
            option.currentText = QString(valueAsString().c_str());
            option.state |= QStyle::State_Enabled;
            option.rect = QRect(0, 0, context.widgetRect.width(), context.widgetRect.height());
            // we have to translate painter here to work around bug in some themes
            context.painter->translate(context.widgetRect.left(), context.widgetRect.top());

            // create a real instance of a combo so that it has the style sheet applied.
            QComboBox widgetForContext;
            context.tree->style()->drawComplexControl(QStyle::CC_ComboBox, &option, context.painter, &widgetForContext);
            context.painter->setPen(QPen(context.tree->palette().color(QPalette::WindowText)));
            QRect textRect = context.tree->style()->subControlRect(QStyle::CC_ComboBox, &option, QStyle::SC_ComboBoxEditField, &widgetForContext);

            textRect.adjust(1, 0, -1, 0);
            context.tree->_drawRowValue(*context.painter, valueAsWString().c_str(), &context.tree->font(), textRect, context.tree->palette().color(QPalette::WindowText), false, false);
            context.painter->translate(-context.widgetRect.left(), -context.widgetRect.top());
        }
    }


    void serializeValue(Serialization::IArchive& ar)
    {
        ar(value_, "value", "Value");
        ar(stringList_, "stringList", "String List");
    }
private:
    Serialization::StringList stringList_;
    string value_;
    const void* handle_;
    Serialization::TypeID type_;
    friend class PropertyRowWidgetStringListValue;
    mutable RowWidthCache widthCache_;
};

using Serialization::StringListStaticValue;
class PropertyRowStringListStaticValue
    : public PropertyRowImpl<StringListStaticValue>
{
public:
    PropertyRowStringListStaticValue()
        : handle_() {}
    PropertyRowWidget* createWidget(QPropertyTree* tree) override;
    string valueAsString() const override { return value_.c_str(); }
    bool assignTo(const Serialization::SStruct& ser) const override
    {
        *((StringListStaticValue*)ser.pointer()) = value_.c_str();
        return true;
    }
    void setValueAndContext(const Serialization::SStruct& ser, [[maybe_unused]] Serialization::IArchive& ar) override
    {
        YASLI_ESCAPE(ser.size() == sizeof(StringListStaticValue), return );
        const StringListStaticValue& stringListValue = *((StringListStaticValue*)(ser.pointer()));
        stringList_.resize(stringListValue.stringList().size());
        for (size_t i = 0; i < stringList_.size(); ++i)
        {
            stringList_[i] = stringListValue.stringList()[i];
        }
        value_ = stringListValue.c_str();
        handle_ = stringListValue.handle();
        type_ = stringListValue.type();
    }

    bool isLeaf() const override { return true; }
    bool isStatic() const override { return false; }
    int widgetSizeMin(const QPropertyTree* tree) const override
    {
        if (userWidgetToContent())
        {
            return widthCache_.getOrUpdate(tree, this, tree->_defaultRowHeight());
        }
        else
        {
            return 80;
        }
    }
    WidgetPlacement widgetPlacement() const override { return WIDGET_VALUE; }
    const void* searchHandle() const override { return handle_; }
    Serialization::TypeID typeId() const override { return type_; }
    void redraw(const PropertyDrawContext& context) override
    {
        if (multiValue())
        {
            context.drawEntry(L" ... ", false, true, 0);
        }
        else if (userReadOnly())
        {
            context.drawValueText(pulledSelected(), valueAsWString().c_str());
        }
        else
        {
            QStyleOptionComboBox option;
            option.currentText = QString(valueAsString().c_str());
            option.state |= QStyle::State_Enabled;
            option.rect = context.widgetRect;

            // create a real instance of a combo so that it has the style sheet applied.
            QComboBox widgetForContext;
            context.tree->style()->drawComplexControl(QStyle::CC_ComboBox, &option, context.painter, &widgetForContext);
            context.painter->setPen(QPen(context.tree->palette().color(QPalette::WindowText)));
            QRect textRect = context.tree->style()->subControlRect(QStyle::CC_ComboBox, &option, QStyle::SC_ComboBoxEditField, &widgetForContext);

            textRect.adjust(1, 0, -1, 0);
            context.tree->_drawRowValue(*context.painter, valueAsWString().c_str(), &context.tree->font(), textRect, context.tree->palette().color(QPalette::WindowText), false, false);
        }
    }


    void serializeValue(Serialization::IArchive& ar)
    {
        ar(value_, "value", "Value");
        ar(stringList_, "stringList", "String List");
    }
private:
    Serialization::StringList stringList_;
    string value_;
    const void* handle_;
    Serialization::TypeID type_;
    friend class PropertyRowWidgetStringListValue;
    mutable RowWidthCache widthCache_;
};


// ---------------------------------------------------------------------------


class PropertyRowWidgetStringListValue
    : public PropertyRowWidget
{
    Q_OBJECT
public:
    PropertyRowWidgetStringListValue(PropertyRowStringListValue* row, QPropertyTree* tree)
        : PropertyRowWidget(row, tree)
        , comboBox_(new QComboBox())
    {
        const Serialization::StringList& stringList = row->stringList_;
        for (size_t i = 0; i < stringList.size(); ++i)
        {
            comboBox_->addItem(stringList[i].c_str());
        }
        comboBox_->setCurrentIndex(stringList.find(row->value_.c_str()));
        connect(comboBox_, SIGNAL(activated(int)), this, SLOT(onChange(int)));
    }

    PropertyRowWidgetStringListValue(PropertyRowStringListStaticValue* row, QPropertyTree* tree)
        : PropertyRowWidget(row, tree)
        , comboBox_(new QComboBox())
    {
        const Serialization::StringList& stringList = row->stringList_;
        for (size_t i = 0; i < stringList.size(); ++i)
        {
            comboBox_->addItem(stringList[i].c_str());
        }
        comboBox_->setCurrentIndex(stringList.find(row->value_.c_str()));
        connect(comboBox_, SIGNAL(currentIndexChanged(int)), this, SLOT(onChange(int)));
    }

    void showPopup() override
    {
        // Here comboBox_->showPopup() should be sufficient, but sadly with Fusion
        // theme ComboBox, when clicked, it fires a mouseReleseTimer, which doesn't
        // happen with showPopup.  It is used to distinguish click-and-hold from
        // simple click. If timer is not fired following mouse release hides combo
        // box. That's why the user click is emulated here.
        QSize size = comboBox_->size();
        QPoint localPoint = QPoint(aznumeric_cast<int>(size.width() * 0.5f), aznumeric_cast<int>(size.height() * 0.5f));
        QMouseEvent ev(QMouseEvent::MouseButtonPress, localPoint, comboBox_->mapToGlobal(localPoint), Qt::LeftButton, Qt::LeftButton, Qt::KeyboardModifiers());
        QApplication::sendEvent(comboBox_, &ev);
    }

    ~PropertyRowWidgetStringListValue()
    {
        comboBox_->hide();
        comboBox_->setParent(0);
        comboBox_->deleteLater();
        comboBox_ = 0;
    }


    void commit(){}
    QWidget* actualWidget() { return comboBox_; }
public slots:
    void onChange(int)
    {
        if (strcmp(this->row()->typeName(), Serialization::TypeID::get<StringListValue>().name()) == 0)
        {
            PropertyRowStringListValue* r = static_cast<PropertyRowStringListValue*>(this->row());
            QByteArray newValue = comboBox_->currentText().toUtf8();
            if (r->value_ != newValue.data())
            {
                model()->rowAboutToBeChanged(r);
                r->value_ = newValue.data();
                model()->rowChanged(r);
            }
            else
            {
                tree_->_cancelWidget();
            }
        }
        else if (strcmp(this->row()->typeName(), Serialization::TypeID::get<StringListStaticValue>().name()) == 0)
        {
            PropertyRowStringListStaticValue* r = static_cast<PropertyRowStringListStaticValue*>(this->row());
            QByteArray newValue = comboBox_->currentText().toUtf8();
            if (r->value_ != newValue.data())
            {
                model()->rowAboutToBeChanged(r);
                r->value_ = newValue.data();
                model()->rowChanged(r);
            }
            else
            {
                tree_->_cancelWidget();
            }
        }
    }
protected:
    QComboBox* comboBox_;
};


#endif // CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYROWSTRINGLISTVALUE_H
