#include "gles.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef GLAD_IMPL_UTIL_C_
#define GLAD_IMPL_UTIL_C_

#ifdef _MSC_VER
#define GLAD_IMPL_UTIL_SSCANF sscanf_s
#else
#define GLAD_IMPL_UTIL_SSCANF sscanf
#endif

#endif /* GLAD_IMPL_UTIL_C_ */

#if GLAD_PLATFORM_ANDROID || GLAD_PLATFORM_EMSCRIPTEN || SNAP_GLAD_LINUX_BASED()

#include <cassert>
#include <mutex>

int GLAD_SNAP_RHI_GL_ES_VERSION_2_0 = 0;
int GLAD_SNAP_RHI_GL_ES_VERSION_3_0 = 0;
int GLAD_SNAP_RHI_GL_ES_VERSION_3_1 = 0;
int GLAD_SNAP_RHI_GL_ES_VERSION_3_2 = 0;
int GLAD_SNAP_RHI_GL_APPLE_clip_distance = 0;
int GLAD_SNAP_RHI_GL_APPLE_framebuffer_multisample = 0;
int GLAD_SNAP_RHI_GL_APPLE_sync = 0;
int GLAD_SNAP_RHI_GL_APPLE_texture_format_BGRA8888 = 0;
int GLAD_SNAP_RHI_GL_ARM_shader_framebuffer_fetch = 0;
int GLAD_SNAP_RHI_GL_ARM_shader_framebuffer_fetch_depth_stencil = 0;
int GLAD_SNAP_RHI_GL_EXT_blend_minmax = 0;
int GLAD_SNAP_RHI_GL_EXT_clip_cull_distance = 0;
int GLAD_SNAP_RHI_GL_EXT_color_buffer_half_float = 0;
int GLAD_SNAP_RHI_GL_EXT_debug_marker = 0;
int GLAD_SNAP_RHI_GL_EXT_discard_framebuffer = 0;
int GLAD_SNAP_RHI_GL_EXT_disjoint_timer_query = 0;
int GLAD_SNAP_RHI_GL_EXT_draw_instanced = 0;
int GLAD_SNAP_RHI_GL_EXT_multisampled_render_to_texture = 0;
int GLAD_SNAP_RHI_GL_EXT_multisampled_render_to_texture2 = 0;
int GLAD_SNAP_RHI_GL_EXT_shader_framebuffer_fetch = 0;
int GLAD_SNAP_RHI_GL_EXT_shader_framebuffer_fetch_non_coherent = 0;
int GLAD_SNAP_RHI_GL_EXT_shader_texture_lod = 0;
int GLAD_SNAP_RHI_GL_EXT_texture_border_clamp = 0;
int GLAD_SNAP_RHI_GL_EXT_texture_filter_anisotropic = 0;
int GLAD_SNAP_RHI_GL_EXT_texture_format_BGRA8888 = 0;
int GLAD_SNAP_RHI_GL_EXT_texture_norm16 = 0;
int GLAD_SNAP_RHI_GL_EXT_texture_rg = 0;
int GLAD_SNAP_RHI_GL_EXT_memory_object = 0;
int GLAD_SNAP_RHI_GL_EXT_memory_object_fd = 0;
int GLAD_SNAP_RHI_GL_KHR_debug = 0;
int GLAD_SNAP_RHI_GL_NV_texture_border_clamp = 0;
int GLAD_SNAP_RHI_GL_OES_EGL_image = 0;
int GLAD_SNAP_RHI_GL_OES_EGL_image_external = 0;
int GLAD_SNAP_RHI_GL_OES_depth24 = 0;
int GLAD_SNAP_RHI_GL_OES_depth32 = 0;
int GLAD_SNAP_RHI_GL_OES_depth_texture = 0;
int GLAD_SNAP_RHI_GL_OES_element_index_uint = 0;
int GLAD_SNAP_RHI_GL_OES_fbo_render_mipmap = 0;
int GLAD_SNAP_RHI_GL_OES_get_program_binary = 0;
int GLAD_SNAP_RHI_GL_OES_mapbuffer = 0;
int GLAD_SNAP_RHI_GL_OES_packed_depth_stencil = 0;
int GLAD_SNAP_RHI_GL_OES_rgb8_rgba8 = 0;
int GLAD_SNAP_RHI_GL_OES_standard_derivatives = 0;
int GLAD_SNAP_RHI_GL_OES_texture_3D = 0;
int GLAD_SNAP_RHI_GL_OES_texture_border_clamp = 0;
int GLAD_SNAP_RHI_GL_OES_texture_float = 0;
int GLAD_SNAP_RHI_GL_OES_texture_float_linear = 0;
int GLAD_SNAP_RHI_GL_OES_texture_half_float = 0;
int GLAD_SNAP_RHI_GL_OES_texture_half_float_linear = 0;
int GLAD_SNAP_RHI_GL_OES_texture_npot = 0;
int GLAD_SNAP_RHI_GL_OVR_multiview = 0;
int GLAD_SNAP_RHI_GL_OVR_multiview2 = 0;
int GLAD_SNAP_RHI_GL_OVR_multiview_multisampled_render_to_texture = 0;

