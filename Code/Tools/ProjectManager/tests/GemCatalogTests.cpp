/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/Utils.h>

#include <GemCatalog/GemSortFilterProxyModel.h>

namespace O3DE::ProjectManager
{
    class GemCatalogTests 
        : public ::UnitTest::LeakDetectionFixture
    {
    public:
        void SetUp() override
        {
            m_gemModel.reset(new GemModel());
        }

        void TearDown() override
        {
            m_gemModel.release();
        }

    protected:
        AZStd::unique_ptr<GemModel> m_gemModel;
    };

    TEST_F(GemCatalogTests, GemCatalog_GemWithDependencies_DisplaysButDoesNotAddDependencies)
    {
        // given 3 gems a,b,c where a depends on b which depends on c
        GemInfo gemA, gemB, gemC;
        QModelIndex indexA, indexB, indexC;
        gemA.m_name = "a";
        gemB.m_name = "b";
        gemC.m_name = "c";

        gemA.m_dependencies = QStringList({ "b" });
        gemB.m_dependencies = QStringList({ "c" });

        indexA = m_gemModel->AddGem(gemA);
        indexB = m_gemModel->AddGem(gemB);
        indexC = m_gemModel->AddGem(gemC);

        m_gemModel->UpdateGemDependencies();

        EXPECT_FALSE(GemModel::IsAdded(indexA));
        EXPECT_FALSE(GemModel::IsAddedDependency(indexB) || GemModel::IsAddedDependency(indexC));

        // when a is added
        GemModel::SetIsAdded(*m_gemModel, indexA, true);

        // expect b and c are now dependencies of an added gem but not themselves added
        // cmake will handle dependencies
        EXPECT_TRUE(GemModel::IsAddedDependency(indexB) && GemModel::IsAddedDependency(indexC));
        EXPECT_FALSE(GemModel::IsAdded(indexB) || GemModel::IsAdded(indexC));

        const QVector<QModelIndex>& gemsToAdd = m_gemModel->GatherGemsToBeAdded();
        EXPECT_TRUE(gemsToAdd.size() == 1);
        EXPECT_EQ(GemModel::GetName(gemsToAdd.at(0)), gemA.m_name);
    }

    class GemCatalogFilterTests
        : public GemCatalogTests 
    {
    public:
        void SetUp() override
        {
            GemCatalogTests::SetUp(); 
            m_proxyModel.reset(new GemSortFilterProxyModel(m_gemModel.get()));
        }

        void TearDown() override
        {
            m_proxyModel.release();
            GemCatalogTests::TearDown(); 
        }

    protected:
        AZStd::unique_ptr<GemSortFilterProxyModel> m_proxyModel;
    };

    class GemCatalogSearchFilterTests
        : public GemCatalogFilterTests
    {
    public:
        void SetUp() override
        {
            GemCatalogFilterTests::SetUp();

            GemInfo gemfilterName, gemfilterDisplayName, gemfilterCreator, gemfilterSummary, gemfilterFeature;

            gemfilterName.m_name = "Name";
            gemfilterDisplayName.m_name = "D";
            gemfilterCreator.m_name = "C";
            gemfilterSummary.m_name = "S";
            gemfilterFeature.m_name = "F";

            gemfilterDisplayName.m_displayName = "Display Name";
            gemfilterCreator.m_origin = "Johnathon Doe";
            gemfilterSummary.m_summary = "Unique Summary";
            gemfilterFeature.m_features.append("Creative Feature");

            m_gemRows.append(m_gemModel->AddGem(gemfilterName).row());
            m_gemRows.append(m_gemModel->AddGem(gemfilterDisplayName).row());
            m_gemRows.append(m_gemModel->AddGem(gemfilterCreator).row());
            m_gemRows.append(m_gemModel->AddGem(gemfilterSummary).row());
            m_gemRows.append(m_gemModel->AddGem(gemfilterFeature).row());
        }

    protected:
        enum RowOrder
        {
            Name,
            DisplayName,
            Creator,
            Summary,
            Features
        };

        QVector<int> m_gemRows;
    };

