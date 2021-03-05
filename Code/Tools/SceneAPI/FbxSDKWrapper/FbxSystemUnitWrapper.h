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

namespace AZ
{
    namespace FbxSDKWrapper
    {
        class FbxSystemUnitWrapper
        {
        public:
            enum Unit
            {
                mm,
                dm,
                cm,
                m,
                km,
                inch,
                foot,
                mile,
                yard,
                unknown
            };

            FbxSystemUnitWrapper() = default;
            FbxSystemUnitWrapper(const FbxSystemUnit& fbxSystemUnit);
            virtual ~FbxSystemUnitWrapper() = default;

            virtual Unit GetUnit() const;
            virtual float GetConversionFactorTo(Unit to);

        protected:
            FbxSystemUnit m_fbxSystemUnit;
        };
    } // namespace FbxSDKWrapper
} // namespace AZ