PFNGLACTIVESHADERPROGRAMPROC glad_snap_rhi_glActiveShaderProgram = NULL;
PFNGLBEGINQUERYPROC glad_snap_rhi_glBeginQuery = NULL;
PFNGLBEGINQUERYEXTPROC glad_snap_rhi_glBeginQueryEXT = NULL;
PFNGLBEGINTRANSFORMFEEDBACKPROC glad_snap_rhi_glBeginTransformFeedback = NULL;
PFNGLBINDBUFFERBASEPROC glad_snap_rhi_glBindBufferBase = NULL;
PFNGLBINDBUFFERRANGEPROC glad_snap_rhi_glBindBufferRange = NULL;
PFNGLBINDIMAGETEXTUREPROC glad_snap_rhi_glBindImageTexture = NULL;
PFNGLBINDPROGRAMPIPELINEPROC glad_snap_rhi_glBindProgramPipeline = NULL;
PFNGLBINDSAMPLERPROC glad_snap_rhi_glBindSampler = NULL;
PFNGLBINDTRANSFORMFEEDBACKPROC glad_snap_rhi_glBindTransformFeedback = NULL;
PFNGLBINDVERTEXARRAYPROC glad_snap_rhi_glBindVertexArray = NULL;
PFNGLBINDVERTEXBUFFERPROC glad_snap_rhi_glBindVertexBuffer = NULL;
PFNGLBLENDBARRIERPROC glad_snap_rhi_glBlendBarrier = NULL;
PFNGLBLENDEQUATIONSEPARATEIPROC glad_snap_rhi_glBlendEquationSeparatei = NULL;
PFNGLBLENDEQUATIONIPROC glad_snap_rhi_glBlendEquationi = NULL;
PFNGLBLENDFUNCSEPARATEIPROC glad_snap_rhi_glBlendFuncSeparatei = NULL;
PFNGLBLENDFUNCIPROC glad_snap_rhi_glBlendFunci = NULL;
PFNGLBLITFRAMEBUFFERPROC glad_snap_rhi_glBlitFramebuffer = NULL;
PFNGLCLEARBUFFERFIPROC glad_snap_rhi_glClearBufferfi = NULL;
PFNGLCLEARBUFFERFVPROC glad_snap_rhi_glClearBufferfv = NULL;
PFNGLCLEARBUFFERIVPROC glad_snap_rhi_glClearBufferiv = NULL;
PFNGLCLEARBUFFERUIVPROC glad_snap_rhi_glClearBufferuiv = NULL;
PFNGLCLIENTWAITSYNCPROC glad_snap_rhi_glClientWaitSync = NULL;
PFNGLCLIENTWAITSYNCAPPLEPROC glad_snap_rhi_glClientWaitSyncAPPLE = NULL;
PFNGLCOLORMASKIPROC glad_snap_rhi_glColorMaski = NULL;
PFNGLCOMPRESSEDTEXIMAGE3DPROC glad_snap_rhi_glCompressedTexImage3D = NULL;
PFNGLCOMPRESSEDTEXIMAGE3DOESPROC glad_snap_rhi_glCompressedTexImage3DOES = NULL;
PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC glad_snap_rhi_glCompressedTexSubImage3D = NULL;
PFNGLCOMPRESSEDTEXSUBIMAGE3DOESPROC glad_snap_rhi_glCompressedTexSubImage3DOES = NULL;
PFNGLCOPYBUFFERSUBDATAPROC glad_snap_rhi_glCopyBufferSubData = NULL;
PFNGLCOPYIMAGESUBDATAPROC glad_snap_rhi_glCopyImageSubData = NULL;
PFNGLCOPYTEXSUBIMAGE3DPROC glad_snap_rhi_glCopyTexSubImage3D = NULL;
PFNGLCOPYTEXSUBIMAGE3DOESPROC glad_snap_rhi_glCopyTexSubImage3DOES = NULL;
PFNGLCREATESHADERPROGRAMVPROC glad_snap_rhi_glCreateShaderProgramv = NULL;
PFNGLDEBUGMESSAGECALLBACKPROC glad_snap_rhi_glDebugMessageCallback = NULL;
PFNGLDEBUGMESSAGECALLBACKKHRPROC glad_snap_rhi_glDebugMessageCallbackKHR = NULL;
PFNGLDEBUGMESSAGECONTROLPROC glad_snap_rhi_glDebugMessageControl = NULL;
PFNGLDEBUGMESSAGECONTROLKHRPROC glad_snap_rhi_glDebugMessageControlKHR = NULL;
PFNGLDEBUGMESSAGEINSERTPROC glad_snap_rhi_glDebugMessageInsert = NULL;
PFNGLDEBUGMESSAGEINSERTKHRPROC glad_snap_rhi_glDebugMessageInsertKHR = NULL;
PFNGLDELETEPROGRAMPIPELINESPROC glad_snap_rhi_glDeleteProgramPipelines = NULL;
PFNGLDELETEQUERIESPROC glad_snap_rhi_glDeleteQueries = NULL;
PFNGLDELETEQUERIESEXTPROC glad_snap_rhi_glDeleteQueriesEXT = NULL;
PFNGLDELETESAMPLERSPROC glad_snap_rhi_glDeleteSamplers = NULL;
PFNGLDELETESYNCPROC glad_snap_rhi_glDeleteSync = NULL;
PFNGLDELETESYNCAPPLEPROC glad_snap_rhi_glDeleteSyncAPPLE = NULL;
PFNGLDELETETRANSFORMFEEDBACKSPROC glad_snap_rhi_glDeleteTransformFeedbacks = NULL;
PFNGLDELETEVERTEXARRAYSPROC glad_snap_rhi_glDeleteVertexArrays = NULL;
PFNGLDISABLEIPROC glad_snap_rhi_glDisablei = NULL;
PFNGLDISCARDFRAMEBUFFEREXTPROC glad_snap_rhi_glDiscardFramebufferEXT = NULL;
PFNGLDISPATCHCOMPUTEPROC glad_snap_rhi_glDispatchCompute = NULL;
PFNGLDISPATCHCOMPUTEINDIRECTPROC glad_snap_rhi_glDispatchComputeIndirect = NULL;
PFNGLDRAWARRAYSINDIRECTPROC glad_snap_rhi_glDrawArraysIndirect = NULL;
PFNGLDRAWARRAYSINSTANCEDPROC glad_snap_rhi_glDrawArraysInstanced = NULL;
PFNGLDRAWARRAYSINSTANCEDEXTPROC glad_snap_rhi_glDrawArraysInstancedEXT = NULL;
PFNGLDRAWBUFFERSPROC glad_snap_rhi_glDrawBuffers = NULL;
PFNGLDRAWELEMENTSBASEVERTEXPROC glad_snap_rhi_glDrawElementsBaseVertex = NULL;
PFNGLDRAWELEMENTSINDIRECTPROC glad_snap_rhi_glDrawElementsIndirect = NULL;
PFNGLDRAWELEMENTSINSTANCEDPROC glad_snap_rhi_glDrawElementsInstanced = NULL;
PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXPROC glad_snap_rhi_glDrawElementsInstancedBaseVertex = NULL;
PFNGLDRAWELEMENTSINSTANCEDEXTPROC glad_snap_rhi_glDrawElementsInstancedEXT = NULL;
PFNGLDRAWRANGEELEMENTSPROC glad_snap_rhi_glDrawRangeElements = NULL;
PFNGLDRAWRANGEELEMENTSBASEVERTEXPROC glad_snap_rhi_glDrawRangeElementsBaseVertex = NULL;
PFNGLEGLIMAGETARGETRENDERBUFFERSTORAGEOESPROC glad_snap_rhi_glEGLImageTargetRenderbufferStorageOES = NULL;
PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glad_snap_rhi_glEGLImageTargetTexture2DOES = NULL;
PFNGLENABLEIPROC glad_snap_rhi_glEnablei = NULL;
PFNGLENDQUERYPROC glad_snap_rhi_glEndQuery = NULL;
PFNGLENDQUERYEXTPROC glad_snap_rhi_glEndQueryEXT = NULL;
PFNGLENDTRANSFORMFEEDBACKPROC glad_snap_rhi_glEndTransformFeedback = NULL;
PFNGLFENCESYNCPROC glad_snap_rhi_glFenceSync = NULL;
PFNGLFENCESYNCAPPLEPROC glad_snap_rhi_glFenceSyncAPPLE = NULL;
PFNGLFLUSHMAPPEDBUFFERRANGEPROC glad_snap_rhi_glFlushMappedBufferRange = NULL;
PFNGLFRAMEBUFFERFETCHBARRIEREXTPROC glad_snap_rhi_glFramebufferFetchBarrierEXT = NULL;
PFNGLFRAMEBUFFERPARAMETERIPROC glad_snap_rhi_glFramebufferParameteri = NULL;
PFNGLFRAMEBUFFERTEXTUREPROC glad_snap_rhi_glFramebufferTexture = NULL;
PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC glad_snap_rhi_glFramebufferTexture2DMultisampleEXT = NULL;
PFNGLFRAMEBUFFERTEXTURE3DOESPROC glad_snap_rhi_glFramebufferTexture3DOES = NULL;
PFNGLFRAMEBUFFERTEXTURELAYERPROC glad_snap_rhi_glFramebufferTextureLayer = NULL;
PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVRPROC glad_snap_rhi_glFramebufferTextureMultisampleMultiviewOVR = NULL;
PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC glad_snap_rhi_glFramebufferTextureMultiviewOVR = NULL;
PFNGLGENPROGRAMPIPELINESPROC glad_snap_rhi_glGenProgramPipelines = NULL;
PFNGLGENQUERIESPROC glad_snap_rhi_glGenQueries = NULL;
PFNGLGENQUERIESEXTPROC glad_snap_rhi_glGenQueriesEXT = NULL;
PFNGLGENSAMPLERSPROC glad_snap_rhi_glGenSamplers = NULL;
PFNGLGENTRANSFORMFEEDBACKSPROC glad_snap_rhi_glGenTransformFeedbacks = NULL;
PFNGLGENVERTEXARRAYSPROC glad_snap_rhi_glGenVertexArrays = NULL;
PFNGLGETACTIVEUNIFORMBLOCKNAMEPROC glad_snap_rhi_glGetActiveUniformBlockName = NULL;
PFNGLGETACTIVEUNIFORMBLOCKIVPROC glad_snap_rhi_glGetActiveUniformBlockiv = NULL;
PFNGLGETACTIVEUNIFORMSIVPROC glad_snap_rhi_glGetActiveUniformsiv = NULL;
PFNGLGETBOOLEANI_VPROC glad_snap_rhi_glGetBooleani_v = NULL;
PFNGLGETBUFFERPARAMETERI64VPROC glad_snap_rhi_glGetBufferParameteri64v = NULL;
PFNGLGETBUFFERPOINTERVPROC glad_snap_rhi_glGetBufferPointerv = NULL;
PFNGLGETBUFFERPOINTERVOESPROC glad_snap_rhi_glGetBufferPointervOES = NULL;
PFNGLGETDEBUGMESSAGELOGPROC glad_snap_rhi_glGetDebugMessageLog = NULL;
PFNGLGETDEBUGMESSAGELOGKHRPROC glad_snap_rhi_glGetDebugMessageLogKHR = NULL;
PFNGLGETFRAGDATALOCATIONPROC glad_snap_rhi_glGetFragDataLocation = NULL;
PFNGLGETFRAMEBUFFERPARAMETERIVPROC glad_snap_rhi_glGetFramebufferParameteriv = NULL;
PFNGLGETGRAPHICSRESETSTATUSPROC glad_snap_rhi_glGetGraphicsResetStatus = NULL;
PFNGLGETINTEGER64I_VPROC glad_snap_rhi_glGetInteger64i_v = NULL;
PFNGLGETINTEGER64VPROC glad_snap_rhi_glGetInteger64v = NULL;
PFNGLGETINTEGER64VAPPLEPROC glad_snap_rhi_glGetInteger64vAPPLE = NULL;
PFNGLGETINTEGER64VEXTPROC glad_snap_rhi_glGetInteger64vEXT = NULL;
PFNGLGETINTEGERI_VPROC glad_snap_rhi_glGetIntegeri_v = NULL;
PFNGLGETINTERNALFORMATIVPROC glad_snap_rhi_glGetInternalformativ = NULL;
PFNGLGETMULTISAMPLEFVPROC glad_snap_rhi_glGetMultisamplefv = NULL;
PFNGLGETOBJECTLABELPROC glad_snap_rhi_glGetObjectLabel = NULL;
PFNGLGETOBJECTLABELKHRPROC glad_snap_rhi_glGetObjectLabelKHR = NULL;
PFNGLGETOBJECTPTRLABELPROC glad_snap_rhi_glGetObjectPtrLabel = NULL;
PFNGLGETOBJECTPTRLABELKHRPROC glad_snap_rhi_glGetObjectPtrLabelKHR = NULL;
PFNGLGETPOINTERVPROC glad_snap_rhi_glGetPointerv = NULL;
PFNGLGETPOINTERVKHRPROC glad_snap_rhi_glGetPointervKHR = NULL;
PFNGLGETPROGRAMBINARYPROC glad_snap_rhi_glGetProgramBinary = NULL;
PFNGLGETPROGRAMBINARYOESPROC glad_snap_rhi_glGetProgramBinaryOES = NULL;
PFNGLGETPROGRAMINTERFACEIVPROC glad_snap_rhi_glGetProgramInterfaceiv = NULL;
PFNGLGETPROGRAMPIPELINEINFOLOGPROC glad_snap_rhi_glGetProgramPipelineInfoLog = NULL;
PFNGLGETPROGRAMPIPELINEIVPROC glad_snap_rhi_glGetProgramPipelineiv = NULL;
PFNGLGETPROGRAMRESOURCEINDEXPROC glad_snap_rhi_glGetProgramResourceIndex = NULL;
PFNGLGETPROGRAMRESOURCELOCATIONPROC glad_snap_rhi_glGetProgramResourceLocation = NULL;
PFNGLGETPROGRAMRESOURCENAMEPROC glad_snap_rhi_glGetProgramResourceName = NULL;
PFNGLGETPROGRAMRESOURCEIVPROC glad_snap_rhi_glGetProgramResourceiv = NULL;
PFNGLGETQUERYOBJECTI64VEXTPROC glad_snap_rhi_glGetQueryObjecti64vEXT = NULL;
PFNGLGETQUERYOBJECTIVEXTPROC glad_snap_rhi_glGetQueryObjectivEXT = NULL;
PFNGLGETQUERYOBJECTUI64VEXTPROC glad_snap_rhi_glGetQueryObjectui64vEXT = NULL;
PFNGLGETQUERYOBJECTUIVPROC glad_snap_rhi_glGetQueryObjectuiv = NULL;
PFNGLGETQUERYOBJECTUIVEXTPROC glad_snap_rhi_glGetQueryObjectuivEXT = NULL;
PFNGLGETQUERYIVPROC glad_snap_rhi_glGetQueryiv = NULL;
PFNGLGETQUERYIVEXTPROC glad_snap_rhi_glGetQueryivEXT = NULL;
PFNGLGETSAMPLERPARAMETERIIVPROC glad_snap_rhi_glGetSamplerParameterIiv = NULL;
PFNGLGETSAMPLERPARAMETERIIVEXTPROC glad_snap_rhi_glGetSamplerParameterIivEXT = NULL;
PFNGLGETSAMPLERPARAMETERIIVOESPROC glad_snap_rhi_glGetSamplerParameterIivOES = NULL;
PFNGLGETSAMPLERPARAMETERIUIVPROC glad_snap_rhi_glGetSamplerParameterIuiv = NULL;
PFNGLGETSAMPLERPARAMETERIUIVEXTPROC glad_snap_rhi_glGetSamplerParameterIuivEXT = NULL;
PFNGLGETSAMPLERPARAMETERIUIVOESPROC glad_snap_rhi_glGetSamplerParameterIuivOES = NULL;
PFNGLGETSAMPLERPARAMETERFVPROC glad_snap_rhi_glGetSamplerParameterfv = NULL;
PFNGLGETSAMPLERPARAMETERIVPROC glad_snap_rhi_glGetSamplerParameteriv = NULL;
PFNGLGETSTRINGIPROC glad_snap_rhi_glGetStringi = NULL;
PFNGLGETSYNCIVPROC glad_snap_rhi_glGetSynciv = NULL;
PFNGLGETSYNCIVAPPLEPROC glad_snap_rhi_glGetSyncivAPPLE = NULL;
PFNGLGETTEXLEVELPARAMETERFVPROC glad_snap_rhi_glGetTexLevelParameterfv = NULL;
PFNGLGETTEXLEVELPARAMETERIVPROC glad_snap_rhi_glGetTexLevelParameteriv = NULL;
PFNGLGETTEXPARAMETERIIVPROC glad_snap_rhi_glGetTexParameterIiv = NULL;
PFNGLGETTEXPARAMETERIIVEXTPROC glad_snap_rhi_glGetTexParameterIivEXT = NULL;
PFNGLGETTEXPARAMETERIIVOESPROC glad_snap_rhi_glGetTexParameterIivOES = NULL;
PFNGLGETTEXPARAMETERIUIVPROC glad_snap_rhi_glGetTexParameterIuiv = NULL;
PFNGLGETTEXPARAMETERIUIVEXTPROC glad_snap_rhi_glGetTexParameterIuivEXT = NULL;
PFNGLGETTEXPARAMETERIUIVOESPROC glad_snap_rhi_glGetTexParameterIuivOES = NULL;
PFNGLGETTRANSFORMFEEDBACKVARYINGPROC glad_snap_rhi_glGetTransformFeedbackVarying = NULL;
PFNGLGETUNIFORMBLOCKINDEXPROC glad_snap_rhi_glGetUniformBlockIndex = NULL;
PFNGLGETUNIFORMINDICESPROC glad_snap_rhi_glGetUniformIndices = NULL;
PFNGLGETUNIFORMUIVPROC glad_snap_rhi_glGetUniformuiv = NULL;
PFNGLGETVERTEXATTRIBIIVPROC glad_snap_rhi_glGetVertexAttribIiv = NULL;
PFNGLGETVERTEXATTRIBIUIVPROC glad_snap_rhi_glGetVertexAttribIuiv = NULL;
PFNGLGETNUNIFORMFVPROC glad_snap_rhi_glGetnUniformfv = NULL;
PFNGLGETNUNIFORMIVPROC glad_snap_rhi_glGetnUniformiv = NULL;
PFNGLGETNUNIFORMUIVPROC glad_snap_rhi_glGetnUniformuiv = NULL;
PFNGLINSERTEVENTMARKEREXTPROC glad_snap_rhi_glInsertEventMarkerEXT = NULL;
PFNGLINVALIDATEFRAMEBUFFERPROC glad_snap_rhi_glInvalidateFramebuffer = NULL;
PFNGLINVALIDATESUBFRAMEBUFFERPROC glad_snap_rhi_glInvalidateSubFramebuffer = NULL;
PFNGLISENABLEDIPROC glad_snap_rhi_glIsEnabledi = NULL;
PFNGLISPROGRAMPIPELINEPROC glad_snap_rhi_glIsProgramPipeline = NULL;
PFNGLISQUERYPROC glad_snap_rhi_glIsQuery = NULL;
PFNGLISQUERYEXTPROC glad_snap_rhi_glIsQueryEXT = NULL;
PFNGLISSAMPLERPROC glad_snap_rhi_glIsSampler = NULL;
PFNGLISSYNCPROC glad_snap_rhi_glIsSync = NULL;
PFNGLISSYNCAPPLEPROC glad_snap_rhi_glIsSyncAPPLE = NULL;
PFNGLISTRANSFORMFEEDBACKPROC glad_snap_rhi_glIsTransformFeedback = NULL;
PFNGLISVERTEXARRAYPROC glad_snap_rhi_glIsVertexArray = NULL;
PFNGLMAPBUFFEROESPROC glad_snap_rhi_glMapBufferOES = NULL;
PFNGLMAPBUFFERRANGEPROC glad_snap_rhi_glMapBufferRange = NULL;
PFNGLMEMORYBARRIERPROC glad_snap_rhi_glMemoryBarrier = NULL;
PFNGLMEMORYBARRIERBYREGIONPROC glad_snap_rhi_glMemoryBarrierByRegion = NULL;
PFNGLMINSAMPLESHADINGPROC glad_snap_rhi_glMinSampleShading = NULL;
PFNGLNAMEDFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC glad_snap_rhi_glNamedFramebufferTextureMultiviewOVR = NULL;
PFNGLOBJECTLABELPROC glad_snap_rhi_glObjectLabel = NULL;
PFNGLOBJECTLABELKHRPROC glad_snap_rhi_glObjectLabelKHR = NULL;
PFNGLOBJECTPTRLABELPROC glad_snap_rhi_glObjectPtrLabel = NULL;
PFNGLOBJECTPTRLABELKHRPROC glad_snap_rhi_glObjectPtrLabelKHR = NULL;
PFNGLPATCHPARAMETERIPROC glad_snap_rhi_glPatchParameteri = NULL;
PFNGLPAUSETRANSFORMFEEDBACKPROC glad_snap_rhi_glPauseTransformFeedback = NULL;
PFNGLPOPDEBUGGROUPPROC glad_snap_rhi_glPopDebugGroup = NULL;
PFNGLPOPDEBUGGROUPKHRPROC glad_snap_rhi_glPopDebugGroupKHR = NULL;
PFNGLPOPGROUPMARKEREXTPROC glad_snap_rhi_glPopGroupMarkerEXT = NULL;
PFNGLPRIMITIVEBOUNDINGBOXPROC glad_snap_rhi_glPrimitiveBoundingBox = NULL;
PFNGLPROGRAMBINARYPROC glad_snap_rhi_glProgramBinary = NULL;
PFNGLPROGRAMBINARYOESPROC glad_snap_rhi_glProgramBinaryOES = NULL;
PFNGLPROGRAMPARAMETERIPROC glad_snap_rhi_glProgramParameteri = NULL;
PFNGLPROGRAMUNIFORM1FPROC glad_snap_rhi_glProgramUniform1f = NULL;
PFNGLPROGRAMUNIFORM1FVPROC glad_snap_rhi_glProgramUniform1fv = NULL;
PFNGLPROGRAMUNIFORM1IPROC glad_snap_rhi_glProgramUniform1i = NULL;
PFNGLPROGRAMUNIFORM1IVPROC glad_snap_rhi_glProgramUniform1iv = NULL;
PFNGLPROGRAMUNIFORM1UIPROC glad_snap_rhi_glProgramUniform1ui = NULL;
PFNGLPROGRAMUNIFORM1UIVPROC glad_snap_rhi_glProgramUniform1uiv = NULL;
PFNGLPROGRAMUNIFORM2FPROC glad_snap_rhi_glProgramUniform2f = NULL;
PFNGLPROGRAMUNIFORM2FVPROC glad_snap_rhi_glProgramUniform2fv = NULL;
PFNGLPROGRAMUNIFORM2IPROC glad_snap_rhi_glProgramUniform2i = NULL;
PFNGLPROGRAMUNIFORM2IVPROC glad_snap_rhi_glProgramUniform2iv = NULL;
PFNGLPROGRAMUNIFORM2UIPROC glad_snap_rhi_glProgramUniform2ui = NULL;
PFNGLPROGRAMUNIFORM2UIVPROC glad_snap_rhi_glProgramUniform2uiv = NULL;
PFNGLPROGRAMUNIFORM3FPROC glad_snap_rhi_glProgramUniform3f = NULL;
PFNGLPROGRAMUNIFORM3FVPROC glad_snap_rhi_glProgramUniform3fv = NULL;
PFNGLPROGRAMUNIFORM3IPROC glad_snap_rhi_glProgramUniform3i = NULL;
PFNGLPROGRAMUNIFORM3IVPROC glad_snap_rhi_glProgramUniform3iv = NULL;
PFNGLPROGRAMUNIFORM3UIPROC glad_snap_rhi_glProgramUniform3ui = NULL;
PFNGLPROGRAMUNIFORM3UIVPROC glad_snap_rhi_glProgramUniform3uiv = NULL;
PFNGLPROGRAMUNIFORM4FPROC glad_snap_rhi_glProgramUniform4f = NULL;
PFNGLPROGRAMUNIFORM4FVPROC glad_snap_rhi_glProgramUniform4fv = NULL;
PFNGLPROGRAMUNIFORM4IPROC glad_snap_rhi_glProgramUniform4i = NULL;
PFNGLPROGRAMUNIFORM4IVPROC glad_snap_rhi_glProgramUniform4iv = NULL;
PFNGLPROGRAMUNIFORM4UIPROC glad_snap_rhi_glProgramUniform4ui = NULL;
PFNGLPROGRAMUNIFORM4UIVPROC glad_snap_rhi_glProgramUniform4uiv = NULL;
PFNGLPROGRAMUNIFORMMATRIX2FVPROC glad_snap_rhi_glProgramUniformMatrix2fv = NULL;
PFNGLPROGRAMUNIFORMMATRIX2X3FVPROC glad_snap_rhi_glProgramUniformMatrix2x3fv = NULL;
PFNGLPROGRAMUNIFORMMATRIX2X4FVPROC glad_snap_rhi_glProgramUniformMatrix2x4fv = NULL;
PFNGLPROGRAMUNIFORMMATRIX3FVPROC glad_snap_rhi_glProgramUniformMatrix3fv = NULL;
PFNGLPROGRAMUNIFORMMATRIX3X2FVPROC glad_snap_rhi_glProgramUniformMatrix3x2fv = NULL;
PFNGLPROGRAMUNIFORMMATRIX3X4FVPROC glad_snap_rhi_glProgramUniformMatrix3x4fv = NULL;
PFNGLPROGRAMUNIFORMMATRIX4FVPROC glad_snap_rhi_glProgramUniformMatrix4fv = NULL;
PFNGLPROGRAMUNIFORMMATRIX4X2FVPROC glad_snap_rhi_glProgramUniformMatrix4x2fv = NULL;
PFNGLPROGRAMUNIFORMMATRIX4X3FVPROC glad_snap_rhi_glProgramUniformMatrix4x3fv = NULL;
PFNGLPUSHDEBUGGROUPPROC glad_snap_rhi_glPushDebugGroup = NULL;
PFNGLPUSHDEBUGGROUPKHRPROC glad_snap_rhi_glPushDebugGroupKHR = NULL;
PFNGLPUSHGROUPMARKEREXTPROC glad_snap_rhi_glPushGroupMarkerEXT = NULL;
PFNGLQUERYCOUNTEREXTPROC glad_snap_rhi_glQueryCounterEXT = NULL;
PFNGLREADBUFFERPROC glad_snap_rhi_glReadBuffer = NULL;
PFNGLREADNPIXELSPROC glad_snap_rhi_glReadnPixels = NULL;
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC glad_snap_rhi_glRenderbufferStorageMultisample = NULL;
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEAPPLEPROC glad_snap_rhi_glRenderbufferStorageMultisampleAPPLE = NULL;
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC glad_snap_rhi_glRenderbufferStorageMultisampleEXT = NULL;
PFNGLRESOLVEMULTISAMPLEFRAMEBUFFERAPPLEPROC glad_snap_rhi_glResolveMultisampleFramebufferAPPLE = NULL;
PFNGLRESUMETRANSFORMFEEDBACKPROC glad_snap_rhi_glResumeTransformFeedback = NULL;
PFNGLSAMPLEMASKIPROC glad_snap_rhi_glSampleMaski = NULL;
PFNGLSAMPLERPARAMETERIIVPROC glad_snap_rhi_glSamplerParameterIiv = NULL;
PFNGLSAMPLERPARAMETERIIVEXTPROC glad_snap_rhi_glSamplerParameterIivEXT = NULL;
PFNGLSAMPLERPARAMETERIIVOESPROC glad_snap_rhi_glSamplerParameterIivOES = NULL;
PFNGLSAMPLERPARAMETERIUIVPROC glad_snap_rhi_glSamplerParameterIuiv = NULL;
PFNGLSAMPLERPARAMETERIUIVEXTPROC glad_snap_rhi_glSamplerParameterIuivEXT = NULL;
PFNGLSAMPLERPARAMETERIUIVOESPROC glad_snap_rhi_glSamplerParameterIuivOES = NULL;
PFNGLSAMPLERPARAMETERFPROC glad_snap_rhi_glSamplerParameterf = NULL;
PFNGLSAMPLERPARAMETERFVPROC glad_snap_rhi_glSamplerParameterfv = NULL;
PFNGLSAMPLERPARAMETERIPROC glad_snap_rhi_glSamplerParameteri = NULL;
PFNGLSAMPLERPARAMETERIVPROC glad_snap_rhi_glSamplerParameteriv = NULL;
PFNGLTEXBUFFERPROC glad_snap_rhi_glTexBuffer = NULL;
PFNGLTEXBUFFERRANGEPROC glad_snap_rhi_glTexBufferRange = NULL;
PFNGLTEXIMAGE3DPROC glad_snap_rhi_glTexImage3D = NULL;
PFNGLTEXIMAGE3DOESPROC glad_snap_rhi_glTexImage3DOES = NULL;
PFNGLTEXPARAMETERIIVPROC glad_snap_rhi_glTexParameterIiv = NULL;
PFNGLTEXPARAMETERIIVEXTPROC glad_snap_rhi_glTexParameterIivEXT = NULL;
PFNGLTEXPARAMETERIIVOESPROC glad_snap_rhi_glTexParameterIivOES = NULL;
PFNGLTEXPARAMETERIUIVPROC glad_snap_rhi_glTexParameterIuiv = NULL;
PFNGLTEXPARAMETERIUIVEXTPROC glad_snap_rhi_glTexParameterIuivEXT = NULL;
PFNGLTEXPARAMETERIUIVOESPROC glad_snap_rhi_glTexParameterIuivOES = NULL;
PFNGLTEXSTORAGE2DPROC glad_snap_rhi_glTexStorage2D = NULL;
PFNGLTEXSTORAGE2DMULTISAMPLEPROC glad_snap_rhi_glTexStorage2DMultisample = NULL;
PFNGLTEXSTORAGE3DPROC glad_snap_rhi_glTexStorage3D = NULL;
PFNGLTEXSTORAGE3DMULTISAMPLEPROC glad_snap_rhi_glTexStorage3DMultisample = NULL;
PFNGLTEXSUBIMAGE3DPROC glad_snap_rhi_glTexSubImage3D = NULL;
PFNGLTEXSUBIMAGE3DOESPROC glad_snap_rhi_glTexSubImage3DOES = NULL;
PFNGLTRANSFORMFEEDBACKVARYINGSPROC glad_snap_rhi_glTransformFeedbackVaryings = NULL;
PFNGLUNIFORM1UIPROC glad_snap_rhi_glUniform1ui = NULL;
PFNGLUNIFORM1UIVPROC glad_snap_rhi_glUniform1uiv = NULL;
PFNGLUNIFORM2UIPROC glad_snap_rhi_glUniform2ui = NULL;
PFNGLUNIFORM2UIVPROC glad_snap_rhi_glUniform2uiv = NULL;
PFNGLUNIFORM3UIPROC glad_snap_rhi_glUniform3ui = NULL;
PFNGLUNIFORM3UIVPROC glad_snap_rhi_glUniform3uiv = NULL;
PFNGLUNIFORM4UIPROC glad_snap_rhi_glUniform4ui = NULL;
PFNGLUNIFORM4UIVPROC glad_snap_rhi_glUniform4uiv = NULL;
PFNGLUNIFORMBLOCKBINDINGPROC glad_snap_rhi_glUniformBlockBinding = NULL;
PFNGLUNIFORMMATRIX2X3FVPROC glad_snap_rhi_glUniformMatrix2x3fv = NULL;
PFNGLUNIFORMMATRIX2X4FVPROC glad_snap_rhi_glUniformMatrix2x4fv = NULL;
PFNGLUNIFORMMATRIX3X2FVPROC glad_snap_rhi_glUniformMatrix3x2fv = NULL;
PFNGLUNIFORMMATRIX3X4FVPROC glad_snap_rhi_glUniformMatrix3x4fv = NULL;
PFNGLUNIFORMMATRIX4X2FVPROC glad_snap_rhi_glUniformMatrix4x2fv = NULL;
PFNGLUNIFORMMATRIX4X3FVPROC glad_snap_rhi_glUniformMatrix4x3fv = NULL;
PFNGLUNMAPBUFFERPROC glad_snap_rhi_glUnmapBuffer = NULL;
PFNGLUNMAPBUFFEROESPROC glad_snap_rhi_glUnmapBufferOES = NULL;
PFNGLUSEPROGRAMSTAGESPROC glad_snap_rhi_glUseProgramStages = NULL;
PFNGLVALIDATEPROGRAMPIPELINEPROC glad_snap_rhi_glValidateProgramPipeline = NULL;
PFNGLVERTEXATTRIBBINDINGPROC glad_snap_rhi_glVertexAttribBinding = NULL;
PFNGLVERTEXATTRIBDIVISORPROC glad_snap_rhi_glVertexAttribDivisor = NULL;
PFNGLVERTEXATTRIBFORMATPROC glad_snap_rhi_glVertexAttribFormat = NULL;
PFNGLVERTEXATTRIBI4IPROC glad_snap_rhi_glVertexAttribI4i = NULL;
PFNGLVERTEXATTRIBI4IVPROC glad_snap_rhi_glVertexAttribI4iv = NULL;
PFNGLVERTEXATTRIBI4UIPROC glad_snap_rhi_glVertexAttribI4ui = NULL;
PFNGLVERTEXATTRIBI4UIVPROC glad_snap_rhi_glVertexAttribI4uiv = NULL;
PFNGLVERTEXATTRIBIFORMATPROC glad_snap_rhi_glVertexAttribIFormat = NULL;
PFNGLVERTEXATTRIBIPOINTERPROC glad_snap_rhi_glVertexAttribIPointer = NULL;
PFNGLVERTEXBINDINGDIVISORPROC glad_snap_rhi_glVertexBindingDivisor = NULL;
PFNGLWAITSYNCPROC glad_snap_rhi_glWaitSync = NULL;
PFNGLWAITSYNCAPPLEPROC glad_snap_rhi_glWaitSyncAPPLE = NULL;
PFNGLCREATEMEMORYOBJECTSEXTPROC glad_snap_rhi_glCreateMemoryObjectsEXT = NULL;
PFNGLDELETEMEMORYOBJECTSEXTPROC glad_snap_rhi_glDeleteMemoryObjectsEXT = NULL;
PFNGLIMPORTMEMORYFDEXTPROC glad_snap_rhi_glImportMemoryFdEXT = NULL;
PFNGLTEXSTORAGEMEM2DEXTPROC glad_snap_rhi_glTexStorageMem2DEXT = NULL;
PFNGLTEXSTORAGEMEM3DEXTPROC glad_snap_rhi_glTexStorageMem3DEXT = NULL;

