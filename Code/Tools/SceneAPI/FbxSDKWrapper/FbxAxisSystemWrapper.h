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

#pragma once

#include <fbxsdk.h>
#include <SceneAPI/SceneCore/DataTypes/MatrixType.h>

namespace AZ
{
    namespace FbxSDKWrapper
    {
        class FbxAxisSystemWrapper
        {
        public:
            enum UpVector
            {
                X,
                Y,
                Z,
                Unknown
            };

            FbxAxisSystemWrapper() = default;
            FbxAxisSystemWrapper(const FbxAxisSystem& fbxAxisSystem);
            virtual ~FbxAxisSystemWrapper() = default;

            virtual UpVector GetUpVector(int& sign) const;
            virtual SceneAPI::DataTypes::MatrixType CalculateConversionTransform(UpVector targetUpAxis);

        protected:
            FbxAxisSystem m_fbxAxisSystem;
        };
    } // namespace FbxSDKWrapper
} // namespace AZ
