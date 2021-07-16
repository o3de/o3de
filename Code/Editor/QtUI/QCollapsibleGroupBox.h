/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#ifndef CRYINCLUDE_EDITORCOMMON_QCOLLAPSIBLEGROUPBOX_H
#define CRYINCLUDE_EDITORCOMMON_QCOLLAPSIBLEGROUPBOX_H

#if !defined(Q_MOC_RUN)
#include <QGroupBox>
#include <QToolButton>
#include <QHash>
#endif


class QCollapsibleGroupBox
    : public QGroupBox
{
    Q_OBJECT
    Q_PROPERTY(bool collapsed READ collapsed WRITE setCollapsed NOTIFY collapsed)
public:
    QCollapsibleGroupBox(QWidget* parent);
    ~QCollapsibleGroupBox();

    bool collapsed() const { return m_collapsed; }

public slots:
    void setCollapsed(bool v);

signals:
    void collapsed(bool);

protected:
    void adaptSize(bool v);
    void resizeEvent(QResizeEvent* event) Q_DECL_OVERRIDE;

    QSize m_expandedSize;
    bool    m_collapsed;
    QToolButton* m_toggleButton;
    QHash<QWidget*, bool> m_visibleState;
};

#endif
