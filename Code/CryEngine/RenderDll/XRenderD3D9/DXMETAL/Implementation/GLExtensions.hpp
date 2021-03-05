/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
//
//  GLExtensions.hpp
//  CryEngine
//
//  Created by Duhon, Eric on 10/29/14.
//
//

#ifndef CryEngine_GLExtensions_hpp
#define CryEngine_GLExtensions_hpp
#pragma once

#if DXGLES && DXGLES_VERSION == DXGLES_VERSION_30 && defined(GL_EXT_separate_shader_objects)
// On OpenGL ES separate shader programs are available as an extesion, so we
// just define the normal api here to avoid ifdefing the entire code
#define GL_VERTEX_SHADER_BIT GL_VERTEX_SHADER_BIT_EXT
#define GL_FRAGMENT_SHADER_BIT GL_FRAGMENT_SHADER_BIT_EXT
#define GL_ALL_SHADER_BITS GL_ALL_SHADER_BITS_EXT
#define GL_PROGRAM_SEPARABLE GL_PROGRAM_SEPARABLE_EXT
#define GL_ACTIVE_PROGRAM GL_ACTIVE_PROGRAM_EXT
#define GL_PROGRAM_PIPELINE_BINDING GL_PROGRAM_PIPELINE_BINDING_EXT

#define glUseProgramStages glUseProgramStagesEXT
#define glActiveShaderProgram glActiveShaderProgramEXT
#define glCreateShaderProgramv glCreateShaderProgramvEXT
#define glBindProgramPipeline glBindProgramPipelineEXT
#define glDeleteProgramPipelines glDeleteProgramPipelinesEXT
#define glGenProgramPipelines glGenProgramPipelinesEXT
#define glIsProgramPipeline glIsProgramPipelineEXT
#define glProgramParameteri glProgramParameteriEXT
#define glGetProgramPipelineiv glGetProgramPipelineivEXT
#define glValidateProgramPipeline glValidateProgramPipelineEXT
#define glGetProgramPipelineInfoLog glGetProgramPipelineInfoLogEXT

#define glProgramUniform1i glProgramUniform1iEXT
#define glProgramUniform2i glProgramUniform2iEXT
#define glProgramUniform2i glProgramUniform2iEXT
#define glProgramUniform2i glProgramUniform2iEXT

#define glProgramUniform1f glProgramUniform1fEXT
#define glProgramUniform2f glProgramUniform2fEXT
#define glProgramUniform3f glProgramUniform3fEXT
#define glProgramUniform4f glProgramUniform4fEXT

#define glProgramUniform1ui glProgramUniform1uiEXT
#define glProgramUniform2ui glProgramUniform2uiEXT
#define glProgramUniform3ui glProgramUniform3uiEXT
#define glProgramUniform4ui glProgramUniform4uiEXT

#define glProgramUniform1iv glProgramUniform1ivEXT
#define glProgramUniform2iv glProgramUniform2ivEXT
#define glProgramUniform3iv glProgramUniform3ivEXT
#define glProgramUniform4iv glProgramUniform4ivEXT

#define glProgramUniform1fv glProgramUniform1fvEXT
#define glProgramUniform2fv glProgramUniform2fvEXT
#define glProgramUniform3fv glProgramUniform3fvEXT
#define glProgramUniform4fv glProgramUniform4fvEXT

#define glProgramUniform1uiv glProgramUniform1uivEXT
#define glProgramUniform2uiv glProgramUniform2uivEXT
#define glProgramUniform3uiv glProgramUniform3uivEXT
#define glProgramUniform4uiv glProgramUniform4uivEXT

#define glProgramUniformMatrix2fv glProgramUniformMatrix2fvEXT
#define glProgramUniformMatrix3fv glProgramUniformMatrix3fvEXT
#define glProgramUniformMatrix4fv glProgramUniformMatrix4fvEXT
#define glProgramUniformMatrix2x3fv glProgramUniformMatrix2x3fvEXT
#define glProgramUniformMatrix3x2fv glProgramUniformMatrix3x2fvEXT
#define glProgramUniformMatrix2x4fv glProgramUniformMatrix2x4fvEXT
#define glProgramUniformMatrix4x2fv glProgramUniformMatrix4x2fvEXT
#define glProgramUniformMatrix3x4fv glProgramUniformMatrix3x4fvEXT
#define glProgramUniformMatrix4x3fv glProgramUniformMatrix4x3fvEXT
#endif

#endif
