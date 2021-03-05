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

#undef glUseProgramStages
EXTENSION_FUNC(glUseProgramStages, glUseProgramStagesEXT, void, GLuint, GLbitfield, GLuint)
#define glUseProgramStages DXGL_EXT_FUNCPTR(glUseProgramStages)
#undef glActiveShaderProgram
EXTENSION_FUNC(glActiveShaderProgram, glActiveShaderProgramEXT, void, GLuint, GLuint)
#define glActiveShaderProgram DXGL_EXT_FUNCPTR(glActiveShaderProgram)
#undef glCreateShaderProgramv
EXTENSION_FUNC(glCreateShaderProgramv, glCreateShaderProgramvEXT, GLuint, GLenum, GLsizei, const GLchar**)
#define glCreateShaderProgramv DXGL_EXT_FUNCPTR(glCreateShaderProgramv)
#undef glBindProgramPipeline
EXTENSION_FUNC(glBindProgramPipeline, glBindProgramPipelineEXT, void, GLuint)
#define glBindProgramPipeline DXGL_EXT_FUNCPTR(glBindProgramPipeline)
#undef glDeleteProgramPipelines
EXTENSION_FUNC(glDeleteProgramPipelines, glDeleteProgramPipelinesEXT, void, GLsizei, const GLuint*)
#define glDeleteProgramPipelines DXGL_EXT_FUNCPTR(glDeleteProgramPipelines)
#undef glGenProgramPipelines
EXTENSION_FUNC(glGenProgramPipelines, glGenProgramPipelinesEXT, void, GLsizei, GLuint*)
#define glGenProgramPipelines DXGL_EXT_FUNCPTR(glGenProgramPipelines)
#undef glIsProgramPipeline
EXTENSION_FUNC(glIsProgramPipeline, glIsProgramPipelineEXT, GLboolean, GLuint)
#define glIsProgramPipeline DXGL_EXT_FUNCPTR(glIsProgramPipeline)
#undef glProgramParameteri
EXTENSION_FUNC(glProgramParameteri, glProgramParameteriEXT, void, GLuint, GLenum, GLint)
#define glProgramParameteri DXGL_EXT_FUNCPTR(glProgramParameteri)
#undef glGetProgramPipelineiv
EXTENSION_FUNC(glGetProgramPipelineiv, glGetProgramPipelineivEXT, void, GLuint, GLenum, GLint*)
#define glGetProgramPipelineiv DXGL_EXT_FUNCPTR(glGetProgramPipelineiv)
#undef glValidateProgramPipeline
EXTENSION_FUNC(glValidateProgramPipeline, glValidateProgramPipelineEXT, void, GLuint)
#define glValidateProgramPipeline DXGL_EXT_FUNCPTR(glValidateProgramPipeline)
#undef glGetProgramPipelineInfoLog
EXTENSION_FUNC(glGetProgramPipelineInfoLog, glGetProgramPipelineInfoLogEXT, void, GLuint, GLsizei, GLsizei*, GLchar*)
#define glGetProgramPipelineInfoLog DXGL_EXT_FUNCPTR(glGetProgramPipelineInfoLog)
#undef glProgramUniform1i
EXTENSION_FUNC(glProgramUniform1i, glProgramUniform1iEXT, void, GLuint, GLint, GLint)
#define glProgramUniform1i DXGL_EXT_FUNCPTR(glProgramUniform1i)
#undef glProgramUniform2i
EXTENSION_FUNC(glProgramUniform2i, glProgramUniform2iEXT, void, GLuint, GLint, GLint, GLint)
#define glProgramUniform2i DXGL_EXT_FUNCPTR(glProgramUniform2i)
#undef glProgramUniform3i
EXTENSION_FUNC(glProgramUniform3i, glProgramUniform3iEXT, void, GLuint, GLint, GLint, GLint, GLint)
#define glProgramUniform3i DXGL_EXT_FUNCPTR(glProgramUniform3i)
#undef glProgramUniform4i
EXTENSION_FUNC(glProgramUniform4i, glProgramUniform4iEXT, void, GLuint, GLint, GLint, GLint, GLint, GLint)
#define glProgramUniform4i DXGL_EXT_FUNCPTR(glProgramUniform4i)
#undef glProgramUniform1f
EXTENSION_FUNC(glProgramUniform1f, glProgramUniform1fEXT, void, GLuint, GLint, GLfloat)
#define glProgramUniform1f DXGL_EXT_FUNCPTR(glProgramUniform1f)
#undef glProgramUniform2f
EXTENSION_FUNC(glProgramUniform2f, glProgramUniform2fEXT, void, GLuint, GLint, GLfloat, GLfloat)
#define glProgramUniform2f DXGL_EXT_FUNCPTR(glProgramUniform2f)
#undef glProgramUniform3f
EXTENSION_FUNC(glProgramUniform3f, glProgramUniform3fEXT, void, GLuint, GLint, GLfloat, GLfloat, GLfloat)
#define glProgramUniform3f DXGL_EXT_FUNCPTR(glProgramUniform3f)
#undef glProgramUniform4f
EXTENSION_FUNC(glProgramUniform4f, glProgramUniform4fEXT, void, GLuint, GLint, GLfloat, GLfloat, GLfloat, GLfloat)
#define glProgramUniform4f DXGL_EXT_FUNCPTR(glProgramUniform4f)
#undef glProgramUniform1ui
EXTENSION_FUNC(glProgramUniform1ui, glProgramUniform1uiEXT, void, GLuint, GLint, GLuint)
#define glProgramUniform1ui DXGL_EXT_FUNCPTR(glProgramUniform1ui)
#undef glProgramUniform2ui
EXTENSION_FUNC(glProgramUniform2ui, glProgramUniform2uiEXT, void, GLuint, GLint, GLuint, GLuint)
#define glProgramUniform2ui DXGL_EXT_FUNCPTR(glProgramUniform2ui)
#undef glProgramUniform3ui
EXTENSION_FUNC(glProgramUniform3ui, glProgramUniform3uiEXT, void, GLuint, GLint, GLuint, GLuint, GLuint)
#define glProgramUniform3ui DXGL_EXT_FUNCPTR(glProgramUniform3ui)
#undef glProgramUniform4ui
EXTENSION_FUNC(glProgramUniform4ui, glProgramUniform4uiEXT, void, GLuint, GLint, GLuint, GLuint, GLuint, GLuint)
#define glProgramUniform4ui DXGL_EXT_FUNCPTR(glProgramUniform4ui)
#undef glProgramUniform1iv
EXTENSION_FUNC(glProgramUniform1iv, glProgramUniform1ivEXT, void, GLuint, GLint, GLsizei, const GLint*)
#define glProgramUniform1iv DXGL_EXT_FUNCPTR(glProgramUniform1iv)
#undef glProgramUniform2iv
EXTENSION_FUNC(glProgramUniform2iv, glProgramUniform2ivEXT, void, GLuint, GLint, GLsizei, const GLint*)
#define glProgramUniform2iv DXGL_EXT_FUNCPTR(glProgramUniform2iv)
#undef glProgramUniform3iv
EXTENSION_FUNC(glProgramUniform3iv, glProgramUniform3ivEXT, void, GLuint, GLint, GLsizei, const GLint*)
#define glProgramUniform3iv DXGL_EXT_FUNCPTR(glProgramUniform3iv)
#undef glProgramUniform4iv
EXTENSION_FUNC(glProgramUniform4iv, glProgramUniform4ivEXT, void, GLuint, GLint, GLsizei, const GLint*)
#define glProgramUniform4iv DXGL_EXT_FUNCPTR(glProgramUniform4iv)
#undef glProgramUniform1fv
EXTENSION_FUNC(glProgramUniform1fv, glProgramUniform1fvEXT, void, GLuint, GLint, GLsizei, const GLfloat*)
#define glProgramUniform1fv DXGL_EXT_FUNCPTR(glProgramUniform1fv)
#undef glProgramUniform2fv
EXTENSION_FUNC(glProgramUniform2fv, glProgramUniform2fvEXT, void, GLuint, GLint, GLsizei, const GLfloat*)
#define glProgramUniform2fv DXGL_EXT_FUNCPTR(glProgramUniform2fv)
#undef glProgramUniform3fv
EXTENSION_FUNC(glProgramUniform3fv, glProgramUniform3fvEXT, void, GLuint, GLint, GLsizei, const GLfloat*)
#define glProgramUniform3fv DXGL_EXT_FUNCPTR(glProgramUniform3fv)
#undef glProgramUniform4fv
EXTENSION_FUNC(glProgramUniform4fv, glProgramUniform4fvEXT, void, GLuint, GLint, GLsizei, const GLfloat*)
#define glProgramUniform4fv DXGL_EXT_FUNCPTR(glProgramUniform4fv)
#undef glProgramUniform1uiv
EXTENSION_FUNC(glProgramUniform1uiv, glProgramUniform1uivEXT, void, GLuint, GLint, GLsizei, const GLuint*)
#define glProgramUniform1uiv DXGL_EXT_FUNCPTR(glProgramUniform1uiv)
#undef glProgramUniform2uiv
EXTENSION_FUNC(glProgramUniform2uiv, glProgramUniform2uivEXT, void, GLuint, GLint, GLsizei, const GLuint*)
#define glProgramUniform2uiv DXGL_EXT_FUNCPTR(glProgramUniform2uiv)
#undef glProgramUniform3uiv
EXTENSION_FUNC(glProgramUniform3uiv, glProgramUniform3uivEXT, void, GLuint, GLint, GLsizei, const GLuint*)
#define glProgramUniform3uiv DXGL_EXT_FUNCPTR(glProgramUniform3uiv)
#undef glProgramUniform4uiv
EXTENSION_FUNC(glProgramUniform4uiv, glProgramUniform4uivEXT, void, GLuint, GLint, GLsizei, const GLuint*)
#define glProgramUniform4uiv DXGL_EXT_FUNCPTR(glProgramUniform4uiv)
#undef glProgramUniformMatrix2fv
EXTENSION_FUNC(glProgramUniformMatrix2fv, glProgramUniformMatrix2fvEXT, void, GLuint, GLint, GLsizei, GLboolean, const GLfloat*)
#define glProgramUniformMatrix2fv DXGL_EXT_FUNCPTR(glProgramUniformMatrix2fv)
#undef glProgramUniformMatrix3fv
EXTENSION_FUNC(glProgramUniformMatrix3fv, glProgramUniformMatrix3fvEXT, void, GLuint, GLint, GLsizei, GLboolean, const GLfloat*)
#define glProgramUniformMatrix3fv DXGL_EXT_FUNCPTR(glProgramUniformMatrix3fv)
#undef glProgramUniformMatrix4fv
EXTENSION_FUNC(glProgramUniformMatrix4fv, glProgramUniformMatrix4fvEXT, void, GLuint, GLint, GLsizei, GLboolean, const GLfloat*)
#define glProgramUniformMatrix4fv DXGL_EXT_FUNCPTR(glProgramUniformMatrix4fv)
#undef glProgramUniformMatrix2x3fv
EXTENSION_FUNC(glProgramUniformMatrix2x3fv, glProgramUniformMatrix2x3fvEXT, void, GLuint, GLint, GLsizei, GLboolean, const GLfloat*)
#define glProgramUniformMatrix2x3fv DXGL_EXT_FUNCPTR(glProgramUniformMatrix2x3fv)
#undef glProgramUniformMatrix3x2fv
EXTENSION_FUNC(glProgramUniformMatrix3x2fv, glProgramUniformMatrix3x2fvEXT, void, GLuint, GLint, GLsizei, GLboolean, const GLfloat*)
#define glProgramUniformMatrix3x2fv DXGL_EXT_FUNCPTR(glProgramUniformMatrix3x2fv)
#undef glProgramUniformMatrix2x4fv
EXTENSION_FUNC(glProgramUniformMatrix2x4fv, glProgramUniformMatrix2x4fvEXT, void, GLuint, GLint, GLsizei, GLboolean, const GLfloat*)
#define glProgramUniformMatrix2x4fv DXGL_EXT_FUNCPTR(glProgramUniformMatrix2x4fv)
#undef glProgramUniformMatrix4x2fv
EXTENSION_FUNC(glProgramUniformMatrix4x2fv, glProgramUniformMatrix4x2fvEXT, void, GLuint, GLint, GLsizei, GLboolean, const GLfloat*)
#define glProgramUniformMatrix4x2fv DXGL_EXT_FUNCPTR(glProgramUniformMatrix4x2fv)
#undef glProgramUniformMatrix3x4fv
EXTENSION_FUNC(glProgramUniformMatrix3x4fv, glProgramUniformMatrix3x4fvEXT, void, GLuint, GLint, GLsizei, GLboolean, const GLfloat*)
#define glProgramUniformMatrix3x4fv DXGL_EXT_FUNCPTR(glProgramUniformMatrix3x4fv)
#undef glProgramUniformMatrix4x3fv
EXTENSION_FUNC(glProgramUniformMatrix4x3fv, glProgramUniformMatrix4x3fvEXT, void, GLuint, GLint, GLsizei, GLboolean, const GLfloat*)
#define glProgramUniformMatrix4x3fv DXGL_EXT_FUNCPTR(glProgramUniformMatrix4x3fv)
#undef glCopyImageSubData
EXTENSION_FUNC(glCopyImageSubData, glCopyImageSubDataEXT, void, GLuint, GLenum, GLint, GLint, GLint, GLint, GLuint, GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei)
#define glCopyImageSubData DXGL_EXT_FUNCPTR(glCopyImageSubData)

