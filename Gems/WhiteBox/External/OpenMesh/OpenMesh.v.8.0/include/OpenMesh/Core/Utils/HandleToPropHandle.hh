#ifndef HANDLETOPROPHANDLE_HH_
#define HANDLETOPROPHANDLE_HH_

#include <OpenMesh/Core/Mesh/Handles.hh>
#include <OpenMesh/Core/Utils/Property.hh>

namespace OpenMesh {

    template<typename ElementT, typename T>
    struct HandleToPropHandle {
    };

    template<typename T>
    struct HandleToPropHandle<OpenMesh::VertexHandle, T> {
        using type = OpenMesh::VPropHandleT<T>;
    };

    template<typename T>
    struct HandleToPropHandle<OpenMesh::HalfedgeHandle, T> {
        using type = OpenMesh::HPropHandleT<T>;
    };

    template<typename T>
    struct HandleToPropHandle<OpenMesh::EdgeHandle, T> {
        using type = OpenMesh::EPropHandleT<T>;
    };

    template<typename T>
    struct HandleToPropHandle<OpenMesh::FaceHandle, T> {
        using type = OpenMesh::FPropHandleT<T>;
    };

    template<typename T>
    struct HandleToPropHandle<void, T> {
        using type = OpenMesh::MPropHandleT<T>;
    };

} // namespace OpenMesh

#endif // HANDLETOPROPHANDLE_HH_