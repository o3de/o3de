/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/Mocks/DataTypes/MockIGraphObject.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            class FiltersTests
                : public ::testing::Test
            {
            protected:
                void SetUp() override
                {
                    m_entries.emplace_back(AZStd::make_shared<DataTypes::MockIGraphObject>(1));
                    m_entries.emplace_back(AZStd::make_shared<DataTypes::MockIGraphObjectAlt>(2));
                    m_entries.emplace_back(AZStd::make_shared<DataTypes::MockIGraphObject>(3));

                    m_constEntries.emplace_back(AZStd::make_shared<const DataTypes::MockIGraphObject>(1));
                    m_constEntries.emplace_back(AZStd::make_shared<const DataTypes::MockIGraphObjectAlt>(2));
                    m_constEntries.emplace_back(AZStd::make_shared<const DataTypes::MockIGraphObject>(3));
                }

                AZStd::vector<AZStd::shared_ptr<DataTypes::IGraphObject>> m_entries;
                AZStd::vector<AZStd::shared_ptr<const DataTypes::IGraphObject>> m_constEntries;
            };

            TEST_F(FiltersTests, MakeDerivedFilterView_FilterTypes_ListsEntryOneAndThree)
            {
                auto view = Containers::MakeDerivedFilterView<DataTypes::MockIGraphObject>(m_entries);
                auto it = view.begin();

                EXPECT_EQ(1, it->m_id);
                EXPECT_EQ(DataTypes::MockIGraphObject::TYPEINFO_Uuid(), it->RTTI_GetType());
                ++it;
                EXPECT_EQ(3, it->m_id);
                EXPECT_EQ(DataTypes::MockIGraphObject::TYPEINFO_Uuid(), it->RTTI_GetType());
                ++it;
                EXPECT_EQ(view.end(), it);
            }

            TEST_F(FiltersTests, MakeDerivedFilterView_FilterConstTypes_ListsEntryOneAndThree)
            {
                auto view = Containers::MakeDerivedFilterView<DataTypes::MockIGraphObject>(m_constEntries);
                auto it = view.begin();

                EXPECT_EQ(1, it->m_id);
                EXPECT_EQ(DataTypes::MockIGraphObject::TYPEINFO_Uuid(), it->RTTI_GetType());
                ++it;
                EXPECT_EQ(3, it->m_id);
                EXPECT_EQ(DataTypes::MockIGraphObject::TYPEINFO_Uuid(), it->RTTI_GetType());
                ++it;
                EXPECT_EQ(view.end(), it);
            }

            TEST_F(FiltersTests, MakeDerivedFilterView_ConstFilterTypes_ListsEntryOneAndThree)
            {
                const AZStd::vector<AZStd::shared_ptr<DataTypes::IGraphObject>>& constEntries = m_entries;
                auto view = Containers::MakeDerivedFilterView<DataTypes::MockIGraphObject>(constEntries);
                auto it = view.begin();

                EXPECT_EQ(1, it->m_id);
                EXPECT_EQ(DataTypes::MockIGraphObject::TYPEINFO_Uuid(), it->RTTI_GetType());
                ++it;
                EXPECT_EQ(3, it->m_id);
                EXPECT_EQ(DataTypes::MockIGraphObject::TYPEINFO_Uuid(), it->RTTI_GetType());
                ++it;
                EXPECT_EQ(view.end(), it);
            }

            TEST_F(FiltersTests, MakeDerivedFilterView_ConstFilterConstTypes_ListsEntryOneAndThree)
            {
                const AZStd::vector<AZStd::shared_ptr<const DataTypes::IGraphObject>>& constEntries = m_constEntries;
                auto view = Containers::MakeDerivedFilterView<DataTypes::MockIGraphObject>(constEntries);
                auto it = view.begin();

                EXPECT_EQ(1, it->m_id);
                EXPECT_EQ(DataTypes::MockIGraphObject::TYPEINFO_Uuid(), it->RTTI_GetType());
                ++it;
                EXPECT_EQ(3, it->m_id);
                EXPECT_EQ(DataTypes::MockIGraphObject::TYPEINFO_Uuid(), it->RTTI_GetType());
                ++it;
                EXPECT_EQ(view.end(), it);
            }

            TEST_F(FiltersTests, MakeDerivedFilterView_ReferenceRetrievedFromIteratorAllowsChangingValueInOriginal_ValueChangedFromOneToTen)
            {
                auto view = Containers::MakeDerivedFilterView<DataTypes::MockIGraphObject>(m_entries);
                view.begin()->m_id = 10;

                EXPECT_EQ(10, azrtti_cast<DataTypes::MockIGraphObject*>(m_entries[0])->m_id);
            }

            TEST_F(FiltersTests, MakeExactFilterView_FilterTypes_ListsEntryOneAndThree)
            {
                auto view = Containers::MakeExactFilterView<DataTypes::MockIGraphObject>(m_entries);
                auto it = view.begin();

                EXPECT_EQ(1, it->m_id);
                EXPECT_EQ(DataTypes::MockIGraphObject::TYPEINFO_Uuid(), it->RTTI_GetType());
                ++it;
                EXPECT_EQ(3, it->m_id);
                EXPECT_EQ(DataTypes::MockIGraphObject::TYPEINFO_Uuid(), it->RTTI_GetType());
                ++it;
                EXPECT_EQ(view.end(), it);
            }

            TEST_F(FiltersTests, MakeExactFilterView_FilterConstTypes_ListsEntryOneAndThree)
            {
                auto view = Containers::MakeExactFilterView<DataTypes::MockIGraphObject>(m_constEntries);
                auto it = view.begin();

                EXPECT_EQ(1, it->m_id);
                EXPECT_EQ(DataTypes::MockIGraphObject::TYPEINFO_Uuid(), it->RTTI_GetType());
                ++it;
                EXPECT_EQ(3, it->m_id);
                EXPECT_EQ(DataTypes::MockIGraphObject::TYPEINFO_Uuid(), it->RTTI_GetType());
                ++it;
                EXPECT_EQ(view.end(), it);
            }

            TEST_F(FiltersTests, MakeExactFilterView_ConstFilterTypes_ListsEntryOneAndThree)
            {
                const AZStd::vector<AZStd::shared_ptr<DataTypes::IGraphObject>>& constEntries = m_entries;
                auto view = Containers::MakeExactFilterView<DataTypes::MockIGraphObject>(constEntries);
                auto it = view.begin();

                EXPECT_EQ(1, it->m_id);
                EXPECT_EQ(DataTypes::MockIGraphObject::TYPEINFO_Uuid(), it->RTTI_GetType());
                ++it;
                EXPECT_EQ(3, it->m_id);
                EXPECT_EQ(DataTypes::MockIGraphObject::TYPEINFO_Uuid(), it->RTTI_GetType());
                ++it;
                EXPECT_EQ(view.end(), it);
            }

            TEST_F(FiltersTests, MakeExactFilterView_ConstFilterConstTypes_ListsEntryOneAndThree)
            {
                const AZStd::vector<AZStd::shared_ptr<const DataTypes::IGraphObject>>& constEntries = m_constEntries;
                auto view = Containers::MakeExactFilterView<DataTypes::MockIGraphObject>(constEntries);
                auto it = view.begin();

                EXPECT_EQ(1, it->m_id);
                EXPECT_EQ(DataTypes::MockIGraphObject::TYPEINFO_Uuid(), it->RTTI_GetType());
                ++it;
                EXPECT_EQ(3, it->m_id);
                EXPECT_EQ(DataTypes::MockIGraphObject::TYPEINFO_Uuid(), it->RTTI_GetType());
                ++it;
                EXPECT_EQ(view.end(), it);
            }

            TEST_F(FiltersTests, MakeExactFilterView_ReferenceRetrievedFromIteratorAllowsChangingValueInOriginal_ValueChangedFromOneToTen)
            {
                auto view = Containers::MakeExactFilterView<DataTypes::MockIGraphObject>(m_entries);
                view.begin()->m_id = 10;

                EXPECT_EQ(10, azrtti_cast<DataTypes::MockIGraphObject*>(m_entries[0])->m_id);
            }
        } // Containers
    } // SceneAPI
} // AZ
