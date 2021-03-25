#pragma once

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

namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            namespace Views
            {
                // View combines begin and end iterators together in a single object.
                //      This reduces the number of functions that are needed to pass
                //      iterators from functions and avoids problems with mismatched
                //      iterators. It also makes it easier to use in ranged-based
                //      for-loops.
                //      Note that const correctness is enforced by the type of
                //      iterator passed, so all versions of begin and end return
                //      the same value.
                template<typename Iterator>
                class View
                {
                public:
                    using iterator = Iterator;
                    using const_iterator = Iterator;

                    View(Iterator begin, Iterator end);

                    Iterator begin();
                    Iterator end();
                    Iterator begin() const;
                    Iterator end() const;
                    Iterator cbegin() const;
                    Iterator cend() const;

                    [[nodiscard]] bool empty() const;

                private:
                    Iterator m_begin;
                    Iterator m_end;
                };

                template<typename Iterator>
                View<Iterator> MakeView(Iterator begin, Iterator end)
                {
                    return View<Iterator>(begin, end);
                }
            } // Views
        } // Containers
    } // SceneAPI
} // AZ

#include <SceneAPI/SceneCore/Containers/Views/View.inl>
