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
