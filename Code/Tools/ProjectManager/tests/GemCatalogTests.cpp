/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/Utils.h>
#include <GemCatalog/GemModel.h>


namespace O3DE::ProjectManager
{
    class GemCatalogTests 
        : public ::UnitTest::ScopedAllocatorSetupFixture
    {
    public:

        GemCatalogTests() = default;
    };

    TEST_F(GemCatalogTests, GemCatalog_Displays_But_Does_Not_Add_Dependencies)
    {
        GemModel* gemModel = new GemModel();

        // given 3 gems a,b,c where a depends on b which depends on c
        GemInfo gemA, gemB, gemC;
        QModelIndex indexA, indexB, indexC;
        gemA.m_name = "a";
        gemB.m_name = "b";
        gemC.m_name = "c";

        gemA.m_dependencies = QStringList({ "b" });
        gemB.m_dependencies = QStringList({ "c" });

        gemModel->AddGem(gemA);
        indexA = gemModel->FindIndexByNameString(gemA.m_name);

        gemModel->AddGem(gemB);
        indexB = gemModel->FindIndexByNameString(gemB.m_name);

        gemModel->AddGem(gemC);
        indexC = gemModel->FindIndexByNameString(gemC.m_name);

        gemModel->UpdateGemDependencies();

        EXPECT_FALSE(GemModel::IsAdded(indexA));
        EXPECT_FALSE(GemModel::IsAddedDependency(indexB) || GemModel::IsAddedDependency(indexC));

        // when a is added
        GemModel::SetIsAdded(*gemModel, indexA, true);

        // expect b and c are now dependencies of an added gem but not themselves added
        // cmake will handle dependencies
        EXPECT_TRUE(GemModel::IsAddedDependency(indexB) && GemModel::IsAddedDependency(indexC));
        EXPECT_TRUE(!GemModel::IsAdded(indexB) && !GemModel::IsAdded(indexC));

        QVector<QModelIndex> gemsToAdd = gemModel->GatherGemsToBeAdded();
        EXPECT_TRUE(gemsToAdd.size() == 1);
        EXPECT_EQ(GemModel::GetName(gemsToAdd.at(0)), gemA.m_name);
    }
}