// static void glad_snap_rhi_gl_load_GL_ES_VERSION_2_0( GLADuserptrloadfunc load, void* userptr) {
//    if(!GLAD_SNAP_RHI_GL_ES_VERSION_2_0) return;
//    glad_snap_rhi_glActiveTexture = (PFNGLACTIVETEXTUREPROC) load(userptr, "glActiveTexture");
//    glad_snap_rhi_glAttachShader = (PFNGLATTACHSHADERPROC) load(userptr, "glAttachShader");
//    glad_snap_rhi_glBindAttribLocation = (PFNGLBINDATTRIBLOCATIONPROC) load(userptr, "glBindAttribLocation");
//    glad_snap_rhi_glBindBuffer = (PFNGLBINDBUFFERPROC) load(userptr, "glBindBuffer");
//    glad_snap_rhi_glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC) load(userptr, "glBindFramebuffer");
//    glad_snap_rhi_glBindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC) load(userptr, "glBindRenderbuffer");
//    glad_snap_rhi_glBindTexture = (PFNGLBINDTEXTUREPROC) load(userptr, "glBindTexture");
//    glad_snap_rhi_glBlendColor = (PFNGLBLENDCOLORPROC) load(userptr, "glBlendColor");
//    glad_snap_rhi_glBlendEquation = (PFNGLBLENDEQUATIONPROC) load(userptr, "glBlendEquation");
//    glad_snap_rhi_glBlendEquationSeparate = (PFNGLBLENDEQUATIONSEPARATEPROC) load(userptr, "glBlendEquationSeparate");
//    glad_snap_rhi_glBlendFunc = (PFNGLBLENDFUNCPROC) load(userptr, "glBlendFunc");
//    glad_snap_rhi_glBlendFuncSeparate = (PFNGLBLENDFUNCSEPARATEPROC) load(userptr, "glBlendFuncSeparate");
//    glad_snap_rhi_glBufferData = (PFNGLBUFFERDATAPROC) load(userptr, "glBufferData");
//    glad_snap_rhi_glBufferSubData = (PFNGLBUFFERSUBDATAPROC) load(userptr, "glBufferSubData");
//    glad_snap_rhi_glCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC) load(userptr, "glCheckFramebufferStatus");
//    glad_snap_rhi_glClear = (PFNGLCLEARPROC) load(userptr, "glClear");
//    glad_snap_rhi_glClearColor = (PFNGLCLEARCOLORPROC) load(userptr, "glClearColor");
//    glad_snap_rhi_glClearDepthf = (PFNGLCLEARDEPTHFPROC) load(userptr, "glClearDepthf");
//    glad_snap_rhi_glClearStencil = (PFNGLCLEARSTENCILPROC) load(userptr, "glClearStencil");
//    glad_snap_rhi_glColorMask = (PFNGLCOLORMASKPROC) load(userptr, "glColorMask");
//    glad_snap_rhi_glCompileShader = (PFNGLCOMPILESHADERPROC) load(userptr, "glCompileShader");
//    glad_snap_rhi_glCompressedTexImage2D = (PFNGLCOMPRESSEDTEXIMAGE2DPROC) load(userptr, "glCompressedTexImage2D");
//    glad_snap_rhi_glCompressedTexSubImage2D = (PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC) load(userptr, "glCompressedTexSubImage2D");
//    glad_snap_rhi_glCopyTexImage2D = (PFNGLCOPYTEXIMAGE2DPROC) load(userptr, "glCopyTexImage2D");
//    glad_snap_rhi_glCopyTexSubImage2D = (PFNGLCOPYTEXSUBIMAGE2DPROC) load(userptr, "glCopyTexSubImage2D");
//    glad_snap_rhi_glCreateProgram = (PFNGLCREATEPROGRAMPROC) load(userptr, "glCreateProgram");
//    glad_snap_rhi_glCreateShader = (PFNGLCREATESHADERPROC) load(userptr, "glCreateShader");
//    glad_snap_rhi_glCullFace = (PFNGLCULLFACEPROC) load(userptr, "glCullFace");
//    glad_snap_rhi_glDeleteBuffers = (PFNGLDELETEBUFFERSPROC) load(userptr, "glDeleteBuffers");
//    glad_snap_rhi_glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC) load(userptr, "glDeleteFramebuffers");
//    glad_snap_rhi_glDeleteProgram = (PFNGLDELETEPROGRAMPROC) load(userptr, "glDeleteProgram");
//    glad_snap_rhi_glDeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC) load(userptr, "glDeleteRenderbuffers");
//    glad_snap_rhi_glDeleteShader = (PFNGLDELETESHADERPROC) load(userptr, "glDeleteShader");
//    glad_snap_rhi_glDeleteTextures = (PFNGLDELETETEXTURESPROC) load(userptr, "glDeleteTextures");
//    glad_snap_rhi_glDepthFunc = (PFNGLDEPTHFUNCPROC) load(userptr, "glDepthFunc");
//    glad_snap_rhi_glDepthMask = (PFNGLDEPTHMASKPROC) load(userptr, "glDepthMask");
//    glad_snap_rhi_glDepthRangef = (PFNGLDEPTHRANGEFPROC) load(userptr, "glDepthRangef");
//    glad_snap_rhi_glDetachShader = (PFNGLDETACHSHADERPROC) load(userptr, "glDetachShader");
//    glad_snap_rhi_glDisable = (PFNGLDISABLEPROC) load(userptr, "glDisable");
//    glad_snap_rhi_glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC) load(userptr,
//    "glDisableVertexAttribArray"); glad_snap_rhi_glDrawArrays = (PFNGLDRAWARRAYSPROC) load(userptr, "glDrawArrays");
//    glad_snap_rhi_glDrawElements = (PFNGLDRAWELEMENTSPROC) load(userptr, "glDrawElements");
//    glad_snap_rhi_glEnable = (PFNGLENABLEPROC) load(userptr, "glEnable");
//    glad_snap_rhi_glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC) load(userptr, "glEnableVertexAttribArray");
//    glad_snap_rhi_glFinish = (PFNGLFINISHPROC) load(userptr, "glFinish");
//    glad_snap_rhi_glFlush = (PFNGLFLUSHPROC) load(userptr, "glFlush");
//    glad_snap_rhi_glFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC) load(userptr, "glFramebufferRenderbuffer");
//    glad_snap_rhi_glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC) load(userptr, "glFramebufferTexture2D");
//    glad_snap_rhi_glFrontFace = (PFNGLFRONTFACEPROC) load(userptr, "glFrontFace");
//    glad_snap_rhi_glGenBuffers = (PFNGLGENBUFFERSPROC) load(userptr, "glGenBuffers");
//    glad_snap_rhi_glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC) load(userptr, "glGenFramebuffers");
//    glad_snap_rhi_glGenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC) load(userptr, "glGenRenderbuffers");
//    glad_snap_rhi_glGenTextures = (PFNGLGENTEXTURESPROC) load(userptr, "glGenTextures");
//    glad_snap_rhi_glGenerateMipmap = (PFNGLGENERATEMIPMAPPROC) load(userptr, "glGenerateMipmap");
//    glad_snap_rhi_glGetActiveAttrib = (PFNGLGETACTIVEATTRIBPROC) load(userptr, "glGetActiveAttrib");
//    glad_snap_rhi_glGetActiveUniform = (PFNGLGETACTIVEUNIFORMPROC) load(userptr, "glGetActiveUniform");
//    glad_snap_rhi_glGetAttachedShaders = (PFNGLGETATTACHEDSHADERSPROC) load(userptr, "glGetAttachedShaders");
//    glad_snap_rhi_glGetAttribLocation = (PFNGLGETATTRIBLOCATIONPROC) load(userptr, "glGetAttribLocation");
//    glad_snap_rhi_glGetBooleanv = (PFNGLGETBOOLEANVPROC) load(userptr, "glGetBooleanv");
//    glad_snap_rhi_glGetBufferParameteriv = (PFNGLGETBUFFERPARAMETERIVPROC) load(userptr, "glGetBufferParameteriv");
//    glad_snap_rhi_glGetError = (PFNGLGETERRORPROC) load(userptr, "glGetError");
//    glad_snap_rhi_glGetFloatv = (PFNGLGETFLOATVPROC) load(userptr, "glGetFloatv");
//    glad_snap_rhi_glGetFramebufferAttachmentParameteriv = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC) load(userptr,
//    "glGetFramebufferAttachmentParameteriv"); glad_snap_rhi_glGetIntegerv = (PFNGLGETINTEGERVPROC) load(userptr,
//    "glGetIntegerv"); glad_snap_rhi_glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC) load(userptr, "glGetProgramInfoLog");
//    glad_snap_rhi_glGetProgramiv = (PFNGLGETPROGRAMIVPROC) load(userptr, "glGetProgramiv");
//    glad_snap_rhi_glGetRenderbufferParameteriv = (PFNGLGETRENDERBUFFERPARAMETERIVPROC) load(userptr,
//    "glGetRenderbufferParameteriv"); glad_snap_rhi_glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC) load(userptr,
//    "glGetShaderInfoLog"); glad_snap_rhi_glGetShaderPrecisionFormat = (PFNGLGETSHADERPRECISIONFORMATPROC) load(userptr,
//    "glGetShaderPrecisionFormat"); glad_snap_rhi_glGetShaderSource = (PFNGLGETSHADERSOURCEPROC) load(userptr,
//    "glGetShaderSource"); glad_snap_rhi_glGetShaderiv = (PFNGLGETSHADERIVPROC) load(userptr, "glGetShaderiv");
//    glad_snap_rhi_glGetString = (PFNGLGETSTRINGPROC) load(userptr, "glGetString"); glad_snap_rhi_glGetTexParameterfv =
//    (PFNGLGETTEXPARAMETERFVPROC) load(userptr, "glGetTexParameterfv"); glad_snap_rhi_glGetTexParameteriv =
//    (PFNGLGETTEXPARAMETERIVPROC) load(userptr, "glGetTexParameteriv"); glad_snap_rhi_glGetUniformLocation =
//    (PFNGLGETUNIFORMLOCATIONPROC) load(userptr, "glGetUniformLocation"); glad_snap_rhi_glGetUniformfv =
//    (PFNGLGETUNIFORMFVPROC) load(userptr, "glGetUniformfv"); glad_snap_rhi_glGetUniformiv = (PFNGLGETUNIFORMIVPROC)
//    load(userptr, "glGetUniformiv"); glad_snap_rhi_glGetVertexAttribPointerv = (PFNGLGETVERTEXATTRIBPOINTERVPROC)
//    load(userptr, "glGetVertexAttribPointerv"); glad_snap_rhi_glGetVertexAttribfv = (PFNGLGETVERTEXATTRIBFVPROC)
//    load(userptr, "glGetVertexAttribfv"); glad_snap_rhi_glGetVertexAttribiv = (PFNGLGETVERTEXATTRIBIVPROC) load(userptr,
//    "glGetVertexAttribiv"); glad_snap_rhi_glHint = (PFNGLHINTPROC) load(userptr, "glHint"); glad_snap_rhi_glIsBuffer =
//    (PFNGLISBUFFERPROC) load(userptr, "glIsBuffer"); glad_snap_rhi_glIsEnabled = (PFNGLISENABLEDPROC) load(userptr,
//    "glIsEnabled"); glad_snap_rhi_glIsFramebuffer = (PFNGLISFRAMEBUFFERPROC) load(userptr, "glIsFramebuffer");
//    glad_snap_rhi_glIsProgram = (PFNGLISPROGRAMPROC) load(userptr, "glIsProgram");
//    glad_snap_rhi_glIsRenderbuffer = (PFNGLISRENDERBUFFERPROC) load(userptr, "glIsRenderbuffer");
//    glad_snap_rhi_glIsShader = (PFNGLISSHADERPROC) load(userptr, "glIsShader");
//    glad_snap_rhi_glIsTexture = (PFNGLISTEXTUREPROC) load(userptr, "glIsTexture");
//    glad_snap_rhi_glLineWidth = (PFNGLLINEWIDTHPROC) load(userptr, "glLineWidth");
//    glad_snap_rhi_glLinkProgram = (PFNGLLINKPROGRAMPROC) load(userptr, "glLinkProgram");
//    glad_snap_rhi_glPixelStorei = (PFNGLPIXELSTOREIPROC) load(userptr, "glPixelStorei");
//    glad_snap_rhi_glPolygonOffset = (PFNGLPOLYGONOFFSETPROC) load(userptr, "glPolygonOffset");
//    glad_snap_rhi_glReadPixels = (PFNGLREADPIXELSPROC) load(userptr, "glReadPixels");
//    glad_snap_rhi_glReleaseShaderCompiler = (PFNGLRELEASESHADERCOMPILERPROC) load(userptr, "glReleaseShaderCompiler");
//    glad_snap_rhi_glRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC) load(userptr, "glRenderbufferStorage");
//    glad_snap_rhi_glSampleCoverage = (PFNGLSAMPLECOVERAGEPROC) load(userptr, "glSampleCoverage");
//    glad_snap_rhi_glScissor = (PFNGLSCISSORPROC) load(userptr, "glScissor");
//    glad_snap_rhi_glShaderBinary = (PFNGLSHADERBINARYPROC) load(userptr, "glShaderBinary");
//    glad_snap_rhi_glShaderSource = (PFNGLSHADERSOURCEPROC) load(userptr, "glShaderSource");
//    glad_snap_rhi_glStencilFunc = (PFNGLSTENCILFUNCPROC) load(userptr, "glStencilFunc");
//    glad_snap_rhi_glStencilFuncSeparate = (PFNGLSTENCILFUNCSEPARATEPROC) load(userptr, "glStencilFuncSeparate");
//    glad_snap_rhi_glStencilMask = (PFNGLSTENCILMASKPROC) load(userptr, "glStencilMask");
//    glad_snap_rhi_glStencilMaskSeparate = (PFNGLSTENCILMASKSEPARATEPROC) load(userptr, "glStencilMaskSeparate");
//    glad_snap_rhi_glStencilOp = (PFNGLSTENCILOPPROC) load(userptr, "glStencilOp");
//    glad_snap_rhi_glStencilOpSeparate = (PFNGLSTENCILOPSEPARATEPROC) load(userptr, "glStencilOpSeparate");
//    glad_snap_rhi_glTexImage2D = (PFNGLTEXIMAGE2DPROC) load(userptr, "glTexImage2D");
//    glad_snap_rhi_glTexParameterf = (PFNGLTEXPARAMETERFPROC) load(userptr, "glTexParameterf");
//    glad_snap_rhi_glTexParameterfv = (PFNGLTEXPARAMETERFVPROC) load(userptr, "glTexParameterfv");
//    glad_snap_rhi_glTexParameteri = (PFNGLTEXPARAMETERIPROC) load(userptr, "glTexParameteri");
//    glad_snap_rhi_glTexParameteriv = (PFNGLTEXPARAMETERIVPROC) load(userptr, "glTexParameteriv");
//    glad_snap_rhi_glTexSubImage2D = (PFNGLTEXSUBIMAGE2DPROC) load(userptr, "glTexSubImage2D");
//    glad_snap_rhi_glUniform1f = (PFNGLUNIFORM1FPROC) load(userptr, "glUniform1f");
//    glad_snap_rhi_glUniform1fv = (PFNGLUNIFORM1FVPROC) load(userptr, "glUniform1fv");
//    glad_snap_rhi_glUniform1i = (PFNGLUNIFORM1IPROC) load(userptr, "glUniform1i");
//    glad_snap_rhi_glUniform1iv = (PFNGLUNIFORM1IVPROC) load(userptr, "glUniform1iv");
//    glad_snap_rhi_glUniform2f = (PFNGLUNIFORM2FPROC) load(userptr, "glUniform2f");
//    glad_snap_rhi_glUniform2fv = (PFNGLUNIFORM2FVPROC) load(userptr, "glUniform2fv");
//    glad_snap_rhi_glUniform2i = (PFNGLUNIFORM2IPROC) load(userptr, "glUniform2i");
//    glad_snap_rhi_glUniform2iv = (PFNGLUNIFORM2IVPROC) load(userptr, "glUniform2iv");
//    glad_snap_rhi_glUniform3f = (PFNGLUNIFORM3FPROC) load(userptr, "glUniform3f");
//    glad_snap_rhi_glUniform3fv = (PFNGLUNIFORM3FVPROC) load(userptr, "glUniform3fv");
//    glad_snap_rhi_glUniform3i = (PFNGLUNIFORM3IPROC) load(userptr, "glUniform3i");
//    glad_snap_rhi_glUniform3iv = (PFNGLUNIFORM3IVPROC) load(userptr, "glUniform3iv");
//    glad_snap_rhi_glUniform4f = (PFNGLUNIFORM4FPROC) load(userptr, "glUniform4f");
//    glad_snap_rhi_glUniform4fv = (PFNGLUNIFORM4FVPROC) load(userptr, "glUniform4fv");
//    glad_snap_rhi_glUniform4i = (PFNGLUNIFORM4IPROC) load(userptr, "glUniform4i");
//    glad_snap_rhi_glUniform4iv = (PFNGLUNIFORM4IVPROC) load(userptr, "glUniform4iv");
//    glad_snap_rhi_glUniformMatrix2fv = (PFNGLUNIFORMMATRIX2FVPROC) load(userptr, "glUniformMatrix2fv");
//    glad_snap_rhi_glUniformMatrix3fv = (PFNGLUNIFORMMATRIX3FVPROC) load(userptr, "glUniformMatrix3fv");
//    glad_snap_rhi_glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC) load(userptr, "glUniformMatrix4fv");
//    glad_snap_rhi_glUseProgram = (PFNGLUSEPROGRAMPROC) load(userptr, "glUseProgram");
//    glad_snap_rhi_glValidateProgram = (PFNGLVALIDATEPROGRAMPROC) load(userptr, "glValidateProgram");
//    glad_snap_rhi_glVertexAttrib1f = (PFNGLVERTEXATTRIB1FPROC) load(userptr, "glVertexAttrib1f");
//    glad_snap_rhi_glVertexAttrib1fv = (PFNGLVERTEXATTRIB1FVPROC) load(userptr, "glVertexAttrib1fv");
//    glad_snap_rhi_glVertexAttrib2f = (PFNGLVERTEXATTRIB2FPROC) load(userptr, "glVertexAttrib2f");
//    glad_snap_rhi_glVertexAttrib2fv = (PFNGLVERTEXATTRIB2FVPROC) load(userptr, "glVertexAttrib2fv");
//    glad_snap_rhi_glVertexAttrib3f = (PFNGLVERTEXATTRIB3FPROC) load(userptr, "glVertexAttrib3f");
//    glad_snap_rhi_glVertexAttrib3fv = (PFNGLVERTEXATTRIB3FVPROC) load(userptr, "glVertexAttrib3fv");
//    glad_snap_rhi_glVertexAttrib4f = (PFNGLVERTEXATTRIB4FPROC) load(userptr, "glVertexAttrib4f");
//    glad_snap_rhi_glVertexAttrib4fv = (PFNGLVERTEXATTRIB4FVPROC) load(userptr, "glVertexAttrib4fv");
//    glad_snap_rhi_glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC) load(userptr, "glVertexAttribPointer");
//    glad_snap_rhi_glViewport = (PFNGLVIEWPORTPROC) load(userptr, "glViewport");
//}
static void glad_snap_rhi_gl_load_GL_ES_VERSION_3_0(GLADuserptrloadfunc load, void* userptr) {
    if (!GLAD_SNAP_RHI_GL_ES_VERSION_3_0)
        return;
    glad_snap_rhi_glBeginQuery = (PFNGLBEGINQUERYPROC)load(userptr, "glBeginQuery");
    glad_snap_rhi_glBeginTransformFeedback = (PFNGLBEGINTRANSFORMFEEDBACKPROC)load(userptr, "glBeginTransformFeedback");
    glad_snap_rhi_glBindBufferBase = (PFNGLBINDBUFFERBASEPROC)load(userptr, "glBindBufferBase");
    glad_snap_rhi_glBindBufferRange = (PFNGLBINDBUFFERRANGEPROC)load(userptr, "glBindBufferRange");
    glad_snap_rhi_glBindSampler = (PFNGLBINDSAMPLERPROC)load(userptr, "glBindSampler");
    glad_snap_rhi_glBindTransformFeedback = (PFNGLBINDTRANSFORMFEEDBACKPROC)load(userptr, "glBindTransformFeedback");
    glad_snap_rhi_glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)load(userptr, "glBindVertexArray");
    glad_snap_rhi_glBlitFramebuffer = (PFNGLBLITFRAMEBUFFERPROC)load(userptr, "glBlitFramebuffer");
    glad_snap_rhi_glClearBufferfi = (PFNGLCLEARBUFFERFIPROC)load(userptr, "glClearBufferfi");
    glad_snap_rhi_glClearBufferfv = (PFNGLCLEARBUFFERFVPROC)load(userptr, "glClearBufferfv");
    glad_snap_rhi_glClearBufferiv = (PFNGLCLEARBUFFERIVPROC)load(userptr, "glClearBufferiv");
    glad_snap_rhi_glClearBufferuiv = (PFNGLCLEARBUFFERUIVPROC)load(userptr, "glClearBufferuiv");
    glad_snap_rhi_glClientWaitSync = (PFNGLCLIENTWAITSYNCPROC)load(userptr, "glClientWaitSync");
    glad_snap_rhi_glCompressedTexImage3D = (PFNGLCOMPRESSEDTEXIMAGE3DPROC)load(userptr, "glCompressedTexImage3D");
    glad_snap_rhi_glCompressedTexSubImage3D = (PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC)load(userptr, "glCompressedTexSubImage3D");
    glad_snap_rhi_glCopyBufferSubData = (PFNGLCOPYBUFFERSUBDATAPROC)load(userptr, "glCopyBufferSubData");
    glad_snap_rhi_glCopyTexSubImage3D = (PFNGLCOPYTEXSUBIMAGE3DPROC)load(userptr, "glCopyTexSubImage3D");
    glad_snap_rhi_glDeleteQueries = (PFNGLDELETEQUERIESPROC)load(userptr, "glDeleteQueries");
    glad_snap_rhi_glDeleteSamplers = (PFNGLDELETESAMPLERSPROC)load(userptr, "glDeleteSamplers");
    glad_snap_rhi_glDeleteSync = (PFNGLDELETESYNCPROC)load(userptr, "glDeleteSync");
    glad_snap_rhi_glDeleteTransformFeedbacks = (PFNGLDELETETRANSFORMFEEDBACKSPROC)load(userptr, "glDeleteTransformFeedbacks");
    glad_snap_rhi_glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC)load(userptr, "glDeleteVertexArrays");
    glad_snap_rhi_glDrawArraysInstanced = (PFNGLDRAWARRAYSINSTANCEDPROC)load(userptr, "glDrawArraysInstanced");
    glad_snap_rhi_glDrawBuffers = (PFNGLDRAWBUFFERSPROC)load(userptr, "glDrawBuffers");
    glad_snap_rhi_glDrawElementsInstanced = (PFNGLDRAWELEMENTSINSTANCEDPROC)load(userptr, "glDrawElementsInstanced");
    glad_snap_rhi_glDrawRangeElements = (PFNGLDRAWRANGEELEMENTSPROC)load(userptr, "glDrawRangeElements");
    glad_snap_rhi_glEndQuery = (PFNGLENDQUERYPROC)load(userptr, "glEndQuery");
    glad_snap_rhi_glEndTransformFeedback = (PFNGLENDTRANSFORMFEEDBACKPROC)load(userptr, "glEndTransformFeedback");
    glad_snap_rhi_glFenceSync = (PFNGLFENCESYNCPROC)load(userptr, "glFenceSync");
    glad_snap_rhi_glFlushMappedBufferRange = (PFNGLFLUSHMAPPEDBUFFERRANGEPROC)load(userptr, "glFlushMappedBufferRange");
    glad_snap_rhi_glFramebufferTextureLayer = (PFNGLFRAMEBUFFERTEXTURELAYERPROC)load(userptr, "glFramebufferTextureLayer");
    glad_snap_rhi_glGenQueries = (PFNGLGENQUERIESPROC)load(userptr, "glGenQueries");
    glad_snap_rhi_glGenSamplers = (PFNGLGENSAMPLERSPROC)load(userptr, "glGenSamplers");
    glad_snap_rhi_glGenTransformFeedbacks = (PFNGLGENTRANSFORMFEEDBACKSPROC)load(userptr, "glGenTransformFeedbacks");
    glad_snap_rhi_glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)load(userptr, "glGenVertexArrays");
    glad_snap_rhi_glGetActiveUniformBlockName =
        (PFNGLGETACTIVEUNIFORMBLOCKNAMEPROC)load(userptr, "glGetActiveUniformBlockName");
    glad_snap_rhi_glGetActiveUniformBlockiv = (PFNGLGETACTIVEUNIFORMBLOCKIVPROC)load(userptr, "glGetActiveUniformBlockiv");
    glad_snap_rhi_glGetActiveUniformsiv = (PFNGLGETACTIVEUNIFORMSIVPROC)load(userptr, "glGetActiveUniformsiv");
    glad_snap_rhi_glGetBufferParameteri64v = (PFNGLGETBUFFERPARAMETERI64VPROC)load(userptr, "glGetBufferParameteri64v");
    glad_snap_rhi_glGetBufferPointerv = (PFNGLGETBUFFERPOINTERVPROC)load(userptr, "glGetBufferPointerv");
    glad_snap_rhi_glGetFragDataLocation = (PFNGLGETFRAGDATALOCATIONPROC)load(userptr, "glGetFragDataLocation");
    glad_snap_rhi_glGetInteger64i_v = (PFNGLGETINTEGER64I_VPROC)load(userptr, "glGetInteger64i_v");
    glad_snap_rhi_glGetInteger64v = (PFNGLGETINTEGER64VPROC)load(userptr, "glGetInteger64v");
    glad_snap_rhi_glGetIntegeri_v = (PFNGLGETINTEGERI_VPROC)load(userptr, "glGetIntegeri_v");
    glad_snap_rhi_glGetInternalformativ = (PFNGLGETINTERNALFORMATIVPROC)load(userptr, "glGetInternalformativ");
    glad_snap_rhi_glGetProgramBinary = (PFNGLGETPROGRAMBINARYPROC)load(userptr, "glGetProgramBinary");
    glad_snap_rhi_glGetQueryObjectuiv = (PFNGLGETQUERYOBJECTUIVPROC)load(userptr, "glGetQueryObjectuiv");
    glad_snap_rhi_glGetQueryiv = (PFNGLGETQUERYIVPROC)load(userptr, "glGetQueryiv");
    glad_snap_rhi_glGetSamplerParameterfv = (PFNGLGETSAMPLERPARAMETERFVPROC)load(userptr, "glGetSamplerParameterfv");
    glad_snap_rhi_glGetSamplerParameteriv = (PFNGLGETSAMPLERPARAMETERIVPROC)load(userptr, "glGetSamplerParameteriv");
    glad_snap_rhi_glGetStringi = (PFNGLGETSTRINGIPROC)load(userptr, "glGetStringi");
    glad_snap_rhi_glGetSynciv = (PFNGLGETSYNCIVPROC)load(userptr, "glGetSynciv");
    glad_snap_rhi_glGetTransformFeedbackVarying =
        (PFNGLGETTRANSFORMFEEDBACKVARYINGPROC)load(userptr, "glGetTransformFeedbackVarying");
    glad_snap_rhi_glGetUniformBlockIndex = (PFNGLGETUNIFORMBLOCKINDEXPROC)load(userptr, "glGetUniformBlockIndex");
    glad_snap_rhi_glGetUniformIndices = (PFNGLGETUNIFORMINDICESPROC)load(userptr, "glGetUniformIndices");
    glad_snap_rhi_glGetUniformuiv = (PFNGLGETUNIFORMUIVPROC)load(userptr, "glGetUniformuiv");
    glad_snap_rhi_glGetVertexAttribIiv = (PFNGLGETVERTEXATTRIBIIVPROC)load(userptr, "glGetVertexAttribIiv");
    glad_snap_rhi_glGetVertexAttribIuiv = (PFNGLGETVERTEXATTRIBIUIVPROC)load(userptr, "glGetVertexAttribIuiv");
    glad_snap_rhi_glInvalidateFramebuffer = (PFNGLINVALIDATEFRAMEBUFFERPROC)load(userptr, "glInvalidateFramebuffer");
    glad_snap_rhi_glInvalidateSubFramebuffer = (PFNGLINVALIDATESUBFRAMEBUFFERPROC)load(userptr, "glInvalidateSubFramebuffer");
    glad_snap_rhi_glIsQuery = (PFNGLISQUERYPROC)load(userptr, "glIsQuery");
    glad_snap_rhi_glIsSampler = (PFNGLISSAMPLERPROC)load(userptr, "glIsSampler");
    glad_snap_rhi_glIsSync = (PFNGLISSYNCPROC)load(userptr, "glIsSync");
    glad_snap_rhi_glIsTransformFeedback = (PFNGLISTRANSFORMFEEDBACKPROC)load(userptr, "glIsTransformFeedback");
    glad_snap_rhi_glIsVertexArray = (PFNGLISVERTEXARRAYPROC)load(userptr, "glIsVertexArray");
    glad_snap_rhi_glMapBufferRange = (PFNGLMAPBUFFERRANGEPROC)load(userptr, "glMapBufferRange");
    glad_snap_rhi_glPauseTransformFeedback = (PFNGLPAUSETRANSFORMFEEDBACKPROC)load(userptr, "glPauseTransformFeedback");
    glad_snap_rhi_glProgramBinary = (PFNGLPROGRAMBINARYPROC)load(userptr, "glProgramBinary");
    glad_snap_rhi_glProgramParameteri = (PFNGLPROGRAMPARAMETERIPROC)load(userptr, "glProgramParameteri");
    glad_snap_rhi_glReadBuffer = (PFNGLREADBUFFERPROC)load(userptr, "glReadBuffer");
    glad_snap_rhi_glRenderbufferStorageMultisample =
        (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC)load(userptr, "glRenderbufferStorageMultisample");
    glad_snap_rhi_glResumeTransformFeedback = (PFNGLRESUMETRANSFORMFEEDBACKPROC)load(userptr, "glResumeTransformFeedback");
    glad_snap_rhi_glSamplerParameterf = (PFNGLSAMPLERPARAMETERFPROC)load(userptr, "glSamplerParameterf");
    glad_snap_rhi_glSamplerParameterfv = (PFNGLSAMPLERPARAMETERFVPROC)load(userptr, "glSamplerParameterfv");
    glad_snap_rhi_glSamplerParameteri = (PFNGLSAMPLERPARAMETERIPROC)load(userptr, "glSamplerParameteri");
    glad_snap_rhi_glSamplerParameteriv = (PFNGLSAMPLERPARAMETERIVPROC)load(userptr, "glSamplerParameteriv");
    glad_snap_rhi_glTexImage3D = (PFNGLTEXIMAGE3DPROC)load(userptr, "glTexImage3D");
    glad_snap_rhi_glTexStorage2D = (PFNGLTEXSTORAGE2DPROC)load(userptr, "glTexStorage2D");
    glad_snap_rhi_glTexStorage3D = (PFNGLTEXSTORAGE3DPROC)load(userptr, "glTexStorage3D");
    glad_snap_rhi_glTexSubImage3D = (PFNGLTEXSUBIMAGE3DPROC)load(userptr, "glTexSubImage3D");
    glad_snap_rhi_glTransformFeedbackVaryings =
        (PFNGLTRANSFORMFEEDBACKVARYINGSPROC)load(userptr, "glTransformFeedbackVaryings");
    glad_snap_rhi_glUniform1ui = (PFNGLUNIFORM1UIPROC)load(userptr, "glUniform1ui");
    glad_snap_rhi_glUniform1uiv = (PFNGLUNIFORM1UIVPROC)load(userptr, "glUniform1uiv");
    glad_snap_rhi_glUniform2ui = (PFNGLUNIFORM2UIPROC)load(userptr, "glUniform2ui");
    glad_snap_rhi_glUniform2uiv = (PFNGLUNIFORM2UIVPROC)load(userptr, "glUniform2uiv");
    glad_snap_rhi_glUniform3ui = (PFNGLUNIFORM3UIPROC)load(userptr, "glUniform3ui");
    glad_snap_rhi_glUniform3uiv = (PFNGLUNIFORM3UIVPROC)load(userptr, "glUniform3uiv");
    glad_snap_rhi_glUniform4ui = (PFNGLUNIFORM4UIPROC)load(userptr, "glUniform4ui");
    glad_snap_rhi_glUniform4uiv = (PFNGLUNIFORM4UIVPROC)load(userptr, "glUniform4uiv");
    glad_snap_rhi_glUniformBlockBinding = (PFNGLUNIFORMBLOCKBINDINGPROC)load(userptr, "glUniformBlockBinding");
    glad_snap_rhi_glUniformMatrix2x3fv = (PFNGLUNIFORMMATRIX2X3FVPROC)load(userptr, "glUniformMatrix2x3fv");
    glad_snap_rhi_glUniformMatrix2x4fv = (PFNGLUNIFORMMATRIX2X4FVPROC)load(userptr, "glUniformMatrix2x4fv");
    glad_snap_rhi_glUniformMatrix3x2fv = (PFNGLUNIFORMMATRIX3X2FVPROC)load(userptr, "glUniformMatrix3x2fv");
    glad_snap_rhi_glUniformMatrix3x4fv = (PFNGLUNIFORMMATRIX3X4FVPROC)load(userptr, "glUniformMatrix3x4fv");
    glad_snap_rhi_glUniformMatrix4x2fv = (PFNGLUNIFORMMATRIX4X2FVPROC)load(userptr, "glUniformMatrix4x2fv");
    glad_snap_rhi_glUniformMatrix4x3fv = (PFNGLUNIFORMMATRIX4X3FVPROC)load(userptr, "glUniformMatrix4x3fv");
    glad_snap_rhi_glUnmapBuffer = (PFNGLUNMAPBUFFERPROC)load(userptr, "glUnmapBuffer");
    glad_snap_rhi_glVertexAttribDivisor = (PFNGLVERTEXATTRIBDIVISORPROC)load(userptr, "glVertexAttribDivisor");
    glad_snap_rhi_glVertexAttribI4i = (PFNGLVERTEXATTRIBI4IPROC)load(userptr, "glVertexAttribI4i");
    glad_snap_rhi_glVertexAttribI4iv = (PFNGLVERTEXATTRIBI4IVPROC)load(userptr, "glVertexAttribI4iv");
    glad_snap_rhi_glVertexAttribI4ui = (PFNGLVERTEXATTRIBI4UIPROC)load(userptr, "glVertexAttribI4ui");
    glad_snap_rhi_glVertexAttribI4uiv = (PFNGLVERTEXATTRIBI4UIVPROC)load(userptr, "glVertexAttribI4uiv");
    glad_snap_rhi_glVertexAttribIPointer = (PFNGLVERTEXATTRIBIPOINTERPROC)load(userptr, "glVertexAttribIPointer");
    glad_snap_rhi_glWaitSync = (PFNGLWAITSYNCPROC)load(userptr, "glWaitSync");
}
static void glad_snap_rhi_gl_load_GL_ES_VERSION_3_1(GLADuserptrloadfunc load, void* userptr) {
    if (!GLAD_SNAP_RHI_GL_ES_VERSION_3_1)
        return;
    glad_snap_rhi_glActiveShaderProgram = (PFNGLACTIVESHADERPROGRAMPROC)load(userptr, "glActiveShaderProgram");
    glad_snap_rhi_glBindImageTexture = (PFNGLBINDIMAGETEXTUREPROC)load(userptr, "glBindImageTexture");
    glad_snap_rhi_glBindProgramPipeline = (PFNGLBINDPROGRAMPIPELINEPROC)load(userptr, "glBindProgramPipeline");
    glad_snap_rhi_glBindVertexBuffer = (PFNGLBINDVERTEXBUFFERPROC)load(userptr, "glBindVertexBuffer");
    glad_snap_rhi_glCreateShaderProgramv = (PFNGLCREATESHADERPROGRAMVPROC)load(userptr, "glCreateShaderProgramv");
    glad_snap_rhi_glDeleteProgramPipelines = (PFNGLDELETEPROGRAMPIPELINESPROC)load(userptr, "glDeleteProgramPipelines");
    glad_snap_rhi_glDispatchCompute = (PFNGLDISPATCHCOMPUTEPROC)load(userptr, "glDispatchCompute");
    glad_snap_rhi_glDispatchComputeIndirect = (PFNGLDISPATCHCOMPUTEINDIRECTPROC)load(userptr, "glDispatchComputeIndirect");
    glad_snap_rhi_glDrawArraysIndirect = (PFNGLDRAWARRAYSINDIRECTPROC)load(userptr, "glDrawArraysIndirect");
    glad_snap_rhi_glDrawElementsIndirect = (PFNGLDRAWELEMENTSINDIRECTPROC)load(userptr, "glDrawElementsIndirect");
    glad_snap_rhi_glFramebufferParameteri = (PFNGLFRAMEBUFFERPARAMETERIPROC)load(userptr, "glFramebufferParameteri");
    glad_snap_rhi_glGenProgramPipelines = (PFNGLGENPROGRAMPIPELINESPROC)load(userptr, "glGenProgramPipelines");
    glad_snap_rhi_glGetBooleani_v = (PFNGLGETBOOLEANI_VPROC)load(userptr, "glGetBooleani_v");
    glad_snap_rhi_glGetFramebufferParameteriv =
        (PFNGLGETFRAMEBUFFERPARAMETERIVPROC)load(userptr, "glGetFramebufferParameteriv");
    glad_snap_rhi_glGetMultisamplefv = (PFNGLGETMULTISAMPLEFVPROC)load(userptr, "glGetMultisamplefv");
    glad_snap_rhi_glGetProgramInterfaceiv = (PFNGLGETPROGRAMINTERFACEIVPROC)load(userptr, "glGetProgramInterfaceiv");
    glad_snap_rhi_glGetProgramPipelineInfoLog =
        (PFNGLGETPROGRAMPIPELINEINFOLOGPROC)load(userptr, "glGetProgramPipelineInfoLog");
    glad_snap_rhi_glGetProgramPipelineiv = (PFNGLGETPROGRAMPIPELINEIVPROC)load(userptr, "glGetProgramPipelineiv");
    glad_snap_rhi_glGetProgramResourceIndex = (PFNGLGETPROGRAMRESOURCEINDEXPROC)load(userptr, "glGetProgramResourceIndex");
    glad_snap_rhi_glGetProgramResourceLocation =
        (PFNGLGETPROGRAMRESOURCELOCATIONPROC)load(userptr, "glGetProgramResourceLocation");
    glad_snap_rhi_glGetProgramResourceName = (PFNGLGETPROGRAMRESOURCENAMEPROC)load(userptr, "glGetProgramResourceName");
    glad_snap_rhi_glGetProgramResourceiv = (PFNGLGETPROGRAMRESOURCEIVPROC)load(userptr, "glGetProgramResourceiv");
    glad_snap_rhi_glGetTexLevelParameterfv = (PFNGLGETTEXLEVELPARAMETERFVPROC)load(userptr, "glGetTexLevelParameterfv");
    glad_snap_rhi_glGetTexLevelParameteriv = (PFNGLGETTEXLEVELPARAMETERIVPROC)load(userptr, "glGetTexLevelParameteriv");
    glad_snap_rhi_glIsProgramPipeline = (PFNGLISPROGRAMPIPELINEPROC)load(userptr, "glIsProgramPipeline");
    glad_snap_rhi_glMemoryBarrier = (PFNGLMEMORYBARRIERPROC)load(userptr, "glMemoryBarrier");
    glad_snap_rhi_glMemoryBarrierByRegion = (PFNGLMEMORYBARRIERBYREGIONPROC)load(userptr, "glMemoryBarrierByRegion");
    glad_snap_rhi_glProgramUniform1f = (PFNGLPROGRAMUNIFORM1FPROC)load(userptr, "glProgramUniform1f");
    glad_snap_rhi_glProgramUniform1fv = (PFNGLPROGRAMUNIFORM1FVPROC)load(userptr, "glProgramUniform1fv");
    glad_snap_rhi_glProgramUniform1i = (PFNGLPROGRAMUNIFORM1IPROC)load(userptr, "glProgramUniform1i");
    glad_snap_rhi_glProgramUniform1iv = (PFNGLPROGRAMUNIFORM1IVPROC)load(userptr, "glProgramUniform1iv");
    glad_snap_rhi_glProgramUniform1ui = (PFNGLPROGRAMUNIFORM1UIPROC)load(userptr, "glProgramUniform1ui");
    glad_snap_rhi_glProgramUniform1uiv = (PFNGLPROGRAMUNIFORM1UIVPROC)load(userptr, "glProgramUniform1uiv");
    glad_snap_rhi_glProgramUniform2f = (PFNGLPROGRAMUNIFORM2FPROC)load(userptr, "glProgramUniform2f");
    glad_snap_rhi_glProgramUniform2fv = (PFNGLPROGRAMUNIFORM2FVPROC)load(userptr, "glProgramUniform2fv");
    glad_snap_rhi_glProgramUniform2i = (PFNGLPROGRAMUNIFORM2IPROC)load(userptr, "glProgramUniform2i");
    glad_snap_rhi_glProgramUniform2iv = (PFNGLPROGRAMUNIFORM2IVPROC)load(userptr, "glProgramUniform2iv");
    glad_snap_rhi_glProgramUniform2ui = (PFNGLPROGRAMUNIFORM2UIPROC)load(userptr, "glProgramUniform2ui");
    glad_snap_rhi_glProgramUniform2uiv = (PFNGLPROGRAMUNIFORM2UIVPROC)load(userptr, "glProgramUniform2uiv");
    glad_snap_rhi_glProgramUniform3f = (PFNGLPROGRAMUNIFORM3FPROC)load(userptr, "glProgramUniform3f");
    glad_snap_rhi_glProgramUniform3fv = (PFNGLPROGRAMUNIFORM3FVPROC)load(userptr, "glProgramUniform3fv");
    glad_snap_rhi_glProgramUniform3i = (PFNGLPROGRAMUNIFORM3IPROC)load(userptr, "glProgramUniform3i");
    glad_snap_rhi_glProgramUniform3iv = (PFNGLPROGRAMUNIFORM3IVPROC)load(userptr, "glProgramUniform3iv");
    glad_snap_rhi_glProgramUniform3ui = (PFNGLPROGRAMUNIFORM3UIPROC)load(userptr, "glProgramUniform3ui");
    glad_snap_rhi_glProgramUniform3uiv = (PFNGLPROGRAMUNIFORM3UIVPROC)load(userptr, "glProgramUniform3uiv");
    glad_snap_rhi_glProgramUniform4f = (PFNGLPROGRAMUNIFORM4FPROC)load(userptr, "glProgramUniform4f");
    glad_snap_rhi_glProgramUniform4fv = (PFNGLPROGRAMUNIFORM4FVPROC)load(userptr, "glProgramUniform4fv");
    glad_snap_rhi_glProgramUniform4i = (PFNGLPROGRAMUNIFORM4IPROC)load(userptr, "glProgramUniform4i");
    glad_snap_rhi_glProgramUniform4iv = (PFNGLPROGRAMUNIFORM4IVPROC)load(userptr, "glProgramUniform4iv");
    glad_snap_rhi_glProgramUniform4ui = (PFNGLPROGRAMUNIFORM4UIPROC)load(userptr, "glProgramUniform4ui");
    glad_snap_rhi_glProgramUniform4uiv = (PFNGLPROGRAMUNIFORM4UIVPROC)load(userptr, "glProgramUniform4uiv");
    glad_snap_rhi_glProgramUniformMatrix2fv = (PFNGLPROGRAMUNIFORMMATRIX2FVPROC)load(userptr, "glProgramUniformMatrix2fv");
    glad_snap_rhi_glProgramUniformMatrix2x3fv =
        (PFNGLPROGRAMUNIFORMMATRIX2X3FVPROC)load(userptr, "glProgramUniformMatrix2x3fv");
    glad_snap_rhi_glProgramUniformMatrix2x4fv =
        (PFNGLPROGRAMUNIFORMMATRIX2X4FVPROC)load(userptr, "glProgramUniformMatrix2x4fv");
    glad_snap_rhi_glProgramUniformMatrix3fv = (PFNGLPROGRAMUNIFORMMATRIX3FVPROC)load(userptr, "glProgramUniformMatrix3fv");
    glad_snap_rhi_glProgramUniformMatrix3x2fv =
        (PFNGLPROGRAMUNIFORMMATRIX3X2FVPROC)load(userptr, "glProgramUniformMatrix3x2fv");
    glad_snap_rhi_glProgramUniformMatrix3x4fv =
        (PFNGLPROGRAMUNIFORMMATRIX3X4FVPROC)load(userptr, "glProgramUniformMatrix3x4fv");
    glad_snap_rhi_glProgramUniformMatrix4fv = (PFNGLPROGRAMUNIFORMMATRIX4FVPROC)load(userptr, "glProgramUniformMatrix4fv");
    glad_snap_rhi_glProgramUniformMatrix4x2fv =
        (PFNGLPROGRAMUNIFORMMATRIX4X2FVPROC)load(userptr, "glProgramUniformMatrix4x2fv");
    glad_snap_rhi_glProgramUniformMatrix4x3fv =
        (PFNGLPROGRAMUNIFORMMATRIX4X3FVPROC)load(userptr, "glProgramUniformMatrix4x3fv");
    glad_snap_rhi_glSampleMaski = (PFNGLSAMPLEMASKIPROC)load(userptr, "glSampleMaski");
    glad_snap_rhi_glTexStorage2DMultisample = (PFNGLTEXSTORAGE2DMULTISAMPLEPROC)load(userptr, "glTexStorage2DMultisample");
    glad_snap_rhi_glUseProgramStages = (PFNGLUSEPROGRAMSTAGESPROC)load(userptr, "glUseProgramStages");
    glad_snap_rhi_glValidateProgramPipeline = (PFNGLVALIDATEPROGRAMPIPELINEPROC)load(userptr, "glValidateProgramPipeline");
    glad_snap_rhi_glVertexAttribBinding = (PFNGLVERTEXATTRIBBINDINGPROC)load(userptr, "glVertexAttribBinding");
    glad_snap_rhi_glVertexAttribFormat = (PFNGLVERTEXATTRIBFORMATPROC)load(userptr, "glVertexAttribFormat");
    glad_snap_rhi_glVertexAttribIFormat = (PFNGLVERTEXATTRIBIFORMATPROC)load(userptr, "glVertexAttribIFormat");
    glad_snap_rhi_glVertexBindingDivisor = (PFNGLVERTEXBINDINGDIVISORPROC)load(userptr, "glVertexBindingDivisor");
}
static void glad_snap_rhi_gl_load_GL_ES_VERSION_3_2(GLADuserptrloadfunc load, void* userptr) {
    if (!GLAD_SNAP_RHI_GL_ES_VERSION_3_2)
        return;
    glad_snap_rhi_glBlendBarrier = (PFNGLBLENDBARRIERPROC)load(userptr, "glBlendBarrier");
    glad_snap_rhi_glBlendEquationSeparatei = (PFNGLBLENDEQUATIONSEPARATEIPROC)load(userptr, "glBlendEquationSeparatei");
    glad_snap_rhi_glBlendEquationi = (PFNGLBLENDEQUATIONIPROC)load(userptr, "glBlendEquationi");
    glad_snap_rhi_glBlendFuncSeparatei = (PFNGLBLENDFUNCSEPARATEIPROC)load(userptr, "glBlendFuncSeparatei");
    glad_snap_rhi_glBlendFunci = (PFNGLBLENDFUNCIPROC)load(userptr, "glBlendFunci");
    glad_snap_rhi_glColorMaski = (PFNGLCOLORMASKIPROC)load(userptr, "glColorMaski");
    glad_snap_rhi_glCopyImageSubData = (PFNGLCOPYIMAGESUBDATAPROC)load(userptr, "glCopyImageSubData");
    glad_snap_rhi_glDebugMessageCallback = (PFNGLDEBUGMESSAGECALLBACKPROC)load(userptr, "glDebugMessageCallback");
    glad_snap_rhi_glDebugMessageControl = (PFNGLDEBUGMESSAGECONTROLPROC)load(userptr, "glDebugMessageControl");
    glad_snap_rhi_glDebugMessageInsert = (PFNGLDEBUGMESSAGEINSERTPROC)load(userptr, "glDebugMessageInsert");
    glad_snap_rhi_glDisablei = (PFNGLDISABLEIPROC)load(userptr, "glDisablei");
    glad_snap_rhi_glDrawElementsBaseVertex = (PFNGLDRAWELEMENTSBASEVERTEXPROC)load(userptr, "glDrawElementsBaseVertex");
    glad_snap_rhi_glDrawElementsInstancedBaseVertex =
        (PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXPROC)load(userptr, "glDrawElementsInstancedBaseVertex");
    glad_snap_rhi_glDrawRangeElementsBaseVertex =
        (PFNGLDRAWRANGEELEMENTSBASEVERTEXPROC)load(userptr, "glDrawRangeElementsBaseVertex");
    glad_snap_rhi_glEnablei = (PFNGLENABLEIPROC)load(userptr, "glEnablei");
    glad_snap_rhi_glFramebufferTexture = (PFNGLFRAMEBUFFERTEXTUREPROC)load(userptr, "glFramebufferTexture");
    glad_snap_rhi_glGetDebugMessageLog = (PFNGLGETDEBUGMESSAGELOGPROC)load(userptr, "glGetDebugMessageLog");
    glad_snap_rhi_glGetGraphicsResetStatus = (PFNGLGETGRAPHICSRESETSTATUSPROC)load(userptr, "glGetGraphicsResetStatus");
    glad_snap_rhi_glGetObjectLabel = (PFNGLGETOBJECTLABELPROC)load(userptr, "glGetObjectLabel");
    glad_snap_rhi_glGetObjectPtrLabel = (PFNGLGETOBJECTPTRLABELPROC)load(userptr, "glGetObjectPtrLabel");
    glad_snap_rhi_glGetPointerv = (PFNGLGETPOINTERVPROC)load(userptr, "glGetPointerv");
    glad_snap_rhi_glGetSamplerParameterIiv = (PFNGLGETSAMPLERPARAMETERIIVPROC)load(userptr, "glGetSamplerParameterIiv");
    glad_snap_rhi_glGetSamplerParameterIuiv = (PFNGLGETSAMPLERPARAMETERIUIVPROC)load(userptr, "glGetSamplerParameterIuiv");
    glad_snap_rhi_glGetTexParameterIiv = (PFNGLGETTEXPARAMETERIIVPROC)load(userptr, "glGetTexParameterIiv");
    glad_snap_rhi_glGetTexParameterIuiv = (PFNGLGETTEXPARAMETERIUIVPROC)load(userptr, "glGetTexParameterIuiv");
    glad_snap_rhi_glGetnUniformfv = (PFNGLGETNUNIFORMFVPROC)load(userptr, "glGetnUniformfv");
    glad_snap_rhi_glGetnUniformiv = (PFNGLGETNUNIFORMIVPROC)load(userptr, "glGetnUniformiv");
    glad_snap_rhi_glGetnUniformuiv = (PFNGLGETNUNIFORMUIVPROC)load(userptr, "glGetnUniformuiv");
    glad_snap_rhi_glIsEnabledi = (PFNGLISENABLEDIPROC)load(userptr, "glIsEnabledi");
    glad_snap_rhi_glMinSampleShading = (PFNGLMINSAMPLESHADINGPROC)load(userptr, "glMinSampleShading");
    glad_snap_rhi_glObjectLabel = (PFNGLOBJECTLABELPROC)load(userptr, "glObjectLabel");
    glad_snap_rhi_glObjectPtrLabel = (PFNGLOBJECTPTRLABELPROC)load(userptr, "glObjectPtrLabel");
    glad_snap_rhi_glPatchParameteri = (PFNGLPATCHPARAMETERIPROC)load(userptr, "glPatchParameteri");
    glad_snap_rhi_glPopDebugGroup = (PFNGLPOPDEBUGGROUPPROC)load(userptr, "glPopDebugGroup");
    glad_snap_rhi_glPrimitiveBoundingBox = (PFNGLPRIMITIVEBOUNDINGBOXPROC)load(userptr, "glPrimitiveBoundingBox");
    glad_snap_rhi_glPushDebugGroup = (PFNGLPUSHDEBUGGROUPPROC)load(userptr, "glPushDebugGroup");
    glad_snap_rhi_glReadnPixels = (PFNGLREADNPIXELSPROC)load(userptr, "glReadnPixels");
    glad_snap_rhi_glSamplerParameterIiv = (PFNGLSAMPLERPARAMETERIIVPROC)load(userptr, "glSamplerParameterIiv");
    glad_snap_rhi_glSamplerParameterIuiv = (PFNGLSAMPLERPARAMETERIUIVPROC)load(userptr, "glSamplerParameterIuiv");
    glad_snap_rhi_glTexBuffer = (PFNGLTEXBUFFERPROC)load(userptr, "glTexBuffer");
    glad_snap_rhi_glTexBufferRange = (PFNGLTEXBUFFERRANGEPROC)load(userptr, "glTexBufferRange");
    glad_snap_rhi_glTexParameterIiv = (PFNGLTEXPARAMETERIIVPROC)load(userptr, "glTexParameterIiv");
    glad_snap_rhi_glTexParameterIuiv = (PFNGLTEXPARAMETERIUIVPROC)load(userptr, "glTexParameterIuiv");
    glad_snap_rhi_glTexStorage3DMultisample = (PFNGLTEXSTORAGE3DMULTISAMPLEPROC)load(userptr, "glTexStorage3DMultisample");
}
static void glad_snap_rhi_gl_load_GL_APPLE_framebuffer_multisample(GLADuserptrloadfunc load, void* userptr) {
    if (!GLAD_SNAP_RHI_GL_APPLE_framebuffer_multisample)
        return;
    glad_snap_rhi_glRenderbufferStorageMultisampleAPPLE =
        (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEAPPLEPROC)load(userptr, "glRenderbufferStorageMultisampleAPPLE");
    glad_snap_rhi_glResolveMultisampleFramebufferAPPLE =
        (PFNGLRESOLVEMULTISAMPLEFRAMEBUFFERAPPLEPROC)load(userptr, "glResolveMultisampleFramebufferAPPLE");
}
static void glad_snap_rhi_gl_load_GL_APPLE_sync(GLADuserptrloadfunc load, void* userptr) {
    if (!GLAD_SNAP_RHI_GL_APPLE_sync)
        return;
    glad_snap_rhi_glClientWaitSyncAPPLE = (PFNGLCLIENTWAITSYNCAPPLEPROC)load(userptr, "glClientWaitSyncAPPLE");
    glad_snap_rhi_glDeleteSyncAPPLE = (PFNGLDELETESYNCAPPLEPROC)load(userptr, "glDeleteSyncAPPLE");
    glad_snap_rhi_glFenceSyncAPPLE = (PFNGLFENCESYNCAPPLEPROC)load(userptr, "glFenceSyncAPPLE");
    glad_snap_rhi_glGetInteger64vAPPLE = (PFNGLGETINTEGER64VAPPLEPROC)load(userptr, "glGetInteger64vAPPLE");
    glad_snap_rhi_glGetSyncivAPPLE = (PFNGLGETSYNCIVAPPLEPROC)load(userptr, "glGetSyncivAPPLE");
    glad_snap_rhi_glIsSyncAPPLE = (PFNGLISSYNCAPPLEPROC)load(userptr, "glIsSyncAPPLE");
    glad_snap_rhi_glWaitSyncAPPLE = (PFNGLWAITSYNCAPPLEPROC)load(userptr, "glWaitSyncAPPLE");
}
static void glad_snap_rhi_gl_load_GL_EXT_debug_marker(GLADuserptrloadfunc load, void* userptr) {
    if (!GLAD_SNAP_RHI_GL_EXT_debug_marker)
        return;
    glad_snap_rhi_glInsertEventMarkerEXT = (PFNGLINSERTEVENTMARKEREXTPROC)load(userptr, "glInsertEventMarkerEXT");
    glad_snap_rhi_glPopGroupMarkerEXT = (PFNGLPOPGROUPMARKEREXTPROC)load(userptr, "glPopGroupMarkerEXT");
    glad_snap_rhi_glPushGroupMarkerEXT = (PFNGLPUSHGROUPMARKEREXTPROC)load(userptr, "glPushGroupMarkerEXT");
}
static void glad_snap_rhi_gl_load_GL_EXT_discard_framebuffer(GLADuserptrloadfunc load, void* userptr) {
    if (!GLAD_SNAP_RHI_GL_EXT_discard_framebuffer)
        return;
    glad_snap_rhi_glDiscardFramebufferEXT = (PFNGLDISCARDFRAMEBUFFEREXTPROC)load(userptr, "glDiscardFramebufferEXT");
}
static void glad_snap_rhi_gl_load_GL_EXT_disjoint_timer_query(GLADuserptrloadfunc load, void* userptr) {
    if (!GLAD_SNAP_RHI_GL_EXT_disjoint_timer_query)
        return;
    glad_snap_rhi_glBeginQueryEXT = (PFNGLBEGINQUERYEXTPROC)load(userptr, "glBeginQueryEXT");
    glad_snap_rhi_glDeleteQueriesEXT = (PFNGLDELETEQUERIESEXTPROC)load(userptr, "glDeleteQueriesEXT");
    glad_snap_rhi_glEndQueryEXT = (PFNGLENDQUERYEXTPROC)load(userptr, "glEndQueryEXT");
    glad_snap_rhi_glGenQueriesEXT = (PFNGLGENQUERIESEXTPROC)load(userptr, "glGenQueriesEXT");
    glad_snap_rhi_glGetInteger64vEXT = (PFNGLGETINTEGER64VEXTPROC)load(userptr, "glGetInteger64vEXT");
    glad_snap_rhi_glGetQueryObjecti64vEXT = (PFNGLGETQUERYOBJECTI64VEXTPROC)load(userptr, "glGetQueryObjecti64vEXT");
    glad_snap_rhi_glGetQueryObjectivEXT = (PFNGLGETQUERYOBJECTIVEXTPROC)load(userptr, "glGetQueryObjectivEXT");
    glad_snap_rhi_glGetQueryObjectui64vEXT = (PFNGLGETQUERYOBJECTUI64VEXTPROC)load(userptr, "glGetQueryObjectui64vEXT");
    glad_snap_rhi_glGetQueryObjectuivEXT = (PFNGLGETQUERYOBJECTUIVEXTPROC)load(userptr, "glGetQueryObjectuivEXT");
    glad_snap_rhi_glGetQueryivEXT = (PFNGLGETQUERYIVEXTPROC)load(userptr, "glGetQueryivEXT");
    glad_snap_rhi_glIsQueryEXT = (PFNGLISQUERYEXTPROC)load(userptr, "glIsQueryEXT");
    glad_snap_rhi_glQueryCounterEXT = (PFNGLQUERYCOUNTEREXTPROC)load(userptr, "glQueryCounterEXT");
}
static void glad_snap_rhi_gl_load_GL_EXT_draw_instanced(GLADuserptrloadfunc load, void* userptr) {
    if (!GLAD_SNAP_RHI_GL_EXT_draw_instanced)
        return;
    glad_snap_rhi_glDrawArraysInstancedEXT = (PFNGLDRAWARRAYSINSTANCEDEXTPROC)load(userptr, "glDrawArraysInstancedEXT");
    glad_snap_rhi_glDrawElementsInstancedEXT = (PFNGLDRAWELEMENTSINSTANCEDEXTPROC)load(userptr, "glDrawElementsInstancedEXT");
}
static void glad_snap_rhi_gl_load_GL_EXT_multisampled_render_to_texture(GLADuserptrloadfunc load, void* userptr) {
    if (!GLAD_SNAP_RHI_GL_EXT_multisampled_render_to_texture)
        return;
    glad_snap_rhi_glFramebufferTexture2DMultisampleEXT =
        (PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC)load(userptr, "glFramebufferTexture2DMultisampleEXT");
    glad_snap_rhi_glRenderbufferStorageMultisampleEXT =
        (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC)load(userptr, "glRenderbufferStorageMultisampleEXT");
}
static void glad_snap_rhi_gl_load_GL_EXT_shader_framebuffer_fetch_non_coherent(GLADuserptrloadfunc load, void* userptr) {
    if (!GLAD_SNAP_RHI_GL_EXT_shader_framebuffer_fetch_non_coherent)
        return;
    glad_snap_rhi_glFramebufferFetchBarrierEXT =
        (PFNGLFRAMEBUFFERFETCHBARRIEREXTPROC)load(userptr, "glFramebufferFetchBarrierEXT");
}
static void glad_snap_rhi_gl_load_GL_EXT_texture_border_clamp(GLADuserptrloadfunc load, void* userptr) {
    if (!GLAD_SNAP_RHI_GL_EXT_texture_border_clamp)
        return;
    glad_snap_rhi_glGetSamplerParameterIivEXT =
        (PFNGLGETSAMPLERPARAMETERIIVEXTPROC)load(userptr, "glGetSamplerParameterIivEXT");
    glad_snap_rhi_glGetSamplerParameterIuivEXT =
        (PFNGLGETSAMPLERPARAMETERIUIVEXTPROC)load(userptr, "glGetSamplerParameterIuivEXT");
    glad_snap_rhi_glGetTexParameterIivEXT = (PFNGLGETTEXPARAMETERIIVEXTPROC)load(userptr, "glGetTexParameterIivEXT");
    glad_snap_rhi_glGetTexParameterIuivEXT = (PFNGLGETTEXPARAMETERIUIVEXTPROC)load(userptr, "glGetTexParameterIuivEXT");
    glad_snap_rhi_glSamplerParameterIivEXT = (PFNGLSAMPLERPARAMETERIIVEXTPROC)load(userptr, "glSamplerParameterIivEXT");
    glad_snap_rhi_glSamplerParameterIuivEXT = (PFNGLSAMPLERPARAMETERIUIVEXTPROC)load(userptr, "glSamplerParameterIuivEXT");
    glad_snap_rhi_glTexParameterIivEXT = (PFNGLTEXPARAMETERIIVEXTPROC)load(userptr, "glTexParameterIivEXT");
    glad_snap_rhi_glTexParameterIuivEXT = (PFNGLTEXPARAMETERIUIVEXTPROC)load(userptr, "glTexParameterIuivEXT");
}
static void glad_snap_rhi_gl_load_GL_KHR_debug(GLADuserptrloadfunc load, void* userptr) {
    if (!GLAD_SNAP_RHI_GL_KHR_debug)
        return;
    glad_snap_rhi_glDebugMessageCallbackKHR = (PFNGLDEBUGMESSAGECALLBACKKHRPROC)load(userptr, "glDebugMessageCallbackKHR");
    glad_snap_rhi_glDebugMessageControlKHR = (PFNGLDEBUGMESSAGECONTROLKHRPROC)load(userptr, "glDebugMessageControlKHR");
    glad_snap_rhi_glDebugMessageInsertKHR = (PFNGLDEBUGMESSAGEINSERTKHRPROC)load(userptr, "glDebugMessageInsertKHR");
    glad_snap_rhi_glGetDebugMessageLogKHR = (PFNGLGETDEBUGMESSAGELOGKHRPROC)load(userptr, "glGetDebugMessageLogKHR");
    glad_snap_rhi_glGetObjectLabelKHR = (PFNGLGETOBJECTLABELKHRPROC)load(userptr, "glGetObjectLabelKHR");
    glad_snap_rhi_glGetObjectPtrLabelKHR = (PFNGLGETOBJECTPTRLABELKHRPROC)load(userptr, "glGetObjectPtrLabelKHR");
    glad_snap_rhi_glGetPointervKHR = (PFNGLGETPOINTERVKHRPROC)load(userptr, "glGetPointervKHR");
    glad_snap_rhi_glObjectLabelKHR = (PFNGLOBJECTLABELKHRPROC)load(userptr, "glObjectLabelKHR");
    glad_snap_rhi_glObjectPtrLabelKHR = (PFNGLOBJECTPTRLABELKHRPROC)load(userptr, "glObjectPtrLabelKHR");
    glad_snap_rhi_glPopDebugGroupKHR = (PFNGLPOPDEBUGGROUPKHRPROC)load(userptr, "glPopDebugGroupKHR");
    glad_snap_rhi_glPushDebugGroupKHR = (PFNGLPUSHDEBUGGROUPKHRPROC)load(userptr, "glPushDebugGroupKHR");
}
static void glad_snap_rhi_gl_load_GL_OES_EGL_image(GLADuserptrloadfunc load, void* userptr) {
    if (!GLAD_SNAP_RHI_GL_OES_EGL_image)
        return;
    glad_snap_rhi_glEGLImageTargetRenderbufferStorageOES =
        (PFNGLEGLIMAGETARGETRENDERBUFFERSTORAGEOESPROC)load(userptr, "glEGLImageTargetRenderbufferStorageOES");
    glad_snap_rhi_glEGLImageTargetTexture2DOES =
        (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)load(userptr, "glEGLImageTargetTexture2DOES");
}
static void glad_snap_rhi_gl_load_GL_OES_get_program_binary(GLADuserptrloadfunc load, void* userptr) {
    if (!GLAD_SNAP_RHI_GL_OES_get_program_binary)
        return;
    glad_snap_rhi_glGetProgramBinaryOES = (PFNGLGETPROGRAMBINARYOESPROC)load(userptr, "glGetProgramBinaryOES");
    glad_snap_rhi_glProgramBinaryOES = (PFNGLPROGRAMBINARYOESPROC)load(userptr, "glProgramBinaryOES");
}
static void glad_snap_rhi_gl_load_GL_OES_mapbuffer(GLADuserptrloadfunc load, void* userptr) {
    if (!GLAD_SNAP_RHI_GL_OES_mapbuffer)
        return;
    glad_snap_rhi_glGetBufferPointervOES = (PFNGLGETBUFFERPOINTERVOESPROC)load(userptr, "glGetBufferPointervOES");
    glad_snap_rhi_glMapBufferOES = (PFNGLMAPBUFFEROESPROC)load(userptr, "glMapBufferOES");
    glad_snap_rhi_glUnmapBufferOES = (PFNGLUNMAPBUFFEROESPROC)load(userptr, "glUnmapBufferOES");
}
static void glad_snap_rhi_gl_load_GL_OES_texture_3D(GLADuserptrloadfunc load, void* userptr) {
    if (!GLAD_SNAP_RHI_GL_OES_texture_3D)
        return;
    glad_snap_rhi_glCompressedTexImage3DOES = (PFNGLCOMPRESSEDTEXIMAGE3DOESPROC)load(userptr, "glCompressedTexImage3DOES");
    glad_snap_rhi_glCompressedTexSubImage3DOES =
        (PFNGLCOMPRESSEDTEXSUBIMAGE3DOESPROC)load(userptr, "glCompressedTexSubImage3DOES");
    glad_snap_rhi_glCopyTexSubImage3DOES = (PFNGLCOPYTEXSUBIMAGE3DOESPROC)load(userptr, "glCopyTexSubImage3DOES");
    glad_snap_rhi_glFramebufferTexture3DOES = (PFNGLFRAMEBUFFERTEXTURE3DOESPROC)load(userptr, "glFramebufferTexture3DOES");
    glad_snap_rhi_glTexImage3DOES = (PFNGLTEXIMAGE3DOESPROC)load(userptr, "glTexImage3DOES");
    glad_snap_rhi_glTexSubImage3DOES = (PFNGLTEXSUBIMAGE3DOESPROC)load(userptr, "glTexSubImage3DOES");
}
static void glad_snap_rhi_gl_load_GL_OES_texture_border_clamp(GLADuserptrloadfunc load, void* userptr) {
    if (!GLAD_SNAP_RHI_GL_OES_texture_border_clamp)
        return;
    glad_snap_rhi_glGetSamplerParameterIivOES =
        (PFNGLGETSAMPLERPARAMETERIIVOESPROC)load(userptr, "glGetSamplerParameterIivOES");
    glad_snap_rhi_glGetSamplerParameterIuivOES =
        (PFNGLGETSAMPLERPARAMETERIUIVOESPROC)load(userptr, "glGetSamplerParameterIuivOES");
    glad_snap_rhi_glGetTexParameterIivOES = (PFNGLGETTEXPARAMETERIIVOESPROC)load(userptr, "glGetTexParameterIivOES");
    glad_snap_rhi_glGetTexParameterIuivOES = (PFNGLGETTEXPARAMETERIUIVOESPROC)load(userptr, "glGetTexParameterIuivOES");
    glad_snap_rhi_glSamplerParameterIivOES = (PFNGLSAMPLERPARAMETERIIVOESPROC)load(userptr, "glSamplerParameterIivOES");
    glad_snap_rhi_glSamplerParameterIuivOES = (PFNGLSAMPLERPARAMETERIUIVOESPROC)load(userptr, "glSamplerParameterIuivOES");
    glad_snap_rhi_glTexParameterIivOES = (PFNGLTEXPARAMETERIIVOESPROC)load(userptr, "glTexParameterIivOES");
    glad_snap_rhi_glTexParameterIuivOES = (PFNGLTEXPARAMETERIUIVOESPROC)load(userptr, "glTexParameterIuivOES");
}
static void glad_snap_rhi_gl_load_GL_OVR_multiview(GLADuserptrloadfunc load, void* userptr) {
    if (!GLAD_SNAP_RHI_GL_OVR_multiview)
        return;
    glad_snap_rhi_glFramebufferTextureMultiviewOVR =
        (PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC)load(userptr, "glFramebufferTextureMultiviewOVR");
}
static void glad_snap_rhi_gl_load_GL_OVR_multiview_multisampled_render_to_texture(GLADuserptrloadfunc load, void* userptr) {
    if (!GLAD_SNAP_RHI_GL_OVR_multiview_multisampled_render_to_texture)
        return;
    glad_snap_rhi_glFramebufferTextureMultisampleMultiviewOVR = (PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVRPROC)load(
        userptr, "glFramebufferTextureMultisampleMultiviewOVR");
}

