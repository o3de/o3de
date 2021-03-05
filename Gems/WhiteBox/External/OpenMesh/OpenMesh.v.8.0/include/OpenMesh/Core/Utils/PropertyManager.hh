/* ========================================================================= *
 *                                                                           *
 *                               OpenMesh                                    *
 *           Copyright (c) 2001-2015, RWTH-Aachen University                 *
 *           Department of Computer Graphics and Multimedia                  *
 *                          All rights reserved.                             *
 *                            www.openmesh.org                               *
 *                                                                           *
 *---------------------------------------------------------------------------*
 * This file is part of OpenMesh.                                            *
 *---------------------------------------------------------------------------*
 *                                                                           *
 * Redistribution and use in source and binary forms, with or without        *
 * modification, are permitted provided that the following conditions        *
 * are met:                                                                  *
 *                                                                           *
 * 1. Redistributions of source code must retain the above copyright notice, *
 *    this list of conditions and the following disclaimer.                  *
 *                                                                           *
 * 2. Redistributions in binary form must reproduce the above copyright      *
 *    notice, this list of conditions and the following disclaimer in the    *
 *    documentation and/or other materials provided with the distribution.   *
 *                                                                           *
 * 3. Neither the name of the copyright holder nor the names of its          *
 *    contributors may be used to endorse or promote products derived from   *
 *    this software without specific prior written permission.               *
 *                                                                           *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS       *
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED *
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A           *
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER *
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,  *
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,       *
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR        *
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF    *
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING      *
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS        *
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.              *
 *                                                                           *
 * ========================================================================= */

#ifndef PROPERTYMANAGER_HH_
#define PROPERTYMANAGER_HH_

#include <OpenMesh/Core/System/config.h>
#include <OpenMesh/Core/Utils/HandleToPropHandle.hh>
#include <sstream>
#include <stdexcept>
#include <string>

namespace OpenMesh {

/**
 * This class is intended to manage the lifecycle of properties.
 * It also defines convenience operators to access the encapsulated
 * property's value.
 *
 * It is recommended to use the factory functions
 * makeTemporaryProperty(), getProperty(), and getOrMakeProperty()
 * to construct a PropertyManager, e.g.
 *
 * \code
 * {
 *     TriMesh mesh;
 *     auto visited = makeTemporaryProperty<VertexHandle, bool>(mesh);
 *
 *     for (auto vh : mesh.vertices()) {
 *         if (!visited[vh]) {
 *             visitComponent(mesh, vh, visited);
 *         }
 *     }
 *     // The property is automatically removed at the end of the scope
 * }
 * \endcode
 */
template<typename PROPTYPE, typename MeshT>
class PropertyManager {
#if (defined(_MSC_VER) && (_MSC_VER >= 1800)) || __cplusplus > 199711L || defined(__GXX_EXPERIMENTAL_CXX0X__)
    public:
        PropertyManager(const PropertyManager&) = delete;
        PropertyManager& operator=(const PropertyManager&) = delete;
#else
    private:
        /**
         * Noncopyable because there aren't no straightforward copy semantics.
         */
        PropertyManager(const PropertyManager&);

        /**
         * Noncopyable because there aren't no straightforward copy semantics.
         */
        PropertyManager& operator=(const PropertyManager&);
#endif

    public:
        /**
         * Constructor.
         *
         * Throws an \p std::runtime_error if \p existing is true and
         * no property named \p propname of the appropriate property type
         * exists.
         *
         * @param mesh The mesh on which to create the property.
         * @param propname The name of the property.
         * @param existing If false, a new property is created and its lifecycle is managed (i.e.
         * the property is deleted upon destruction of the PropertyManager instance). If true,
         * the instance merely acts as a convenience wrapper around an existing property with no
         * lifecycle management whatsoever.
         *
         * @see PropertyManager::createIfNotExists, makePropertyManagerFromNew,
         * makePropertyManagerFromExisting, makePropertyManagerFromExistingOrNew
         */
        PropertyManager(MeshT &mesh, const char *propname, bool existing = false) : mesh_(&mesh), retain_(existing), name_(propname) {
            if (existing) {
                if (!mesh_->get_property_handle(prop_, propname)) {
                    std::ostringstream oss;
                    oss << "Requested property handle \"" << propname << "\" does not exist.";
                    throw std::runtime_error(oss.str());
                }
            } else {
                mesh_->add_property(prop_, propname);
            }
        }

