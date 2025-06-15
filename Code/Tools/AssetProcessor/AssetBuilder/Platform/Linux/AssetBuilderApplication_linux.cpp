/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AssetBuilderApplication.h>

void AssetBuilderApplication::QueryApplicationType(AZ::ApplicationTypeQuery& appType) const
{
    appType.m_maskValue = AZ::ApplicationTypeQuery::Masks::Tool | AZ::ApplicationTypeQuery::Masks::AssetProcessor;
}

void AssetBuilderApplication::InstallCtrlHandler()
{
}
