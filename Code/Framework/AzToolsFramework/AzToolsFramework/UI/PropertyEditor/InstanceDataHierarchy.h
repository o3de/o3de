/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef PROPERTY_EDITOR_INSTANCE_DATA_HIERARCHY_H
#define PROPERTY_EDITOR_INSTANCE_DATA_HIERARCHY_H

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/RTTI/AttributeReader.h>

#include <algorithm>

namespace AzToolsFramework
{
    using DynamicEditDataProvider = AZStd::function<const AZ::Edit::ElementData*(const void* /*objectPtr*/, const AZ::SerializeContext::ClassData* /*classData*/)>;

    class InstanceDataHierarchy;
    class ComponentEditor;
    /*
     * InstanceDataNode contains mapping of a class' structure
     * to a list of instances of such class for editing purposes.
     * InstanceDataNodes are generated when an InstanceDatahierarchy
     * is built.
     * Once data
     */
    class InstanceDataNode
    {
        friend class InstanceDataHierarchy;
        template<typename WrappedType>
        friend class RpePropertyHandlerWrapper;

    public:
        typedef AZStd::list<InstanceDataNode>   NodeContainer;
        AZ_CLASS_ALLOCATOR(InstanceDataNode, AZ::PoolAllocator)

        /// Field address structure, which is a string of the 64-bit addresses of each ancestor along the hierarchy.
        using Identifier = AZ::u64;
        static const Identifier InvalidIdentifier = static_cast<Identifier>(~0);
        using Address = AZStd::vector<Identifier>;

        /**
         * Comparison flags set on nodes as deltas are detected versus the comparison instance, if set.
         */
        enum class ComparisonFlags : AZ::u32
        {
            None       = 0,
            New        = (1 << 0),  ///< Node is new to the root instance (not found in comparison instance).
            Differs    = (1 << 1),  ///< Node exists in both the comparison and root instances, but values differ.
            Removed    = (1 << 2),  ///< Node exists in the source hierarchy, but not the target.
        };

        InstanceDataNode()
            : m_parent(nullptr)
            , m_root(nullptr)
            , m_classData(nullptr)
            , m_classElement(nullptr)
            , m_elementEditData(nullptr)
            , m_context(nullptr)
            , m_comparisonFlags(0)
            , m_comparisonNode(nullptr)
            , m_matched(false)
            , m_identifier(InvalidIdentifier)
            , m_groupElementData(nullptr)
        {
        }

        /// Read a value into a variable of type T. Returns true if values read are the same across all instances in this node.
        template<class T>
        bool Read(T& value);

        bool ReadRaw(void*& valuePtr, AZ::TypeId valueType);

        /// Write a value to the node (same value is written to all instances regardless of their original value).
        template<class T>
        void Write(const T& value);

        void WriteRaw(const void* valuePtr, AZ::TypeId valueType);

        /// Check if have more than one instance for this node.
        bool    IsMultiInstance() const             { return m_instances.size() > 1; }
        size_t  GetNumInstances() const             { return m_instances.size(); }
        bool    HasInstances() const                { return !m_instances.empty(); }
        void*   GetInstance(size_t idx) const;
        void**  GetInstanceAddress(size_t idx) const;
        void*   FirstInstance() const { return GetInstance(0); }

        InstanceDataNode*                           GetRoot() const                 { return m_root; }
        InstanceDataNode*                           GetParent() const               { return m_parent; }
        NodeContainer&                              GetChildren()                   { return m_children; }
        const NodeContainer&                        GetChildren() const             { return m_children; }

        const AZ::SerializeContext::ClassData*      GetClassMetadata() const        { return m_classData; }
        const AZ::SerializeContext::ClassElement*   GetElementMetadata() const      { return m_classElement; }
        const AZ::Edit::ElementData*                GetElementEditMetadata() const  { return m_elementEditData; }
        const AZ::Edit::ElementData*                GetGroupElementMetadata() const { return m_groupElementData; }

        AZ::SerializeContext*                       GetSerializeContext() const     { return m_context; }