#if defined(GL_ES_VERSION_3_0) || defined(GL_VERSION_3_0)
#define GLAD_SNAP_RHI_GL_IS_SOME_NEW_VERSION 1
#else
#define GLAD_SNAP_RHI_GL_IS_SOME_NEW_VERSION 0
#endif

static void glad_snap_rhi_gl_load_GL_EXT_memory_object(GLADuserptrloadfunc load, void* userptr) {
    if (!GLAD_SNAP_RHI_GL_EXT_memory_object)
        return;
    glad_snap_rhi_glCreateMemoryObjectsEXT = (PFNGLCREATEMEMORYOBJECTSEXTPROC)load(userptr, "glCreateMemoryObjectsEXT");
    glad_snap_rhi_glDeleteMemoryObjectsEXT = (PFNGLDELETEMEMORYOBJECTSEXTPROC)load(userptr, "glDeleteMemoryObjectsEXT");
    glad_snap_rhi_glTexStorageMem2DEXT = (PFNGLTEXSTORAGEMEM2DEXTPROC)load(userptr, "glTexStorageMem2DEXT");
    glad_snap_rhi_glTexStorageMem3DEXT = (PFNGLTEXSTORAGEMEM3DEXTPROC)load(userptr, "glTexStorageMem3DEXT");
}
static void glad_snap_rhi_gl_load_GL_EXT_memory_object_fd(GLADuserptrloadfunc load, void* userptr) {
    if (!GLAD_SNAP_RHI_GL_EXT_memory_object_fd)
        return;
    glad_snap_rhi_glImportMemoryFdEXT = (PFNGLIMPORTMEMORYFDEXTPROC)load(userptr, "glImportMemoryFdEXT");
}

