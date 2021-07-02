#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

                template<typename Iterator>
                [[nodiscard]] bool View<Iterator>::empty() const
                {
                    return m_begin == m_end;
                }
            }; // Views
        } // Containers
    } // SceneAPI
} // AZ
