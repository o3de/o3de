/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScriptCanvas/Core/SlotExecutionMap.h>
#include <ScriptCanvas/Core/SubgraphInterfaceUtility.h>

#include "FunctionCallNode.h"
#include "FunctionCallNodeIsOutOfDate.h"

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
           bool IsFunctionCallNodeOutOfDate(const IsFunctionCallOutOfDateConfig& config)
           {
                if (Grammar::IsFunctionSourceIdNodeable(config.sourceId))
                {
                    const bool latestIsNodeable = config.latestInterface.IsUserNodeable();

                    if (!config.compare.ignorePurityChanges && !latestIsNodeable)
                    {
                        AZ_Warning("ScriptCanvas", false, "FunctionCallNode %s source interface has changed. The node must be recreated.", config.node.GetName().data());
                        return true;
                    }

                    if (IsFunctionCallNodeOutOfDateLatents(config))
                    {
                        return true;
                    }

                    if (IsFunctionCallNodeOutOfDateNodeable(config))
                    {
                        return true;
                    }
                }
                else
                {
                    return IsFunctionCallNodeOutOfDatePure(config);
                }

                return false;
            }

            bool IsFunctionCallNodeOutOfDateLatents(const IsFunctionCallOutOfDateConfig& config)
            {
                const bool latestIsNodeable = config.latestInterface.IsUserNodeable();

                // if latents are missing or changed return true
                for (const auto& latentMapEntry : config.slotMap.GetLatents())
                {
                    if (auto slot = config.node.GetSlot(latentMapEntry.slotId))
                    {
                        if (slot->IsConnected())
                        {
                            if (!latestIsNodeable)
                            {
                                AZ_Warning("ScriptCanvas", false, "FunctionCallNode %s is exposing a latent function that has been deleted from the source. The node must be deleted.", config.node.GetName().data());
                                return true;
                            }

                            if (const auto* newLatent = config.latestInterface.FindLatent(latentMapEntry.interfaceSourceId))
                            {
                                if (const auto* oldLatent = config.originalInterface.FindLatent(latentMapEntry.interfaceSourceId))
                                {
                                    if (IsOutOfDate(config.compare, *oldLatent, *newLatent))
                                    {
                                        AZ_Warning("ScriptCanvas", false, "FunctionCallNode %s is exposing a latent function that has been changed in the source. THe node must be recreated.", config.node.GetName().data());
                                        return true;
                                    }
                                }
                                else
                                {
                                    AZ_Warning("ScriptCanvas", false, "FunctionCallNode %s is exposing a latent function that has been deleted from the source. The node must be deleted.", config.node.GetName().data());
                                    return true;
                                }
                            }
                            else
                            {
                                AZ_Error("ScriptCanvas", false, "FunctionCallNode %s is exposing a latent function with a slot that cannot be found in its execution map.", config.node.GetName().data());
                                return true;
                            }
                        }
                    }
                }

                return false;
            }

            bool IsFunctionCallNodeOutOfDateNodeable(const IsFunctionCallOutOfDateConfig& config)
            {
                // if in/outs are connected, and missing/changed from the the version, return true
                for (const auto& inMapEntry : config.slotMap.GetIns())
                {
                    if (auto slot = config.node.GetSlot(inMapEntry.slotId))
                    {
                        if (slot->IsConnected())
                        {
                            if (const auto* newIn = config.latestInterface.FindIn(inMapEntry.interfaceSourceId))
                            {
                                if (const auto* oldIn = config.originalInterface.FindIn(inMapEntry.interfaceSourceId))
                                {
                                    if (IsOutOfDate(config.compare, *oldIn, *newIn))
                                    {
                                        AZ_Warning("ScriptCanvas", false, "FunctionCallNode %s is calling a function that has been changed in the source. THe node must be recreated.", config.node.GetName().data());
                                        return true;
                                    }
                                }
                                else
                                {
                                    AZ_Warning("ScriptCanvas", false, "FunctionCallNode %s is calling a function that has been deleted from the source. The node must be recreated.", config.node.GetName().data());
                                    return true;
                                }
                            }
                            else
                            {
                                AZ_Error("ScriptCanvas", false, "FunctionCallNode %s is calling a function with a slot that doesn't refer to a function in the source graph.", config.node.GetName().data());
                                return true;
                            }
                        }
                    }
                    else
                    {
                        AZ_Error("ScriptCanvas", false, "FunctionCallNode %s is calling a function with a slot that cannot be found in its execution map.", config.node.GetName().data());
                        return true;
                    }
                }

                return false;
            }

            bool IsFunctionCallNodeOutOfDatePure(const IsFunctionCallOutOfDateConfig& config)
            {
                // check the pure call for being out of date
                if (auto oldIn = config.originalInterface.FindIn(config.sourceId))
                {
                    if (auto newIn = config.latestInterface.FindIn(config.sourceId))
                    {
                        if (IsOutOfDate(config.compare, *oldIn, *newIn))
                        {
                            AZ_Warning("ScriptCanvas", false, "FunctionCallNode %s is calling a function that has been changed in the source. THe node must be recreated.", config.node.GetName().data());
                            return true;
                        }
                    }
                    else
                    {
                        AZ_Warning("ScriptCanvas", false, "FunctionCallNode %s is calling a function that has been deleted from the source. The node must be deleted.", config.node.GetName().data());
                        return true;
                    }
                }
                else
                {
                    AZ_Error("ScriptCanvas", false, "FunctionCallNode %s is calling a function with a slot that cannot be found in its execution map.", config.node.GetName().data());
                    return true;
                }

                return false;
            }

            bool IsOutOfDate(const FunctionCallNodeCompareConfig& config, const Grammar::In& theOld, const Grammar::In& theNew)
            {
                if (!config.ignorePurityChanges && theOld.isPure != theNew.isPure)
                {
                    return true;
                }

                if (!config.ignoreInNameChanges)
                {
                    if (!AZ::StringFunc::Equal(theOld.displayName.c_str(), theNew.displayName.c_str()))
                    {
                        return true;
                    }

                    if (!AZ::StringFunc::Equal(theOld.parsedName.c_str(), theNew.parsedName.c_str()))
                    {
                        return true;
                    }
                }

                if (!(theOld.inputs == theNew.inputs))
                {
                    return true;
                }

                if (!(theOld.outs == theNew.outs))
                {
                    return true;
                }

                if (!config.ignoreInSourceIdChanges && !(theOld.sourceID == theNew.sourceID))
                {
                    return true;
                }

                return false;
            }

            bool IsOutOfDate(const FunctionCallNodeCompareConfig& config, const Grammar::Input& inputOld, const Grammar::Input& inputNew)
            {
                if (!config.ignoreInputNameChanges)
                {
                    if (!AZ::StringFunc::Equal(inputOld.displayName.c_str(), inputNew.displayName.c_str()))
                    {
                        return true;
                    }

                    if (!AZ::StringFunc::Equal(inputOld.parsedName.c_str(), inputNew.parsedName.c_str()))
                    {
                        return true;
                    }
                }

                if (config.ignoreInputDefaultValueChanges)
                {
                    if (inputOld.datum.GetType() != inputNew.datum.GetType())
                    {
                        return true;
                    }
                }
                else if (!(inputOld.datum == inputNew.datum)) 
                {
                    return true;
                }

                if (!config.ignoreInputSourceIdChanges && !(inputOld.sourceID == inputNew.sourceID))
                {
                    return true;
                }

                return false;
            }

            bool IsOutOfDate(const FunctionCallNodeCompareConfig& config, const Grammar::Out& theOld, const Grammar::Out& theNew)
            {
                if (!config.ignoreOutNameChanges)
                {
                    if (!AZ::StringFunc::Equal(theOld.displayName.c_str(), theNew.displayName.c_str()))
                    {
                        return true;
                    }

                    if (!AZ::StringFunc::Equal(theOld.parsedName.c_str(), theNew.parsedName.c_str()))
                    {
                        return true;
                    }
                }

                if (!config.ignoreOutSourceIdChanges && !Grammar::OutIdIsEqual(theOld.sourceID, theNew.sourceID))
                {
                    return true;
                }

                if (!(theOld.outputs == theNew.outputs))
                {
                    return true;
                }

                if (!(theOld.returnValues == theNew.returnValues))
                {
                    return true;
                }

                return false;
            }

            bool IsOutOfDate(const FunctionCallNodeCompareConfig& config, const Grammar::Output& theOld, const Grammar::Output& theNew)
            {
                if (!config.ignoreOutputNameChanges)
                {
                    if (!AZ::StringFunc::Equal(theOld.displayName.c_str(), theNew.displayName.c_str()))
                    {
                        return true;
                    }

                    if (!AZ::StringFunc::Equal(theOld.parsedName.c_str(), theNew.parsedName.c_str()))
                    {
                        return true;
                    }
                }

                if (!(theOld.type == theNew.type))
                {
                    return true;
                }

                if (!config.ignoreOutputSourceIdChanges && !(theOld.sourceID == theNew.sourceID))
                {
                    return true;
                }

                return false;
            }
        }
    }
}
