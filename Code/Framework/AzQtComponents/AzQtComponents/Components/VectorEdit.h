/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/AzQtComponentsAPI.h>

#include <QWidget>
AZ_PUSH_DISABLE_WARNING(4244, "-Wunknown-warning-option") // 4251: conversion from 'int' to 'float', possible loss of data
#include <QVector3D>
AZ_POP_DISABLE_WARNING
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QStringList>
#include <QColor>
#endif

class QPixmap;

namespace AzQtComponents
{
    class AZ_QT_COMPONENTS_API VectorEditElement
        : public QWidget
    {
        Q_OBJECT

    public:
        // TODO: maybe share them somewhere (in a gadget)
        enum Flavor
        {
            Plain = 0,
            Information,
            Question,
            Invalid,
            Valid
        };
        Q_ENUM(Flavor)
        explicit VectorEditElement(const QString& label, QWidget* parent = nullptr);

        float value() const;
        void setValue(float);
        QString label() const;
        void setLabel(const QString&);
        QColor color() const;
        void setColor(const QColor&);
        void setFlavor(VectorEditElement::Flavor flavor);

    Q_SIGNALS:
        void valueChanged();

    private:
        void updateColor();
        QLineEdit* const m_lineEdit;
        QLabel* const m_label;
        QColor m_color;
        Flavor m_flavor;
    };

    class AZ_QT_COMPONENTS_API VectorEdit
        : public QWidget
    {
        Q_OBJECT
        Q_PROPERTY(QVector3D vector READ vector WRITE setVector NOTIFY vectorChanged)
        Q_PROPERTY(AzQtComponents::VectorEditElement::Flavor flavor READ flavor WRITE setFlavor NOTIFY flavorChanged)

    public:
        explicit VectorEdit(QWidget* parent = 0);

        QVector3D vector() const;
        float x() const;
        float y() const;
        float z() const;

        QString xLabel() const;
        QString yLabel() const;
        QString zLabel() const;
        void setLabels(const QString& xLabel, const QString& yLabel, const QString& zLabel);
        void setLabels(const QStringList& labels);

        QColor xColor() const;
        QColor yColor() const;
        QColor zColor() const;
        void setColors(const QColor& xColor, const QColor& yColor, const QColor& zColor);
        void setXColor(const QColor& color);
        void setYColor(const QColor& color);
        void setZColor(const QColor& color);

        VectorEditElement::Flavor flavor() const;
        void setFlavor(VectorEditElement::Flavor flavor);
        void setPixmap(const QPixmap&);

    public Q_SLOTS:
        void setVector(QVector3D vec);
        void setVector(float xValue, float yValue, float zValue);
        void setX(float xValue);
        void setY(float yValue);
        void setZ(float zValue);

    Q_SIGNALS:
        void vectorChanged();
        void flavorChanged();

    private:
        AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: 'AzQtComponents::VectorEdit::m_editElements': class 'QList<AzQtComponents::VectorEditElement *>' needs to have dll-interface to be used by clients of class 'AzQtComponents::VectorEdit'
        QList<VectorEditElement*> m_editElements;
        AZ_POP_DISABLE_WARNING
        VectorEditElement::Flavor m_flavor;
        QLabel* const m_iconLabel;
    };
} // namespace AzQtComponents


