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

#include "EditorCommon_precompiled.h"
#include "PropertyRowTagList.h"
#include "PropertyRowString.h"
#include "QPropertyTree.h"
#include "Serialization/Decorators/TagList.h"
#include "Serialization/ClassFactory.h"
#include "Serialization/IArchive.h"
#include "Serialization/Decorators/TagListImpl.h"
#include <QMenu>

PropertyRowTagList::PropertyRowTagList()
    : source_(0)
{
}

PropertyRowTagList::~PropertyRowTagList()
{
    if (source_)
    {
        source_->Release();
    }
}

void PropertyRowTagList::generateMenu(QMenu& item, QPropertyTree* tree, [[maybe_unused]] bool addActions)
{
    if (userReadOnly() || isFixedSize())
    {
        return;
    }

    if (!source_)
    {
        return;
    }

    TagListMenuHandler* handler = new TagListMenuHandler();
    handler->tree = tree;
    handler->row = this;
    tree->addMenuHandler(handler);

    unsigned int numGroups = source_->GroupCount();
    for (unsigned int group = 0; group < numGroups; ++group)
    {
        unsigned int tagCount = source_->TagCount(group);
        if (tagCount == 0)
        {
            continue;
        }
        const char* groupName = source_->GroupName(group);
        QString title = QString("From ") + groupName;
        QMenu* menu = item.addMenu(title);
        for (unsigned int tagIndex = 0; tagIndex < tagCount; ++tagIndex)
        {
            QString str;
            str = source_->TagValue(group, tagIndex);
            const char* desc = source_->TagDescription(group, tagIndex);
            if (desc && desc[0] != '\0')
            {
                str += "\t";
                str += desc;
            }
            QAction* action = menu->addAction(str);
            QString tag = QString::fromLocal8Bit(source_->TagValue(group, tagIndex));
            action->setData(QVariant(tag));
            QObject::connect(action, SIGNAL(triggered()), handler, SLOT(onMenuAddTag()));
        }
    }


    QAction* action = item.addAction("Add");
    action->setData(QVariant(QString()));
    QObject::connect(action, SIGNAL(triggered()), handler, SLOT(onMenuAddTag()));

    PropertyRowContainer::generateMenu(item, tree, false);
}

void PropertyRowTagList::addTag(const char* tag, QPropertyTree* tree)
{
    Serialization::SharedPtr<PropertyRowTagList> ref(this);

    PropertyRow* child = addElement(tree, false);
    if (child && strcmp(child->typeName(), "string") == 0)
    {
        PropertyRowString* stringRow = static_cast<PropertyRowString*>(child);
        tree->model()->rowAboutToBeChanged(stringRow);
        stringRow->setValue(tag, stringRow->searchHandle(), stringRow->typeId());
        tree->model()->rowChanged(stringRow);
    }
}

void TagListMenuHandler::onMenuAddTag()
{
    if (QAction* action = qobject_cast<QAction*>(sender()))
    {
        QString str = action->data().toString();
        row->addTag(str.toLocal8Bit().data(), tree);
    }
}

void PropertyRowTagList::setValueAndContext(const Serialization::IContainer& value, Serialization::IArchive& ar)
{
    if (source_)
    {
        source_->Release();
    }
    source_ = ar.FindContext<ITagSource>();
    if (source_)
    {
        source_->AddRef();
    }

    PropertyRowContainer::setValueAndContext(value, ar);
}


REGISTER_PROPERTY_ROW(TagList, PropertyRowTagList)
DECLARE_SEGMENT(PropertyRowTagList)

#include <QPropertyTree/moc_PropertyRowTagList.cpp>
