/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace Archive
{
    // System Component TypeIds
    inline constexpr const char* ArchiveSystemComponentTypeId = "{5D9DCDB5-F989-44F3-8976-08F25DF44706}";
    inline constexpr const char* ArchiveEditorSystemComponentTypeId = "{533B3B2B-FA48-43A0-8874-C43F04F29C74}";

    // Module derived classes TypeIds
    inline constexpr const char* ArchiveModuleInterfaceTypeId = "{B181A37C-0D48-451C-8A45-3B4D6F79BA36}";
    inline constexpr const char* ArchiveModuleTypeId = "{8E86FE7C-BC3A-4BB9-88CB-8044E6252F68}";
    // The Editor Module by default is mutually exclusive with the Client Module
    // so they use the Same TypeId
    inline constexpr const char* ArchiveEditorModuleTypeId = ArchiveModuleTypeId;

    // Interface TypeIds
    inline constexpr const char* ArchiveRequestsTypeId = "{0C10E90B-F810-454C-A7AA-D3A30FDD431E}";
} // namespace Archive