        /// Flags management when using a comparison instance.
        void MarkNewVersusComparison();                     ///< This node does not exist in the comparison instance's hierarchy.
        void MarkDifferentVersusComparison();               ///< This node differs from that of the comparison instance.
        void MarkRemovedVersusComparison();                 ///< This node does not exist in the target hierarchy.
        void ClearComparisonData();                         ///< Clear comparison flags (for re-computation).
        bool IsNewVersusComparison() const;                 ///< Has this node been flagged as new vs. the comparison instance?
        bool IsDifferentVersusComparison() const;           ///< Has this node been flagged as different from the comparison instance?
        bool IsRemovedVersusComparison() const;             ///< Is this node not in the comparison instance?
        const InstanceDataNode* GetComparisonNode() const;  ///< Retrieves the corresopnding node in the comparison hierarchy, if relevant.
        bool HasChangesVersusComparison(bool includeChildren) const;    /// Has this node (or children if specified) changed in any way from their comparison node
        
        /*
        //@{ Send notification for read/write events (in specific order)
        // NLawson:  Disabling this until we know what to do about it
        //           the user currently gets to poke values into members in response to change notification, or whenever
        //           so we can't guarintee that these are called.  I'd rather have them not called at all, than
        //      a) have the user use the honor system to remember to call them or;
        //      b) called sometimes and sometimes not

        void ReadStart();
        void ReadEnd();
        void WriteStart();
        void WriteEnd();
        //@}
        */

        typedef AZStd::function<const AZ::SerializeContext::ClassData* (const AZ::Uuid& classId, const AZ::Uuid& typeId, AZ::SerializeContext* context)> SelectClassCallback;
        typedef AZStd::function<bool (void* dataPtr, const AZ::SerializeContext::ClassElement* classElement, bool noDefaultData, AZ::SerializeContext* context)> FillDataClassCallback;
        typedef AZStd::function<void (const AZ::Uuid& classId)> CustomElementCreationHandler;

        bool CreateContainerElement(const SelectClassCallback& selectClass, const FillDataClassCallback& fillData);

        bool ChildMatchesAddress(const InstanceDataNode::Address& elementAddress) const;

        /// Calculate full hierarchical address of this node, which is a vector of node-local addresses
        /// from the node's identifier up to the top of the hierarchy (Address = {[node] [nodeparent] ... [root]})
        Address ComputeAddress() const;

        /// Check the element edit data, class element, and class data for the specified attribute
        AZ::Edit::Attribute* FindAttribute(AZ::Edit::AttributeId nameCrc) const;

        /// Read the value T of the specified attribute into value if it exists and shares the same value across all instances
        template <class T>
        bool ReadAttribute(AZ::Edit::AttributeId nameCrc, T& value, bool readChildAttributes = false) const;

    protected:
        typedef AZStd::vector<void*> InstanceArray;

        InstanceArray                               m_instances;        // A list of pointers to instances mapped by this node
        InstanceDataNode*                           m_parent;
        InstanceDataNode*                           m_root;
        NodeContainer                               m_children;
        const AZ::SerializeContext::ClassData*      m_classData;
        const AZ::SerializeContext::ClassElement*   m_classElement;
        const AZ::Edit::ElementData*                m_elementEditData;
        AZ::SerializeContext*                       m_context;
        AZ::u32                                     m_comparisonFlags;
        const InstanceDataNode*                     m_comparisonNode;
        bool                                        m_matched;          // true if this node was matched across all instances, used internally when the hierarchy is built.
        Identifier                                  m_identifier;       // Local identifier for this node (name crc, or persistent Id / index among siblings in container case).
        const AZ::Edit::ElementData*                m_groupElementData; // Group data for this item

    private:
        template<class T>
        void CheckCast(const T* castType);
    };

    template<typename T>
    inline void InstanceDataNode::CheckCast(const T* castType)
    {
        AZ_UNUSED(castType);
#if defined(AZ_COMPILER_MSVC)
        AZ_Assert(castType, "Could not cast from the existing typeid %s to the actual typeid %s required.", GetClassMetadata()->m_typeId.ToString<AZStd::string>().c_str(), AZ::SerializeTypeInfo<T>::GetUuid().ToString<AZStd::string>().c_str());
#else
        AZ_Assert(castType, "Could not cast from the existing typeid %s to the actual typeid %s required.", GetClassMetadata()->m_typeId.template ToString<AZStd::string>().c_str(), AZ::SerializeTypeInfo<T>::GetUuid().template ToString<AZStd::string>().c_str());
#endif // defined(AZ_COMPILER_MSVC)
    }

