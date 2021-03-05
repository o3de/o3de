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

#ifndef CRYINCLUDE_EDITOR_LENSFLAREEDITOR_LENSFLAREITEMTREE_H
#define CRYINCLUDE_EDITOR_LENSFLAREEDITOR_LENSFLAREITEMTREE_H
#pragma once

#if !defined(Q_MOC_RUN)
#include "LensFlareElement.h"
#include "ILensFlareListener.h"

#include <QTreeView>
#endif

class QMouseEvent;
class CLensFlareItem;

class CLensFlareItemTree
    : public QTreeView
{
    Q_OBJECT

public:
    CLensFlareItemTree(QWidget* pParent = nullptr);
    ~CLensFlareItemTree();

protected:
    void startDrag(Qt::DropActions supportedDropActions) override;
    void mousePressEvent(QMouseEvent* event) override;

private:

    void AssignLensFlareToLightEntity(CViewport* pViewport) const;
};
#endif // CRYINCLUDE_EDITOR_LENSFLAREEDITOR_LENSFLAREITEMTREE_H