    TEST_F(GemCatalogSearchFilterTests, GemCatalogFilters_SearchStringName_ShowsNameGems)
    {
        m_proxyModel->SetSearchString("Name");

        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemRows[Name], QModelIndex()));
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemRows[DisplayName], QModelIndex()));
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[Creator], QModelIndex()));
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[Summary], QModelIndex()));
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[Features], QModelIndex()));
    }

    TEST_F(GemCatalogSearchFilterTests, GemCatalogFilters_SearchStringDisplayName_ShowsDisplayNameGem)
    {
        m_proxyModel->SetSearchString("Display Name");

        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[Name], QModelIndex()));
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemRows[DisplayName], QModelIndex()));
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[Creator], QModelIndex()));
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[Summary], QModelIndex()));
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[Features], QModelIndex()));
    }

    TEST_F(GemCatalogSearchFilterTests, GemCatalogFilters_SearchStringCreator_ShowsCreatorGem)
    {
        m_proxyModel->SetSearchString("Johnathon Doe");

        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[Name], QModelIndex()));
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[DisplayName], QModelIndex()));
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemRows[Creator], QModelIndex()));
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[Summary], QModelIndex()));
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[Features], QModelIndex()));
    }

    TEST_F(GemCatalogSearchFilterTests, GemCatalogFilters_SearchStringSummary_ShowsSummaryGem)
    {
        m_proxyModel->SetSearchString("Unique Summary");

        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[Name], QModelIndex()));
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[DisplayName], QModelIndex()));
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[Creator], QModelIndex()));
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemRows[Summary], QModelIndex()));
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[Features], QModelIndex()));
    }

    TEST_F(GemCatalogSearchFilterTests, GemCatalogFilters_SearchStringFeatures_ShowsFeatureGem)
    {
        m_proxyModel->SetSearchString("Creative");

        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[Name], QModelIndex()));
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[DisplayName], QModelIndex()));
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[Creator], QModelIndex()));
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[Summary], QModelIndex()));
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemRows[Features], QModelIndex()));
    }

    TEST_F(GemCatalogSearchFilterTests, GemCatalogFilters_SearchStringEmpty_ShowsAll)
    {
        m_proxyModel->SetSearchString("");

        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemRows[Name], QModelIndex()));
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemRows[DisplayName], QModelIndex()));
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemRows[Creator], QModelIndex()));
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemRows[Summary], QModelIndex()));
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemRows[Features], QModelIndex()));
    }

    TEST_F(GemCatalogSearchFilterTests, GemCatalogFilters_SearchStringCommonCharacter_ShowsAll)
    {
        // All gems contain "a" in a searchable field so all should be shown
        m_proxyModel->SetSearchString("a");

        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemRows[Name], QModelIndex()));
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemRows[DisplayName], QModelIndex()));
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemRows[Creator], QModelIndex()));
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemRows[Summary], QModelIndex()));
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemRows[Features], QModelIndex()));
    }

    TEST_F(GemCatalogSearchFilterTests, GemCatalogFilters_SearchStringDifferentCaseCommonCharacter_ShowsAll)
    {
        // No gems contain the character "A" but search should be case insensitive
        m_proxyModel->SetSearchString("A");

        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemRows[Name], QModelIndex()));
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemRows[DisplayName], QModelIndex()));
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemRows[Creator], QModelIndex()));
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemRows[Summary], QModelIndex()));
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemRows[Features], QModelIndex()));
    }

    TEST_F(GemCatalogSearchFilterTests, GemCatalogFilters_SearchStringNoneContainCharacter_ShowsNone)
    {
        // No gems contain the character "z" or "Z" so none should be shown
        m_proxyModel->SetSearchString("z");

        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[Name], QModelIndex()));
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[DisplayName], QModelIndex()));
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[Creator], QModelIndex()));
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[Summary], QModelIndex()));
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[Features], QModelIndex()));
    }

    TEST_F(GemCatalogSearchFilterTests, GemCatalogFilters_SearchStringPartialMatchString_ShowsNone)
    {
        // Token matching is currently not supported
        // The whole string must match a substring
        m_proxyModel->SetSearchString("Name Token");

        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[Name], QModelIndex()));
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[DisplayName], QModelIndex()));
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[Creator], QModelIndex()));
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[Summary], QModelIndex()));
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[Features], QModelIndex()));
    }

    class GemCatalogSelectedActiveFilterTests
        : public GemCatalogFilterTests
    {
    public:
        void SetUp() override
        {
            GemCatalogFilterTests::SetUp();

            GemInfo gemSelected, gemSelectedDep, gemUnselected, gemUnselectedDep, gemActive, gemInactive;

            gemSelected.m_name = "selected";
            gemSelectedDep.m_name = "selectedDep";
            gemUnselected.m_name = "unselected";
            gemUnselectedDep.m_name = "unselectedDep";
            gemActive.m_name = "active";
            gemInactive.m_name = "inactive";

            gemSelected.m_dependencies = QStringList({ "selectedDep" });
            gemUnselected.m_dependencies = QStringList({ "unselectedDep" });

            m_gemIndices.append(m_gemModel->AddGem(gemSelected));
            m_gemIndices.append(m_gemModel->AddGem(gemSelectedDep));
            m_gemIndices.append(m_gemModel->AddGem(gemUnselected));
            m_gemIndices.append(m_gemModel->AddGem(gemUnselectedDep));
            m_gemIndices.append(m_gemModel->AddGem(gemActive));
            m_gemIndices.append(m_gemModel->AddGem(gemInactive));

            m_gemModel->UpdateGemDependencies();

            // Set intial state of catalog with the to be unselected gem currently added along with active gem
            GemModel::SetIsAdded(*m_gemModel, m_gemIndices[Unselected], true);
            GemModel::SetWasPreviouslyAdded(*m_gemModel, m_gemIndices[Unselected], true);
            GemModel::SetIsAdded(*m_gemModel, m_gemIndices[Active], true);
            GemModel::SetWasPreviouslyAdded(*m_gemModel, m_gemIndices[Active], true);

            // Add selected gem and remove unselected gem
            GemModel::SetIsAdded(*m_gemModel, m_gemIndices[Selected], true);
            GemModel::SetIsAdded(*m_gemModel, m_gemIndices[Unselected], false);
        }

    protected:
        enum IndexOrder
        {
            Selected,
            SelectedDep,
            Unselected,
            UnselectedDep,
            Active,
            Inactive
        };

        QVector<QModelIndex> m_gemIndices;
    };

    TEST_F(GemCatalogSelectedActiveFilterTests, GemCatalogFilters_SelectedActiveIntialState_AddedGemsAndDependenciesAreAdded)
    {
        // Check if gems are all in expected state
        // if this test fails all other Selected/Active tests are invalid
        EXPECT_TRUE(GemModel::IsAdded(m_gemIndices[Selected]));
        EXPECT_TRUE(GemModel::IsAddedDependency(m_gemIndices[SelectedDep]));
        EXPECT_FALSE(GemModel::IsAdded(m_gemIndices[Unselected]));
        EXPECT_FALSE(GemModel::IsAddedDependency(m_gemIndices[UnselectedDep]));
        EXPECT_TRUE(GemModel::IsAdded(m_gemIndices[Active]));
        EXPECT_FALSE(GemModel::IsAdded(m_gemIndices[Inactive]));
    }

    TEST_F(GemCatalogSelectedActiveFilterTests, GemCatalogFilters_SelectedActiveNoFilter_ShowsAll)
    {
        // Filter is clear
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemIndices[Selected].row(), QModelIndex()));
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemIndices[SelectedDep].row(), QModelIndex()));
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemIndices[Unselected].row(), QModelIndex()));
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemIndices[UnselectedDep].row(), QModelIndex()));
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemIndices[Active].row(), QModelIndex()));
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemIndices[Inactive].row(), QModelIndex()));
    }

    TEST_F(GemCatalogSelectedActiveFilterTests, GemCatalogFilters_FilterSelected_ShowsSelectedAndDependencies)
    {
        // Check selected filter
        // Selected dependencies should also be shown
        m_proxyModel->SetGemSelected(GemSortFilterProxyModel::GemSelected::Selected);

        EXPECT_TRUE(m_proxyModel->GetGemSelected() == GemSortFilterProxyModel::GemSelected::Selected);
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemIndices[Selected].row(), QModelIndex()));
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemIndices[SelectedDep].row(), QModelIndex()));
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemIndices[Unselected].row(), QModelIndex()));
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemIndices[UnselectedDep].row(), QModelIndex()));
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemIndices[Active].row(), QModelIndex()));
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemIndices[Inactive].row(), QModelIndex()));
    }

    TEST_F(GemCatalogSelectedActiveFilterTests, GemCatalogFilters_FilterUnselected_ShowsUnselectedAndDependencies)
    {
        // Check unselected filter
        // Unselected dependencies should also be shown
        m_proxyModel->SetGemSelected(GemSortFilterProxyModel::GemSelected::Unselected);

        EXPECT_TRUE(m_proxyModel->GetGemSelected() == GemSortFilterProxyModel::GemSelected::Unselected);
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemIndices[Selected].row(), QModelIndex()));
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemIndices[SelectedDep].row(), QModelIndex()));
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemIndices[Unselected].row(), QModelIndex()));
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemIndices[UnselectedDep].row(), QModelIndex()));
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemIndices[Active].row(), QModelIndex()));
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemIndices[Inactive].row(), QModelIndex()));
    }

    TEST_F(GemCatalogSelectedActiveFilterTests, GemCatalogFilters_FilterSelectedAndUnselected_ShowsAllChangesAndDependencies)
    {
        // Check both un/selected filter
        m_proxyModel->SetGemSelected(GemSortFilterProxyModel::GemSelected::Both);

        EXPECT_TRUE(m_proxyModel->GetGemSelected() == GemSortFilterProxyModel::GemSelected::Both);
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemIndices[Selected].row(), QModelIndex()));
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemIndices[SelectedDep].row(), QModelIndex()));
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemIndices[Unselected].row(), QModelIndex()));
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemIndices[UnselectedDep].row(), QModelIndex()));
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemIndices[Active].row(), QModelIndex()));
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemIndices[Inactive].row(), QModelIndex()));
    }

    TEST_F(GemCatalogSelectedActiveFilterTests, GemCatalogFilters_FilterActive_ShowsActive)
    {
        // Check active filter
        // Active dependencies should also be shown
        m_proxyModel->SetGemActive(GemSortFilterProxyModel::GemActive::Active);

        EXPECT_TRUE(m_proxyModel->GetGemActive() == GemSortFilterProxyModel::GemActive::Active);
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemIndices[Selected].row(), QModelIndex()));
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemIndices[SelectedDep].row(), QModelIndex()));
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemIndices[Unselected].row(), QModelIndex()));
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemIndices[UnselectedDep].row(), QModelIndex()));
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemIndices[Active].row(), QModelIndex()));
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemIndices[Inactive].row(), QModelIndex()));
    }

    TEST_F(GemCatalogSelectedActiveFilterTests, GemCatalogFilters_FilterActive_ShowsInactive)
    {
        // Check inactive filter
        m_proxyModel->SetGemActive(GemSortFilterProxyModel::GemActive::Inactive);

        EXPECT_TRUE(m_proxyModel->GetGemActive() == GemSortFilterProxyModel::GemActive::Inactive);
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemIndices[Selected].row(), QModelIndex()));
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemIndices[SelectedDep].row(), QModelIndex()));
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemIndices[Unselected].row(), QModelIndex()));
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemIndices[UnselectedDep].row(), QModelIndex()));
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemIndices[Active].row(), QModelIndex()));
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemIndices[Inactive].row(), QModelIndex()));
    }

    class GemCatalogMiscFilterTests
        : public GemCatalogFilterTests
    {
    public:
        void SetUp() override
        {
            GemCatalogFilterTests::SetUp();

            GemInfo gemA, gemB, gemC;

            gemA.m_name = "Default Audio";
            gemB.m_name = "Mobile UX";
            gemC.m_name = "City Props";

            gemA.m_gemOrigin = GemInfo::GemOrigin::Open3DEngine;
            gemB.m_gemOrigin = GemInfo::GemOrigin::Local;
            gemC.m_gemOrigin = GemInfo::GemOrigin::Remote;

            gemA.m_types = GemInfo::Type::Code;
            gemB.m_types = GemInfo::Type::Code | GemInfo::Type::Tool;
            gemC.m_types = GemInfo::Type::Asset;

            using Plat = GemInfo::Platform;
            gemA.m_platforms = Plat::Windows;
            gemB.m_platforms = Plat::Android | Plat::iOS;
            gemC.m_platforms = Plat::Android | Plat::iOS | Plat::Linux | Plat::macOS | Plat::Windows; 

            gemA.m_features = QStringList({ "Audio", "Framework", "SDK" });
            gemB.m_features = QStringList({ "Framework", "Tools", "UI" });
            gemC.m_features = QStringList({ "Assets", "Content", "Environment" });

            m_gemRows.append(m_gemModel->AddGem(gemA).row());
            m_gemRows.append(m_gemModel->AddGem(gemB).row());
            m_gemRows.append(m_gemModel->AddGem(gemC).row());
        }

    protected:
        enum RowOrder
        {
            DefaultAudio,
            MobileUX,
            CityProps
        };

        QVector<int> m_gemRows;
    };

    TEST_F(GemCatalogMiscFilterTests, GemCatalogFilters_MiscNoFilter_ShowsAll)
    {
        // No filter
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemRows[DefaultAudio], QModelIndex()));
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemRows[MobileUX], QModelIndex()));
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemRows[CityProps], QModelIndex()));
    }

    TEST_F(GemCatalogMiscFilterTests, GemCatalogFilters_FilterSingleOrigin_ShowsOriginMatch)
    {
        m_proxyModel->SetGemOrigins(GemInfo::GemOrigin::Open3DEngine);

        EXPECT_TRUE(m_proxyModel->GetGemOrigins() == GemInfo::GemOrigin::Open3DEngine);
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemRows[DefaultAudio], QModelIndex()));
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[MobileUX], QModelIndex()));
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[CityProps], QModelIndex()));

        m_proxyModel->SetGemOrigins(GemInfo::GemOrigin::Local);

        EXPECT_TRUE(m_proxyModel->GetGemOrigins() == GemInfo::GemOrigin::Local);
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[DefaultAudio], QModelIndex()));
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemRows[MobileUX], QModelIndex()));
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[CityProps], QModelIndex()));

        m_proxyModel->SetGemOrigins(GemInfo::GemOrigin::Remote);

        EXPECT_TRUE(m_proxyModel->GetGemOrigins() == GemInfo::GemOrigin::Remote);
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[DefaultAudio], QModelIndex()));
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[MobileUX], QModelIndex()));
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemRows[CityProps], QModelIndex()));
    }

    TEST_F(GemCatalogMiscFilterTests, GemCatalogFilters_FilterMultipleOrigins_ShowsMultipleOriginMatches)
    {
        m_proxyModel->SetGemOrigins(GemInfo::GemOrigin::Open3DEngine | GemInfo::GemOrigin::Local);

        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemRows[DefaultAudio], QModelIndex()));
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemRows[MobileUX], QModelIndex()));
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[CityProps], QModelIndex()));
    }

    TEST_F(GemCatalogMiscFilterTests, GemCatalogFilters_FilterSingleType_ShowsTypeMatch)
    {
        m_proxyModel->SetTypes(GemInfo::Type::Code);

        EXPECT_TRUE(m_proxyModel->GetTypes() == GemInfo::Type::Code);
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemRows[DefaultAudio], QModelIndex()));
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemRows[MobileUX], QModelIndex()));
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[CityProps], QModelIndex()));

        m_proxyModel->SetTypes(GemInfo::Type::Tool);

        EXPECT_TRUE(m_proxyModel->GetTypes() == GemInfo::Type::Tool);
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[DefaultAudio], QModelIndex()));
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemRows[MobileUX], QModelIndex()));
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[CityProps], QModelIndex()));

        m_proxyModel->SetTypes(GemInfo::Type::Asset);

        EXPECT_TRUE(m_proxyModel->GetTypes() == GemInfo::Type::Asset);
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[DefaultAudio], QModelIndex()));
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[MobileUX], QModelIndex()));
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemRows[CityProps], QModelIndex()));
    }

    TEST_F(GemCatalogMiscFilterTests, GemCatalogFilters_FilterMultipleTypes_ShowsMultipleTypeMatches)
    {
        m_proxyModel->SetTypes(GemInfo::Type::Tool | GemInfo::Type::Asset);

        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[DefaultAudio], QModelIndex()));
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemRows[MobileUX], QModelIndex()));
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemRows[CityProps], QModelIndex()));
    }

    TEST_F(GemCatalogMiscFilterTests, GemCatalogFilters_FilterSinglePlatform_ShowsPlatformMatch)
    {
        m_proxyModel->SetPlatforms(GemInfo::Platform::Windows);

        EXPECT_TRUE(m_proxyModel->GetPlatforms() == GemInfo::Platform::Windows);
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemRows[DefaultAudio], QModelIndex()));
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[MobileUX], QModelIndex()));
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemRows[CityProps], QModelIndex()));

        m_proxyModel->SetPlatforms(GemInfo::Platform::Android);

        EXPECT_TRUE(m_proxyModel->GetPlatforms() == GemInfo::Platform::Android);
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[DefaultAudio], QModelIndex()));
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemRows[MobileUX], QModelIndex()));
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemRows[CityProps], QModelIndex()));

        m_proxyModel->SetPlatforms(GemInfo::Platform::macOS);

        EXPECT_TRUE(m_proxyModel->GetPlatforms() == GemInfo::Platform::macOS);
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[DefaultAudio], QModelIndex()));
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[MobileUX], QModelIndex()));
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemRows[CityProps], QModelIndex()));
    }

    TEST_F(GemCatalogMiscFilterTests, GemCatalogFilters_FilterMultiplePlatforms_ShowsMultiplePlatformMatches)
    {
        m_proxyModel->SetPlatforms(GemInfo::Platform::Android | GemInfo::Platform::iOS);

        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[DefaultAudio], QModelIndex()));
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemRows[MobileUX], QModelIndex()));
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemRows[CityProps], QModelIndex()));
    }

    TEST_F(GemCatalogMiscFilterTests, GemCatalogFilters_FilterSingleFeature_ShowsFeatureMatch)
    {
        m_proxyModel->SetFeatures({ "Audio" });

        EXPECT_TRUE(m_proxyModel->GetFeatures().contains("Audio"));
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemRows[DefaultAudio], QModelIndex()));
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[MobileUX], QModelIndex()));
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[CityProps], QModelIndex()));

        m_proxyModel->SetFeatures({ "Tools", });

        EXPECT_TRUE(m_proxyModel->GetFeatures().contains("Tools"));
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[DefaultAudio], QModelIndex()));
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemRows[MobileUX], QModelIndex()));
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[CityProps], QModelIndex()));

        m_proxyModel->SetFeatures({ "Environment" });

        EXPECT_TRUE(m_proxyModel->GetFeatures().contains( "Environment" ));
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[DefaultAudio], QModelIndex()));
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[MobileUX], QModelIndex()));
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemRows[CityProps], QModelIndex()));
    }

    TEST_F(GemCatalogMiscFilterTests, GemCatalogFilters_FilterMultipleFeatures_ShowsMultipleFeatureMatches)
    {
        m_proxyModel->SetFeatures({ "Assets", "Framework" });

        EXPECT_TRUE(m_proxyModel->GetFeatures().contains({ "Assets", "Framework" }));
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemRows[DefaultAudio], QModelIndex()));
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemRows[MobileUX], QModelIndex()));
        EXPECT_TRUE(m_proxyModel->filterAcceptsRow(m_gemRows[CityProps], QModelIndex()));
    }

    TEST_F(GemCatalogMiscFilterTests, GemCatalogFilters_FilterPartialMatchFeature_ShowsNone)
    {
        // Features must be an exact match to filter by them directly
        m_proxyModel->SetFeatures({ "Frame" });

        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[DefaultAudio], QModelIndex()));
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[MobileUX], QModelIndex()));
        EXPECT_FALSE(m_proxyModel->filterAcceptsRow(m_gemRows[CityProps], QModelIndex()));
    }

    TEST_F(GemCatalogMiscFilterTests, GemCatalogFilters_ResetFilters_FiltersReset)
    {
        // Set up all filter values that are reset
        m_proxyModel->SetSearchString("Name");
        m_proxyModel->SetGemSelected(GemSortFilterProxyModel::GemSelected::Selected);
        m_proxyModel->SetGemActive(GemSortFilterProxyModel::GemActive::Active);
        m_proxyModel->SetGemOrigins(GemInfo::GemOrigin::Open3DEngine);
        m_proxyModel->SetPlatforms(GemInfo::Platform::Windows);
        m_proxyModel->SetTypes(GemInfo::Type::Code);
        m_proxyModel->SetFeatures({ "Audio" });
        
        m_proxyModel->ResetFilters(true);

        EXPECT_TRUE(m_proxyModel->GetGemSelected() == GemSortFilterProxyModel::GemSelected::NoFilter);
        EXPECT_TRUE(m_proxyModel->GetGemActive() == GemSortFilterProxyModel::GemActive::NoFilter);
        EXPECT_TRUE(m_proxyModel->GetGemOrigins() == GemInfo::GemOrigins({}));
        EXPECT_TRUE(m_proxyModel->GetPlatforms() == GemInfo::Platforms({}));
        EXPECT_TRUE(m_proxyModel->GetTypes() == GemInfo::Types({}));
        EXPECT_TRUE(m_proxyModel->GetFeatures().isEmpty());
    }
}
