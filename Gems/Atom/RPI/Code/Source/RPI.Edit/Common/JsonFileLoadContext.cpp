/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Common/JsonFileLoadContext.h>

namespace AZ
{
    namespace RPI
    {
        // Note, we use string not string_view on purpose because m_thisFilePath.push_back() will make a new string anyway.
        void JsonFileLoadContext::PushFilePath(AZStd::string path)
        {
            m_thisFilePath.push_back(AZStd::move(path));
        }

        AZStd::string_view JsonFileLoadContext::GetFilePath() const
        {
            if (m_thisFilePath.empty())
            {
                return "";
            }
            else
            {
                return m_thisFilePath.back();
            }
        }

        void JsonFileLoadContext::PopFilePath()
        {
            m_thisFilePath.pop_back();
        }

    } // namespace RPI
} // namespace AZ
