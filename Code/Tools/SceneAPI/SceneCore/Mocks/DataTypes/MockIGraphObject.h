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

#include <AzTest/AzTest.h>
#include <SceneAPI/SceneCore/DataTypes/IGraphObject.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class MockIGraphObject
                : public DataTypes::IGraphObject
            {
            public:
                AZ_RTTI(MockIGraphObject, "{66A082CC-851D-4E1F-ABBD-45B58A216CFA}", DataTypes::IGraphObject);

                MockIGraphObject()
                    : m_id(0)
                {}

                explicit MockIGraphObject(int id)
                    : m_id(id)
                {}
                ~MockIGraphObject() override = default;

                int m_id;
            };

            class MockIGraphObjectAlt
                : public DataTypes::IGraphObject
            {
            public:
                AZ_RTTI(MockIGraphObjectAlt, "{9DABB454-C60F-4718-8325-E8A95B15DAF5}", DataTypes::IGraphObject);

                MockIGraphObjectAlt()
                    : m_id(0)
                {}

                explicit MockIGraphObjectAlt(int id)
                    : m_id(id)
                {}
                ~MockIGraphObjectAlt() override = default;

                int m_id;
            };

        }  // namespace DataTypes
    }  //namespace SceneAPI
}  // namespace AZ