static int glad_snap_rhi_gl_get_extensions(int version,
                                     const char** out_exts,
                                     unsigned int* out_num_exts_i,
                                     char*** out_exts_i) {
#if GLAD_SNAP_RHI_GL_IS_SOME_NEW_VERSION
    if (GLAD_VERSION_MAJOR(version) < 3) {
#else
    (void)version;
    (void)out_num_exts_i;
    (void)out_exts_i;
#endif
        //        if (glad_snap_rhi_glGetString == NULL) {
        //            return 0;
        //        }
        *out_exts = (const char*)glGetString(GL_EXTENSIONS);
#if GLAD_SNAP_RHI_GL_IS_SOME_NEW_VERSION
    } else {
        unsigned int index = 0;
        unsigned int num_exts_i = 0;
        char** exts_i = NULL;
        if (glad_snap_rhi_glGetStringi == NULL) {
            return 0;
        }
        glGetIntegerv(GL_NUM_EXTENSIONS, (int*)&num_exts_i);
        if (num_exts_i > 0) {
            exts_i = (char**)malloc(num_exts_i * (sizeof *exts_i));
        }
        if (exts_i == NULL) {
            return 0;
        }
        for (index = 0; index < num_exts_i; index++) {
            const char* gl_str_tmp = (const char*)glad_snap_rhi_glGetStringi(GL_EXTENSIONS, index);
            size_t len = strlen(gl_str_tmp) + 1;

            char* local_str = (char*)malloc(len * sizeof(char));
            if (local_str != NULL) {
                memcpy(local_str, gl_str_tmp, len * sizeof(char));
            }

            exts_i[index] = local_str;
        }

        *out_num_exts_i = num_exts_i;
        *out_exts_i = exts_i;
    }
#endif
    return 1;
}
static void glad_snap_rhi_gl_free_extensions(char** exts_i, unsigned int num_exts_i) {
    if (exts_i != NULL) {
        unsigned int index;
        for (index = 0; index < num_exts_i; index++) {
            free((void*)(exts_i[index]));
        }
        free((void*)exts_i);
        exts_i = NULL;
    }
}
static int glad_snap_rhi_gl_has_extension(
    int version, const char* exts, unsigned int num_exts_i, char** exts_i, const char* ext) {
    if (GLAD_VERSION_MAJOR(version) < 3 || !GLAD_SNAP_RHI_GL_IS_SOME_NEW_VERSION) {
        const char* extensions;
        const char* loc;
        const char* terminator;
        extensions = exts;
        if (extensions == NULL || ext == NULL) {
            return 0;
        }
        while (1) {
            loc = strstr(extensions, ext);
            if (loc == NULL) {
                return 0;
            }
            terminator = loc + strlen(ext);
            if ((loc == extensions || *(loc - 1) == ' ') && (*terminator == ' ' || *terminator == '\0')) {
                return 1;
            }
            extensions = terminator;
        }
    } else {
        unsigned int index;
        for (index = 0; index < num_exts_i; index++) {
            const char* e = exts_i[index];
            if (strcmp(e, ext) == 0) {
                return 1;
            }
        }
    }
    return 0;
}

