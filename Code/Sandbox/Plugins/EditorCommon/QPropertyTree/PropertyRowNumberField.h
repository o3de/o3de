// Modifications copyright Amazon.com, Inc. or its affiliates.   
/**
 *  wWidgets - Lightweight UI Toolkit.
 *  Copyright (C) 2009-2011 Evgeny Andreeshchev <eugene.andreeshchev@gmail.com>
 *                          Alexander Kotliar <alexander.kotliar@gmail.com>
 * 
 *  This code is distributed under the MIT License:
 *                          http://www.opensource.org/licenses/MIT
 */
#ifndef CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYROWNUMBERFIELD_H
#define CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYROWNUMBERFIELD_H
#pragma once

#if !defined(Q_MOC_RUN)
#include "PropertyRow.h"
#include <QLineEdit>
#endif

class PropertyRowNumberField;
class PropertyRowWidgetNumber : public PropertyRowWidget
{
    Q_OBJECT
public:
    PropertyRowWidgetNumber(PropertyTreeModel* mode, PropertyRowNumberField* numberField, QPropertyTree* tree);
    ~PropertyRowWidgetNumber(){
        if (entry_)
            entry_->setParent(0);
        entry_->deleteLater();
        entry_ = 0;
    }

    void commit();
    QWidget* actualWidget() { return entry_; }
public slots:
    void onEditingFinished();
protected:
    QLineEdit* entry_;
    PropertyRowNumberField* row_;
    QPropertyTree* tree_;
};

// ---------------------------------------------------------------------------

class PropertyRowNumberField : public PropertyRow
{
public:
    PropertyRowNumberField();
    WidgetPlacement widgetPlacement() const override{ return WIDGET_VALUE; }
    int widgetSizeMin(const QPropertyTree* tree) const override;

    PropertyRowWidget* createWidget(QPropertyTree* tree) override;
    bool isLeaf() const override{ return true; }
    bool isStatic() const override{ return false; }
    bool inlineInShortArrays() const override{ return true; }
    void redraw(const PropertyDrawContext& context) override;
    bool onActivate(const PropertyActivationEvent& e) override;
    bool onMouseDown(QPropertyTree* tree, QPoint point, bool& changed) override;
    void onMouseUp(QPropertyTree* tree, QPoint point) override;
    void onMouseDrag(const PropertyDragEvent& e) override;
    void onMouseStill(const PropertyDragEvent& e) override;
    bool getHoverInfo(PropertyHoverInfo* hit, const QPoint& cursorPos, const QPropertyTree* tree) const;

    virtual void startIncrement() = 0;
    virtual void endIncrement(QPropertyTree* tree) = 0;
    virtual void incrementLog(float screenFraction, float valueFieldFraction) = 0;
    virtual bool setValueFromString(const char* str) = 0;
    virtual double sliderPosition() const = 0;

    mutable RowWidthCache widthCache_;
    bool pressed_ : 1;
    bool dragStarted_ : 1;
};


#endif // CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYROWNUMBERFIELD_H
