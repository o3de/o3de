# Atom Bindless

## Introduction

Atom supports two styles of shader resource access: the traditional "direct binding" approach, and "bindless."
The former binding strategy is exposed via the `ShaderResourceGroup` (i.e. SRG) abstraction. In this binding mode, a shader resource is described in an `azsl` file (or `azsli` header) like so:

```hlsl
ShaderResourceGroupSemantic SRG_PerMaterial
{
    FrequencyId = 2;
};

ShaderResourceGroup MyMaterial : SRG_PerMaterial
{
    Texture2D<float> m_textureResource;
};

float4 PSMain(uint2 uv : TEXCOORD) : SV_Target0 {
    return MyMaterial::m_textureResource(uv);
}
```

When compiled, the SRG declaration is reflected to indicate that this shader requires a texture input associated with a `FrequencyId` of 2.
The compiler then assigns a binding slot and "space" associated with the input.
In C++, the SRG then allows the user to associate an `ImageView` with the `m_textureResource` input slot.
This slot location varies per-platform, and can be queried by name as part of the interface.
Other engines have other names for this sort of reflection and input mapping interface, but these concepts for resource binding are otherwise fairly universal.

## Why Bindless

In many circumstances, binding resources as above is either not possible, or scales poorly.
The primary reason to use bindless is because prior to a draw or dispatch, the set of resources needed by the shader are not known.
Consider the following cases:

### GPU-driven scene submission

When culling geometry from object bounding boxes, and composing index buffers from a compute shader in a meshlet rendering pipeline, each thread invocation in a compute shader needs to programmatically retrieve the mesh and meshlet metadata, and finally the index buffers to generate a list of vertex/meshlet indices that are ultimately visible.

Maintaining all the data above in giant contiguous buffers is a potential strategy, but as data streams in and out of the scene, fragmentation is inevitable. A much cleaner way to access the data is via bindless, with each thread invocation leveraging its dispatch ID to determine what buffers to pull.

### Terrain/Decals/Raytracing

In other scenarios, the raw or dispatch doesn't know what resources are needed at submission time. For example, a compute shader evaluating terrain materials may read the texels of a "material mask" texture, whose contents are then used to sample a different set of textures to blend between. A raytracing shader may need to do a similar query based on which triangle was hit per-ray. Decals might render based on data written out by a prior decal culling job (operating similar to a point-light cull).

In all these situations, its beneficial to have access to all the material resources used in the frame at once, and this isn't practical with the direct binding approach.

### Performance considerations

In addition to the use cases mentioned above, there is some overhead associated with direct resource binding, some of which is unique to Atom, some of which is general to all engines.

In particular, direct binding implies descriptor duplication. The same texture or resource is very likely needed in several places, across several draws or dispatches.
During shader compilation, the SRG layout is computed in isolation from all other shaders, so the final "root signature" (spelled "descriptor set layout" in Vulkan) is unlikely to match from shader to shader.
This means that very few descriptor ranges can actually be shared between draws and dispatches, even if the resources used are nearly identical.
Some engines manage this by enforcing a more "ossified" descriptor layout, but the mechanism provided for declaring shader resources in Atom is extremely flexible, so this isn't an option.