        PropertyManager() : mesh_(0), retain_(false) {
        }

        ~PropertyManager() {
            deleteProperty();
        }

        void swap(PropertyManager &rhs) {
            std::swap(mesh_, rhs.mesh_);
            std::swap(prop_, rhs.prop_);
            std::swap(retain_, rhs.retain_);
            std::swap(name_, rhs.name_);
        }

        static bool propertyExists(MeshT &mesh, const char *propname) {
            PROPTYPE dummy;
            return mesh.get_property_handle(dummy, propname);
        }

        bool isValid() const { return mesh_ != 0; }
        operator bool() const { return isValid(); }

        const PROPTYPE &getRawProperty() const { return prop_; }

        const std::string &getName() const { return name_; }

        MeshT &getMesh() const { return *mesh_; }

#if (defined(_MSC_VER) && (_MSC_VER >= 1800)) || __cplusplus > 199711L || defined(__GXX_EXPERIMENTAL_CXX0X__)
        /// Only for pre C++11 compatibility.

        typedef PropertyManager<PROPTYPE, MeshT> Proxy;

        /**
         * Move constructor. Transfers ownership (delete responsibility).
         */
        PropertyManager(PropertyManager &&rhs) : mesh_(rhs.mesh_), prop_(rhs.prop_), retain_(rhs.retain_), name_(rhs.name_) {
            rhs.retain_ = true;
        }

        /**
         * Move assignment. Transfers ownership (delete responsibility).
         */
        PropertyManager &operator=(PropertyManager &&rhs) {
            if (&rhs != this) {
                deleteProperty();
                mesh_ = rhs.mesh_;
                prop_ = rhs.prop_;
                retain_ = rhs.retain_;
                name_ = rhs.name_;
                rhs.retain_ = true;
            }
            return *this;
        }

        /**
         * Create a property manager for the supplied property and mesh.
         * If the property doesn't exist, it is created. In any case,
         * lifecycle management is disabled.
         *
         * @see makePropertyManagerFromExistingOrNew
         */
        static PropertyManager createIfNotExists(MeshT &mesh, const char *propname) {
            PROPTYPE dummy_prop;
            PropertyManager pm(mesh, propname, mesh.get_property_handle(dummy_prop, propname));
            pm.retain();
            return std::move(pm);
        }

        /**
         * Like createIfNotExists() with two parameters except, if the property
         * doesn't exist, it is initialized with the supplied value over
         * the supplied range after creation. If the property already exists,
         * this method has the exact same effect as the two parameter version.
         * Lifecycle management is disabled in any case.
         *
         * @see makePropertyManagerFromExistingOrNew
         */
        template<typename PROP_VALUE, typename ITERATOR_TYPE>
        static PropertyManager createIfNotExists(MeshT &mesh, const char *propname,
                const ITERATOR_TYPE &begin, const ITERATOR_TYPE &end,
                const PROP_VALUE &init_value) {
            const bool exists = propertyExists(mesh, propname);
            PropertyManager pm(mesh, propname, exists);
            pm.retain();
            if (!exists)
                pm.set_range(begin, end, init_value);
            return std::move(pm);
        }

        /**
         * Like createIfNotExists() with two parameters except, if the property
         * doesn't exist, it is initialized with the supplied value over
         * the supplied range after creation. If the property already exists,
         * this method has the exact same effect as the two parameter version.
         * Lifecycle management is disabled in any case.
         *
         * @see makePropertyManagerFromExistingOrNew
         */
        template<typename PROP_VALUE, typename ITERATOR_RANGE>
        static PropertyManager createIfNotExists(MeshT &mesh, const char *propname,
                const ITERATOR_RANGE &range, const PROP_VALUE &init_value) {
            return createIfNotExists(
                    mesh, propname, range.begin(), range.end(), init_value);
        }

