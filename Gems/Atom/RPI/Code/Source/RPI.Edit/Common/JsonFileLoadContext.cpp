/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
*or, if provided, by the license below or the license accompanying this file.Do not
* remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
*WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