    template<class T>
    bool InstanceDataNode::Read(T& value)
    {
        const T* firstInstanceCast = static_cast<T*>(GetSerializeContext()->DownCast(FirstInstance(), GetClassMetadata()->m_typeId, AZ::SerializeTypeInfo<T>::GetUuid()));
        CheckCast(firstInstanceCast);

        value = *firstInstanceCast;

        // check all instance values are the same
        return std::all_of(m_instances.begin(), m_instances.end(), [this, firstInstanceCast](void* instance)
        {
            T* instanceCast = static_cast<T*>(GetSerializeContext()->DownCast(instance, GetClassMetadata()->m_typeId, AZ::SerializeTypeInfo<T>::GetUuid()));
            CheckCast(instanceCast);

            return *instanceCast == *firstInstanceCast;
        });
    }

    template<class T>
    void InstanceDataNode::Write(const T& value)
    {
        for (void* instance : m_instances)
        {
            T* instanceCast = static_cast<T*>(GetSerializeContext()->DownCast(instance, GetClassMetadata()->m_typeId, AZ::SerializeTypeInfo<T>::GetUuid()));
            CheckCast(instanceCast);

            *instanceCast = value;
        }
    }

    template <class T>
    bool InstanceDataNode::ReadAttribute(AZ::Edit::AttributeId nameCrc, T& value, bool readChildAttributes) const
    {
        AZ::Edit::Attribute* attribute = InstanceDataNode::FindAttribute(nameCrc);
        if (readChildAttributes)
        {
            if (attribute && !attribute->m_describesChildren)
            {
                attribute = nullptr;
            }
        }
        else if (!attribute || attribute->m_describesChildren)
        {
            return m_parent ? m_parent->ReadAttribute(nameCrc, value, true) : false;
        }

        if (attribute)
        {
            T lastValue{};
            T newValue{};
            for (size_t i = 0, numInstances = GetNumInstances(); i < numInstances; ++i)
            {
                AZ::AttributeReader reader(GetInstance(i), attribute);
                // Bail if we fail to read the attribute
                if (!reader.Read<T>(newValue))
                {
                    return false;
                }

                // Bail if our instances disagree about the value
                if (i != 0 && newValue != lastValue)
                {
                    return false;
                }

                lastValue = newValue;
            }

            value = newValue;
            return true;
        }
        return false;
    }

