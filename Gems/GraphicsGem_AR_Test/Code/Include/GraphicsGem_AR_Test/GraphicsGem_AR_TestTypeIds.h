/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace GraphicsGem_AR_Test
{
    // System Component TypeIds
    inline constexpr const char* GraphicsGem_AR_TestSystemComponentTypeId = "${SysCompClassId}";
    inline constexpr const char* GraphicsGem_AR_TestEditorSystemComponentTypeId = "${EditorSysCompClassId}";

    // Module derived classes TypeIds
    inline constexpr const char* GraphicsGem_AR_TestModuleInterfaceTypeId = "{E22C9D32-526E-481F-8440-29E489EEB349}";
    inline constexpr const char* GraphicsGem_AR_TestModuleTypeId = "${ModuleClassId}";
    // The Editor Module by default is mutually exclusive with the Client Module
    // so they use the Same TypeId
    inline constexpr const char* GraphicsGem_AR_TestEditorModuleTypeId = GraphicsGem_AR_TestModuleTypeId;

    // Interface TypeIds
    inline constexpr const char* GraphicsGem_AR_TestRequestsTypeId = "{D6D4B90B-2392-40D1-8177-9C71383B57AD}";
} // namespace GraphicsGem_AR_Test
