/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <MCore/Source/CommandGroup.h>
#include <EMotionFX/Source/Parameter/ValueParameter.h>

namespace EMotionFX
{
    class AnimGraph;

    // Interface class to be used for anim graph objects that get affected by changes in parameters. This interface
    // can deal with both: changes of parameters in a mask and addition/removal/reordering of parameters in the AnimGraph.
    //
    class ObjectAffectedByParameterChanges
    {
    public:
        AZ_RTTI(ObjectAffectedByParameterChanges, "{5B0BC730-F5FF-490F-93E6-706080058D08}")

        virtual ~ObjectAffectedByParameterChanges() = default;

        // Add parameters that are required in the mask for this node. For example, a BlendTreeParameterNode can 
        // add the parameters that belong to connected ports. This is using after the user inputs a new mask into
        // the UI. 
        virtual void AddRequiredParameters(AZStd::vector<AZStd::string>& parameterNames) const;

        virtual AZStd::vector<AZStd::string> GetParameters() const;
        virtual AnimGraph* GetParameterAnimGraph() const;

        virtual void ParameterMaskChanged(const AZStd::vector<AZStd::string>& newParameterMask);

        // This method is called whenever a new parameter is being added. A node implementing this interface has to 
        // call or not the event manager to notify about changes in ports. If it is not affected by this new parameter
        // then it can do nothing.
        virtual void ParameterAdded(const AZStd::string& newParameterName);
        
        // Similar as above, this method is called after renaming a parameter.
        virtual void ParameterRenamed(const AZStd::string& oldParameterName, const AZStd::string& newParameterName);
        
        // Similar as above, this method is called when parameters change order.
        virtual void ParameterOrderChanged(const ValueParameterVector& beforeChange, const ValueParameterVector& afterChange);
        
        // Similar as above, this method is called when a parameter is removed.
        virtual void ParameterRemoved(const AZStd::string& oldParameterName);

        // This method is called when building the command group for removing a parameter.
        virtual void BuildParameterRemovedCommands([[maybe_unused]] MCore::CommandGroup& commandGroup, [[maybe_unused]] const AZStd::string& parameterNameToBeRemoved) {}

        // Convenient function to sort parameters based on the order they appear in the animgraph. It also removes duplicates
        static void SortAndRemoveDuplicates(AnimGraph* animGraph, AZStd::vector<AZStd::string>& parameterNames);
    };
}   // namespace EMotionFX