        PropertyManager duplicate(const char *clone_name) {
            PropertyManager pm(*mesh_, clone_name, false);
            pm.mesh_->property(pm.prop_) = mesh_->property(prop_);
            return pm;
        }

        /**
         * Included for backwards compatibility with non-C++11 version.
         */
        PropertyManager move() {
            return std::move(*this);
        }

#else
        class Proxy {
            private:
                Proxy(MeshT *mesh_, PROPTYPE prop_, bool retain_, const std::string &name_) :
                    mesh_(mesh_), prop_(prop_), retain_(retain_), name_(name_) {}
                MeshT *mesh_;
                PROPTYPE prop_;
                bool retain_;
                std::string name_;

                friend class PropertyManager;
        };

        operator Proxy() {
            Proxy p(mesh_, prop_, retain_, name_);
            mesh_ = 0;
            retain_ = true;
            return p;
        }

        Proxy move() {
            return (Proxy)*this;
        }

        PropertyManager(Proxy p) : mesh_(p.mesh_), prop_(p.prop_), retain_(p.retain_), name_(p.name_) {}

        PropertyManager &operator=(Proxy p) {
            PropertyManager(p).swap(*this);
            return *this;
        }

        /**
         * Create a property manager for the supplied property and mesh.
         * If the property doesn't exist, it is created. In any case,
         * lifecycle management is disabled.
         *
         * @see makePropertyManagerFromExistingOrNew
         */
        static Proxy createIfNotExists(MeshT &mesh, const char *propname) {
            PROPTYPE dummy_prop;
            PropertyManager pm(mesh, propname, mesh.get_property_handle(dummy_prop, propname));
            pm.retain();
            return (Proxy)pm;
        }

        /**
         * Like createIfNotExists() with two parameters except, if the property
         * doesn't exist, it is initialized with the supplied value over
         * the supplied range after creation. If the property already exists,
         * this method has the exact same effect as the two parameter version.
         * Lifecycle management is disabled in any case.
         *
         * @see makePropertyManagerFromExistingOrNew
         */
        template<typename PROP_VALUE, typename ITERATOR_TYPE>
        static Proxy createIfNotExists(MeshT &mesh, const char *propname,
                const ITERATOR_TYPE &begin, const ITERATOR_TYPE &end,
                const PROP_VALUE &init_value) {
            const bool exists = propertyExists(mesh, propname);
            PropertyManager pm(mesh, propname, exists);
            pm.retain();
            if (!exists)
                pm.set_range(begin, end, init_value);
            return (Proxy)pm;
        }

        Proxy duplicate(const char *clone_name) {
            PropertyManager pm(*mesh_, clone_name, false);
            pm.mesh_->property(pm.prop_) = mesh_->property(prop_);
            return (Proxy)pm;
        }
#endif

        /**
         * \brief Disable lifecycle management for this property.
         *
         * If this method is called, the encapsulated property will not be deleted
         * upon destruction of the PropertyManager instance.
         */
        inline void retain(bool doRetain = true) {
            retain_ = doRetain;
        }

        /**
         * Access the encapsulated property.
         */
        inline PROPTYPE &operator* () {
            return prop_;
        }

        /**
         * Access the encapsulated property.
         */
        inline const PROPTYPE &operator* () const {
            return prop_;
        }

        /**
         * Enables convenient access to the encapsulated property.
         *
         * For a usage example see this class' documentation.
         *
         * @param handle A handle of the appropriate handle type. (I.e. \p VertexHandle for \p VPropHandleT, etc.)
         */
        template<typename HandleType>
        inline typename PROPTYPE::reference operator[] (const HandleType &handle) {
            return mesh_->property(prop_, handle);
        }

        /**
         * Enables convenient access to the encapsulated property.
         *
         * For a usage example see this class' documentation.
         *
         * @param handle A handle of the appropriate handle type. (I.e. \p VertexHandle for \p VPropHandleT, etc.)
         */
        template<typename HandleType>
        inline typename PROPTYPE::const_reference operator[] (const HandleType &handle) const {
            return mesh_->property(prop_, handle);
        }

