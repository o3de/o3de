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

#ifndef CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_CONTEXTLIST_H
#define CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_CONTEXTLIST_H
#pragma once

#include <Serialization/IArchive.h>

namespace Serialization
{
    class CContextList
    {
    public:
        template<class T>
        void Update(T* contextObject)
        {
            for (size_t i = 0; i < links_.size(); ++i)
            {
                if (links_[i]->type == TypeID::get<T>())
                {
                    links_[i]->contextObject = (void*)contextObject;
                    return;
                }
            }

            SContextLink* newLink = new SContextLink;
            newLink->type = TypeID::get<T>();
            newLink->outer = links_.empty() ? connectedList_ : links_.back();
            newLink->contextObject = (void*)contextObject;
            tail_.outer = newLink;
            links_.push_back(newLink);
        }

        CContextList()
        {
            tail_.outer = 0;
            tail_.contextObject = 0;
            connectedList_ = 0;
        }

        explicit CContextList(SContextLink* connectedList)
        {
            tail_.outer = 0;
            tail_.contextObject = 0;
            connectedList_ = connectedList;
        }

        ~CContextList()
        {
            for (size_t i = 0; i < links_.size(); ++i)
            {
                delete links_[i];
            }
            links_.clear();
        }

        SContextLink* Tail() { return &tail_; }
    private:
        SContextLink tail_;
        std::vector<SContextLink*> links_;
        SContextLink* connectedList_;
    };
}

#endif // CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_CONTEXTLIST_H
