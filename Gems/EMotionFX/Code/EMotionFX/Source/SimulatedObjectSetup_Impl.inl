/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(SimulatedCommon, EMotionFX::ActorAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(SimulatedJoint, EMotionFX::ActorAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(SimulatedObject, EMotionFX::ActorAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(SimulatedObjectSetup, EMotionFX::ActorAllocator)

    void SimulatedJoint::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<SimulatedJoint>()
                ->Version(4)
                ->Field("skeletonJointIndex", &SimulatedJoint::m_jointIndex)
                ->Field("skeletonJointName", &SimulatedJoint::m_jointName)
                ->Field("coneAngleLimit", &SimulatedJoint::m_coneAngleLimit)
                ->Field("mass", &SimulatedJoint::m_mass)
                ->Field("radius", &SimulatedJoint::m_radius)
                ->Field("stiffness", &SimulatedJoint::m_stiffness)
                ->Field("damping", &SimulatedJoint::m_damping)
                ->Field("gravityFactor", &SimulatedJoint::m_gravityFactor)
                ->Field("friction", &SimulatedJoint::m_friction)
                ->Field("pinned", &SimulatedJoint::m_pinned)
                ->Field("autoExcludeGeometric", &SimulatedJoint::m_autoExcludeGeometric)
                ->Field("autoExcludeMode", &SimulatedJoint::m_autoExcludeMode)
                ->Field("colliderExclusionTags", &SimulatedJoint::m_colliderExclusionTags);

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<SimulatedJoint>("SimulatedJoints", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::SpinBox, &SimulatedJoint::m_coneAngleLimit, "Joint angle limit", "The maximum allowed angle in all directions, where 180 means there is no limit.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 180.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.5f)
                    ->DataElement(AZ::Edit::UIHandlers::SpinBox, &SimulatedJoint::m_mass, "Mass", "The mass of the joint.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 3.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                    ->DataElement(AZ::Edit::UIHandlers::SpinBox, &SimulatedJoint::m_radius, "Collision radius", "The collision radius, which is the distance the joint will stay away from colliders.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.001f)
                    ->DataElement(AZ::Edit::UIHandlers::SpinBox, &SimulatedJoint::m_stiffness, "Stiffness", "The stiffness, where a value of zero means it will purely be affected by momentum and gravity. Higher values (like 150) will pull it more towards the original pose.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 300.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                    ->DataElement(AZ::Edit::UIHandlers::SpinBox, &SimulatedJoint::m_damping, "Damping", "The damping amount. Higher values dampen the movement of the joint faster.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0001f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.001f)
                    ->DataElement(AZ::Edit::UIHandlers::SpinBox, &SimulatedJoint::m_gravityFactor, "Gravity factor", "The gravity multiplier on the regular world gravity of -9.81 units per second. A value of 2 will act like there is twice the amount of gravity.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 20.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                    ->DataElement(AZ::Edit::UIHandlers::SpinBox, &SimulatedJoint::m_friction, "Friction", "The friction factor, where 0 means it will slide over the colliding surface like ice, while a value of 1 makes it slide less on contact.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SimulatedJoint::m_pinned, "Pinned", "Pinned joints follow the original joint, so in a way they are pinned to a given skeletal joint. Unpinned joints can move freely away from the joint they are linked to. Root joints are always pinned.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &SimulatedJoint::GetPinnedOptionVisibility)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SimulatedJoint::m_autoExcludeGeometric, "Geometric auto exclude", "When enabled we will check whether the joint is inside the collider that is tested for automatic exclusion from collision detection. If not, we just use the list of colliders that are relevant.")
                    ->DataElement(AZ_CRC_CE("SimulatedJointColliderExclusionTags"), &SimulatedJoint::m_colliderExclusionTags, "Collider exclusions", "Ignore collision detection with the colliders inside this list.")
                    ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::HideChildren)
                    ->Attribute(AZ_CRC_CE("SimulatedObject"), &SimulatedJoint::GetSimulatedObject)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &SimulatedJoint::m_autoExcludeMode, "Auto exclude mode", "The mode used to automatically place colliders on the collision exclusion list. This option controls which colliders are automatically added to the exclusion list when they this joint is inside the collider.")
                    ->EnumAttribute(AutoExcludeMode::None, "None")
                    ->EnumAttribute(AutoExcludeMode::Self, "Self")
                    ->EnumAttribute(AutoExcludeMode::SelfAndNeighbors, "Self and neighbors")
                    ->EnumAttribute(AutoExcludeMode::All, "All");
            }
        }
    }

    SimulatedJoint::SimulatedJoint(SimulatedObject* object, size_t skeletonJointIndex)
        : m_object(object)
        , m_jointIndex(skeletonJointIndex)
    {
        // Calling this function to update the joint name of joint index.
        InitAfterLoading(object);
    }

    bool SimulatedJoint::InitAfterLoading(SimulatedObject* object)
    {
        if (!object)
        {
            return false;
        }

        SetSimulatedObject(object);

        // Both joint index and joint name has been serialized for validation purpose.
        // First check if the name is empty - if so, either you have just created this simulated joint by index, or it's coming from an older format (version 1).
        if (m_jointName.empty())
        {
            const Skeleton* skeleton = object->GetSimulatedObjectSetup()->GetActor()->GetSkeleton();
            if (m_jointIndex >= skeleton->GetNumNodes())
            {
                AZ_Warning("EMotionFX", false, "Cannot create a simulated joint with index %d because it is out of bounds.", m_jointIndex);
                return false;
            }
            const Node* node = skeleton->GetNode(m_jointIndex);
            if (!node)
            {
                AZ_Warning("EMotionFX", false, "Cannot find a valid emfx node with joint index %d.", m_jointIndex);
                return false;
            }
            m_jointName = node->GetName();
            return true;
        }

        // If the joint name has been set before calling this function, validate the joint index to see if it is still valid based on the name.
        const Node* node = object->GetSimulatedObjectSetup()->GetActor()->GetSkeleton()->FindNodeByName(m_jointName.c_str());
        if (!node)
        {
            AZ_Warning("EMotionFX", false, "Cannot find a valid emfx node with joint name %s. If you changed the name of the skeleton, you have to re-add the node in the simulated object setup.", m_jointName.c_str());
            return false;
        }
        else
        {
            if (node->GetNodeIndex() != m_jointIndex)
            {
                AZ_Warning("EMotionFX", false, "Detected changes in the skeleton hierachy. Joint %s index has updated from %d to %d.", m_jointName.c_str(), m_jointIndex, node->GetNodeIndex());
                m_jointIndex = node->GetNodeIndex();
            }
        }

        return true;
    }

    SimulatedJoint* SimulatedJoint::FindParentSimulatedJoint() const
    {
        const Actor* actor = m_object->GetSimulatedObjectSetup()->GetActor();
        const Skeleton* skeleton = actor->GetSkeleton();
        const Node* skeletonJoint = skeleton->GetNode(m_jointIndex);
        if (!skeletonJoint)
        {
            return nullptr;
        }
        const size_t parentIndex = skeletonJoint->GetParentIndex();
        return m_object->FindSimulatedJointBySkeletonJointIndex(parentIndex);
    }

    SimulatedJoint* SimulatedJoint::FindChildSimulatedJoint(size_t childIndex) const
    {
        const Actor* actor = m_object->GetSimulatedObjectSetup()->GetActor();
        const Skeleton* skeleton = actor->GetSkeleton();
        const Node* skeletonJoint = skeleton->GetNode(m_jointIndex);
        if (!skeletonJoint)
        {
            return nullptr;
        }
        const size_t childCount = skeletonJoint->GetNumChildNodes();
        size_t count = 0;
        for (size_t i = 0; i < childCount; ++i)
        {
            const size_t skeletonChildJointIndex = skeletonJoint->GetChildIndex(i);
            if (m_object->FindSimulatedJointBySkeletonJointIndex(skeletonChildJointIndex))
            {
                if (count == childIndex)
                {
                    return m_object->FindSimulatedJointBySkeletonJointIndex(skeletonChildJointIndex);
                }
                count++;
            }
        }
        return nullptr;
    }

    AZ::Outcome<size_t> SimulatedJoint::CalculateSimulatedJointIndex() const
    {
        const auto& joints = m_object->GetSimulatedJoints();
        auto resultIt = AZStd::find(joints.begin(), joints.end(), this);
        if (resultIt != joints.end())
        {
            return AZ::Success(static_cast<size_t>(AZStd::distance(joints.begin(), resultIt)));
        }

        return AZ::Failure();
    }

    size_t SimulatedJoint::CalculateNumChildSimulatedJoints() const
    {
        const Actor* actor = m_object->GetSimulatedObjectSetup()->GetActor();
        const Skeleton* skeleton = actor->GetSkeleton();
        const Node* skeletonJoint = skeleton->GetNode(m_jointIndex);
        if (!skeletonJoint)
        {
            return 0;
        }
        const size_t childCount = skeletonJoint->GetNumChildNodes();
        size_t count = 0;
        for (size_t i = 0; i < childCount; ++i)
        {
            const size_t childIndex = skeletonJoint->GetChildIndex(i);
            if (m_object->FindSimulatedJointBySkeletonJointIndex(childIndex))
            {
                count++;
            }
        }
        return count;
    }

    size_t SimulatedJoint::CalculateNumChildSimulatedJointsRecursive() const
    {
        size_t sum = 0;
        const size_t numChildren = CalculateNumChildSimulatedJoints();
        sum += numChildren;
        for (size_t i = 0; i < numChildren; ++i)
        {
            sum += FindChildSimulatedJoint(i)->CalculateNumChildSimulatedJointsRecursive();
        }
        return sum;
    }

    size_t SimulatedJoint::CalculateChildIndex() const
    {
        const Actor* actor = m_object->GetSimulatedObjectSetup()->GetActor();
        const SimulatedJoint* parentJoint = FindParentSimulatedJoint();
        if (parentJoint)
        {
            const Node* parentSkeletonJoint = actor->GetSkeleton()->GetNode(parentJoint->GetSkeletonJointIndex());
            if (!parentSkeletonJoint)
            {
                return 0;
            }
            const size_t numChildSkeletonJoints = parentSkeletonJoint->GetNumChildNodes();
            size_t childSimulatedJointIndex = 0;
            for (size_t i = 0; i < numChildSkeletonJoints; ++i)
            {
                size_t childJointIndex = parentSkeletonJoint->GetChildIndex(i);
                SimulatedJoint* childSimulatedJoint = m_object->FindSimulatedJointBySkeletonJointIndex(childJointIndex);
                if (childSimulatedJoint)
                {
                    if (childSimulatedJoint == this)
                    {
                        return childSimulatedJointIndex;
                    }

                    childSimulatedJointIndex++;
                }
            }
            AZ_Error("EMotionFX", false, "Joint should exist in the parent's child node list.");
            return numChildSkeletonJoints;
        }

        // If the simuated joint doesn't have a parent joint, it should be a root joint.
        size_t rootJointIndex = m_object->GetSimulatedRootJointIndex(this);
        AZ_Error("EMotionFX", rootJointIndex != InvalidIndex, "This joint should be a root joint.");
        return rootJointIndex;
    }

    bool SimulatedJoint::IsRootJoint() const
    {
        if (m_object)
        {
            return m_object->GetSimulatedRootJointIndex(this) != InvalidIndex;
        }

        return false;
    }

    AZ::Crc32 SimulatedJoint::GetPinnedOptionVisibility() const
    {
        return IsRootJoint() ? AZ::Edit::PropertyVisibility::Hide : AZ::Edit::PropertyVisibility::Show;
    }

    //-------------------------------------------------------------------------

    bool compare_simulated_joints(const SimulatedJoint* a, const SimulatedJoint* b)
    {
        return a->GetSkeletonJointIndex() < b->GetSkeletonJointIndex();
    }

    SimulatedObject::SimulatedObject(SimulatedObjectSetup* setup, const AZStd::string& objectName)
        : m_objectName(objectName)
        , m_simulatedObjectSetup(setup)
    {
    }

    SimulatedObject::~SimulatedObject()
    {
        Clear();
    }

    void SimulatedObject::Clear()
    {
        const size_t jointCount = m_joints.size();
        for (size_t i = 0; i < jointCount; ++i)
        {
            delete m_joints[i];
        }
        m_joints.clear();
        m_rootJoints.clear();
    }

    SimulatedJoint* SimulatedObject::FindSimulatedJointBySkeletonJointIndex(size_t skeletonJointIndex) const
    {
        for (SimulatedJoint* joint : m_joints)
        {
            if (joint->GetSkeletonJointIndex() == skeletonJointIndex)
            {
                return joint;
            }
        }

        return nullptr;
    }

    SimulatedJoint* SimulatedObject::GetSimulatedRootJoint(size_t rootIndex) const
    {
        return m_rootJoints[rootIndex];
    }

    size_t SimulatedObject::GetNumSimulatedRootJoints() const
    {
        return m_rootJoints.size();
    }

    size_t SimulatedObject::GetSimulatedRootJointIndex(const SimulatedJoint* rootJoint) const
    {
        const auto found = AZStd::find(m_rootJoints.begin(), m_rootJoints.end(), rootJoint);
        if (found != m_rootJoints.end())
        {
            return AZStd::distance(m_rootJoints.begin(), found);
        }

        return InvalidIndex;
    }

    void SimulatedObject::Reflect(AZ::ReflectContext* context)
    {
        SimulatedJoint::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<SimulatedObject>()
                ->Version(2)
                ->Field("objectName", &SimulatedObject::m_objectName)
                ->Field("joints", &SimulatedObject::m_joints)
                ->Field("gravityFactor", &SimulatedObject::m_gravityFactor)
                ->Field("stiffnessFactor", &SimulatedObject::m_stiffnessFactor)
                ->Field("dampingFactor", &SimulatedObject::m_dampingFactor)
                ->Field("colliderTags", &SimulatedObject::m_colliderTags);

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<SimulatedObject>("SimulatedObject", "Simulated object properties")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ_CRC_CE("SimulatedObjectName"), &SimulatedObject::m_objectName, "Object name", "Object name")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SimulatedObject::m_joints, "Joints to be simulated", "The numbers of joints that belong to this simulated object.")
                        ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::HideChildren)
                        ->Attribute(AZ::Edit::Attributes::ValueText, &SimulatedObject::GetJointsTextOverride)
                    ->DataElement(AZ::Edit::UIHandlers::SpinBox, &SimulatedObject::m_gravityFactor, "Gravity factor", "The gravity multiplier, which is a multiplier over the individual joint gravity values.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 20.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                    ->DataElement(AZ::Edit::UIHandlers::SpinBox, &SimulatedObject::m_stiffnessFactor, "Stiffness factor", "The stiffness multiplier, which is a multiplier over the individual joint stiffness values.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                    ->DataElement(AZ::Edit::UIHandlers::SpinBox, &SimulatedObject::m_dampingFactor, "Damping factor", "The damping multiplier, which is a multiplier over the individual joint damping values.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                    ->DataElement(AZ_CRC_CE("SimulatedObjectColliderTags"), &SimulatedObject::m_colliderTags, "Collides with", "The list of collider tags which define what to collide with.")
                        ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::HideChildren)
                        ;
            }
        }
    }

    bool SimulatedObject::ContainsSimulatedJoint(const SimulatedJoint* joint) const
    {
        return AZStd::find(m_joints.begin(), m_joints.end(), joint) != m_joints.end();
    }

    void SimulatedObject::InitAfterLoading(SimulatedObjectSetup* setup)
    {
        SetSimulatedObjectSetup(setup);

        for (auto itr = m_joints.begin(); itr != m_joints.end(); /*nothing*/)
        {
            // Remove the joints that failed to load.
            if (!(*itr)->InitAfterLoading(this))
            {
                delete *itr;
                itr = m_joints.erase(itr);
            }
            else
            {
                ++itr;
            }
        }

        SortJointList();
        BuildRootJointList();
    }

    SimulatedJoint* SimulatedObject::AddSimulatedJoint(size_t jointIndex)
    {
        AddSimulatedJoints({ jointIndex });
        return FindSimulatedJointBySkeletonJointIndex(jointIndex);
    }

    void SimulatedObject::AddSimulatedJoints(AZStd::vector<size_t> jointIndexes)
    {
        AZStd::sort(jointIndexes.begin(), jointIndexes.end());

        MergeAndMakeJoints(jointIndexes);

        BuildRootJointList();
    }

    void SimulatedObject::AddSimulatedJointAndChildren(size_t jointIndex)
    {
        AZStd::vector<size_t> jointsToAdd;
        AZStd::queue<size_t> toVisit;
        toVisit.emplace(jointIndex);

        const Skeleton* skeleton = m_simulatedObjectSetup->GetActor()->GetSkeleton();

        // Collect all the joint indices to add
        while (!toVisit.empty())
        {
            const size_t currentIndex = toVisit.front();
            toVisit.pop();

            jointsToAdd.emplace_back(currentIndex);

            const EMotionFX::Node* node = skeleton->GetNode(currentIndex);
            if (node)
            {
                const size_t childNodeCount = node->GetNumChildNodes();
                for (size_t i = 0; i < childNodeCount; ++i)
                {
                    const size_t childNodeIndex = node->GetChildIndex(i);
                    toVisit.emplace(childNodeIndex);
                }
            }
            else
            {
                AZ_Warning("EMotionFX", skeleton->GetNode(jointIndex), "Joint index %d is invalid", jointIndex);
            }
        }

        AZStd::sort(jointsToAdd.begin(), jointsToAdd.end());

        MergeAndMakeJoints(jointsToAdd);

        BuildRootJointList();
    }

    void SimulatedObject::MergeAndMakeJoints(const AZStd::vector<size_t>& jointsToAdd)
    {
        AZStd::vector<SimulatedJoint*> newJointList;

        // Merge the two lists, removing duplicates
        auto jointsToAddIt = jointsToAdd.begin();
        const auto jointsToAddEnd = jointsToAdd.end();
        auto currentJointsIt = m_joints.begin();
        const auto currentJointsEnd = m_joints.end();
        while (jointsToAddIt != jointsToAddEnd && currentJointsIt != currentJointsEnd)
        {
            if ((*currentJointsIt)->GetSkeletonJointIndex() == *jointsToAddIt)
            {
                newJointList.emplace_back(*currentJointsIt);
                ++currentJointsIt;
                ++jointsToAddIt;
            }
            else if ((*currentJointsIt)->GetSkeletonJointIndex() < *jointsToAddIt)
            {
                newJointList.emplace_back(*currentJointsIt);
                ++currentJointsIt;
            }
            else
            {
                newJointList.emplace_back(aznew SimulatedJoint(this, *jointsToAddIt));
                ++jointsToAddIt;
            }
        }
        while (jointsToAddIt != jointsToAddEnd)
        {
            newJointList.emplace_back(aznew SimulatedJoint(this, *jointsToAddIt));
            ++jointsToAddIt;
        }
        while (currentJointsIt != currentJointsEnd)
        {
            newJointList.emplace_back(*currentJointsIt);
            ++currentJointsIt;
        }

        m_joints = AZStd::move(newJointList);
    }

    AZStd::string SimulatedObject::GetColliderTag(int index) const
    {
        return m_colliderTags[index];
    }

    AZStd::string SimulatedObject::GetJointsTextOverride() const
    {
        const size_t jointCounts = m_joints.size();
        return AZStd::string::format("%zu joint%s selected", jointCounts, jointCounts == 1? "" : "s");
    }

    void SimulatedObject::RemoveSimulatedJoint(size_t jointIndex, bool removeChildren)
    {
        // If we order the joints storage so that the leaf node always comes late than its parent, we can do the remove in one iteration.
        bool removed = false;
        for (auto itr = m_joints.begin(); itr != m_joints.end(); ++itr)
        {
            SimulatedJoint* joint = (*itr);
            if (joint->GetSkeletonJointIndex() == jointIndex)
            {
                removed = true;

                // Remove the joint from the joint list.
                m_joints.erase(itr);

                // Check if the removed joint is in the root joint list.
                const auto found = AZStd::find(m_rootJoints.begin(), m_rootJoints.end(), joint);
                if (found != m_rootJoints.end())
                {
                    if (m_rootJoints.size() == 1)
                    {
                        // If this is the only root joint, build the root joint list from scratch.
                        BuildRootJointList();
                    }
                    else
                    {
                        m_rootJoints.erase(found);
                    }
                }

                delete joint;
                break;
            }
        }

        if (removed && removeChildren)
        {
            const Actor* actor = m_simulatedObjectSetup->GetActor();
            const EMotionFX::Node* node = actor->GetSkeleton()->GetNode(jointIndex);
            size_t childNodeCount = node->GetNumChildNodes();
            for (size_t i = 0; i < childNodeCount; ++i)
            {
                const size_t childNodeIndex = node->GetChildIndex(i);
                if (FindSimulatedJointBySkeletonJointIndex(childNodeIndex))
                {
                    RemoveSimulatedJoint(childNodeIndex, true);
                }
            }
        }
    }

    // Build a list of root joints under the object.
    void SimulatedObject::BuildRootJointList()
    {
        m_rootJoints.clear();
        AZStd::unordered_set<const SimulatedJoint*> seenJoints;
        AZStd::unordered_set<const SimulatedJoint*> toCheck;
        toCheck.insert(m_joints.begin(), m_joints.end());

        for (SimulatedJoint* joint : m_joints)
        {
            const SimulatedJoint* current = joint;
            AZStd::unordered_set<const SimulatedJoint*> currentParents;
            currentParents.emplace(current);
            toCheck.erase(toCheck.find(joint));

            while (current && current->GetSkeletonJointIndex() != InvalidIndex && !((seenJoints.find(current) != seenJoints.end()) || (toCheck.find(current) != toCheck.end())))
            {
                current = current->FindParentSimulatedJoint();
            }

            if (!current || current->GetSkeletonJointIndex() == InvalidIndex)
            {
                // We reached the top of the model without seeing any other
                // model index (or parent thereof) in modelIndices. This is a
                // root item of the selection.
                m_rootJoints.emplace_back(joint);
                seenJoints.emplace(joint);
            }
            else
            {
                // current is not a root, so we can mark all parents between
                // index..current as non-roots too
                seenJoints.insert(currentParents.begin(), currentParents.end());
            }
        }
    }

    void SimulatedObject::SortJointList()
    {
        AZStd::sort(m_joints.begin(), m_joints.end(), compare_simulated_joints);
    }

    const AZStd::vector<AZStd::string>& SimulatedObject::GetColliderTags() const
    {
        return m_colliderTags;
    }

    void SimulatedObject::SetColliderTags(const AZStd::vector<AZStd::string>& tags)
    {
        m_colliderTags = tags;
    }

    const AZStd::string& SimulatedObject::GetName() const
    {
        return m_objectName;
    }

    void SimulatedObject::SetName(const AZStd::string& newName)
    {
        m_objectName = newName;
    }

    float SimulatedObject::GetGravityFactor() const
    {
        return m_gravityFactor;
    }

    void SimulatedObject::SetGravityFactor(float newGravityFactor)
    {
        m_gravityFactor = newGravityFactor;
    }

    float SimulatedObject::GetStiffnessFactor() const
    {
        return m_stiffnessFactor;
    }

    void SimulatedObject::SetStiffnessFactor(float newStiffnessFactor)
    {
        m_stiffnessFactor = newStiffnessFactor;
    }

    float SimulatedObject::GetDampingFactor() const
    {
        return m_dampingFactor;
    }

    void SimulatedObject::SetDampingFactor(float newDampingFactor)
    {
        m_dampingFactor = newDampingFactor;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    SimulatedObjectSetup::SimulatedObjectSetup(Actor* actor)
        : m_actor(actor)
    {
    }

    SimulatedObjectSetup::~SimulatedObjectSetup()
    {
        const size_t jointCount = m_simulatedObjects.size();
        for (size_t i = 0; i < jointCount; ++i)
        {
            delete m_simulatedObjects[i];
        }
        m_simulatedObjects.clear();
    }

    void SimulatedObjectSetup::Reflect(AZ::ReflectContext* context)
    {
        SimulatedObject::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<SimulatedObjectSetup>()
                ->Version(1)
                ->Field("simulatedObjects", &SimulatedObjectSetup::m_simulatedObjects);

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<SimulatedObjectSetup>("SimulatedObjectSetup", "Simulated object setup properties")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);
            }
        }
    }

    SimulatedObject* SimulatedObjectSetup::AddSimulatedObject(const AZStd::string& objectName)
    {
        SimulatedObject* object = aznew SimulatedObject(this, objectName);
        m_simulatedObjects.emplace_back(object);
        if (objectName.empty())
        {
            const AZStd::string name = "Simulated Object " + AZStd::to_string(GetNumSimulatedObjects());
            object->SetName(name.c_str());
        }
        return object;
    }

    SimulatedObject* SimulatedObjectSetup::InsertSimulatedObjectAt(size_t index)
    {
        SimulatedObject* newObject = aznew SimulatedObject(this);
        m_simulatedObjects.insert(m_simulatedObjects.begin() + index, newObject);
        return newObject;
    }

    void SimulatedObjectSetup::RemoveSimulatedObject(size_t objectIndex)
    {
        if (m_simulatedObjects.empty() || m_simulatedObjects.size() <= objectIndex)
        {
            return;
        }

        delete m_simulatedObjects[objectIndex];
        m_simulatedObjects.erase(m_simulatedObjects.begin() + objectIndex);
    }

    SimulatedObject* SimulatedObjectSetup::GetSimulatedObject(size_t index) const
    {
        if (index < GetNumSimulatedObjects())
        {
            return m_simulatedObjects[index];
        }

        return nullptr;
    }

    SimulatedObject* SimulatedObjectSetup::FindSimulatedObjectByJoint(const SimulatedJoint* joint) const
    {
        for (SimulatedObject* object : m_simulatedObjects)
        {
            if (object->ContainsSimulatedJoint(joint))
            {
                return object;
            }
        }

        return nullptr;
    }

    SimulatedObject* SimulatedObjectSetup::FindSimulatedObjectByName(const char* name) const
    {
        auto found = AZStd::find_if(m_simulatedObjects.begin(), m_simulatedObjects.end(), [name](const SimulatedObject* object) {
            return object->GetName() == name;
        });
        return (found != m_simulatedObjects.end()) ? *found : nullptr;
    }

    bool SimulatedObjectSetup::IsSimulatedObjectNameUnique(const AZStd::string& newNameCandidate, const SimulatedObject* checkedSimulatedObject) const
    {
        for (const SimulatedObject* simulatedObject : m_simulatedObjects)
        {
            if (checkedSimulatedObject != simulatedObject && simulatedObject->GetName() == newNameCandidate)
            {
                return false;
            }
        }

        return true;
    }

    AZ::Outcome<size_t> SimulatedObjectSetup::FindSimulatedObjectIndex(const SimulatedObject* object) const
    {
        const size_t objectCount = GetNumSimulatedObjects();
        for (size_t i = 0; i < objectCount; ++i)
        {
            if (object == m_simulatedObjects[i])
            {
                return AZ::Success(i);
            }
        }

        return AZ::Failure();
    }

    void SimulatedObjectSetup::InitAfterLoad(Actor* actor)
    {
        m_actor = actor;
        for (SimulatedObject* object : m_simulatedObjects)
        {
            object->InitAfterLoading(this);
        }
    }

    AZStd::shared_ptr<SimulatedObjectSetup> SimulatedObjectSetup::Clone(Actor* newActor) const
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (!serializeContext)
        {
            AZ_Error("EMotionFX", false, "Can't get serialize context from component application.");
            return nullptr;
        }

        AZStd::vector<AZ::u8> buffer;
        AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8>> stream(&buffer);
        const bool serializeResult = AZ::Utils::SaveObjectToStream<EMotionFX::SimulatedObjectSetup>(stream, AZ::ObjectStream::ST_BINARY, this, serializeContext);
        if (serializeResult)
        {
            AZ::ObjectStream::FilterDescriptor loadFilter(nullptr, AZ::ObjectStream::FILTERFLAG_IGNORE_UNKNOWN_CLASSES);
            SimulatedObjectSetup* clone = AZ::Utils::LoadObjectFromBuffer<EMotionFX::SimulatedObjectSetup>(buffer.data(), buffer.size(), serializeContext, loadFilter);
            clone->InitAfterLoad(newActor);
            return AZStd::shared_ptr<SimulatedObjectSetup>(clone);
        }

        return nullptr;
    }
} // namespace EMotionFX