        /**
         * Conveniently set the property for an entire range of values.
         *
         * Examples:
         * \code
         * MeshT mesh;
         * PropertyManager<VPropHandleT<double>, MeshT> distance(
         *     mesh, "distance.plugin-example.i8.informatik.rwth-aachen.de");
         * distance.set_range(
         *     mesh.vertices_begin(), mesh.vertices_end(),
         *     std::numeric_limits<double>::infinity());
         * \endcode
         * or
         * \code
         * MeshT::VertexHandle vh;
         * distance.set_range(
         *     mesh.vv_begin(vh), mesh.vv_end(vh),
         *     std::numeric_limits<double>::infinity());
         * \endcode
         *
         * @param begin Start iterator. Needs to dereference to HandleType.
         * @param end End iterator. (Exclusive.)
         * @param value The value the range will be set to.
         */
        template<typename HandleTypeIterator, typename PROP_VALUE>
        void set_range(HandleTypeIterator begin, HandleTypeIterator end,
                const PROP_VALUE &value) {
            for (; begin != end; ++begin)
                (*this)[*begin] = value;
        }

#if (defined(_MSC_VER) && (_MSC_VER >= 1800)) || __cplusplus > 199711L || defined(__GXX_EXPERIMENTAL_CXX0X__)
        template<typename HandleTypeIteratorRange, typename PROP_VALUE>
        void set_range(const HandleTypeIteratorRange &range,
                const PROP_VALUE &value) {
            set_range(range.begin(), range.end(), value);
        }
#endif

        /**
         * Conveniently transfer the values managed by one property manager
         * onto the values managed by a different property manager.
         *
         * @param begin Start iterator. Needs to dereference to HandleType. Will
         * be used with this property manager.
         * @param end End iterator. (Exclusive.) Will be used with this property
         * manager.
         * @param dst_propmanager The destination property manager.
         * @param dst_begin Start iterator. Needs to dereference to the
         * HandleType of dst_propmanager. Will be used with dst_propmanager.
         * @param dst_end End iterator. (Exclusive.)
         * Will be used with dst_propmanager. Used to double check the bounds.
         */
        template<typename HandleTypeIterator, typename PROPTYPE_2,
                 typename MeshT_2, typename HandleTypeIterator_2>
        void copy_to(HandleTypeIterator begin, HandleTypeIterator end,
                PropertyManager<PROPTYPE_2, MeshT_2> &dst_propmanager,
                HandleTypeIterator_2 dst_begin, HandleTypeIterator_2 dst_end) const {

            for (; begin != end && dst_begin != dst_end; ++begin, ++dst_begin) {
                dst_propmanager[*dst_begin] = (*this)[*begin];
            }
        }

        template<typename RangeType, typename PROPTYPE_2,
                 typename MeshT_2, typename RangeType_2>
        void copy_to(const RangeType &range,
                PropertyManager<PROPTYPE_2, MeshT_2> &dst_propmanager,
                const RangeType_2 &dst_range) const {
            copy_to(range.begin(), range.end(), dst_propmanager,
                    dst_range.begin(), dst_range.end());
        }

        /**
         * Copy the values of a property from a source range to
         * a target range. The source range must not be smaller than the
         * target range.
         *
         * @param prop_name Name of the property to copy. Must exist on the
         * source mesh. Will be created on the target mesh if it doesn't exist.
         *
         * @param src_mesh Source mesh from which to copy.
         * @param src_range Source range which to copy. Must not be smaller than
         * dst_range.
         * @param dst_mesh Destination mesh on which to copy.
         * @param dst_range Destination range.
         */
        template<typename RangeType, typename MeshT_2, typename RangeType_2>
        static void copy(const char *prop_name,
                MeshT &src_mesh, const RangeType &src_range,
                MeshT_2 &dst_mesh, const RangeType_2 &dst_range) {

            typedef OpenMesh::PropertyManager<PROPTYPE, MeshT> DstPM;
            DstPM dst(DstPM::createIfNotExists(dst_mesh, prop_name));

            typedef OpenMesh::PropertyManager<PROPTYPE, MeshT_2> SrcPM;
            SrcPM src(src_mesh, prop_name, true);

            src.copy_to(src_range, dst, dst_range);
        }

