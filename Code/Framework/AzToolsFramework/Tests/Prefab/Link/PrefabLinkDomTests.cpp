/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Prefab/PrefabTestDomUtils.h>
#include <Prefab/Link/PrefabLinkDomTestFixture.h>

namespace UnitTest
{
    using PrefabLinkDomTest = PrefabLinkDomTestFixture;

    // Mock patches to use for validating tests.
    static constexpr const AZStd::string_view patchesValue = R"(
            [
                {
                    "op": "add",
                    "path": "Entities/Entity1/Components/ComponentA/IntValue",
                    "value": 10
                },
                {
                    "op": "remove",
                    "path": "Entities/Entity2/Components/ComponentB/FloatValue"
                },
                {
                    "op": "replace",
                    "path": "Entities/Entity1/Components/ComponentC/StringValue",
                    "value": "replacedString"
                }
            ])";

    TEST_F(PrefabLinkDomTest, GetLinkDomRetainsPatchOrder)
    {
        PrefabDom newLinkDom;
        newLinkDom.Parse(R"(
            {
                "Source": "PathToSourceTemplate"
            })");

        PrefabDom patchesCopy;
        patchesCopy.Parse(patchesValue.data());
        newLinkDom.AddMember(rapidjson::StringRef(PrefabDomUtils::PatchesName), AZStd::move(patchesCopy), newLinkDom.GetAllocator());
        m_link->SetLinkDom(newLinkDom);

        // Get the link DOM and verify that it matches the DOM used for SetLinkDom().
        PrefabDom fetchedLinkDom;
        m_link->GetLinkDom(fetchedLinkDom, fetchedLinkDom.GetAllocator());
        EXPECT_EQ(AZ::JsonSerialization::Compare(newLinkDom, fetchedLinkDom), AZ::JsonSerializerCompareResult::Equal);
    }

    TEST_F(PrefabLinkDomTest, AddPatchesToLinkRetainsPatchOrder)
    {
        PrefabDom newLinkDom;
        newLinkDom.Parse(R"(
            {
                "Source": "PathToSourceTemplate"
            })");

        PrefabDom patchesCopy;
        patchesCopy.Parse(patchesValue.data());
        m_link->SetLinkPatches(patchesCopy);
        newLinkDom.AddMember(rapidjson::StringRef(PrefabDomUtils::PatchesName), AZStd::move(patchesCopy), newLinkDom.GetAllocator());

        // Get the link DOM and verify that it matches the DOM used for AddPatchesToLink().
        PrefabDom fetchedLinkDom;
        m_link->GetLinkDom(fetchedLinkDom, fetchedLinkDom.GetAllocator());
        EXPECT_EQ(AZ::JsonSerialization::Compare(newLinkDom, fetchedLinkDom), AZ::JsonSerializerCompareResult::Equal);
    }
} // namespace UnitTest
