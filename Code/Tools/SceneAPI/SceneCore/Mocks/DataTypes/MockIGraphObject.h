#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

                void CloneAttributesFrom([[maybe_unused]] const IGraphObject* sourceObject) override {}

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

                void CloneAttributesFrom([[maybe_unused]] const IGraphObject* sourceObject) override {}

                int m_id;
            };

        }  // namespace DataTypes
    }  //namespace SceneAPI
}  // namespace AZ
