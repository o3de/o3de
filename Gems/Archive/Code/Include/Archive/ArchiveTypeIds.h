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

    // Archive Writer TypeIds
    inline constexpr const char* IArchiveWriterTypeId = "{6C966C29-8D98-4FCD-AEE5-CFFF80EEB561}";
    inline constexpr const char* ArchiveWriterTypeId = "{DACA9F90-C8D5-41CB-8400-F0B39BFC4A28}";

    // Archive Reader TypeIds
    inline constexpr const char* IArchiveReaderTypeId = "{FF23A098-E900-4361-94DC-34CC56E0C67E}";
    inline constexpr const char* ArchiveReaderTypeId = "{03CF9E2D-D063-4912-9789-56275DCD4DFD}";

    // Archive Factory TypeIds
    inline constexpr const char* IArchiveWriterFactoryTypeId = "{D96EB527-D174-4BF5-8521-EB2658821ED7}";
    inline constexpr const char* ArchiveWriterFactoryTypeId = "{1B4F8F63-5D36-4BF4-B88E-003A0B8F667B}";
    inline constexpr const char* IArchiveReaderFactoryTypeId = "{6E33EEA8-2059-47EE-B614-90BA1D9F03A7}";
    inline constexpr const char* ArchiveReaderFactoryTypeId = "{9B27ABB6-A3C1-4548-BA80-42BECDD0510F}";
} // namespace Archive