Another consideration is descriptor fragmentation. In D3D12 specifically, descriptors are suballocated from a large GPU-visible heap.
Resources declared using direct bindings are generally allocated as contiguous ranges within this heap.
This means that as draws and dispatches are invoked by the scene, contiguous chunks of the heap are suballocated and deallocated, fragmenting the heap over time.
Prior to the unified bindless solution offered in [PR #8410](https://github.com/o3de/o3de/pull/8410), this meant that larger resource arrays were allocated as large contiguous arrays, resulting in an accelerated fragmentation that would require frequent per-frame defragmentation as a result (along with the descriptor copying needed).
Bindless sidesteps the issue of fragmentation altogether by introducing one level of indirection.
That is, resources accessed via bindless are done so by retriving first the resource index (through some external means), and then retrieving the resource from a shared descriptor array.
Because descriptors in this shared array do not need to be strictly contiguous (holes permitted), fragmentation is now a non-issue.

Yet another important consideration for bindless performance is draw and dispatch CPU submission time.
Draws and dispatches that use different sets of resources cannot generally use the same root signature and descriptor sets, so the command recording time is impacted by needing to swap root signatures, descriptors, and handle versioning.
With bindless, it's possible to coalesce many disjoint root signatures into a smaller group of root signature "families," which could even share many descriptor tables and constants.

Bindless is not strictly a win of course, so it's useful to consider what we give up when pursuing bindless as the primary resource binding strategy. First, we introduce one level of indirection which has an impact on resource access latency. In general, occupancy is the key mitigation strategy provided by the GPU, so in cases where the GPU is fully occupied, this latency-hit is not expected to be frame-impacting, but workloads with little to no occupancy will certainly see a longer critical path. An additional consideration is that the driver no longer "sees" the set of resources that will be used by a draw or dispatch, and so can't employ any preloading or code-gen tricks to accelerate the shader execution. Ultimately, the mitigation here is identical to the first case - high occupancy workloads are needed to hide latency. Probably the most tricky case to handle are cases where a shader is heavily VRAM bound, indicating that increased occupancy would hurt performance, but such cases should be handled reflexively, and of course, the direct binding strategy is always available.

In summary, however, bindless resource access is expected to have dramatic frame time impact, not necessarily due to the performance characteristics of bindless resource access itself, but because the flexibility it affords enables an entire class of GPU-driven rendering pipelines that would otherwise be impossible.

## Bindless Interface in O3DE

In `Bindless.azsli`, a shader resource group is defined with a set of resources, each type of which is associated with an unbounded array (e.g. `Texture2D m_Texture2D[]`).
These resources map to a global table containing descriptors that are _stable_ in memory. That is, for the lifetime of an image or buffer view descriptor, the descriptor's location in the heap will not change.
In C++, this location can be queried using `ImageView::GetReadIndex`, `ImageView::GetReadWriteIndex`, and similar methods on the `BufferView` interface.
These constants can be bound directly as root arguments (i.e. push constants), or authored in a buffer.

### D3D12 Implementation

In D3D12, descriptors are all suballocated from a single unified heap, and descriptor ranges are permitted to alias (i.e. a range of descriptors may contain descriptors referring to different resource types).
This heap is partitioned into a _static_ region and a _dynamic_ region.
The dynamic region is used for the direct binding approach, and for cases where contiguous descriptor ranges are needed.
As views are created, an associated descriptor is also created in the next available free-slot in the static region.
To access the static descriptors in D3D12, the root signature associates all unbounded arrays in the designated `Bindless` SRG to the static region.
All such ranges have the _same base offset_ and are completely overlapping to ensure the ranges can encompass all the resources that may be needed in the frame.

### Vulkan Implementation

In contrast to D3D12, Vulkan descriptors are allocated from a strongly-typed pool of descriptors.
Being strongly typed, these descriptors cannot alias, and so the indices assigned to each static descriptor is given a distinct offset based on descriptor type (i.e. a separate allocator is used per-type, instead of sharing a global allocator).
The `Bindless` SRG is associated with a single descriptor set, allocated from a singular descriptor pool (see `Vulkan/Code/Source/RHI/BindlessDescriptorPool.cpp`).
The descriptor bindings in the global layout request several feature bit capabilities:

- Partially bound (we don't want to have to write entries for all descriptors in the range before use)
- Update after bind (we need to be able to write entries after the set is bound in slots that we know are unused because we have CPU-side tracking)

#### Vulkan TODO

- The `VkDescriptorSetLayout` created in `BindlessDescriptorPool.cpp` doesn't leverage the existing `ShaderResourceGroup` abstraction due to differences in feature requirements that are incompatible with the existing unbounded array support. An attempt could be made to unify this, so that the layout of the `Bindless` SRG can be reflected from a pseudo-shader instead of hardcoded
- The bindings are all hardcoded to require `100,000` elements, but this is a gross overestimation of usage for many resource types (e.g. UAV textures, UAV buffers). To relax this requirement, the `PipelineLayout` compilation needs to change to adjust the unbounded array descriptor count estimation based on descriptor type.

### Metal Implementation

Metal currently has support for Bindless via two approaches. Approach 1 is the current implementation.
- Approach 1 (Unbounded arrays with an explicit limit of 100k) - We create one Argument buffer that encompasses all the bounded unbound arrays. Since the resource types can not alias like Dx12 we create separate regions for read only textures, read write textures, read only buffers and read write buffers. Each section gets enough space for 100k views. 
- Approach 2 (Unbounded array with no limit) - Assuming spirv-cross adds support for it we create an argument buffer per resource type (read only textures, read write textures, read only buffers, read write buffers) and we create a  root argument buffer which acts as a container for all argument buffers per bindless resource type. 
For both approaches when a view is created we add it to the global heap and at runtime we bind the appropriate AB as needed.

### Metal TODOs

- Add Sprir-v Cross support for approach 2 to work, once Sprir-v cross adds the support.
- Similar to Vulkan the bindings are all hardcoded to require `100,000` elements, but this is a gross overestimation of usage for many resource types (e.g. UAV textures, UAV buffers). To relax this requirement, the `PipelineLayout` compilation needs to change to adjust the unbounded array descriptor count estimation based on descriptor type.
- Need to enable bindless based on hardware support for Argument Buffers Tier 2 (only iOS).


### General TODOs

- A restriction exists that hardcodes the number of unbounded arrays in an SRG to `8`. This was originally `2` (one SRV and one UAV unbounded array), but in reality, there shouldn't be any restrction at all in the `Bindless` case specifically, since all ranges can overlap. Only in the unbounded-and-contiugous case (non-`Bindless` SRGs) should the original limit of `2` be honored (see `RHI.Reflect/DX12/PipelineLayoutDescriptor.h`)
- Make Bindless SRG layout data driven https://github.com/o3de/o3de/issues/13324