    /*
     * InstanceDataHierarchy contains a hierarchy of InstanceDataNodes.
     * It supports mapping of multiple root instances.
     * On build, it produces an intersecting set of the hierarchy of
     * all the root instances.
     * To generate the hierarchy:
     *  1) Add instances to the hierarchy by calling AddRootInstance().
     *  2) Generate the hierarchy by calling Build().
     *  3) Call GetRootNode() to retrieve the resulting root node. If the
     *     return value is NULL, no intersecting hierarchy was found (root types differ).
     */
    class InstanceDataHierarchy
        : public InstanceDataNode
    {
    public:
        AZ_CLASS_ALLOCATOR(InstanceDataHierarchy, AZ::PoolAllocator)

        InstanceDataHierarchy();

        enum Flags
        {
            None = 0x00,
            IgnoreKeyValuePairs = 0x01, // Don't collapse key/value pairs into single entries (useful to e.g. allow key editing)
        };

        void                AddRootInstance(void* instance, const AZ::Uuid& classId);
        void                AddComparisonInstance(void* instance, const AZ::Uuid& classId);
        bool                ContainsRootInstance(const void* instance) const;

        template<typename T>
        void AddRootInstance(T* instance)
        {
            AddRootInstance(instance, instance->RTTI_GetType());
        }

        template<typename T>
        void AddComparisonInstance(T* instance)
        {
            AddComparisonInstance(instance, instance->RTTI_GetType());
        }

        InstanceDataNode*   GetRootNode()   { return m_matched ? this : NULL; }

        /// Sets flags that can mutate building behavior.
        /// @see InstanceDataHierarchy::Flags
        void SetBuildFlags(AZ::u8 flags);

        void FixupEditData()
        {
            // if we ended up with anything to display, we need to do another pass to fix up container element nodes.
            if (m_matched)
            {
                FixupEditData(this, 0);
            }
        };

        /// Builds the intersecting hierarchy using all the root instances added.
        /// If a comparison instance is set, nodes will also be flagged based on detected deltas (\ref ComparisonFlags).
        void Build(AZ::SerializeContext* sc, unsigned int accessFlags, DynamicEditDataProvider dynamicEditDataProvider = DynamicEditDataProvider(), ComponentEditor* editorParent = nullptr);

        /// Re-compares root instance against specified Compare instance and updates node flags accordingly.
        /// \return true if all instances match the comparison instance.
        bool RefreshComparisonData(unsigned int accessFlags, DynamicEditDataProvider dynamicEditDataProvider);

        /// Callback to receive notification of nodes found in the target hierarchy, but not the source hierarchy.
        typedef AZStd::function<void(InstanceDataNode* /*targetNode*/, AZStd::vector<AZ::u8>& /*data*/)> NewNodeCB;

        /// Callback to receive notification of nodes found in the source hierarchy, but not the target hierarchy.
        typedef AZStd::function<void(const InstanceDataNode* /*sourceNode*/, InstanceDataNode* /*targetNodeParent*/)> RemovedNodeCB;

        /// Callback to receive notification of nodes found in both hierarchies, but with differing values.
        typedef AZStd::function<void(const InstanceDataNode* /*sourceNode*/, InstanceDataNode* /*targetNode*/, AZStd::vector<AZ::u8>& /*sourceData*/, AZStd::vector<AZ::u8>& /*targetData*/)> ChangedNodeCB;

        /// Function to compare two value nodes.
        /// Nodes are guaranteed to be valid, and to have the same class types.
        /// @return True if nodes equivalent, false otherwise.
        typedef AZStd::function<bool(const InstanceDataNode* /*sourceNode*/, const InstanceDataNode* /*targetNode*/)> ValueComparisonFunction;

        /// The default ValueComparisonFunction.
        /// Compares values using the SerializeContext.
        static bool DefaultValueComparisonFunction(const InstanceDataNode* sourceNode, const InstanceDataNode* targetNode);

        /// Set a custom ValueComparisonFunction.
        void SetValueComparisonFunction(const ValueComparisonFunction& function);

        /// Locate a node by full address.
        InstanceDataNode* FindNodeByAddress(const InstanceDataNode::Address& address) const;

        /// Locate a node by partial address (bfs to find closest match)
        InstanceDataNode* FindNodeByPartialAddress(const InstanceDataNode::Address& address) const;

        /// Utility to compare two instance hierarchies.
        /// When SetComparisonInstance() is used, this utility is internally invoked in order to mark nodes
        /// as new, removed, or overridden.
        static void CompareHierarchies(const InstanceDataNode* sourceNode, InstanceDataNode* targetNode,
            const ValueComparisonFunction& valueComparisonFunction,
            AZ::SerializeContext* context,
            NewNodeCB newNodeCallback,
            RemovedNodeCB removedNodeCallback,
            ChangedNodeCB changedNodeCallback);

        /// Callback to receive notification of container child nodes found in the target hierarchy, but not the source hierarchy that will be removed.
        typedef AZStd::function<void(InstanceDataNode* /*targetNode*/)> ContainerChildNodeBeingRemovedCB;

        /// Callback to receive notification of container child nodes found in the source hierarchy, but not the target hierarchy that will be copied to target.
        typedef AZStd::function<void(const InstanceDataNode* /*sourceNode*/, InstanceDataNode* /*targetNodeParent*/)> ContainerChildNodeBeingCreatedCB;

        /// Utility to copy data from one instance node to another.
        /// Internal checks are conducted to ensure types match.
        /// \param sourceNode Root of source hierarchy from which data will be read
        /// \param targetNode Root of matching target hierarchy to which data will be copied from the source
        /// \param context Serialize context; if not provided, default app context will be used
        /// \param containerChildNodeBeingRemovedCB External callback to invoke when an element is about to be removed
        /// \param containerChildNodeBeingCreatedCB External callback to invoke when an element is about to be added
        /// \param filterElementAddress Optional address of a container element node to isolate the operation to (ignore sibling elements that differ)
        ///                             This is used, for example, when pushing element adds/removals under a container, without pushing all elements/contents.
        /// \return true Status of the copy. True if copy was successful. Check debug output for failure reasons.
        static bool CopyInstanceData(
            const InstanceDataNode* sourceNode, 
            InstanceDataNode* targetNode, 
            AZ::SerializeContext* context = nullptr, 
            ContainerChildNodeBeingRemovedCB containerChildNodeBeingRemovedCB = nullptr, 
            ContainerChildNodeBeingCreatedCB containerChildNodeBeingCreatedCB = nullptr,
            const InstanceDataNode::Address& filterElementAddress = InstanceDataNode::Address());

    protected:
        struct SupplementalEditData
        {
            using AttributePtr = AZStd::unique_ptr<AZ::Edit::Attribute>;

            AZ::Edit::ElementData               m_editElementData;
            AZStd::string                       m_displayLabel;
            AZStd::vector<AttributePtr>         m_attributes;
        };
        typedef AZStd::list<SupplementalEditData>   SupplementalEditDataContainer;

        struct EditDataOverride
        {
            AZ::Edit::DynamicEditDataProvider*  m_override;
            InstanceDataNode*                   m_overridingNode;
            void*                               m_overridingInstance;
        };
        typedef AZStd::vector<EditDataOverride> EditDataOverrideStack;

        typedef AZStd::list<AZ::SerializeContext::ClassElement> SupplementalElementDataContainer;

        struct InstanceData
        {
            InstanceData()
                : m_instance(nullptr) {}
            InstanceData(void* instance, const AZ::Uuid& classId)
                : m_instance(instance)
                , m_classId(classId)
            {
            }

            void*       m_instance;
            AZ::Uuid    m_classId;
        };

        typedef AZStd::vector<InstanceData> InstanceDataArray;

        /// Utility to tidy up hierarchy after build is complete. Recursive
        /// \param node node to fix up
        /// \param siblingIdx index of this child in its parents children
        void FixupEditData(InstanceDataNode* node, int siblingIdx);

        void EnumerateUIElements(InstanceDataNode* node, DynamicEditDataProvider dynamicEditDataProvider);

        bool BeginNode(void* instance, const AZ::SerializeContext::ClassData* classData, const AZ::SerializeContext::ClassElement* classElement, DynamicEditDataProvider dynamicEditDataProvider);
        bool EndNode();

        static void CompareHierarchies(const InstanceDataNode* sourceNode, InstanceDataNode* targetNode,
            AZStd::vector<AZ::u8>& tempSourceBuffer,
            AZStd::vector<AZ::u8>& tempTargetBuffer,
            const ValueComparisonFunction& valueComparisonFunction,
            AZ::SerializeContext* context,
            NewNodeCB newNodeCallback,
            RemovedNodeCB removedNodeCallback,
            ChangedNodeCB changedNodeCallback);

        InstanceDataNode*                                       m_curParentNode;
        bool                                                    m_isMerging;
        bool                                                    m_nodeDiscarded;
        int                                                     m_childIndexOverride = -1;
        InstanceDataArray                                       m_rootInstances;            ///< Array with aggregated root instances.
        SupplementalElementDataContainer                        m_supplementalElementData;  ///< List of additional element data generated during traversal for elements.
        SupplementalEditDataContainer                           m_supplementalEditData;     ///< List of additional edit data generated during traversal for elements.
        EditDataOverrideStack                                   m_editDataOverrides;
        InstanceDataArray                                       m_comparisonInstances;      ///< Optional comparison instance for Override recognition.
        AZStd::vector<AZStd::unique_ptr<InstanceDataHierarchy>> m_comparisonHierarchies;    ///< Hierarchy representing comparison instance.
        ValueComparisonFunction                                 m_valueComparisonFunction;  ///< Customizable function for comparing value nodes.
        AZ::u8                                                  m_buildFlags = 0;           ///< Flags to customize behavior during Build.
    };
} // namespace AZ

#endif  // PROPERTY_EDITOR_INSTANCE_DATA_HIERARCHY_H
#pragma once
