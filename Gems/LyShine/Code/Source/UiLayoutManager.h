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

#include <LyShine/Bus/UiLayoutManagerBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiLayoutManager
    : public UiLayoutManagerBus::Handler
{
public: // member functions

    UiLayoutManager(AZ::EntityId canvasEntityId);
    ~UiLayoutManager();

    // UiLayoutManagerBus interface implementation
    void MarkToRecomputeLayout(AZ::EntityId entityId) override;
    void MarkToRecomputeLayoutsAffectedByLayoutCellChange(AZ::EntityId entityId, bool isDefaultLayoutCell) override;
    void UnmarkAllLayouts() override;
    void RecomputeMarkedLayouts() override;
    void ComputeLayoutForElementAndDescendants(AZ::EntityId entityId) override;
    // ~UiLayoutManagerBus

    bool HasMarkedLayouts() const { return !m_elementsToRecomputeLayout.empty(); }

private: // member functions

    AZ_DISABLE_COPY_MOVE(UiLayoutManager);

    void AddToRecomputeLayoutList(AZ::EntityId entityId);
    bool IsParentOfElement(AZ::EntityId checkParentEntity, AZ::EntityId checkChildEntity);

private: // data

    //! Elements that need to recompute their layouts. Parents should be ahead of their children
    AZStd::list<AZ::EntityId> m_elementsToRecomputeLayout;
};
