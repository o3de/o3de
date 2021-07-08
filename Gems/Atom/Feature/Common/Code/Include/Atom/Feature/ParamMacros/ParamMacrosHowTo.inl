/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

//
// This is a quick guide on how to use the parameter macros included in this folder
// 
// Part I: The Pattern
// 
// The aim of this macro system is to allow users to define parameters once and have the macros generate a bunch of boilerplate code for each defined parameter.
// While these macros makes things a little more complex upfront, the ability to add and remove variables in just one place without having to dig
// through multiple files every time is great for iteration speed and maintenance. To accomplish this, we use a pattern similar to the following example:
// 
// Let's say we want to specify class members in one place and have the members, getters and setters be auto generated.
// First, we define a macro MY_CLASS_PARAMS that uses a yet-undefined macro MAKE_PARAM(TYPE, NAME): 
// 
//      #define MY_CLASS_PARAMS             \
//          MAKE_PARAM(float, Width)        \
//          MAKE_PARAM(float, Height)       \
//          MAKE_PARAM(float, Depth)        \
// 
// Now we need only specify what MAKE_PARAM needs to do and then we can call the MY_CLASS_PARAMS macro to apply the logic to all defined params. For instance:
// 
//      #define MAKE_PARAM(TYPE, NAME) TYPE m_##NAME;
//      MY_CLASS_PARAMS
//      #undef MAKE_PARAM
// 
// This will generate the members as follows:
// 
//      float m_Width;
//      float m_Height;
//      float m_Depth;
// 
// Now elsewhere in the class definition we can generate getters and setters:
// 
//      #define MAKE_PARAM(TYPE, NAME)                          \
//          TYPE Get##NAME() { return m_##NAME; }               \
//          void Set##NAME(TYPE NAME) { m_##NAME = NAME; }      \
// 
//      MY_CLASS_PARAMS
// 
//      #undef MAKE_PARAM
// 
// This will generate the following:
// 
//      float GetWidth() { return m_Width; }
//      void SetWidth(float Width) { m_Width = Width; }
//      float GetHeight() { return m_Height; }
//      void SetHeight(float Width) { m_Height = Height; }
//      float GetDepth() { return m_Width; }
//      void SetDepth(float Depth) { m_Depth = Depth; }
// 
// If we wanted to generate further code for each variable, we need only redefine the MAKE_PARAM macro and invoke MY_CLASS_PARAMS
// 
// ___________________________________________________________________________________________________________________________________________________
// 
// Part II: Using .inl files
// 
// A key difference between the above example and our macro system is that we put macro definitions in .inl files so they can be easily reused 
// If we were to reuse the above example, it would look something like this:
// 
// GenerateMembers.inl
// 
//      #define MAKE_PARAM(TYPE, NAME) TYPE m_##NAME;
// 
// GenerateGettersAndSetters.inl
// 
//      #define MAKE_PARAM(TYPE, NAME)                          \
//          TYPE Get##NAME() { return m_##NAME; }               \
//          void Set##NAME(TYPE NAME) { m_##NAME = NAME; }      \
// 
// BoxParams.inl
// 
//          MAKE_PARAM(float, Width)        \
//          MAKE_PARAM(float, Height)       \
//          MAKE_PARAM(float, Depth)        \
// 
// CylinderParams.inl
// 
//          MAKE_PARAM(float, Radius)       \
//          MAKE_PARAM(float, Height)       \
// 
// Now we can use these .inl files to generate two classes, each with members, getters and setters
// 
//      class Box
//      {
//          // Auto-gen members from BoxParams.inl
//          #include<GenerateMembers.inl>
//          #include<BoxParams.inl>
//          #undef MAKE_PARAM
//      
//          // Auto-gen getters and setters from BoxParams.inl
//          #include<GenerateGettersAndSetters.inl>
//          #include<BoxParams.inl>
//          #undef MAKE_PARAM
//      }
// 
//      class Cylinder
//      {
//          // Auto-gen members from CylinderParams.inl
//          #include<GenerateMembers.inl>
//          #include<CylinderParams.inl>
//          #undef MAKE_PARAM
//      
//          // Auto-gen getters and setters from CylinderParams.inl
//          #include<GenerateGettersAndSetters.inl>
//          #include<CylinderParams.inl>
//          #undef MAKE_PARAM
//      }
// 
// This will result in the following code:
// 
//      class Box
//      {
//          // Auto-gen members from BoxParams.inl
//          float m_Width;
//          float m_Height;
//          float m_Depth;
//      
//          // Auto-gen getters and setters from BoxParams.inl
//          float GetWidth() { return m_Width; }
//          void SetWidth(float Width) { m_Width = Width; }
//          float GetHeight() { return m_Height; }
//          void SetHeight(float Width) { m_Height = Height; }
//          float GetDepth() { return m_Width; }
//          void SetDepth(float Depth) { m_Depth = Depth; }
//      }
//
//      class Cylinder
//      {
//          // Auto-gen members from CylinderParams.inl
//          float m_Radius;
//          float m_Height;
//      
//          // Auto-gen getters and setters from CylinderParams.inl
//          float GetRadius() { return m_Radius; }
//          void SetRadius(float Radius) { m_Radius = Raidus; }
//          float GetHeight() { return m_Height; }
//          void SetHeight(float Width) { m_Height = Height; }
//      }
// 
// As you can see, this macro pattern allows us to add a member to Box or Cylinder by adding a single line to BoxParams.inl or CylinderParams.inl
// Because of the number of classes an boiler plate code involved in creating Open 3D Engine Component, this macro system allows us to change one line
// in one file instead of changing over a dozens of lines in half a dozen files.
// 
// ___________________________________________________________________________________________________________________________________________________
// 
// Part III: Using the Macros for Post Process members
// 
// If you want to create a new post process, you can create a .inl file (see DepthOfFieldParams.inl for an example) and declare members using the macros below:
//
//      #define AZ_GFX_BOOL_PARAM(Name, MemberName, DefaultValue)
//      #define AZ_GFX_FLOAT_PARAM(Name, MemberName, DefaultValue)
//      #define AZ_GFX_UINT32_PARAM(Name, MemberName, DefaultValue)
//      #define AZ_GFX_VEC2_PARAM(Name, MemberName, DefaultValue)
//      #define AZ_GFX_VEC3_PARAM(Name, MemberName, DefaultValue)
//      #define AZ_GFX_VEC4_PARAM(Name, MemberName, DefaultValue)
//
// Where:
// Name - The name of the param that will be used for reflection and appended to setters and getters, for example Width
// MemberName - The name of the member defined inside your class, for example m_width
// DefaultValue - The default value that the member will be statically initialized to, for example 1.0f
// BOOL, FLOAT, UINT32, VEC2 etc. all designate the type of the param you are defining
// 
// If you have a custom type for your parameter, you can use the AZ_GFX_COMMON_PARAM macro:
// AZ_GFX_COMMON_PARAM(Name, MemberName, DefaultValue, ValueType)
// The keywords here are the same as above, with the addition of ValueType: the custom type you want your param to be
// 
// Example usages:
// 
//      #define AZ_GFX_VEC3_PARAM(Position, m_position, Vector3(0.0f, 0.0f, 0.0f))
//
//      #define AZ_GFX_COMMON_PARAM(Format, m_format, Format::Unknown, FormatEnum)
// 
// ___________________________________________________________________________________________________________________________________________________
// 
// Part IV: Using the Macros for Post Process overrides
// 
// The Post Process System allows users to specify whether settings should be overridden or not on a per-member basis.
// To enable this, when you declare a member that can be overridden by higher priority Post Process Settings, in addition
// to using the above macros to define the member, you should also use one of the following to specify the override:
//
//      #define AZ_GFX_ANY_PARAM_BOOL_OVERRIDE(Name, MemberName, ValueType)
//      #define AZ_GFX_INTEGER_PARAM_FLOAT_OVERRIDE(Name, MemberName, ValueType)
//      #define AZ_GFX_FLOAT_PARAM_FLOAT_OVERRIDE(Name, MemberName, ValueType)
//  
// Where: 
// Name - The name of the param that will be used for reflection and appended to setters and getters, for example Width
// MemberName - The name of the member defined inside your class, for example m_width
// ValueType - The type of the parameter you defined (bool, float, uint32_t, Vector2 etc.)
// 
// A bit more details on each of the override macros: 
// 
// AZ_GFX_ANY_PARAM_BOOL_OVERRIDE can be used for params of any type. The override variable will be a bool (checkbox in the UI).
// The override application is a simple binary operation: take all of the source or the target (no lerp) depending on the bool.
// 
// AZ_GFX_INTEGER_PARAM_FLOAT_OVERRIDE should be used for params of integer types (int, uint, integer vectors...) The override variable
// will be a float from 0.0 to 1.0 (slider in the UI). The override will lerp between target and source using the override float
// variable. Note that for this reason the param integer type must support multiplication by a float value.
// 
// AZ_GFX_FLOAT_PARAM_FLOAT_OVERRIDE should be used for params of floating point types (float, double, Vector4...) The override variable
// will be a float from 0.0 to 1.0 (slider in the UI). The override will lerp between target and source using the override float.
// 
// Example usage:
// 
//      #define AZ_GFX_VEC3_PARAM(Position, m_position, Vector3(0.0f, 0.0f, 0.0f))
//      #define AZ_GFX_FLOAT_PARAM_FLOAT_OVERRIDE(Position, m_position, Vector3)
// 
// ___________________________________________________________________________________________________________________________________________________
// 
// Part V: Defining functionality with .inl files
// 
// There are many .inl fies in this folder that provide pre-defined behaviors for params declared with the above macros
// To use these files, start by including the .inl file that specifies the behavior, then include your own .inl that defines your params
// then include EndParams.inl (this will #undef all used macros so as to avoid collisions with subsequent macro usage further in the file)
//
// Here is an example of how that looks:
// 
//      #include <Atom/Feature/ParamMacros/StartParamFunctionsVirtual.inl>       <- The behavior you want (behavior is described in each file)
//      #include <Atom/Feature/PostProcess/PostProcessParams.inl>                <- Your file in which you declare your params
//      #include <Atom/Feature/ParamMacros/EndParams.inl>                        <- This #undef a bunch of macros to avoid conflicts
//  
// You may of course use your own custom behaviors by specifying your definition for the param and override macros.
// You can specify each macro individually (AZ_GFX_BOOL_PARAM, AZ_GFX_FLOAT_PARAM, AZ_GFX_UINT32_PARAM, AZ_GFX_VEC2_PARAM, etc.)
// or you can specify the AZ_GFX_COMMON_PARAM and AZ_GFX_COMMON_OVERRIDE macros if there's no difference between variable types.
// 
// AZ_GFX_COMMON_PARAM and AZ_GFX_COMMON_OVERRIDE are helper macros that the other _PARAM and _OVERRIDE macros can be mapped to
// using MapAllCommon.inl, allowing you to specify one definition for all types rather than each type individually.
// Here is an example of how we can use AZ_GFX_COMMON_PARAM and AZ_GFX_COMMON_OVERRIDE to auto-generate getters for our member parameters:
// 
//      #define AZ_GFX_COMMON_PARAM(Name, MemberName, DefaultValue, ValueType)                               \
//              ValueType Get##Name() const override { return MemberName; }                                  \
//      
//      #define AZ_GFX_COMMON_OVERRIDE(Name, MemberName, ValueType, OverrideValueType)                       \
//              OverrideValueType Get##Name##Override() const override { return MemberName##Override; }      \
//      
//      #include <Atom/Feature/ParamMacros/MapAllCommon.inl>
//      #include <Atom/Feature/MyCustomFeature/MyCustomPostProcess.inl>
//      #include <Atom/Feature/ParamMacros/EndParams.inl>
//
