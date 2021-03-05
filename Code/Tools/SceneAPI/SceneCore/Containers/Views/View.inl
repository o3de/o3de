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
                template<typename Iterator>
                View<Iterator>::View(Iterator begin, Iterator end)
                    : m_begin(begin)
                    , m_end(end)
                {
                }

                template<typename Iterator>
                Iterator View<Iterator>::begin()
                {
                    return m_begin;
                }

                template<typename Iterator>
                Iterator View<Iterator>::end()
                {
                    return m_end;
                }

                template<typename Iterator>
                Iterator View<Iterator>::begin() const
                {
                    return m_begin;
                }

                template<typename Iterator>
                Iterator View<Iterator>::end() const
                {
                    return m_end;
                }

                template<typename Iterator>
                Iterator View<Iterator>::cbegin() const
                {
                    return m_begin;
                }

                template<typename Iterator>
                Iterator View<Iterator>::cend() const
                {
                    return m_end;
                }
            }; // Views
        } // Containers
    } // SceneAPI
} // AZ
