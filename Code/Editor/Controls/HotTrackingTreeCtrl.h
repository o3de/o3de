/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_CONTROLS_HOTTRACKINGTREECTRL_H
#define CRYINCLUDE_EDITOR_CONTROLS_HOTTRACKINGTREECTRL_H
#pragma once

#if !defined(Q_MOC_RUN)
#include <QTreeWidget>
#endif

class CHotTrackingTreeCtrl
    : public QTreeWidget
{
    Q_OBJECT

public:
    CHotTrackingTreeCtrl(QWidget* parent = 0);
    virtual ~CHotTrackingTreeCtrl(){};

protected:
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    QTreeWidgetItem* m_hHoverItem;
};
#endif // CRYINCLUDE_EDITOR_CONTROLS_HOTTRACKINGTREECTRL_H
