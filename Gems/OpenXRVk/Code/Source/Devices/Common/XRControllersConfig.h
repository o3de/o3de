/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once


#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <OpenXRVk/OpenXRVkActionsInterface.h>

namespace OpenXRVk
{
	//! Configuration that is used by XRControllerAnimationsComponent to animate an VR controller.
	struct XRControllersConfig final
	{
		enum class ControlItemType : AZ::u32 {
			Boolean,
			Float,
			Vector2
		};

		AZ_RTTI(XRControllersConfig, "{A32512BD-124A-723B-AA24-214BCD02CCAA}");
		AZ_CLASS_ALLOCATOR(XRControllersConfig, AZ::SystemAllocator);

		static void Reflect(AZ::ReflectContext* context);
		
		// Editor configuration properties
		ControlItemType m_controlItemType;
		AZStd::string m_animGraphParameter;
		AZStd::string m_actionName;

		// Runtime variables
		IOpenXRActions::ActionHandle m_actionHandle;
		bool m_prevBoolean = false;
		float m_prevFloat = 0;
	};	

} // namespace OpenXRVk
