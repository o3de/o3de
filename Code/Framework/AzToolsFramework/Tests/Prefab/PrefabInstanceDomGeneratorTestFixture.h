/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Prefab/PrefabTestFixture.h>

namespace UnitTest
{
    using namespace AzToolsFramework::Prefab;

    //! Fixture for testing instance DOM generation based on the focused prefab via existing template DOMS
    class PrefabInstanceDomGeneratorTestFixture
        : public PrefabTestFixture
    {
    protected:
        void SetUpEditorFixtureImpl() override;

        void GenerateAndValidateInstanceDom(const Instance& instance, EntityAlias entityAlias, float expectedValue);
        void GenerateAndValidateEntityDom(const AZ::Entity& entity, float expectedValue);

        // Protected members
        const float m_entityOverrideValueOnLevel = 1.0f;
        const float m_entityOverrideValueOnCar = 2.0f;
        const float m_entityValueOnWheel = 3.0f;
        const float m_wheelContainerOverrideValueOnLevel = 1.0f;

        InstanceOptionalReference m_carInstance;
        InstanceOptionalReference m_wheelInstance;
        EntityAlias m_tireAlias;

        PrefabFocusPublicInterface* m_prefabFocusPublicInterface = nullptr;

    private:
        void SetUpInstanceHierarchy();
        void InitializePrefabTemplates();

        //! Finds an entity in the provided instance by recursing through its nested prefab hierarchy
        EntityOptionalReference FindEntityInInstanceHierarchy(
            Instance& instance,
            EntityAlias entityAlias);

        //! Helpers that apply patches to in-memory template DOMs to simulate overrides
        void GenerateWorldXEntityPatch(
            const EntityAlias& entityAlias,
            float updatedXValue,
            Instance& owningInstance,
            PrefabDom& patchOut);
        void UpdateWorldXEntityPatch(PrefabDom& patch, double newValue);
        void ApplyEntityPatchToTemplate(
            PrefabDom& patch,
            const EntityAlias& entityAlias,
            const Instance& owningInstance,
            const Instance& targetInstance);

        // Private members
        InstanceDomGeneratorInterface* m_instanceDomGeneratorInterface = nullptr;
    };
}