    private:
        void deleteProperty() {
            if (!retain_)
                mesh_->remove_property(prop_);
        }

    private:
        MeshT *mesh_;
        PROPTYPE prop_;
        bool retain_;
        std::string name_;
};

/** @relates PropertyManager
 *
 * Creates a new property whose lifetime is limited to the current scope.
 *
 * Used for temporary properties. Shadows any existing properties of
 * matching name and type.
 *
 * Example:
 * @code
 * PolyMesh m;
 * {
 *     auto is_quad = makeTemporaryProperty<FaceHandle, bool>(m);
 *     for (auto& fh : m.faces()) {
 *         is_quad[fh] = (m.valence(fh) == 4);
 *     }
 *     // The property is automatically removed from the mesh at the end of the scope.
 * }
 * @endcode
 *
 * @param mesh The mesh on which the property is created
 * @param propname (optional) The name of the created property
 * @tparam ElementT Element type of the created property, e.g. VertexHandle, HalfedgeHandle, etc.
 * @tparam T Value type of the created property, e.g., \p double, \p int, etc.
 * @tparam MeshT Type of the mesh. Can often be inferred from \p mesh
 * @returns A PropertyManager handling the lifecycle of the property
 */
template<typename ElementT, typename T, typename MeshT>
PropertyManager<typename HandleToPropHandle<ElementT, T>::type, MeshT>
makeTemporaryProperty(MeshT &mesh, const char *propname = "") {
    return PropertyManager<typename HandleToPropHandle<ElementT, T>::type, MeshT>(mesh, propname, false);
}

/** @relates PropertyManager
 *
 * Obtains a handle to a named property.
 *
 * Example:
 * @code
 * PolyMesh m;
 * {
 *     try {
 *         auto is_quad = getProperty<FaceHandle, bool>(m, "is_quad");
 *         // Use is_quad here.
 *     }
 *     catch (const std::runtime_error& e) {
 *         // There is no is_quad face property on the mesh.
 *     }
 * }
 * @endcode
 *
 * @pre Property with the name \p propname of matching type exists.
 * @throws std::runtime_error if no property with the name \p propname of
 * matching type exists.
 * @param mesh The mesh on which the property is created
 * @param propname The name of the created property
 * @tparam ElementT Element type of the created property, e.g. VertexHandle, HalfedgeHandle, etc.
 * @tparam T Value type of the created property, e.g., \p double, \p int, etc.
 * @tparam MeshT Type of the mesh. Can often be inferred from \p mesh
 * @returns A PropertyManager wrapping the property
 */
template<typename ElementT, typename T, typename MeshT>
PropertyManager<typename HandleToPropHandle<ElementT, T>::type, MeshT>
getProperty(MeshT &mesh, const char *propname) {
    return PropertyManager<typename HandleToPropHandle<ElementT, T>::type, MeshT>(mesh, propname, true);
}

/** @relates PropertyManager
 *
 * Obtains a handle to a named property if it exists or creates a new one otherwise.
 *
 * Used for creating or accessing permanent properties.
 *
 * Example:
 * @code
 * PolyMesh m;
 * {
 *     auto is_quad = getOrMakeProperty<FaceHandle, bool>(m, "is_quad");
 *     for (auto& fh : m.faces()) {
 *         is_quad[fh] = (m.valence(fh) == 4);
 *     }
 *     // The property remains on the mesh after the end of the scope.
 * }
 * {
 *     // Retrieve the property from the previous scope.
 *     auto is_quad = getOrMakeProperty<FaceHandle, bool>(m, "is_quad");
 *     // Use is_quad here.
 * }
 * @endcode
 *
 * @param mesh The mesh on which the property is created
 * @param propname The name of the created property
 * @tparam ElementT Element type of the created property, e.g. VertexHandle, HalfedgeHandle, etc.
 * @tparam T Value type of the created property, e.g., \p double, \p int, etc.
 * @tparam MeshT Type of the mesh. Can often be inferred from \p mesh
 * @returns A PropertyManager wrapping the property
 */
template<typename ElementT, typename T, typename MeshT>
PropertyManager<typename HandleToPropHandle<ElementT, T>::type, MeshT>
getOrMakeProperty(MeshT &mesh, const char *propname) {
    return PropertyManager<typename HandleToPropHandle<ElementT, T>::type, MeshT>::createIfNotExists(mesh, propname);
}

/** @relates PropertyManager
 * @deprecated Use makeTemporaryProperty() instead.
 *
 * Creates a new property whose lifecycle is managed by the returned
 * PropertyManager.
 *
 * Intended for temporary properties. Shadows any existing properties of
 * matching name and type.
 */
template<typename PROPTYPE, typename MeshT>
OM_DEPRECATED("Use makeTemporaryProperty instead.")
PropertyManager<PROPTYPE, MeshT> makePropertyManagerFromNew(MeshT &mesh, const char *propname)
{
    return PropertyManager<PROPTYPE, MeshT>(mesh, propname, false);
}

/** \relates PropertyManager
 * @deprecated Use getProperty() instead.
 *
 * Creates a non-owning wrapper for an existing mesh property (no lifecycle
 * management).
 *
 * Intended for convenient access.
 *
 * @pre Property with the name \p propname of matching type exists.
 * @throws std::runtime_error if no property with the name \p propname of
 * matching type exists.
 */
template<typename PROPTYPE, typename MeshT>
OM_DEPRECATED("Use getProperty instead.")
PropertyManager<PROPTYPE, MeshT> makePropertyManagerFromExisting(MeshT &mesh, const char *propname)
{
    return PropertyManager<PROPTYPE, MeshT>(mesh, propname, true);
}

/** @relates PropertyManager
 * @deprecated Use getOrMakeProperty() instead.
 *
 * Creates a non-owning wrapper for a mesh property (no lifecycle management).
 * If the given property does not exist, it is created.
 *
 * Intended for creating or accessing persistent properties.
 */
template<typename PROPTYPE, typename MeshT>
OM_DEPRECATED("Use getOrMakeProperty instead.")
PropertyManager<PROPTYPE, MeshT> makePropertyManagerFromExistingOrNew(MeshT &mesh, const char *propname)
{
    return PropertyManager<PROPTYPE, MeshT>::createIfNotExists(mesh, propname);
}

/** @relates PropertyManager
 * Like the two parameter version of makePropertyManagerFromExistingOrNew()
 * except it initializes the property with the specified value over the
 * specified range if it needs to be created. If the property already exists,
 * this function has the exact same effect as the two parameter version.
 *
 * Creates a non-owning wrapper for a mesh property (no lifecycle management).
 * If the given property does not exist, it is created.
 *
 * Intended for creating or accessing persistent properties.
 */
template<typename PROPTYPE, typename MeshT,
    typename ITERATOR_TYPE, typename PROP_VALUE>
PropertyManager<PROPTYPE, MeshT> makePropertyManagerFromExistingOrNew(
        MeshT &mesh, const char *propname,
        const ITERATOR_TYPE &begin, const ITERATOR_TYPE &end,
        const PROP_VALUE &init_value) {
    return PropertyManager<PROPTYPE, MeshT>::createIfNotExists(
            mesh, propname, begin, end, init_value);
}

/** @relates PropertyManager
 * Like the two parameter version of makePropertyManagerFromExistingOrNew()
 * except it initializes the property with the specified value over the
 * specified range if it needs to be created. If the property already exists,
 * this function has the exact same effect as the two parameter version.
 *
 * Creates a non-owning wrapper for a mesh property (no lifecycle management).
 * If the given property does not exist, it is created.
 *
 * Intended for creating or accessing persistent properties.
 */
template<typename PROPTYPE, typename MeshT,
    typename ITERATOR_RANGE, typename PROP_VALUE>
PropertyManager<PROPTYPE, MeshT> makePropertyManagerFromExistingOrNew(
        MeshT &mesh, const char *propname,
        const ITERATOR_RANGE &range,
        const PROP_VALUE &init_value) {
    return makePropertyManagerFromExistingOrNew<PROPTYPE, MeshT>(
            mesh, propname, range.begin(), range.end(), init_value);
}

} /* namespace OpenMesh */
#endif /* PROPERTYMANAGER_HH_ */
