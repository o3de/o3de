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

#ifndef CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYROWTAGLIST_H
#define CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYROWTAGLIST_H
#pragma once

#if !defined(Q_MOC_RUN)
#include "PropertyRowContainer.h"
#endif

struct ITagSource;

class PropertyRowTagList
    : public PropertyRowContainer
{
public:
    PropertyRowTagList();
    ~PropertyRowTagList();
    void setValueAndContext(const Serialization::IContainer& value, Serialization::IArchive& ar) override;
    void generateMenu(QMenu& item, QPropertyTree* tree, bool addActions) override;
    void addTag(const char* tag, QPropertyTree* tree);

private:
    using PropertyRow::setValueAndContext;
    ITagSource* source_;
};

struct TagListMenuHandler
    : public PropertyRowMenuHandler
{
    Q_OBJECT
public:

    PropertyRowTagList * row;
    QPropertyTree* tree;
public slots:
    void onMenuAddTag();
};

#endif // CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYROWTAGLIST_H