static GLADapiproc glad_snap_rhi_gl_get_proc_from_userptr(void* userptr, const char* name) {
    return (GLAD_GNUC_EXTENSION(GLADapiproc(*)(const char* name)) userptr)(name);
}

static int glad_snap_rhi_gl_find_extensions_gles2(int version) {
    const char* exts = NULL;
    unsigned int num_exts_i = 0;
    char** exts_i = NULL;
    if (!glad_snap_rhi_gl_get_extensions(version, &exts, &num_exts_i, &exts_i))
        return 0;

    GLAD_SNAP_RHI_GL_APPLE_clip_distance =
        glad_snap_rhi_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_APPLE_clip_distance");
    GLAD_SNAP_RHI_GL_APPLE_framebuffer_multisample =
        glad_snap_rhi_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_APPLE_framebuffer_multisample");
    GLAD_SNAP_RHI_GL_APPLE_sync = glad_snap_rhi_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_APPLE_sync");
    GLAD_SNAP_RHI_GL_APPLE_texture_format_BGRA8888 =
        glad_snap_rhi_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_APPLE_texture_format_BGRA8888");
    GLAD_SNAP_RHI_GL_ARM_shader_framebuffer_fetch =
        glad_snap_rhi_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARM_shader_framebuffer_fetch");
    GLAD_SNAP_RHI_GL_ARM_shader_framebuffer_fetch_depth_stencil =
        glad_snap_rhi_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARM_shader_framebuffer_fetch_depth_stencil");
    GLAD_SNAP_RHI_GL_EXT_blend_minmax = glad_snap_rhi_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_blend_minmax");
    GLAD_SNAP_RHI_GL_EXT_clip_cull_distance =
        glad_snap_rhi_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_clip_cull_distance");
    GLAD_SNAP_RHI_GL_EXT_color_buffer_half_float =
        glad_snap_rhi_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_color_buffer_half_float");
    GLAD_SNAP_RHI_GL_EXT_debug_marker = glad_snap_rhi_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_debug_marker");
    GLAD_SNAP_RHI_GL_EXT_discard_framebuffer =
        glad_snap_rhi_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_discard_framebuffer");
    GLAD_SNAP_RHI_GL_EXT_disjoint_timer_query =
        glad_snap_rhi_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_disjoint_timer_query");
    GLAD_SNAP_RHI_GL_EXT_draw_instanced =
        glad_snap_rhi_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_draw_instanced");
    GLAD_SNAP_RHI_GL_EXT_multisampled_render_to_texture =
        glad_snap_rhi_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_multisampled_render_to_texture");
    GLAD_SNAP_RHI_GL_EXT_multisampled_render_to_texture2 =
        glad_snap_rhi_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_multisampled_render_to_texture2");
    GLAD_SNAP_RHI_GL_EXT_shader_framebuffer_fetch =
        glad_snap_rhi_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_shader_framebuffer_fetch");
    GLAD_SNAP_RHI_GL_EXT_shader_framebuffer_fetch_non_coherent =
        glad_snap_rhi_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_shader_framebuffer_fetch_non_coherent");
    GLAD_SNAP_RHI_GL_EXT_shader_texture_lod =
        glad_snap_rhi_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_shader_texture_lod");
    GLAD_SNAP_RHI_GL_EXT_texture_border_clamp =
        glad_snap_rhi_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_texture_border_clamp");
    GLAD_SNAP_RHI_GL_EXT_texture_filter_anisotropic =
        glad_snap_rhi_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_texture_filter_anisotropic");
    GLAD_SNAP_RHI_GL_EXT_texture_format_BGRA8888 =
        glad_snap_rhi_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_texture_format_BGRA8888");
    GLAD_SNAP_RHI_GL_EXT_texture_norm16 =
        glad_snap_rhi_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_texture_norm16");
    GLAD_SNAP_RHI_GL_EXT_texture_rg = glad_snap_rhi_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_texture_rg");
    GLAD_SNAP_RHI_GL_EXT_memory_object = glad_snap_rhi_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_memory_object");
    GLAD_SNAP_RHI_GL_EXT_memory_object_fd =
        glad_snap_rhi_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_memory_object_fd");
    GLAD_SNAP_RHI_GL_KHR_debug = glad_snap_rhi_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_KHR_debug");
    GLAD_SNAP_RHI_GL_NV_texture_border_clamp =
        glad_snap_rhi_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_NV_texture_border_clamp");
    GLAD_SNAP_RHI_GL_OES_EGL_image = glad_snap_rhi_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_OES_EGL_image");
    GLAD_SNAP_RHI_GL_OES_EGL_image_external =
        glad_snap_rhi_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_OES_EGL_image_external");
    GLAD_SNAP_RHI_GL_OES_depth24 = glad_snap_rhi_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_OES_depth24");
    GLAD_SNAP_RHI_GL_OES_depth32 = glad_snap_rhi_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_OES_depth32");
    GLAD_SNAP_RHI_GL_OES_depth_texture = glad_snap_rhi_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_OES_depth_texture");
    GLAD_SNAP_RHI_GL_OES_element_index_uint =
        glad_snap_rhi_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_OES_element_index_uint");
    GLAD_SNAP_RHI_GL_OES_fbo_render_mipmap =
        glad_snap_rhi_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_OES_fbo_render_mipmap");
    GLAD_SNAP_RHI_GL_OES_get_program_binary =
        glad_snap_rhi_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_OES_get_program_binary");
    GLAD_SNAP_RHI_GL_OES_mapbuffer = glad_snap_rhi_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_OES_mapbuffer");
    GLAD_SNAP_RHI_GL_OES_packed_depth_stencil =
        glad_snap_rhi_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_OES_packed_depth_stencil");
    GLAD_SNAP_RHI_GL_OES_rgb8_rgba8 = glad_snap_rhi_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_OES_rgb8_rgba8");
    GLAD_SNAP_RHI_GL_OES_standard_derivatives =
        glad_snap_rhi_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_OES_standard_derivatives");
    GLAD_SNAP_RHI_GL_OES_texture_3D = glad_snap_rhi_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_OES_texture_3D");
    GLAD_SNAP_RHI_GL_OES_texture_border_clamp =
        glad_snap_rhi_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_OES_texture_border_clamp");
    GLAD_SNAP_RHI_GL_OES_texture_float = glad_snap_rhi_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_OES_texture_float");
    GLAD_SNAP_RHI_GL_OES_texture_float_linear =
        glad_snap_rhi_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_OES_texture_float_linear");
    GLAD_SNAP_RHI_GL_OES_texture_half_float =
        glad_snap_rhi_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_OES_texture_half_float");
    GLAD_SNAP_RHI_GL_OES_texture_half_float_linear =
        glad_snap_rhi_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_OES_texture_half_float_linear");
    GLAD_SNAP_RHI_GL_OES_texture_npot = glad_snap_rhi_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_OES_texture_npot");
    GLAD_SNAP_RHI_GL_OVR_multiview = glad_snap_rhi_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_OVR_multiview");
    GLAD_SNAP_RHI_GL_OVR_multiview2 = glad_snap_rhi_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_OVR_multiview2");
    GLAD_SNAP_RHI_GL_OVR_multiview_multisampled_render_to_texture =
        glad_snap_rhi_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_OVR_multiview_multisampled_render_to_texture");

    glad_snap_rhi_gl_free_extensions(exts_i, num_exts_i);

    return 1;
}

