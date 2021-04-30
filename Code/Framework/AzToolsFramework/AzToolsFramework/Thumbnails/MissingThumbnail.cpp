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

#include <AzCore/Utils/Utils.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzToolsFramework/Thumbnails/MissingThumbnail.h>

namespace AzToolsFramework
{
    namespace Thumbnailer
    {
        static constexpr const char* MissingIconPath = "Assets/Editor/Icons/AssetBrowser/Default_16.svg";

        MissingThumbnail::MissingThumbnail()
            : Thumbnail(MAKE_TKEY(ThumbnailKey))
        {
            AZStd::string absoluteIconPath;
            AZ::StringFunc::Path::Join(AZ::Utils::GetEnginePath().c_str(), MissingIconPath, absoluteIconPath);

            m_pixmap.load(absoluteIconPath.c_str());
            m_state = m_pixmap.isNull() ? State::Failed : State::Ready;
        }

    } // namespace Thumbnailer
} // namespace AzToolsFramework

#include "Thumbnails/moc_MissingThumbnail.cpp"
