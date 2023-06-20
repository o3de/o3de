/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Prefab/PrefabInstanceDomGeneratorTestFixture.h>

namespace UnitTest
{
    using PrefabInstanceDomGeneratorTests = PrefabInstanceDomGeneratorTestFixture;

    TEST_F(PrefabInstanceDomGeneratorTests, GenerateInstanceDomForDescendantOfFocusedLevel)
    {
        // Generate a prefab DOM for the Wheel instance while the Level is in focus
        GenerateAndValidateInstanceDom(m_wheelInstance->get(), m_tireAlias, m_entityOverrideValueOnLevel);
    }

    TEST_F(PrefabInstanceDomGeneratorTests, GenerateInstanceDomForFocusedPrefab)
    {
        // Generate a prefab DOM for the Wheel instance while the Wheel instance is in focus
        m_prefabFocusPublicInterface->FocusOnOwningPrefab(m_wheelInstance->get().GetContainerEntityId());
        GenerateAndValidateInstanceDom(m_wheelInstance->get(), m_tireAlias, m_entityValueOnWheel);
    }

    TEST_F(PrefabInstanceDomGeneratorTests, GenerateInstanceDomForAncestorOfFocusedPrefab)
    {
        // Generate a prefab DOM for the Car instance while the Wheel instance is in focus
        m_prefabFocusPublicInterface->FocusOnOwningPrefab(m_wheelInstance->get().GetContainerEntityId());
        GenerateAndValidateInstanceDom(m_carInstance->get(), m_tireAlias, m_entityValueOnWheel);
    }

    TEST_F(PrefabInstanceDomGeneratorTests, GenerateEntityDomForDescendantOfFocusedPrefab)
    {
        const AZ::Entity& tireEntity = m_wheelInstance->get().GetEntity(m_tireAlias)->get();

        // Focus is on Level by default
        GenerateAndValidateEntityDom(tireEntity, m_entityOverrideValueOnLevel);

        // Change focus to Car
        m_prefabFocusPublicInterface->FocusOnOwningPrefab(m_carInstance->get().GetContainerEntityId());
        GenerateAndValidateEntityDom(tireEntity, m_entityOverrideValueOnCar);

        // Change focus to Wheel
        m_prefabFocusPublicInterface->FocusOnOwningPrefab(m_wheelInstance->get().GetContainerEntityId());
        GenerateAndValidateEntityDom(tireEntity, m_entityValueOnWheel);
    }

    TEST_F(PrefabInstanceDomGeneratorTests, GenerateEntityDomForContainerOfFocusedPrefab)
    {
        const AZ::Entity& containerEntity = m_wheelInstance->get().GetContainerEntity()->get();

        // Change focus to Wheel, so the container entity DOM should come from the root
        m_prefabFocusPublicInterface->FocusOnOwningPrefab(m_wheelInstance->get().GetContainerEntityId());
        GenerateAndValidateEntityDom(containerEntity, m_wheelContainerOverrideValueOnLevel);
    }
} // namespace UnitTest