static int glad_snap_rhi_gl_find_core_gles2(void) {
    int i, major, minor;
    const char* version;
    const char* prefixes[] = {"OpenGL ES-CM ", "OpenGL ES-CL ", "OpenGL ES ", NULL};
    version = (const char*)glGetString(GL_VERSION);
    if (!version)
        return 0;
    for (i = 0; prefixes[i]; i++) {
        const size_t length = strlen(prefixes[i]);
        if (strncmp(version, prefixes[i], length) == 0) {
            version += length;
            break;
        }
    }

    GLAD_IMPL_UTIL_SSCANF(version, "%d.%d", &major, &minor);

    GLAD_SNAP_RHI_GL_ES_VERSION_2_0 = (major == 2 && minor >= 0) || major > 2;
    GLAD_SNAP_RHI_GL_ES_VERSION_3_0 = (major == 3 && minor >= 0) || major > 3;
    GLAD_SNAP_RHI_GL_ES_VERSION_3_1 = (major == 3 && minor >= 1) || major > 3;
    GLAD_SNAP_RHI_GL_ES_VERSION_3_2 = (major == 3 && minor >= 2) || major > 3;

    return GLAD_MAKE_VERSION(major, minor);
}

int gladLoadGLES2UserPtr(GLADuserptrloadfunc load, void* userptr) {
    int version;

    //    glad_snap_rhi_glGetString = (PFNGLGETSTRINGPROC) load(userptr, "glGetString");
    //    if(glad_snap_rhi_glGetString == NULL) return 0;
    if (glGetString(GL_VERSION) == NULL)
        return 0;
    version = glad_snap_rhi_gl_find_core_gles2();

    //    glad_snap_rhi_gl_load_GL_ES_VERSION_2_0(load, userptr);
    glad_snap_rhi_gl_load_GL_ES_VERSION_3_0(load, userptr);
    glad_snap_rhi_gl_load_GL_ES_VERSION_3_1(load, userptr);
    glad_snap_rhi_gl_load_GL_ES_VERSION_3_2(load, userptr);

    if (!glad_snap_rhi_gl_find_extensions_gles2(version))
        return 0;
    glad_snap_rhi_gl_load_GL_APPLE_framebuffer_multisample(load, userptr);
    glad_snap_rhi_gl_load_GL_APPLE_sync(load, userptr);
    glad_snap_rhi_gl_load_GL_EXT_debug_marker(load, userptr);
    glad_snap_rhi_gl_load_GL_EXT_discard_framebuffer(load, userptr);
    glad_snap_rhi_gl_load_GL_EXT_disjoint_timer_query(load, userptr);
    glad_snap_rhi_gl_load_GL_EXT_draw_instanced(load, userptr);
    glad_snap_rhi_gl_load_GL_EXT_multisampled_render_to_texture(load, userptr);
    glad_snap_rhi_gl_load_GL_EXT_shader_framebuffer_fetch_non_coherent(load, userptr);
    glad_snap_rhi_gl_load_GL_EXT_texture_border_clamp(load, userptr);
    glad_snap_rhi_gl_load_GL_EXT_memory_object(load, userptr);
    glad_snap_rhi_gl_load_GL_EXT_memory_object_fd(load, userptr);
    glad_snap_rhi_gl_load_GL_KHR_debug(load, userptr);
    glad_snap_rhi_gl_load_GL_OES_EGL_image(load, userptr);
    glad_snap_rhi_gl_load_GL_OES_get_program_binary(load, userptr);
    glad_snap_rhi_gl_load_GL_OES_mapbuffer(load, userptr);
    glad_snap_rhi_gl_load_GL_OES_texture_3D(load, userptr);
    glad_snap_rhi_gl_load_GL_OES_texture_border_clamp(load, userptr);
    glad_snap_rhi_gl_load_GL_OVR_multiview(load, userptr);
    glad_snap_rhi_gl_load_GL_OVR_multiview_multisampled_render_to_texture(load, userptr);

    return version;
}

int gladLoadGLES2Func(GLADloadfunc load) {
    return gladLoadGLES2UserPtr(glad_snap_rhi_gl_get_proc_from_userptr, GLAD_GNUC_EXTENSION(void*) load);
}

int gladLoadGLES(GLADloadfunc load) {
    //    int result = gladLoadEGL(display);
    //    assert(result > 0);
    return gladLoadGLES2Func(load);
}

int gladLoadGLESSafe(GLADloadfunc load) {
    static std::once_flag of;
    static int retValue = 0;
    std::call_once(of, [&] { retValue = gladLoadGLES(load); });
    return retValue;
}

#endif // GLAD_PLATFORM_ANDROID || GLAD_PLATFORM_EMSCRIPTEN || SNAP_GLAD_LINUX_BASED()
