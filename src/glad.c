#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glad/glad.h>

#ifndef GLAD_IMPL_UTIL_C_
#define GLAD_IMPL_UTIL_C_
#ifdef _MSC_VER
#define GLAD_IMPL_inline __inline
#else
#define GLAD_IMPL_inline inline
#endif
#endif /* GLAD_IMPL_UTIL_C_ */

#ifdef __cplusplus
extern "C" {
#endif

int GLAD_GL_VERSION_1_0 = 0;
int GLAD_GL_VERSION_1_1 = 0;
int GLAD_GL_VERSION_1_2 = 0;
int GLAD_GL_VERSION_1_3 = 0;
int GLAD_GL_VERSION_1_4 = 0;
int GLAD_GL_VERSION_1_5 = 0;
int GLAD_GL_VERSION_2_0 = 0;
int GLAD_GL_VERSION_2_1 = 0;
int GLAD_GL_VERSION_3_0 = 0;
int GLAD_GL_VERSION_3_1 = 0;
int GLAD_GL_VERSION_3_2 = 0;
int GLAD_GL_VERSION_3_3 = 0;

PFNGLACTIVETEXTUREPROC glad_glActiveTexture = NULL;
PFNGLATTACHSHADERPROC glad_glAttachShader = NULL;
PFNGLBEGINQUERYPROC glad_glBeginQuery = NULL;
PFNGLBEGINTRANSFORMFEEDBACKPROC glad_glBeginTransformFeedback = NULL;
PFNGLBINDATTRIBLOCATIONPROC glad_glBindAttribLocation = NULL;
PFNGLBINDBUFFERPROC glad_glBindBuffer = NULL;
PFNGLBINDBUFFERBASEPROC glad_glBindBufferBase = NULL;
PFNGLBINDBUFFERRANGEPROC glad_glBindBufferRange = NULL;
PFNGLBINDFRAGDATALOCATIONPROC glad_glBindFragDataLocation = NULL;
PFNGLBINDFRAGDATALOCATIONINDEXEDPROC glad_glBindFragDataLocationIndexed = NULL;
PFNGLINDFRAMEBUFFERPROC glad_glBindFramebuffer = NULL;
PFNGLBINDRENDERBUFFERPROC glad_glBindRenderbuffer = NULL;
PFNGLBINDSAMPLERPROC glad_glBindSampler = NULL;
PFNGLBINDTEXTUREPROC glad_glBindTexture = NULL;
PFNGLBINDVERTEXARRAYPROC glad_glBindVertexArray = NULL;
PFNGLBLENDCOLORPROC glad_glBlendColor = NULL;
PFNGLBLENDEQUATIONPROC glad_glBlendEquation = NULL;
PFNGLBLENDEQUATIONSEPARATEPROC glad_glBlendEquationSeparate = NULL;
PFNGLBLENDFUNCPROC glad_glBlendFunc = NULL;
PFNGLBLENDFUNCSEPARATEPROC glad_glBlendFuncSeparate = NULL;
PFNGLBLITFRAMEBUFFERPROC glad_glBlitFramebuffer = NULL;
PFNGLBUFFERDATAPROC glad_glBufferData = NULL;
PFNGLBUFFERSUBDATAPROC glad_glBufferSubData = NULL;
PFNGLCHECKFRAMEBUFFERSTATUSPROC glad_glCheckFramebufferStatus = NULL;
PFNGLCLAMPCOLORPROC glad_glClampColor = NULL;
PFNGLCLEARPROC glad_glClear = NULL;
PFNGLCLEARBUFFERFIPROC glad_glClearBufferfi = NULL;
PFNGLCLEARBUFFERFVPROC glad_glClearBufferfv = NULL;
PFNGLCLEARBUFFERIVPROC glad_glClearBufferiv = NULL;
PFNGLCLEARBUFFERUIVPROC glad_glClearBufferuiv = NULL;
PFNGLCLEARCOLORPROC glad_glClearColor = NULL;
PFNGLCLEARDEPTHPROC glad_glClearDepth = NULL;
PFNGLCLEARSTENCILPROC glad_glClearStencil = NULL;
PFNGLCLIENTWAITSYNCPROC glad_glClientWaitSync = NULL;
PFNGLCOLORMASKPROC glad_glColorMask = NULL;
PFNGLCOLORMASKSEPARATEPROC glad_glColorMaskSeparate = NULL;
PFNGLCOMPILESHADERPROC glad_glCompileShader = NULL;
PFNGLCOMPRESSEDTEXIMAGE1DPROC glad_glCompressedTexImage1D = NULL;
PFNGLCOMPRESSEDTEXIMAGE2DPROC glad_glCompressedTexImage2D = NULL;
PFNGLCOMPRESSEDTEXIMAGE3DPROC glad_glCompressedTexImage3D = NULL;
PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC glad_glCompressedTexSubImage1D = NULL;
PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC glad_glCompressedTexSubImage2D = NULL;
PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC glad_glCompressedTexSubImage3D = NULL;
PFNGLCOPYBUFFERSUBDATAPROC glad_glCopyBufferSubData = NULL;
PFNGLCOPYTEXIMAGE1DPROC glad_glCopyTexImage1D = NULL;
PFNGLCOPYTEXIMAGE2DPROC glad_glCopyTexImage2D = NULL;
PFNGLCOPYTEXSUBIMAGE1DPROC glad_glCopyTexSubImage1D = NULL;
PFNGLCOPYTEXSUBIMAGE2DPROC glad_glCopyTexSubImage2D = NULL;
PFNGLCOPYTEXSUBIMAGE3DPROC glad_glCopyTexSubImage3D = NULL;
PFNGLCREATEPROGRAMPROC glad_glCreateProgram = NULL;
PFNGLCREATESHADERPROC glad_glCreateShader = NULL;
PFNGLCULLFACEPROC glad_glCullFace = NULL;
PFNGLDELETEBUFFERSPROC glad_glDeleteBuffers = NULL;
PFNGLDELETEFRAMEBUFFERSPROC glad_glDeleteFramebuffers = NULL;
PFNGLDELETEPROGRAMPROC glad_glDeleteProgram = NULL;
PFNGLDELETEQUERIESPROC glad_glDeleteQueries = NULL;
PFNGLDELETERENDERBUFFERSPROC glad_glDeleteRenderbuffers = NULL;
PFNGLDELETESAMPLERSPROC glad_glDeleteSamplers = NULL;
PFNGLDELETESHADERPROC glad_glDeleteShader = NULL;
PFNGLDELETESYNCPROC glad_glDeleteSync = NULL;
PFNGLDELETETEXTURESPROC glad_glDeleteTextures = NULL;
PFNGLDELETEVERTEXARRAYSPROC glad_glDeleteVertexArrays = NULL;
PFNGLDEPTHFUNCPROC glad_glDepthFunc = NULL;
PFNGLDEPTHMASKPROC glad_glDepthMask = NULL;
PFNGLDEPTHRANGEPROC glad_glDepthRange = NULL;
PFNGLDETACHSHADERPROC glad_glAttachShader = NULL;
PFNGLDISABLEPROC glad_glDisable = NULL;
PFNGLDISABLEVERTEXATTRIBARRAYPROC glad_glDisableVertexAttribArray = NULL;
PFNGLDISABLEIPROC glad_glDisablei = NULL;
PFNGLDRAWARRAYSPROC glad_glDrawArrays = NULL;
PFNGLDRAWARRAYSINDIRECTPROC glad_glDrawArraysIndirect = NULL;
PFNGLDRAWARRAYSINSTANCEDPROC glad_glDrawArraysInstanced = NULL;
PFNGLDRAWBUFFERPROC glad_glDrawBuffer = NULL;
PFNGLDRAWBUFFERSPROC glad_glDrawBuffers = NULL;
PFNGLDRAWELEMENTSPROC glad_glDrawElements = NULL;
PFNGLDRAWELEMENTSBASEVERTEXPROC glad_glDrawElementsBaseVertex = NULL;
PFNGLDRAWELEMENTSINDIRECTPROC glad_glDrawElementsIndirect = NULL;
PFNGLDRAWELEMENTSINSTANCEDPROC glad_glDrawElementsInstanced = NULL;
PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXPROC glad_glDrawElementsInstancedBaseVertex = NULL;
PFNGLDRAWRANGEELEMENTSPROC glad_glDrawRangeElements = NULL;
PFNGLDRAWRANGEELEMENTSBASEVERTEXPROC glad_glDrawRangeElementsBaseVertex = NULL;
PFNGLENABLEPROC glad_glEnable = NULL;
PFNGLENABLEVERTEXATTRIBARRAYPROC glad_glEnableVertexAttribArray = NULL;
PFNGLENABLEIPROC glad_glEnablei = NULL;
PFNGLENDQUERYPROC glad_glEndQuery = NULL;
PFNGLENDTRANSFORMFEEDBACKPROC glad_glEndTransformFeedback = NULL;
PFNGLFENCESYNCPROC glad_glFenceSync = NULL;
PFNGLFINISHPROC glad_glFinish = NULL;
PFNGLFLUSHPROC glad_glFlush = NULL;
PFNGLFLUSHMAPPEDBUFFERRANGEPROC glad_glFlushMappedBufferRange = NULL;
PFNGLFRAMEBUFFERRENDERBUFFERPROC glad_glFramebufferRenderbuffer = NULL;
PFNGLFRAMEBUFFERTEXTUREPROC glad_glFramebufferTexture = NULL;
PFNGLFRAMEBUFFERTEXTURE1DPROC glad_glFramebufferTexture1D = NULL;
PFNGLFRAMEBUFFERTEXTURE2DPROC glad_glFramebufferTexture2D = NULL;
PFNGLFRAMEBUFFERTEXTURE3DPROC glad_glFramebufferTexture3D = NULL;
PFNGLFRAMEBUFFERTEXTURELAYERPROC glad_glFramebufferTextureLayer = NULL;
PFNGLFRONTFACEPROC glad_glFrontFace = NULL;
PFNGLGENBUFFERSPROC glad_glGenBuffers = NULL;
PFNGLGENERATEMIPMAPPROC glad_glGenerateMipmap = NULL;
PFNGLGENFRAMEBUFFERSPROC glad_glGenFramebuffers = NULL;
PFNGLGENQUERIESPROC glad_glGenQueries = NULL;
PFNGLGENRENDERBUFFERSPROC glad_glGenRenderbuffers = NULL;
PFNGLGENSAMPLERSPROC glad_glGenSamplers = NULL;
PFNGLGENTEXTURESPROC glad_glGenTextures = NULL;
PFNGLGENVERTEXARRAYSPROC glad_glGenVertexArrays = NULL;
PFNGLGETACTIVEATTRIBPROC glad_glGetActiveAttrib = NULL;
PFNGLGETACTIVESUBROUTINENAMEPROC glad_glGetActiveSubroutineName = NULL;
PFNGLGETACTIVESUBROUTINEUNIFORMNAMEPROC glad_glGetActiveSubroutineUniformName = NULL;
PFNGLGETACTIVEUNIFORMPROC glad_glGetActiveUniform = NULL;
PFNGLGETACTIVEUNIFORMBLOCKNAMEPROC glad_glGetActiveUniformBlockName = NULL;
PFNGLGETACTIVEUNIFORMBLOCKIVPROC glad_glGetActiveUniformBlockiv = NULL;
PFNGLGETACTIVEUNIFORMSIVPROC glad_glGetActiveUniformsiv = NULL;
PFNGLGETATTACHEDSHADERSPROC glad_glGetAttachedShaders = NULL;
PFNGLGETATTRIBLOCATIONPROC glad_glGetAttribLocation = NULL;
PFNGLGETBOOLEANI_VPROC glad_glGetBooleani_v = NULL;
PFNGLGETBOOLEANVPROC glad_glGetBooleanv = NULL;
PFNGLGETBUFFERPARAMETERI64VPROC glad_glGetBufferParameteri64v = NULL;
PFNGLGETBUFFERPARAMETERIVPROC glad_glGetBufferParameteriv = NULL;
PFNGLGETBUFFERPOINTERVPROC glad_glGetBufferPointerv = NULL;
PFNGLGETBUFFERSUBDATAPROC glad_glGetBufferSubData = NULL;
PFNGLGETCOMPRESSEDTEXIMAGEPROC glad_glGetCompressedTexImage = NULL;
PFNGLGETDOUBLEVPROC glad_glGetDoublev = NULL;
PFNGLGETERRORPROC glad_glGetError = NULL;
PFNGLGETFLOATVPROC glad_glGetFloatv = NULL;
PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC glad_glGetFramebufferAttachmentParameteriv = NULL;
PFNGLGETINTEGER64I_VPROC glad_glGetInteger64i_v = NULL;
PFNGLGETINTEGER64VPROC glad_glGetInteger64v = NULL;
PFNGLGETINTEGERI_VPROC glad_glGetIntegeri_v = NULL;
PFNGLGETINTEGERVPROC glad_glGetIntegerv = NULL;
PFNGLGETMULTISAMPLEFVPROC glad_glGetMultisamplefv = NULL;
PFNGLGETPROGRAMBINARYPROC glad_glGetProgramBinary = NULL;
PFNGLGETPROGRAMINFOLOGPROC glad_glGetProgramInfoLog = NULL;
PFNGLGETPROGRAMSTAGEIVPROC glad_glGetProgramStageiv = NULL;
PFNGLGETPROGRAMIVPROC glad_glGetProgramiv = NULL;
PFNGLGETQUERYOBJECTI64VPROC glad_glGetQueryObjecti64v = NULL;
PFNGLGETQUERYOBJECTUI64VPROC glad_glGetQueryObjectui64v = NULL;
PFNGLGETQUERYOBJECTIVPROC glad_glGetQueryObjectiv = NULL;
PFNGLGETQUERYOBJECTUIVPROC glad_glGetQueryObjectuiv = NULL;
PFNGLGETQUERYIVPROC glad_glGetQueryiv = NULL;
PFNGLGETRENDERBUFFERPARAMETERIVPROC glad_glGetRenderbufferParameteriv = NULL;
PFNGLGETSAMPLERPARAMETERFVPROC glad_glGetSamplerParameterfv = NULL;
PFNGLGETSAMPLERPARAMETERIVPROC glad_glGetSamplerParameteriv = NULL;
PFNGLGETSHADERINFOLOGPROC glad_glGetShaderInfoLog = NULL;
PFNGLGETSHADERSOURCEPROC glad_glGetShaderSource = NULL;
PFNGLGETSHADERIVPROC glad_glGetShaderiv = NULL;
PFNGLGETSTRINGPROC glad_glGetString = NULL;
PFNGLGETSTRINGIPROC glad_glGetStringi = NULL;
PFNGLGETSUBROUTINEINDEXPROC glad_glGetSubroutineIndex = NULL;
PFNGLGETSUBROUTINEUNIFORMLOCATIONPROC glad_glGetSubroutineUniformLocation = NULL;
PFNGLGETSYNCIVPROC glad_glGetSynciv = NULL;
PFNGLGETTEXIMAGEPROC glad_glGetTexImage = NULL;
PFNGLGETTEXLEVELPARAMETERFVPROC glad_glGetTexLevelParameterfv = NULL;
PFNGLGETTEXLEVELPARAMETERIVPROC glad_glGetTexLevelParameteriv = NULL;
PFNGLGETTEXPARAMETERIIVPROC glad_glGetTexParameterIiv = NULL;
PFNGLGETTEXPARAMETERIUIVPROC glad_glGetTexParameterIuiv = NULL;
PFNGLGETTEXPARAMETERFVPROC glad_glGetTexParameterfv = NULL;
PFNGLGETTEXPARAMETERIVPROC glad_glGetTexParameteriv = NULL;
PFNGLGETTRANSFORMFEEDBACKVARYINGPROC glad_glGetTransformFeedbackVarying = NULL;
PFNGLGETUNIFORMBLOCKINDEXPROC glad_glGetUniformBlockIndex = NULL;
PFNGLGETUNIFORMLOCATIONPROC glad_glGetUniformLocation = NULL;
PFNGLGETUNIFORMSUBROUTINEUIVPROC glad_glGetUniformSubroutineuiv = NULL;
PFNGLGETUNIFORMDVPROC glad_glGetUniformdv = NULL;
PFNGLGETUNIFORMFVPROC glad_glGetUniformfv = NULL;
PFNGLGETUNIFORMFVPROC glad_glGetUniformfv = NULL;
PFNGLGETUNIFORMIVPROC glad_glGetUniformiv = NULL;
PFNGLGETUNIFORMUIVPROC glad_glGetUniformuiv = NULL;
PFNGLGETVERTEXATTRIBIIVPROC glad_glGetVertexAttribIiv = NULL;
PFNGLGETVERTEXATTRIBIUIVPROC glad_glGetVertexAttribIuiv = NULL;
PFNGLGETVERTEXATTRIBLDVPROC glad_glGetVertexAttribLdv = NULL;
PFNGLGETVERTEXATTRIBDVPROC glad_glGetVertexAttribdv = NULL;
PFNGLGETVERTEXATTRIBFVPROC glad_glGetVertexAttribfv = NULL;
PFNGLGETVERTEXATTRIBIT_POINTERVPROC glad_glGetVertexAttribPointerv = NULL;
PFNGLGETVERTEXATTRIBIVPROC glad_glGetVertexAttribiv = NULL;
PFNGLHINTPROC glad_glHint = NULL;
PFNGLISBUFFERPROC glad_glIsBuffer = NULL;
PFNGLISENABLEDPROC glad_glIsEnabled = NULL;
PFNGLISENABLEDIPROC glad_glIsEnabledi = NULL;
PFNGLISFRAMEBUFFERPROC glad_glIsFramebuffer = NULL;
PFNGLISPROGRAMPROC glad_glIsProgram = NULL;
PFNGLISPROGRAMPIPELINEPROC glad_glIsProgramPipeline = NULL;
PFNGLISQUERYPROC glad_glIsQuery = NULL;
PFNGLISRENDERBUFFERPROC glad_glIsRenderbuffer = NULL;
PFNGLISSAMPLERPROC glad_glIsSampler = NULL;
PFNGLISSHADERPROC glad_glIsShader = NULL;
PFNGLISTEXTUREPROC glad_glIsTexture = NULL;
PFNGLISTRANSFORMFEEDBACKPROC glad_glIsTransformFeedback = NULL;
PFNGLISVERTEXARRAYPROC glad_glIsVertexArray = NULL;
PFNGLLINEWIDTHPROC glad_glLineWidth = NULL;
PFNGLLINKPROGRAMPROC glad_glLinkProgram = NULL;
PFNGLLOGICOPPROC glad_glLogicOp = NULL;
PFNGLMAPBUFFERPROC glad_glMapBuffer = NULL;
PFNGLMAPBUFFERRANGEPROC glad_glMapBufferRange = NULL;
PFNGLMINSAMPLESHADINGPROC glad_glMinSampleShading = NULL;
PFNGLMULTIDRAWARRAYSPROC glad_glMultiDrawArrays = NULL;
PFNGLMULTIDRAWELEMENTSPROC glad_glMultiDrawElements = NULL;
PFNGLMULTIDRAWELEMENTSBASEVERTEXPROC glad_glMultiDrawElementsBaseVertex = NULL;
PFNGLPATCHPARAMETERFPROC glad_glPatchParameterfv = NULL;
PFNGLPATCHPARAMETERIPROC glad_glPatchParameteri = NULL;
PFNGLPAUSETRANSFORMFEEDBACKPROC glad_glPauseTransformFeedback = NULL;
PFNGLPIXELSTOREFPROC glad_glPixelStoref = NULL;
PFNGLPIXELSTOREIPROC glad_glPixelStorei = NULL;
PFNGLPOINTPARAMETERFPROC glad_glPointParameterf = NULL;
PFNGLPOINTPARAMETERFVPROC glad_glPointParameterfv = NULL;
PFNGLPOINTPARAMETERIPROC glad_glPointParameteri = NULL;
PFNGLPOINTPARAMETERIVPROC glad_glPointParameteriv = NULL;
PFNGLPOINTSIZEPROC glad_glPointSize = NULL;
PFNGLPOLYGONMODEPROC glad_glPolygonMode = NULL;
PFNGLPOLYGONOFFSETPROC glad_glPolygonOffset = NULL;
PFNGLPRIMITIVERESTARTINDEXPROC glad_glPrimitiveRestartIndex = NULL;
PFNGLPROGRAMBINARYPROC glad_glProgramBinary = NULL;
PFNGLPROGRAMPARAMETERIPROC glad_glProgramParameteri = NULL;
PFNGLPROGRAMUNIFORM1DPROC glad_glProgramUniform1d = NULL;
PFNGLPROGRAMUNIFORM1DVPROC glad_glProgramUniform1dv = NULL;
PFNGLPROGRAMUNIFORM1FPROC glad_glProgramUniform1f = NULL;
PFNGLPROGRAMUNIFORM1FVPROC glad_glProgramUniform1fv = NULL;
PFNGLPROGRAMUNIFORM1IPROC glad_glProgramUniform1i = NULL;
PFNGLPROGRAMUNIFORM1IVPROC glad_glProgramUniform1iv = NULL;
PFNGLPROGRAMUNIFORM1UIPROC glad_glProgramUniform1ui = NULL;
PFNGLPROGRAMUNIFORM1UIVPROC glad_glProgramUniform1uiv = NULL;
PFNGLPROGRAMUNIFORM2DPROC glad_glProgramUniform2d = NULL;
PFNGLPROGRAMUNIFORM2DVPROC glad_glProgramUniform2dv = NULL;
PFNGLPROGRAMUNIFORM2FPROC glad_glProgramUniform2f = NULL;
PFNGLPROGRAMUNIFORM2FVPROC glad_glProgramUniform2fv = NULL;
PFNGLPROGRAMUNIFORM2IPROC glad_glProgramUniform2i = NULL;
PFNGLPROGRAMUNIFORM2IVPROC glad_glProgramUniform2iv = NULL;
PFNGLPROGRAMUNIFORM2UIPROC glad_glProgramUniform2ui = NULL;
PFNGLPROGRAMUNIFORM2UIVPROC glad_glProgramUniform2uiv = NULL;
PFNGLPROGRAMUNIFORM3DPROC glad_glProgramUniform3d = NULL;
PFNGLPROGRAMUNIFORM3DVPROC glad_glProgramUniform3dv = NULL;
PFNGLPROGRAMUNIFORM3FPROC glad_glProgramUniform3f = NULL;
PFNGLPROGRAMUNIFORM3FVPROC glad_glProgramUniform3fv = NULL;
PFNGLPROGRAMUNIFORM3IPROC glad_glProgramUniform3i = NULL;
PFNGLPROGRAMUNIFORM3IVPROC glad_glProgramUniform3iv = NULL;
PFNGLPROGRAMUNIFORM3UIPROC glad_glProgramUniform3ui = NULL;
PFNGLPROGRAMUNIFORM3UIVPROC glad_glProgramUniform3uiv = NULL;
PFNGLPROGRAMUNIFORM4DPROC glad_glProgramUniform4d = NULL;
PFNGLPROGRAMUNIFORM4DVPROC glad_glProgramUniform4dv = NULL;
PFNGLPROGRAMUNIFORM4FPROC glad_glProgramUniform4f = NULL;
PFNGLPROGRAMUNIFORM4FVPROC glad_glProgramUniform4fv = NULL;
PFNGLPROGRAMUNIFORM4IPROC glad_glProgramUniform4i = NULL;
PFNGLPROGRAMUNIFORM4IVPROC glad_glProgramUniform4iv = NULL;
PFNGLPROGRAMUNIFORM4UIPROC glad_glProgramUniform4ui = NULL;
PFNGLPROGRAMUNIFORM4UIVPROC glad_glProgramUniform4uiv = NULL;
PFNGLPROGRAMUNIFORMMATRIX2DVPROC glad_glProgramUniformMatrix2dv = NULL;
PFNGLPROGRAMUNIFORMMATRIX2FVPROC glad_glProgramUniformMatrix2fv = NULL;
PFNGLPROGRAMUNIFORMMATRIX2X3DVPROC glad_glProgramUniformMatrix2x3dv = NULL;
PFNGLPROGRAMUNIFORMMATRIX2X3FVPROC glad_glProgramUniformMatrix2x3fv = NULL;
PFNGLPROGRAMUNIFORMMATRIX2X4DVPROC glad_glProgramUniformMatrix2x4dv = NULL;
PFNGLPROGRAMUNIFORMMATRIX2X4FVPROC glad_glProgramUniformMatrix2x4fv = NULL;
PFNGLPROGRAMUNIFORMMATRIX3DVPROC glad_glProgramUniformMatrix3dv = NULL;
PFNGLPROGRAMUNIFORMMATRIX3FVPROC glad_glProgramUniformMatrix3fv = NULL;
PFNGLPROGRAMUNIFORMMATRIX3X2DVPROC glad_glProgramUniformMatrix3x2dv = NULL;
PFNGLPROGRAMUNIFORMMATRIX3X2FVPROC glad_glProgramUniformMatrix3x2fv = NULL;
PFNGLPROGRAMUNIFORMMATRIX3X4DVPROC glad_glProgramUniformMatrix3x4dv = NULL;
PFNGLPROGRAMUNIFORMMATRIX3X4FVPROC glad_glProgramUniformMatrix3x4fv = NULL;
PFNGLPROGRAMUNIFORMMATRIX4DVPROC glad_glProgramUniformMatrix4dv = NULL;
PFNGLPROGRAMUNIFORMMATRIX4FVPROC glad_glProgramUniformMatrix4fv = NULL;
PFNGLPROGRAMUNIFORMMATRIX4X2DVPROC glad_glProgramUniformMatrix4x2dv = NULL;
PFNGLPROGRAMUNIFORMMATRIX4X2FVPROC glad_glProgramUniformMatrix4x2fv = NULL;
PFNGLPROGRAMUNIFORMMATRIX4X3DVPROC glad_glProgramUniformMatrix4x3dv = NULL;
PFNGLPROGRAMUNIFORMMATRIX4X3FVPROC glad_glProgramUniformMatrix4x3fv = NULL;
PFNGLPROVOKINGVERTEXPROC glad_glProvokingVertex = NULL;
PFNGLQUERYCOUNTERPROC glad_glQueryCounter = NULL;
PFNGLREADBUFFERPROC glad_glReadBuffer = NULL;
PFNGLREADPIXELSPROC glad_glReadPixels = NULL;
PFNGLRELEASESHADERCOMPILERPROC glad_glReleaseShaderCompiler = NULL;
PFNGLRENDERBUFFERSTORAGEPROC glad_glRenderbufferStorage = NULL;
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC glad_glRenderbufferStorageMultisample = NULL;
PFNGLRESUMETRANSFORMFEEDBACKPROC glad_glResumeTransformFeedback = NULL;
PFNGLSAMPLECOVERAGEPROC glad_glSampleCoverage = NULL;
PFNGLSAMPLEMASKIPROC glad_glSampleMaski = NULL;
PFNGLSAMPLEPARAMETERFPROC glad_glSamplerParameterf = NULL;
PFNGLSAMPLEPARAMETERFVPROC glad_glSamplerParameterfv = NULL;
PFNGLSAMPLEPARAMETERIPROC glad_glSamplerParameteri = NULL;
PFNGLSAMPLEPARAMETERIVPROC glad_glSamplerParameteriv = NULL;
PFNGLSCISSORPROC glad_glScissor = NULL;
PFNGLSCISSORINDEXEDPROC glad_glScissorIndexed = NULL;
PFNGLSCISSORINDEXEDVPROC glad_glScissorIndexedv = NULL;
PFNGLSHADERBINARYPROC glad_glShaderBinary = NULL;
PFNGLSHADERSOURCEPROC glad_glShaderSource = NULL;
PFNGLSTENCILFUNCPROC glad_glStencilFunc = NULL;
PFNGLSTENCILFUNCSEPARATEPROC glad_glStencilFuncSeparate = NULL;
PFNGLSTENCILMASKPROC glad_glStencilMask = NULL;
PFNGLSTENCILMASKSEPARATEPROC glad_glStencilMaskSeparate = NULL;
PFNGLSTENCILOPPROC glad_glStencilOp = NULL;
PFNGLSTENCILOPSEPARATEPROC glad_glStencilOpSeparate = NULL;
PFNGLTEXBUFFERPROC glad_glTexBuffer = NULL;
PFNGLTEXIMAGE1DPROC glad_glTexImage1D = NULL;
PFNGLTEXIMAGE2DPROC glad_glTexImage2D = NULL;
PFNGLTEXIMAGE2DMULTISAMPLEPROC glad_glTexImage2DMultisample = NULL;
PFNGLTEXIMAGE3DPROC glad_glTexImage3D = NULL;
PFNGLTEXIMAGE3DMULTISAMPLEPROC glad_glTexImage3DMultisample = NULL;
PFNGLTEXPARAMETERFPROC glad_glTexParameterf = NULL;
PFNGLTEXPARAMETERFVPROC glad_glTexParameterfv = NULL;
PFNGLTEXPARAMETERIPROC glad_glTexParameteri = NULL;
PFNGLTEXPARAMETERIVPROC glad_glTexParameteriv = NULL;
PFNGLTEXSUBIMAGE1DPROC glad_glTexSubImage1D = NULL;
PFNGLTEXSUBIMAGE2DPROC glad_glTexSubImage2D = NULL;
PFNGLTEXSUBIMAGE3DPROC glad_glTexSubImage3D = NULL;
PFNGLTRANSFORMFEEDBACKVARYINGSPROC glad_glTransformFeedbackVaryings = NULL;
PFNGLUNIFORM1DPROC glad_glUniform1d = NULL;
PFNGLUNIFORM1DVPROC glad_glUniform1dv = NULL;
PFNGLUNIFORM1FPROC glad_glUniform1f = NULL;
PFNGLUNIFORM1FVPROC glad_glUniform1fv = NULL;
PFNGLUNIFORM1IPROC glad_glUniform1i = NULL;
PFNGLUNIFORM1IVPROC glad_glUniform1iv = NULL;
PFNGLUNIFORM1UIPROC glad_glUniform1ui = NULL;
PFNGLUNIFORM1UIVPROC glad_glUniform1uiv = NULL;
PFNGLUNIFORM2DPROC glad_glUniform2d = NULL;
PFNGLUNIFORM2DVPROC glad_glUniform2dv = NULL;
PFNGLUNIFORM2FPROC glad_glUniform2f = NULL;
PFNGLUNIFORM2FVPROC glad_glUniform2fv = NULL;
PFNGLUNIFORM2IPROC glad_glUniform2i = NULL;
PFNGLUNIFORM2IVPROC glad_glUniform2iv = NULL;
PFNGLUNIFORM2UIPROC glad_glUniform2ui = NULL;
PFNGLUNIFORM2UIVPROC glad_glUniform2uiv = NULL;
PFNGLUNIFORM3DPROC glad_glUniform3d = NULL;
PFNGLUNIFORM3DVPROC glad_glUniform3dv = NULL;
PFNGLUNIFORM3FPROC glad_glUniform3f = NULL;
PFNGLUNIFORM3FVPROC glad_glUniform3fv = NULL;
PFNGLUNIFORM3IPROC glad_glUniform3i = NULL;
PFNGLUNIFORM3IVPROC glad_glUniform3iv = NULL;
PFNGLUNIFORM3UIPROC glad_glUniform3ui = NULL;
PFNGLUNIFORM3UIVPROC glad_glUniform3uiv = NULL;
PFNGLUNIFORM4DPROC glad_glUniform4d = NULL;
PFNGLUNIFORM4DVPROC glad_glUniform4dv = NULL;
PFNGLUNIFORM4FPROC glad_glUniform4f = NULL;
PFNGLUNIFORM4FVPROC glad_glUniform4fv = NULL;
PFNGLUNIFORM4IPROC glad_glUniform4i = NULL;
PFNGLUNIFORM4IVPROC glad_glUniform4iv = NULL;
PFNGLUNIFORM4UIPROC glad_glUniform4ui = NULL;
PFNGLUNIFORM4UIVPROC glad_glUniform4uiv = NULL;
PFNGLUNIFORMBLOCKBINDINGPROC glad_glUniformBlockBinding = NULL;
PFNGLUNIFORMMATRIX2DVPROC glad_glUniformMatrix2dv = NULL;
PFNGLUNIFORMMATRIX2FVPROC glad_glUniformMatrix2fv = NULL;
PFNGLUNIFORMMATRIX2X3DVPROC glad_glUniformMatrix2x3dv = NULL;
PFNGLUNIFORMMATRIX2X3FVPROC glad_glUniformMatrix2x3fv = NULL;
PFNGLUNIFORMMATRIX2X4DVPROC glad_glUniformMatrix2x4dv = NULL;
PFNGLUNIFORMMATRIX2X4FVPROC glad_glUniformMatrix2x4fv = NULL;
PFNGLUNIFORMMATRIX3DVPROC glad_glUniformMatrix3dv = NULL;
PFNGLUNIFORMMATRIX3FVPROC glad_glUniformMatrix3fv = NULL;
PFNGLUNIFORMMATRIX3X2DVPROC glad_glUniformMatrix3x2dv = NULL;
PFNGLUNIFORMMATRIX3X2FVPROC glad_glUniformMatrix3x2fv = NULL;
PFNGLUNIFORMMATRIX3X4DVPROC glad_glUniformMatrix3x4dv = NULL;
PFNGLUNIFORMMATRIX3X4FVPROC glad_glUniformMatrix3x4fv = NULL;
PFNGLUNIFORMMATRIX4DVPROC glad_glUniformMatrix4dv = NULL;
PFNGLUNIFORMMATRIX4FVPROC glad_glUniformMatrix4fv = NULL;
PFNGLUNIFORMMATRIX4X2DVPROC glad_glUniformMatrix4x2dv = NULL;
PFNGLUNIFORMMATRIX4X2FVPROC glad_glUniformMatrix4x2fv = NULL;
PFNGLUNIFORMMATRIX4X3DVPROC glad_glUniformMatrix4x3dv = NULL;
PFNGLUNIFORMMATRIX4X3FVPROC glad_glUniformMatrix4x3fv = NULL;
PFNGLUNLINKPROGRAMPROC glad_glUnmapBuffer = NULL;
PFNGLUNMAPNAMEDBUFFERPROC glad_glUnmapNamedBuffer = NULL;
PFNGLUSEPROGRAMPROC glad_glUseProgram = NULL;
PFNGLUSEPROGRAMSTAGESPROC glad_glUseProgramStages = NULL;
PFNGLVALIDATEPROGRAMPROC glad_glValidateProgram = NULL;
PFNGLVALIDATEPROGRAMPIPELINEPROC glad_glValidateProgramPipeline = NULL;
PFNGLVERTEXATTRIB1DPROC glad_glVertexAttrib1d = NULL;
PFNGLVERTEXATTRIB1DVPROC glad_glVertexAttrib1dv = NULL;
PFNGLVERTEXATTRIB1FPROC glad_glVertexAttrib1f = NULL;
PFNGLVERTEXATTRIB1FVPROC glad_glVertexAttrib1fv = NULL;
PFNGLVERTEXATTRIB1SPROC glad_glVertexAttrib1s = NULL;
PFNGLVERTEXATTRIB1SVPROC glad_glVertexAttrib1sv = NULL;
PFNGLVERTEXATTRIB2DPROC glad_glVertexAttrib2d = NULL;
PFNGLVERTEXATTRIB2DVPROC glad_glVertexAttrib2dv = NULL;
PFNGLVERTEXATTRIB2FPROC glad_glVertexAttrib2f = NULL;
PFNGLVERTEXATTRIB2FVPROC glad_glVertexAttrib2fv = NULL;
PFNGLVERTEXATTRIB2SPROC glad_glVertexAttrib2s = NULL;
PFNGLVERTEXATTRIB2SVPROC glad_glVertexAttrib2sv = NULL;
PFNGLVERTEXATTRIB3DPROC glad_glVertexAttrib3d = NULL;
PFNGLVERTEXATTRIB3DVPROC glad_glVertexAttrib3dv = NULL;
PFNGLVERTEXATTRIB3FPROC glad_glVertexAttrib3f = NULL;
PFNGLVERTEXATTRIB3FVPROC glad_glVertexAttrib3fv = NULL;
PFNGLVERTEXATTRIB3SPROC glad_glVertexAttrib3s = NULL;
PFNGLVERTEXATTRIB3SVPROC glad_glVertexAttrib3sv = NULL;
PFNGLVERTEXATTRIB4NBVPROC glad_glVertexAttrib4Nbv = NULL;
PFNGLVERTEXATTRIB4NIVPROC glad_glVertexAttrib4Niv = NULL;
PFNGLVERTEXATTRIB4NSVPROC glad_glVertexAttrib4Nsv = NULL;
PFNGLVERTEXATTRIB4NUBPROC glad_glVertexAttrib4Nub = NULL;
PFNGLVERTEXATTRIB4NUBVPROC glad_glVertexAttrib4Nubv = NULL;
PFNGLVERTEXATTRIB4NUIVPROC glad_glVertexAttrib4Nuiv = NULL;
PFNGLVERTEXATTRIB4NUSVPROC glad_glVertexAttrib4Nusv = NULL;
PFNGLVERTEXATTRIB4BVPROC glad_glVertexAttrib4bv = NULL;
PFNGLVERTEXATTRIB4DPROC glad_glVertexAttrib4d = NULL;
PFNGLVERTEXATTRIB4DVPROC glad_glVertexAttrib4dv = NULL;
PFNGLVERTEXATTRIB4FPROC glad_glVertexAttrib4f = NULL;
PFNGLVERTEXATTRIB4FVPROC glad_glVertexAttrib4fv = NULL;
PFNGLVERTEXATTRIB4IVPROC glad_glVertexAttrib4iv = NULL;
PFNGLVERTEXATTRIB4SPROC glad_glVertexAttrib4s = NULL;
PFNGLVERTEXATTRIB4SVPROC glad_glVertexAttrib4sv = NULL;
PFNGLVERTEXATTRIB4UBVPROC glad_glVertexAttrib4ubv = NULL;
PFNGLVERTEXATTRIB4UIVPROC glad_glVertexAttrib4uiv = NULL;
PFNGLVERTEXATTRIB4USVPROC glad_glVertexAttrib4usv = NULL;
PFNGLVERTEXATTRIBBINDINGPROC glad_glVertexAttribBinding = NULL;
PFNGLVERTEXATTRIBDIVISORPROC glad_glVertexAttribDivisor = NULL;
PFNGLVERTEXATTRIBF1VPROC glad_glVertexAttribFormat = NULL; // Fixed pointer typings
PFNGLVERTEXATTRIBI1IPROC glad_glVertexAttribI1i = NULL;
PFNGLVERTEXATTRIBI1IVPROC glad_glVertexAttribI1iv = NULL;
PFNGLVERTEXATTRIBI1UIPROC glad_glVertexAttribI1ui = NULL;
PFNGLVERTEXATTRIBI1UIIVPROC glad_glVertexAttribI1uiiv = NULL;
PFNGLVERTEXATTRIBI2IPROC glad_glVertexAttribI2i = NULL;
PFNGLVERTEXATTRIBI2IVPROC glad_glVertexAttribI2iv = NULL;
PFNGLVERTEXATTRIBI2UIPROC glad_glVertexAttribI2ui = NULL;
PFNGLVERTEXATTRIBI2UIIVPROC glad_glVertexAttribI2uiiv = NULL;
PFNGLVERTEXATTRIBI3IPROC glad_glVertexAttribI3i = NULL;
PFNGLVERTEXATTRIBI3IVPROC glad_glVertexAttribI3iv = NULL;
PFNGLVERTEXATTRIBI3UIPROC glad_glVertexAttribI3ui = NULL;
PFNGLVERTEXATTRIBI3UIIVPROC glad_glVertexAttribI3uiiv = NULL;
PFNGLVERTEXATTRIBI4BVPROC glad_glVertexAttribI4bv = NULL;
PFNGLVERTEXATTRIBI4IPROC glad_glVertexAttribI4i = NULL;
PFNGLVERTEXATTRIBI4IVPROC glad_glVertexAttribI4iv = NULL;
PFNGLVERTEXATTRIBI4SVPROC glad_glVertexAttribI4sv = NULL;
PFNGLVERTEXATTRIBI4UBVPROC glad_glVertexAttribI4ubv = NULL;
PFNGLVERTEXATTRIBI4UIPROC glad_glVertexAttribI4ui = NULL;
PFNGLVERTEXATTRIBI4UIIVPROC glad_glVertexAttribI4uiiv = NULL;
PFNGLVERTEXATTRIBI4USVPROC glad_glVertexAttribI4usv = NULL;
PFNGLVERTEXATTRIBIFORMATPROC glad_glVertexAttribIFormat = NULL;
PFNGLVERTEXATTRIBIPOINTERPROC glad_glVertexAttribIPointer = NULL;
PFNGLVERTEXATTRIBL1DPROC glad_glVertexAttribL1d = NULL;
PFNGLVERTEXATTRIBL1DVPROC glad_glVertexAttribL1dv = NULL;
PFNGLVERTEXATTRIBL2DPROC glad_glVertexAttribL2d = NULL;
PFNGLVERTEXATTRIBL2DVPROC glad_glVertexAttribL2dv = NULL;
PFNGLVERTEXATTRIBL3DPROC glad_glVertexAttribL3d = NULL;
PFNGLVERTEXATTRIBL3DVPROC glad_glVertexAttribL3dv = NULL;
PFNGLVERTEXATTRIBL4DPROC glad_glVertexAttribL4d = NULL;
PFNGLVERTEXATTRIBL4DVPROC glad_glVertexAttribL4dv = NULL;
PFNGLVERTEXATTRIBLFORMATPROC glad_glVertexAttribLFormat = NULL;
PFNGLVERTEXATTRIBLPOINTERPROC glad_glVertexAttribLPointer = NULL;
PFNGLVERTEXATTRIBPOINTERPROC glad_glVertexAttribPointer = NULL;
PFNGLVERTEXBINDINGDIVISORPROC glad_glVertexBindingDivisor = NULL;
PFNGLVIEWPORTPROC glad_glViewport = NULL;
PFNGLVIEWPORTARRAYVPROC glad_glViewportArrayv = NULL;
PFNGLVIEWPORTINDEXEDFPROC glad_glViewportIndexedf = NULL;
PFNGLVIEWPORTINDEXEDFVPROC glad_glViewportIndexedfv = NULL;
PFNGLWAITSYNCPROC glad_glWaitSync = NULL;

static int find_GL_version(void) {
    int major, minor;
    const char* version;
    const char* prefixes[] = {
        "OpenGL ES-CM ",
        "OpenGL ES-CL ",
        "OpenGL ES ",
        NULL
    };

    int i;
    version = (const char*)glGetString(GL_VERSION);
    if (!version) return 0;

    for (i = 0; prefixes[i] != NULL; i++) {
        const char *prefix = prefixes[i];
        size_t length = strlen(prefix);
        if (strncmp(version, prefix, length) == 0) {
            version += length;
            break;
        }
    }

    major = (int)version[0] - '0';
    minor = (int)version[2] - '0';
    return (major * 10) + minor;
}

static int GLAD_num_extensions = 0;
static const char* const* GLAD_extensions = NULL;

static int has_ext(const char *ext) {
    int i;
    if (GLAD_num_extensions < 0) {
        if (GLAD_extensions == NULL) return 0;
        for (i = 0; GLAD_extensions[i] != NULL; i++) {
            if (strcmp(GLAD_extensions[i], ext) == 0) return 1;
        }
        return 0;
    }
    for (i = 0; i < GLAD_num_extensions; i++) {
        if (strcmp(GLAD_extensions[i], ext) == 0) return 1;
    }
    return 0;
}

static void load_GL_VERSION_1_0(GLADloadproc load) {
    glad_glCullFace = (PFNGLCULLFACEPROC)load("glCullFace");
    glad_glFrontFace = (PFNGLFRONTFACEPROC)load("glFrontFace");
    glad_glHint = (PFNGLHINTPROC)load("glHint");
    glad_glLineWidth = (PFNGLLINEWIDTHPROC)load("glLineWidth");
    glad_glPointSize = (PFNGLPOINTSIZEPROC)load("glPointSize");
    glad_glPolygonMode = (PFNGLPOLYGONMODEPROC)load("glPolygonMode");
    glad_glScissor = (PFNGLSCISSORPROC)load("glScissor");
    glad_glTexParameterf = (PFNGLTEXPARAMETERFPROC)load("glTexParameterf");
    glad_glTexParameterfv = (PFNGLTEXPARAMETERFVPROC)load("glTexParameterfv");
    glad_glTexParameteri = (PFNGLTEXPARAMETERIPROC)load("glTexParameteri");
    glad_glTexParameteriv = (PFNGLTEXPARAMETERIVPROC)load("glTexParameteriv");
    glad_glTexImage1D = (PFNGLTEXIMAGE1DPROC)load("glTexImage1D");
    glad_glTexImage2D = (PFNGLTEXIMAGE2DPROC)load("glTexImage2D");
    glad_glDrawBuffer = (PFNGLDRAWBUFFERPROC)load("glDrawBuffer");
    glad_glClear = (PFNGLCLEARPROC)load("glClear");
    glad_glClearColor = (PFNGLCLEARCOLORPROC)load("glClearColor");
    glad_glClearStencil = (PFNGLCLEARSTENCILPROC)load("glClearStencil");
    glad_glClearDepth = (PFNGLCLEARDEPTHPROC)load("glClearDepth");
    glad_glStencilMask = (PFNGLSTENCILMASKPROC)load("glStencilMask");
    glad_glColorMask = (PFNGLCOLORMASKPROC)load("glColorMask");
    glad_glDepthMask = (PFNGLDEPTHMASKPROC)load("glDepthMask");
    glad_glDisable = (PFNGLDISABLEPROC)load("glDisable");
    glad_glEnable = (PFNGLENABLEPROC)load("glEnable");
    glad_glFinish = (PFNGLFINISHPROC)load("glFinish");
    glad_glFlush = (PFNGLFLUSHPROC)load("glFlush");
    glad_glBlendFunc = (PFNGLBLENDFUNCPROC)load("glBlendFunc");
    glad_glLogicOp = (PFNGLLOGICOPPROC)load("glLogicOp");
    glad_glStencilFunc = (PFNGLSTENCILFUNCPROC)load("glStencilFunc");
    glad_glStencilOp = (PFNGLSTENCILOPPROC)load("glStencilOp");
    glad_glDepthFunc = (PFNGLDEPTHFUNCPROC)load("glDepthFunc");
    glad_glPixelStoref = (PFNGLPIXELSTOREFPROC)load("glPixelStoref");
    glad_glPixelStorei = (PFNGLPIXELSTOREIPROC)load("glPixelStorei");
    glad_glReadPixels = (PFNGLREADPIXELSPROC)load("glReadPixels");
    glad_glGetBooleanv = (PFNGLGETBOOLEANVPROC)load("glGetBooleanv");
    glad_glGetDoublev = (PFNGLGETDOUBLEVPROC)load("glGetDoublev");
    glad_glGetError = (PFNGLGETERRORPROC)load("glGetError");
    glad_glGetFloatv = (PFNGLGETFLOATVPROC)load("glGetFloatv");
    glad_glGetIntegerv = (PFNGLGETINTEGERVPROC)load("glGetIntegerv");
    glad_glGetString = (PFNGLGETSTRINGPROC)load("glGetString");
    glad_glGetTexImage = (PFNGLGETTEXIMAGEPROC)load("glGetTexImage");
    glad_glGetTexParameterfv = (PFNGLGETTEXPARAMETERFVPROC)load("glGetTexParameterfv");
    glad_glGetTexParameteriv = (PFNGLGETTEXPARAMETERIVPROC)load("glGetTexParameteriv");
    glad_glGetTexLevelParameterfv = (PFNGLGETTEXLEVELPARAMETERFVPROC)load("glGetTexLevelParameterfv");
    glad_glGetTexLevelParameteriv = (PFNGLGETTEXLEVELPARAMETERIVPROC)load("glGetTexLevelParameteriv");
    glad_glIsEnabled = (PFNGLISENABLEDPROC)load("glIsEnabled");
    glad_glDepthRange = (PFNGLDEPTHRANGEPROC)load("glDepthRange");
    glad_glViewport = (PFNGLVIEWPORTPROC)load("glViewport");
}
static void load_GL_VERSION_1_1(GLADloadproc load) {
    glad_glDrawArrays = (PFNGLDRAWARRAYSPROC)load("glDrawArrays");
    glad_glDrawElements = (PFNGLDRAWELEMENTSPROC)load("glDrawElements");
    glad_glPolygonOffset = (PFNGLPOLYGONOFFLETPROC)load("glPolygonOffset");
    glad_glCopyTexImage1D = (PFNGLCOPYTEXIMAGE1DPROC)load("glCopyTexImage1D");
    glad_glCopyTexImage2D = (PFNGLCOPYTEXIMAGE2DPROC)load("glCopyTexImage2D");
    glad_glCopyTexSubImage1D = (PFNGLCOPYTEXSUBIMAGE1DPROC)load("glCopyTexSubImage1D");
    glad_glCopyTexSubImage2D = (PFNGLCOPYTEXSUBIMAGE2DPROC)load("glCopyTexSubImage2D");
    glad_glTexSubImage1D = (PFNGLTEXSUBIMAGE1DPROC)load("glTexSubImage1D");
    glad_glTexSubImage2D = (PFNGLTEXSUBIMAGE2DPROC)load("glTexSubImage2D");
    glad_glBindTexture = (PFNGLBINDTEXTUREPROC)load("glBindTexture");
    glad_glDeleteTextures = (PFNGLDELETETEXTURESPROC)load("glDeleteTextures");
    glad_glGenTextures = (PFNGLGENTEXTURESPROC)load("glGenTextures");
    glad_glIsTexture = (PFNGLISTEXTUREPROC)load("glIsTexture");
}

static void load_GL_VERSION_1_2(GLADloadproc load) {
    glad_glDrawRangeElements = (PFNGLDRAWRANGEELEMENTSPROC)load("glDrawRangeElements");
    glad_glTexImage3D = (PFNGLTEXIMAGE3DPROC)load("glTexImage3D");
    glad_glTexSubImage3D = (PFNGLTEXSUBIMAGE3DPROC)load("glTexSubImage3D");
    glad_glCopyTexSubImage3D = (PFNGLCOPYTEXSUBIMAGE3DPROC)load("glCopyTexSubImage3D");
}

static void load_GL_VERSION_1_3(GLADloadproc load) {
    glad_glActiveTexture = (PFNGLACTIVETEXTUREPROC)load("glActiveTexture");
    glad_glSampleCoverage = (PFNGLSAMPLECOVERAGEPROC)load("glSampleCoverage");
    glad_glCompressedTexImage3D = (PFNGLCOMPRESSEDTEXIMAGE3DPROC)load("glCompressedTexImage3D");
    glad_glCompressedTexImage2D = (PFNGLCOMPRESSEDTEXIMAGE2DPROC)load("glCompressedTexImage2D");
    glad_glCompressedTexImage1D = (PFNGLCOMPRESSEDTEXIMAGE1DPROC)load("glCompressedTexImage1D");
    glad_glCompressedTexSubImage3D = (PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC)load("glCompressedTexSubImage3D");
    glad_glCompressedTexSubImage2D = (PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC)load("glCompressedTexSubImage2D");
    glad_glCompressedTexSubImage1D = (PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC)load("glCompressedTexSubImage1D");
    glad_glGetCompressedTexImage = (PFNGLGETCOMPRESSEDTEXIMAGEPROC)load("glGetCompressedTexImage");
}

static void load_GL_VERSION_1_4(GLADloadproc load) {
    glad_glBlendFuncSeparate = (PFNGLBLENDFUNCSEPARATEPROC)load("glBlendFuncSeparate");
    glad_glMultiDrawArrays = (PFNGLMULTIDRAWARRAYSPROC)load("glMultiDrawArrays");
    glad_glMultiDrawElements = (PFNGLMULTIDRAWELEMENTSPROC)load("glMultiDrawElements");
    glad_glPointParameterf = (PFNGLPOINTPARAMETERFPROC)load("glPointParameterf");
    glad_glPointParameterfv = (PFNGLPOINTPARAMETERFVPROC)load("glPointParameterfv");
    glad_glPointParameteri = (PFNGLPOINTPARAMETERIPROC)load("glPointParameteri");
    glad_glPointParameteriv = (PFNGLPOINTPARAMETERIVPROC)load("glPointParameteriv");
    glad_glBlendColor = (PFNGLBLENDCOLORPROC)load("glBlendColor");
    glad_glBlendEquation = (PFNGLBLENDEQUATIONPROC)load("glBlendEquation");
}

static void load_GL_VERSION_1_5(GLADloadproc load) {
    glad_glGenQueries = (PFNGLGENQUERIESPROC)load("glGenQueries");
    glad_glDeleteQueries = (PFNGLDELETEQUERIESPROC)load("glDeleteQueries");
    glad_glIsQuery = (PFNGLISQUERYPROC)load("glIsQuery");
    glad_glBeginQuery = (PFNGLBEGINQUERIESPROC)load("glBeginQuery");
    glad_glEndQuery = (PFNGLENDQUERIESPROC)load("glEndQuery");
    glad_glGetQueryiv = (PFNGLGETQUERYIVPROC)load("glGetQueryiv");
    glad_glGetQueryObjectiv = (PFNGLGETQUERYOBJECTIVPROC)load("glGetQueryObjectiv");
    glad_glGetQueryObjectuiv = (PFNGLGETQUERYOBJECTUIVPROC)load("glGetQueryObjectuiv");
    glad_glBindBuffer = (PFNGLBINDBUFFERPROC)load("glBindBuffer");
    glad_glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)load("glDeleteBuffers");
    glad_glGenBuffers = (PFNGLGENBUFFERSPROC)load("glGenBuffers");
    glad_glIsBuffer = (PFNGLISBUFFERPROC)load("glIsBuffer");
    glad_glBufferData = (PFNGLBUFFERDATAPROC)load("glBufferData");
    glad_glBufferSubData = (PFNGLBUFFERSUBDATAPROC)load("glBufferSubData");
    glad_glGetBufferSubData = (PFNGLGETBUFFERSUBDATAPROC)load("glGetBufferSubData");
    glad_glMapBuffer = (PFNGLMAPBUFFERPROC)load("glMapBuffer");
    glad_glUnmapBuffer = (PFNGLUNMAPBUFFERPROC)load("glUnmapBuffer");
    glad_glGetBufferParameteriv = (PFNGLGETBUFFERPARAMETERIVPROC)load("glGetBufferParameteriv");
    glad_glGetBufferPointerv = (PFNGLGETBUFFERPOINTERVPROC)load("glGetBufferPointerv");
}
static void load_GL_VERSION_2_0(GLADloadproc load) {
    glad_glBlendEquationSeparate = (PFNGLBLENDEQUATIONSEPARATEPROC)load("glBlendEquationSeparate");
    glad_glDrawBuffers = (PFNGLDRAWBUFFERSPROC)load("glDrawBuffers");
    glad_glStencilOpSeparate = (PFNGLSTENCILOPSEPARATEPROC)load("glStencilOpSeparate");
    glad_glStencilFuncSeparate = (PFNGLSTENCILFUNCSEPARATEPROC)load("glStencilFuncSeparate");
    glad_glStencilMaskSeparate = (PFNGLSTENCILMASKSEPARATEPROC)load("glStencilMaskSeparate");
    glad_glAttachShader = (PFNGLATTACHSHADERPROC)load("glAttachShader");
    glad_glBindAttribLocation = (PFNGLBINDATTRIBLOCATIONPROC)load("glBindAttribLocation");
    glad_glCompileShader = (PFNGLCOMPILESHADERPROC)load("glCompileShader");
    glad_glCreateProgram = (PFNGLCREATEPROGRAMPROC)load("glCreateProgram");
    glad_glCreateShader = (PFNGLCREATESHADERPROC)load("glCreateShader");
    glad_glDeleteProgram = (PFNGLDELETEPROGRAMPROC)load("glDeleteProgram");
    glad_glDeleteShader = (PFNGLDELETESHADERPROC)load("glDeleteShader");
    glad_glDetachShader = (PFNGLDETACHSHADERPROC)load("glDetachShader");
    glad_glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)load("glDisableVertexAttribArray");
    glad_glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)load("glEnableVertexAttribArray");
    glad_glGetActiveAttrib = (PFNGLGETACTIVEATTRIBLOCATIONPROC)load("glGetActiveAttrib");
    glad_glGetActiveUniform = (PFNGLGETACTIVEUNIFORMPROC)load("glGetActiveUniform");
    glad_glGetAttachedShaders = (PFNGLGETATTACHEDSHADERSPROC)load("glGetAttachedShaders");
    glad_glGetAttribLocation = (PFNGLGETATTRIBLOCATIONPROC)load("glGetAttribLocation");
    glad_glGetProgramiv = (PFNGLGETPROGRAMIVPROC)load("glGetProgramiv");
    glad_glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)load("glGetProgramInfoLog");
    glad_glGetShaderiv = (PFNGLGETSHADERIVPROC)load("glGetShaderiv");
    glad_glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)load("glGetShaderInfoLog");
    glad_glGetShaderSource = (PFNGLGETSHADERSOURCEPROC)load("glGetShaderSource");
    glad_glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)load("glGetUniformLocation");
    glad_glGetUniformfv = (PFNGLGETUNIFORMFVPROC)load("glGetUniformfv");
    glad_glGetUniformiv = (PFNGLGETUNIFORIVPROC)load("glGetUniformiv");
    glad_glGetVertexAttribdv = (PFNGLGETVERTEXATTRIBDVPROC)load("glGetVertexAttribdv");
    glad_glGetVertexAttribfv = (PFNGLGETVERTEXATTRIBFVPROC)load("glGetVertexAttribfv");
    glad_glGetVertexAttribiv = (PFNGLGETVERTEXATTRIBIVPROC)load("glGetVertexAttribiv");
    glad_glGetVertexAttribPointerv = (PFNGLGETVERTEXATTRIBPOINTERVPROC)load("glGetVertexAttribPointerv");
    glad_glIsProgram = (PFNGLISPROGRAMPROC)load("glIsProgram");
    glad_glIsShader = (PFNGLISSHADERPROC)load("glIsShader");
    glad_glLinkProgram = (PFNGLLINKPROGRAMPROC)load("glLinkProgram");
    glad_glShaderSource = (PFNGLSHADERSOURCEPROC)load("glShaderSource");
    glad_glUseProgram = (PFNGLUSEPROGRAMPROC)load("glUseProgram");
    glad_glValidateProgram = (PFNGLVALIDATEPROGRAMPROC)load("glValidateProgram");
    glad_glUniform1f = (PFNGLUNIFORM1FPROC)load("glUniform1f");
    glad_glUniform1fv = (PFNGLUNIFORM1FVPROC)load("glUniform1fv");
    glad_glUniform1i = (PFNGLUNIFORM1IPROC)load("glUniform1i");
    glad_glUniform1iv = (PFNGLUNIFORM1IVPROC)load("glUniform1iv");
    glad_glUniform2f = (PFNGLUNIFORM2FPROC)load("glUniform2f");
    glad_glUniform2fv = (PFNGLUNIFORM2FVPROC)load("glUniform2fv");
    glad_glUniform2i = (PFNGLUNIFORM2IPROC)load("glUniform2i");
    glad_glUniform2iv = (PFNGLUNIFORM2IVPROC)load("glUniform2iv");
    glad_glUniform3f = (PFNGLUNIFORM3FPROC)load("glUniform3f");
    glad_glUniform3fv = (PFNGLUNIFORM3FVPROC)load("glUniform3fv");
    glad_glUniform3i = (PFNGLUNIFORM3IPROC)load("glUniform3i");
    glad_glUniform3iv = (PFNGLUNIFORM3IVPROC)load("glUniform3iv");
    glad_glUniform4f = (PFNGLUNIFORM4FPROC)load("glUniform4f");
    glad_glUniform4fv = (PFNGLUNIFORM4FVPROC)load("glUniform4fv");
    glad_glUniform4i = (PFNGLUNIFORM4IPROC)load("glUniform4i");
    glad_glUniform4iv = (PFNGLUNIFORM4IVPROC)load("glUniform4iv");
    glad_glUniformMatrix2fv = (PFNGLUNIFORMMATRIX2FVPROC)load("glUniformMatrix2fv");
    glad_glUniformMatrix3fv = (PFNGLUNIFORMMATRIX3FVPROC)load("glUniformMatrix3fv");
    glad_glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)load("glUniformMatrix4fv");
    glad_glVertexAttrib1d = (PFNGLVERTEXATTRIB1DPROC)load("glVertexAttrib1d");
    glad_glVertexAttrib1dv = (PFNGLVERTEXATTRIB1DVPROC)load("glVertexAttrib1dv");
    glad_glVertexAttrib1f = (PFNGLVERTEXATTRIB1FPROC)load("glVertexAttrib1f");
    glad_glVertexAttrib1fv = (PFNGLVERTEXATTRIB1FVPROC)load("glVertexAttrib1fv");
    glad_glVertexAttrib1s = (PFNGLVERTEXATTRIB1SPROC)load("glVertexAttrib1s");
    glad_glVertexAttrib1sv = (PFNGLVERTEXATTRIB1SVPROC)load("glVertexAttrib1sv");
    glad_glVertexAttrib2d = (PFNGLVERTEXATTRIB2DPROC)load("glVertexAttrib2d");
    glad_glVertexAttrib2dv = (PFNGLVERTEXATTRIB2DVPROC)load("glVertexAttrib2dv");
    glad_glVertexAttrib2f = (PFNGLVERTEXATTRIB2FPROC)load("glVertexAttrib2f");
    glad_glVertexAttrib2fv = (PFNGLVERTEXATTRIB2FVPROC)load("glVertexAttrib2fv");
    glad_glVertexAttrib2s = (PFNGLVERTEXATTRIB2SPROC)load("glVertexAttrib2s");
    glad_glVertexAttrib2sv = (PFNGLVERTEXATTRIB2SVPROC)load("glVertexAttrib2sv");
    glad_glVertexAttrib3d = (PFNGLVERTEXATTRIB3DPROC)load("glVertexAttrib3d");
    glad_glVertexAttrib3dv = (PFNGLVERTEXATTRIB3DVPROC)load("glVertexAttrib3dv");
    glad_glVertexAttrib3f = (PFNGLVERTEXATTRIB3FPROC)load("glVertexAttrib3f");
    glad_glVertexAttrib3fv = (PFNGLVERTEXATTRIB3FVPROC)load("glVertexAttrib3fv");
    glad_glVertexAttrib3s = (PFNGLVERTEXATTRIB3SPROC)load("glVertexAttrib3s");
    glad_glVertexAttrib3sv = (PFNGLVERTEXATTRIB3SVPROC)load("glVertexAttrib3sv");
    glad_glVertexAttrib4Nbv = (PFNGLVERTEXATTRIB4NBVPROC)load("glVertexAttrib4Nbv");
    glad_glVertexAttrib4Niv = (PFNGLVERTEXATTRIB4NIVPROC)load("glVertexAttrib4Niv");
    glad_glVertexAttrib4Nsv = (PFNGLVERTEXATTRIB4NSVPROC)load("glVertexAttrib4Nsv");
    glad_glVertexAttrib4Nub = (PFNGLVERTEXATTRIB4NUBPROC)load("glVertexAttrib4Nub");
    glad_glVertexAttrib4Nubv = (PFNGLVERTEXATTRIB4NUBVPROC)load("glVertexAttrib4Nubv");
    glad_glVertexAttrib4Nuiv = (PFNGLVERTEXATTRIB4NUIVPROC)load("glVertexAttrib4Nuiv");
    glad_glVertexAttrib4Nusv = (PFNGLVERTEXATTRIB4NUSVPROC)load("glVertexAttrib4Nusv");
    glad_glVertexAttrib4bv = (PFNGLVERTEXATTRIB4BVPROC)load("glVertexAttrib4bv");
    glad_glVertexAttrib4d = (PFNGLVERTEXATTRIB4DPROC)load("glVertexAttrib4d");
    glad_glVertexAttrib4dv = (PFNGLVERTEXATTRIB4DVPROC)load("glVertexAttrib4dv");
    glad_glVertexAttrib4f = (PFNGLVERTEXATTRIB4FPROC)load("glVertexAttrib4f");
    glad_glVertexAttrib4fv = (PFNGLVERTEXATTRIB4FVPROC)load("glVertexAttrib4fv");
    glad_glVertexAttrib4iv = (PFNGLVERTEXATTRIB4IVPROC)load("glVertexAttrib4iv");
    glad_glVertexAttrib4s = (PFNGLVERTEXATTRIB4SPROC)load("glVertexAttrib4s");
    glad_glVertexAttrib4sv = (PFNGLVERTEXATTRIB4SVPROC)load("glVertexAttrib4sv");
    glad_glVertexAttrib4ubv = (PFNGLVERTEXATTRIB4UBVPROC)load("glVertexAttrib4ubv");
    glad_glVertexAttrib4uiv = (PFNGLVERTEXATTRIB4UIVPROC)load("glVertexAttrib4uiv");
    glad_glVertexAttrib4usv = (PFNGLVERTEXATTRIB4USVPROC)load("glVertexAttrib4usv");
    glad_glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)load("glVertexAttribPointer");
}

static void load_GL_VERSION_2_1(GLADloadproc load) {
    glad_glUniformMatrix2x3fv = (PFNGLUNIFORMMATRIX2X3FVPROC)load("glUniformMatrix2x3fv");
    glad_glUniformMatrix3x2fv = (PFNGLUNIFORMMATRIX3X2FVPROC)load("glUniformMatrix3x2fv");
    glad_glUniformMatrix2x4fv = (PFNGLUNIFORMMATRIX2X4FVPROC)load("glUniformMatrix2x4fv");
    glad_glUniformMatrix4x2fv = (PFNGLUNIFORMMATRIX4X2FVPROC)load("glUniformMatrix4x2fv");
    glad_glUniformMatrix3x4fv = (PFNGLUNIFORMMATRIX3X4FVPROC)load("glUniformMatrix3x4fv");
    glad_glUniformMatrix4x3fv = (PFNGLUNIFORMMATRIX4X3FVPROC)load("glUniformMatrix4x3fv");
}
static void load_GL_VERSION_3_0(GLADloadproc load) {
    glad_glColorMaski = (PFNGLCOLORMASKIPROC)load("glColorMaski");
    glad_glGetBooleani_v = (PFNGLGETBOOLEANI_VPROC)load("glGetBooleani_v");
    glad_glGetIntegeri_v = (PFNGLGETINTEGERI_VPROC)load("glGetIntegeri_v");
    glad_glEnablei = (PFNGLENABLEIPROC)load("glEnablei");
    glad_glDisablei = (PFNGLDISABLEIPROC)load("glDisablei");
    glad_glIsEnabledi = (PFNGLISENABLEDIPROC)load("glIsEnabledi");
    glad_glBeginTransformFeedback = (PFNGLBEGINTRANSFORMFEEDBACKPROC)load("glBeginTransformFeedback");
    glad_glEndTransformFeedback = (PFNGLENDTRANSFORMFEEDBACKPROC)load("glEndTransformFeedback");
    glad_glBindBufferRange = (PFNGLBINDBUFFERRANGEPROC)load("glBindBufferRange");
    glad_glBindBufferBase = (PFNGLBINDBUFFERBASEPROC)load("glBindBufferBase");
    glad_glTransformFeedbackVaryings = (PFNGLTRANSFORMFEEDBACKVARYINGSPROC)load("glTransformFeedbackVaryings");
    glad_glGetTransformFeedbackVarying = (PFNGLGETTRANSFORMFEEDBACKVARYINGPROC)load("glGetTransformFeedbackVarying");
    glad_clampColor = (PFNGLCLAMPCOLORPROC)load("glClampColor");
    glad_glBeginConditionalRender = (PFNGLBEGINCONDITIONALRENDERPROC)load("glBeginConditionalRender");
    glad_glEndConditionalRender = (PFNGLENDCONDITIONALRENDERPROC)load("glEndConditionalRender");
    glad_glVertexAttribIPointer = (PFNGLVERTEXATTRIBIPOINTERPROC)load("glVertexAttribIPointer");
    glad_glGetVertexAttribIiv = (PFNGLGETVERTEXATTRIBIIVPROC)load("glGetVertexAttribIiv");
    glad_glGetVertexAttribIuiv = (PFNGLGETVERTEXATTRIBIUIVPROC)load("glGetVertexAttribIuiv");
    glad_glVertexAttribI1i = (PFNGLVERTEXATTRIBI1IPROC)load("glVertexAttribI1i");
    glad_glVertexAttribI2i = (PFNGLVERTEXATTRIBI2IPROC)load("glVertexAttribI2i");
    glad_glVertexAttribI3i = (PFNGLVERTEXATTRIBI3IPROC)load("glVertexAttribI3i");
    glad_glVertexAttribI4i = (PFNGLVERTEXATTRIBI4IPROC)load("glVertexAttribI4i");
    glad_glVertexAttribI1ui = (PFNGLVERTEXATTRIBI1UIPROC)load("glVertexAttribI1ui");
    glad_glVertexAttribI2ui = (PFNGLVERTEXATTRIBI2UIPROC)load("glVertexAttribI2ui");
    glad_glVertexAttribI3ui = (PFNGLVERTEXATTRIBI3UIPROC)load("glVertexAttribI3ui");
    glad_glVertexAttribI4ui = (PFNGLVERTEXATTRIBI4UIPROC)load("glVertexAttribI4ui");
    glad_glVertexAttribI1iv = (PFNGLVERTEXATTRIBI1IVPROC)load("glVertexAttribI1iv");
    glad_glVertexAttribI2iv = (PFNGLVERTEXATTRIBI2IVPROC)load("glVertexAttribI2iv");
    glad_glVertexAttribI3iv = (PFNGLVERTEXATTRIBI3IVPROC)load("glVertexAttribI3iv");
    glad_glVertexAttribI4iv = (PFNGLVERTEXATTRIBI4IVPROC)load("glVertexAttribI4iv");
    glad_glVertexAttribI1uiiv = (PFNGLVERTEXATTRIBI1UIIVPROC)load("glVertexAttribI1uiiv");
    glad_glVertexAttribI2uiiv = (PFNGLVERTEXATTRIBI2UIIVPROC)load("glVertexAttribI2uiiv");
    glad_glVertexAttribI3uiiv = (PFNGLVERTEXATTRIBI3UIIVPROC)load("glGetVertexAttribI3uiiv"); // Manual mapping fix
    glad_glVertexAttribI4uiiv = (PFNGLVERTEXATTRIBI4UIIVPROC)load("glVertexAttribI4uiiv");
    glad_glVertexAttribI4bv = (PFNGLVERTEXATTRIBI4BVPROC)load("glVertexAttribI4bv");
    glad_glVertexAttribI4sv = (PFNGLVERTEXATTRIBI4SVPROC)load("glVertexAttribI4sv");
    glad_glVertexAttribI4ubv = (PFNGLVERTEXATTRIBI4UBVPROC)load("glVertexAttribI4ubv");
    glad_glVertexAttribI4usv = (PFNGLVERTEXATTRIBI4USVPROC)load("glVertexAttribI4usv");
    glad_glGetUniformuiv = (PFNGLGETUNIFORMUIVPROC)load("glGetUniformuiv");
    glad_glBindFragDataLocation = (PFNGLBINDFRAGDATALOCATIONPROC)load("glBindFragDataLocation");
    glad_glGetFragDataLocation = (PFNGLGETFRAGDATALOCATIONPROC)load("glGetFragDataLocation");
    glad_glUniform1ui = (PFNGLUNIFORM1UIPROC)load("glUniform1ui");
    glad_glUniform1uiv = (PFNGLUNIFORM1UIVPROC)load("glUniform1uiv");
    glad_glUniform2ui = (PFNGLUNIFORM2UIPROC)load("glUniform2ui");
    glad_glUniform2uiv = (PFNGLUNIFORM2UIVPROC)load("glUniform2uiv");
    glad_glUniform3ui = (PFNGLUNIFORM3UIPROC)load("glUniform3ui");
    glad_glUniform3uiv = (PFNGLUNIFORM3UIVPROC)load("glUniform3uiv");
    glad_glUniform4ui = (PFNGLUNIFORM4UIPROC)load("glUniform4ui");
    glad_glUniform4uiv = (PFNGLUNIFORM4UIVPROC)load("glUniform4uiv");
    glad_glTexParameterIiv = (PFNGLTEXPARAMETERIIVPROC)load("glTexParameterIiv");
    glad_glTexParameterIuiv = (PFNGLTEXPARAMETERIUIVPROC)load("glTexParameterIuiv");
    glad_glGetTexParameterIiv = (PFNGLGETTEXPARAMETERIIVPROC)load("glGetTexParameterIiv");
    glad_glGetTexParameterIuiv = (PFNGLGETTEXPARAMETERIUIVPROC)load("glGetTexParameterIuiv");
    glad_glClearBufferiv = (PFNGLCLEARBUFFERIVPROC)load("glClearBufferiv");
    glad_glClearBufferuiv = (PFNGLCLEARBUFFERUIVPROC)load("glClearBufferuiv");
    glad_glClearBufferfv = (PFNGLCLEARBUFFERFVPROC)load("glClearBufferfv");
    glad_glClearBufferfi = (PFNGLCLEARBUFFERFIPROC)load("glClearBufferfi");
    glad_glGetStringi = (PFNGLGETSTRINGIPROC)load("glGetStringi");
    glad_glIsRenderbuffer = (PFNGLISRENDERBUFFERPROC)load("glIsRenderbuffer");
    glad_glBindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC)load("glBindRenderbuffer");
    glad_glDeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC)load("glDeleteRenderbuffers");
    glad_glGenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC)load("glGenRenderbuffers");
    glad_glRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC)load("glRenderbufferStorage");
    glad_glGetRenderbufferParameteriv = (PFNGLGETRENDERBUFFERPARAMETERIVPROC)load("glGetRenderbufferParameteriv");
    glad_glIsFramebuffer = (PFNGLISFRAMEBUFFERPROC)load("glIsFramebuffer");
    glad_glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)load("glBindFramebuffer");
    glad_glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)load("glDeleteFramebuffers");
    glad_glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)load("glGenFramebuffers");
    glad_glCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)load("glCheckFramebufferStatus");
    glad_glFramebufferTexture1D = (PFNGLFRAMEBUFFERTEXTURE1DPROC)load("glFramebufferTexture1D");
    glad_glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)load("glFramebufferTexture2D");
    glad_glFramebufferTexture3D = (PFNGLFRAMEBUFFERTEXTURE3DPROC)load("glFramebufferTexture3D");
    glad_glFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)load("glFramebufferRenderbuffer");
    glad_glGetFramebufferAttachmentParameteriv = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC)load("glGetFramebufferAttachmentParameteriv");
    glad_glGenerateMipmap = (PFNGLGENERATEMIPMAPPROC)load("glGenerateMipmap");
    glad_glBlitFramebuffer = (PFNGLBLITFRAMEBUFFERPROC)load("glBlitFramebuffer");
    glad_glRenderbufferStorageMultisample = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC)load("glRenderbufferStorageMultisample");
    glad_glFramebufferTextureLayer = (PFNGLFRAMEBUFFERTEXTURELAYERPROC)load("glFramebufferTextureLayer");
    glad_glMapBufferRange = (PFNGLMAPBUFFERRANGEPROC)load("glMapBufferRange");
    glad_glFlushMappedBufferRange = (PFNGLFLUSHMAPPEDBUFFERRANGEPROC)load("glFlushMappedBufferRange");
    glad_glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)load("glBindVertexArray");
    glad_glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC)load("glDeleteVertexArrays");
    glad_glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)load("glGenVertexArrays");
    glad_glIsVertexArray = (PFNGLISVERTEXARRAYPROC)load("glIsVertexArray");
}

static void load_GL_VERSION_3_1(GLADloadproc load) {
    glad_glDrawArraysInstanced = (PFNGLDRAWARRAYSINSTANCEDPROC)load("glDrawArraysInstanced");
    glad_glDrawElementsInstanced = (PFNGLDRAWELEMENTSINSTANCEDPROC)load("glDrawElementsInstanced");
    glad_glTexBuffer = (PFNGLTEXBUFFERPROC)load("glTexBuffer");
    glad_glPrimitiveRestartIndex = (PFNGLPRIMITIVERESTARTINDEXPROC)load("glPrimitiveRestartIndex");
    glad_glCopyBufferSubData = (PFNGLCOPYBUFFERSUBDATAPROC)load("glCopyBufferSubData");
}

static void load_GL_VERSION_3_2(GLADloadproc load) {
    glad_glDrawElementsBaseVertex = (PFNGLDRAWELEMENTSBASEVERTEXPROC)load("glDrawElementsBaseVertex");
    glad_glDrawRangeElementsBaseVertex = (PFNGLDRAWRANGEELEMENTSBASEVERTEXPROC)load("glDrawRangeElementsBaseVertex");
    glad_glDrawElementsInstancedBaseVertex = (PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXPROC)load("glDrawElementsInstancedBaseVertex");
    glad_glMultiDrawElementsBaseVertex = (PFNGLMULTIDRAWELEMENTSBASEVERTEXPROC)load("glMultiDrawElementsBaseVertex");
    glad_glProvokingVertex = (PFNGLPROVOKINGVERTEXPROC)load("glProvokingVertex");
    glad_glFenceSync = (PFNGLFENCESYNCPROC)load("glFenceSync");
    glad_glIsSync = (PFNGLISSYNCPROC)load("glIsSync");
    glad_glDeleteSync = (PFNGLDELETESYNCPROC)load("glDeleteSync");
    glad_glClientWaitSync = (PFNGLCLIENTWAITSYNCPROC)load("glClientWaitSync");
    glad_glWaitSync = (PFNGLWAITSYNCPROC)load("glWaitSync");
    glad_glGetInteger64v = (PFNGLGETINTEGER64VPROC)load("glGetInteger64v");
    glad_glGetSynciv = (PFNGLGETSYNCIVPROC)load("glGetSynciv");
    glad_glGetInteger64i_v = (PFNGLGETINTEGER64I_VPROC)load("glGetInteger64i_v");
    glad_glGetBufferParameteri64v = (PFNGLGETBUFFERPARAMETERI64VPROC)load("glGetBufferParameteri64v");
    glad_glFramebufferTexture = (PFNGLFRAMEBUFFERTEXTUREPROC)load("glFramebufferTexture");
    glad_glTexImage2DMultisample = (PFNGLTEXIMAGE2DMULTISAMPLEPROC)load("glTexImage2DMultisample");
    glad_glTexImage3DMultisample = (PFNGLTEXIMAGE3DMULTISAMPLEPROC)load("glTexImage3DMultisample");
    glad_glGetMultisamplefv = (PFNGLGETMULTISAMPLEFVPROC)load("glGetMultisamplefv");
    glad_glSampleMaski = (PFNGLSAMPLEMASKIPROC)load("glSampleMaski");
}

static void load_GL_VERSION_3_3(GLADloadproc load) {
    glad_glBindFragDataLocationIndexed = (PFNGLBINDFRAGDATALOCATIONINDEXEDPROC)load("glBindFragDataLocationIndexed");
    glad_glGetFragDataIndex = (PFNGLGETFRAGDATAINDEXPROC)load("glGetFragDataIndex");
    glad_glGenSamplers = (PFNGLGENSAMPLERSPROC)load("glGenSamplers");
    glad_glDeleteSamplers = (PFNGLDELETESAMPLERSPROC)load("glDeleteSamplers");
    glad_glIsSampler = (PFNGLISSAMPLERPROC)load("glIsSampler");
    glad_glBindSampler = (PFNGLBINDSAMPLERPROC)load("glBindSampler");
    glad_glSamplerParameteri = (PFNGLSAMPLERPARAMETERIPROC)load("glSamplerParameteri");
    glad_glSamplerParameteriv = (PFNGLSAMPLERPARAMETERIVPROC)load("glSamplerParameteriv");
    glad_glSamplerParameterf = (PFNGLSAMPLERPARAMETERFPROC)load("glSamplerParameterf");
    glad_glSamplerParameterfv = (PFNGLSAMPLERPARAMETERFVPROC)load("glSamplerParameterfv");
    glad_glSamplerParameterIiv = (PFNGLSAMPLERPARAMETERIIVPROC)load("glSamplerParameterIiv");
    glad_glSamplerParameterIuiv = (PFNGLSAMPLERPARAMETERIUIVPROC)load("glSamplerParameterIuiv");
    glad_glGetSamplerParameteriv = (PFNGLGETSAMPLERPARAMETERIVPROC)load("glGetSamplerParameteriv");
    glad_glGetSamplerParameterIiv = (PFNGLGETSAMPLERPARAMETERIIVPROC)load("glGetSamplerParameterIiv");
    glad_glGetSamplerParameterfv = (PFNGLGETSAMPLERPARAMETERFVPROC)load("glGetSamplerParameterfv");
    glad_glGetSamplerParameterIuiv = (PFNGLGETSAMPLERPARAMETERIUIVPROC)load("glGetSamplerParameterIuiv");
    glad_glQueryCounter = (PFNGLQUERYCOUNTERPROC)load("glQueryCounter");
    glad_glGetQueryObjecti64v = (PFNGLGETQUERYOBJECTI64VPROC)load("glGetQueryObjecti64v");
    glad_glGetQueryObjectui64v = (PFNGLGETQUERYOBJECTUI64VPROC)load("glGetQueryObjectui64v");
    glad_glVertexAttribDivisor = (PFNGLVERTEXATTRIBDIVISORPROC)load("glVertexAttribDivisor");
    glad_glVertexAttribP1ui = (PFNGLVERTEXATTRIBP1UIPROC)load("glVertexAttribP1ui");
    glad_glVertexAttribP1uiv = (PFNGLVERTEXATTRIBP1UIVPROC)load("glVertexAttribP1uiv");
    glad_glVertexAttribP2ui = (PFNGLVERTEXATTRIBP2UIPROC)load("glVertexAttribP2ui");
    glad_glVertexAttribP2uiv = (PFNGLVERTEXATTRIBP2UIVPROC)load("glVertexAttribP2uiv");
    glad_glVertexAttribP3ui = (PFNGLVERTEXATTRIBP3UIPROC)load("glVertexAttribP3ui");
    glad_glVertexAttribP3uiv = (PFNGLVERTEXATTRIBP3UIVPROC)load("glVertexAttribP3uiv");
    glad_glVertexAttribP4ui = (PFNGLVERTEXATTRIBP4UIPROC)load("glVertexAttribP4ui");
    glad_glVertexAttribP4uiv = (PFNGLVERTEXATTRIBP4UIVPROC)load("glVertexAttribP4uiv");
}

static void load_GL_VERSION_4_0(GLADloadproc load) {
    glad_glMinSampleShading = (PFNGLMINSAMPLESHADINGPROC)load("glMinSampleShading");
    glad_glBlendEquationi = (PFNGLBLENDEQUATIONIPROC)load("glBlendEquationi");
    glad_glBlendEquationSeparatei = (PFNGLBLENDEQUATIONSEPARATEIPROC)load("glBlendEquationSeparatei");
    glad_glBlendFunci = (PFNGLBLENDFUNCIPROC)load("glBlendFunci");
    glad_glBlendFuncSeparatei = (PFNGLBLENDFUNCSEPARATEIPROC)load("glBlendFuncSeparatei");
    glad_glDrawArraysIndirect = (PFNGLDRAWARRAYSINDIRECTPROC)load("glDrawArraysIndirect");
    glad_glDrawElementsIndirect = (PFNGLDRAWELEMENTSINDIRECTPROC)load("glDrawElementsIndirect");
    glad_glUniform1d = (PFNGLUNIFORM1DPROC)load("glUniform1d");
    glad_glUniform2d = (PFNGLUNIFORM2DPROC)load("glUniform2d");
    glad_glUniform3d = (PFNGLUNIFORM3DPROC)load("glUniform3d");
    glad_glUniform4d = (PFNGLUNIFORM4DPROC)load("glUniform4d");
    glad_glUniform1dv = (PFNGLUNIFORM1DVPROC)load("glUniform1dv");
    glad_glUniform2dv = (PFNGLUNIFORM2DVPROC)load("glUniform2dv");
    glad_glUniform3dv = (PFNGLUNIFORM3DVPROC)load("glUniform3dv");
    glad_glUniform4dv = (PFNGLUNIFORM4DVPROC)load("glUniform4dv");
    glad_glUniformMatrix2dv = (PFNGLUNIFORMMATRIX2DVPROC)load("glUniformMatrix2dv");
    glad_glUniformMatrix3dv = (PFNGLUNIFORMMATRIX3DVPROC)load("glUniformMatrix3dv");
    glad_glUniformMatrix4dv = (PFNGLUNIFORMMATRIX4DVPROC)load("glUniformMatrix4dv");
    glad_glUniformMatrix2x3dv = (PFNGLUNIFORMMATRIX2X3DVPROC)load("glUniformMatrix2x3dv");
    glad_glUniformMatrix2x4dv = (PFNGLUNIFORMMATRIX2X4DVPROC)load("glUniformMatrix2x4dv");
    glad_glUniformMatrix3x2dv = (PFNGLUNIFORMMATRIX3X2DVPROC)load("glUniformMatrix3x2dv");
    glad_glUniformMatrix3x4dv = (PFNGLUNIFORMMATRIX3X4DVPROC)load("glUniformMatrix3x4dv");
    glad_glUniformMatrix4x2dv = (PFNGLUNIFORMMATRIX4X2DVPROC)load("glUniformMatrix4x2dv");
    glad_glUniformMatrix4x3dv = (PFNGLUNIFORMMATRIX4X3DVPROC)load("glUniformMatrix4x3dv");
    glad_glGetUniformdv = (PFNGLGETUNIFORMDVPROC)load("glGetUniformdv");
    glad_getSubroutineUniformLocation = (PFNGLGETSUBROUTINEUNIFORMLOCATIONPROC)load("glGetSubroutineUniformLocation");
    glad_glGetSubroutineIndex = (PFNGLGETSUBROUTINEINDEXPROC)load("glGetSubroutineIndex");
    glad_glGetActiveSubroutineUniformiv = (PFNGLGETACTIVESUBROUTINEUNIFORMIVPROC)load("glGetActiveSubroutineUniformiv");
    glad_glGetActiveSubroutineUniformName = (PFNGLGETACTIVESUBROUTINEUNIFORMNAMEPROC)load("glGetActiveSubroutineUniformName");
    glad_glGetActiveSubroutineName = (PFNGLGETACTIVESUBROUTINENAMEPROC)load("glGetActiveSubroutineName");
    glad_glUniformSubroutinesuiv = (PFNGLUNIFORMSUBROUTINESUIVPROC)load("glUniformSubroutinesuiv");
    glad_glGetUniformSubroutineuiv = (PFNGLGETUNIFORMSUBROUTINEUIVPROC)load("glGetUniformSubroutineuiv");
    glad_glGetProgramStageiv = (PFNGLGETPROGRAMSTAGEIVPROC)load("glGetProgramStageiv");
    glad_glPatchParameteri = (PFNGLPATCHPARAMETERIPROC)load("glPatchParameteri");
    glad_glPatchParameterfv = (PFNGLPATCHPARAMETERFVPROC)load("glPatchParameterfv");
    glad_glBindTransformFeedback = (PFNGLBINDTRANSFORMFEEDBACKPROC)load("glBindTransformFeedback");
    glad_glDeleteTransformFeedbacks = (PFNGLDELETETRANSFORMFEEDBACKSPROC)load("glDeleteTransformFeedbacks");
    glad_glGenTransformFeedbacks = (PFNGLGENTRANSFORMFEEDBACKSPROC)load("glGenTransformFeedbacks");
    glad_glIsTransformFeedback = (PFNGLISTRANSFORMFEEDBACKPROC)load("glIsTransformFeedback");
    glad_glPauseTransformFeedback = (PFNGLPAUSETRANSFORMFEEDBACKPROC)load("glPauseTransformFeedback");
    glad_glResumeTransformFeedback = (PFNGLRESUMETRANSFORMFEEDBACKPROC)load("glResumeTransformFeedback");
    glad_glDrawTransformFeedback = (PFNGLDRAWTRANSFORMFEEDBACKPROC)load("glDrawTransformFeedback");
    glad_glDrawTransformFeedbackStream = (PFNGLDRAWTRANSFORMFEEDBACKSTREAMPROC)load("glDrawTransformFeedbackStream");
    glad_glBeginQueryIndexed = (PFNGLBEGINQUERYINDEXEDPROC)load("glBeginQueryIndexed");
    glad_glEndQueryIndexed = (PFNGLENDQUERYINDEXEDPROC)load("glEndQueryIndexed");
    glad_glGetQueryIndexediv = (PFNGLGETQUERYINDEXEDIVPROC)load("glGetQueryIndexediv");
}

static void load_GL_VERSION_4_1(GLADloadproc load) {
    glad_glReleaseShaderCompiler = (PFNGLRELEASESHADERCOMPILERPROC)load("glReleaseShaderCompiler");
    glad_glShaderBinary = (PFNGLSHADERBINARYPROC)load("glShaderBinary");
    glad_glGetShaderPrecisionFormat = (PFNGLGETSHADERPRECISIONFORMATPROC)load("glGetShaderPrecisionFormat");
    glad_glDepthRangef = (PFNGLDEPTHRANGEFPROC)load("glDepthRangef");
    glad_glClearDepthf = (PFNGLCLEARDEPTHFPROC)load("glClearDepthf");
    glad_glGetProgramBinary = (PFNGLGETPROGRAMBINARYPROC)load("glGetProgramBinary");
    glad_glProgramBinary = (PFNGLPROGRAMBINARYPROC)load("glProgramBinary");
    glad_glProgramParameteri = (PFNGLPROGRAMPARAMETERIPROC)load("glProgramParameteri");
    glad_glUseProgramStages = (PFNGLUSEPROGRAMSTAGESPROC)load("glUseProgramStages");
    glad_glActiveShaderProgram = (PFNGLACTIVESHADERPROGRAMPROC)load("glActiveShaderProgram");
    glad_glCreateShaderProgramv = (PFNGLCREATESHADERPROGRAMVPROC)load("glCreateShaderProgramv");
    glad_glBindProgramPipeline = (PFNGLBINDPROGRAMPIPELINEPROC)load("glBindProgramPipeline");
    glad_glDeleteProgramPipelines = (PFNGLDELETEPROGRAMPIPELINESPROC)load("glDeleteProgramPipelines");
    glad_glGenProgramPipelines = (PFNGLGENPROGRAMPIPELINESPROC)load("glGenProgramPipelines");
    glad_glIsProgramPipeline = (PFNGLISPROGRAMPIPELINEPROC)load("glIsProgramPipeline");
    glad_glGetProgramPipelineiv = (PFNGLGETPROGRAMPIPELINEIVPROC)load("glGetProgramPipelineiv");
    glad_glProgramUniform1i = (PFNGLPROGRAMUNIFORM1IPROC)load("glProgramUniform1i");
    glad_glProgramUniform1iv = (PFNGLPROGRAMUNIFORM1IVPROC)load("glProgramUniform1iv");
    glad_glProgramUniform1f = (PFNGLPROGRAMUNIFORM1FPROC)load("glProgramUniform1f");
    glad_glProgramUniform1fv = (PFNGLPROGRAMUNIFORM1FVPROC)load("glProgramUniform1fv");
    glad_glProgramUniformMatrix4fv = (PFNGLPROGRAMUNIFORMMATRIX4FVPROC)load("glProgramUniformMatrix4fv");
    glad_glValidateProgramPipeline = (PFNGLVALIDATEPROGRAMPIPELINEPROC)load("glValidateProgramPipeline");
    glad_glGetProgramPipelineInfoLog = (PFNGLGETPROGRAMPIPELINEINFOLOGPROC)load("glGetProgramPipelineInfoLog");
}

static void load_GL_VERSION_4_2(GLADloadproc load) {
    glad_glDrawArraysInstancedBaseInstance = (PFNGLDRAWARRAYSINSTANCEDBASEINSTANCEPROC)load("glDrawArraysInstancedBaseInstance");
    glad_glDrawElementsInstancedBaseVertexBaseInstance = (PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXBASEINSTANCEPROC)load("glDrawElementsInstancedBaseVertexBaseInstance");
    glad_glBindImageTexture = (PFNGLBINDIMAGETEXTUREPROC)load("glBindImageTexture");
    glad_glMemoryBarrier = (PFNGLMEMORYBARRIERPROC)load("glMemoryBarrier");
    glad_glTexStorage1D = (PFNGLTEXSTORAGE1DPROC)load("glTexStorage1D");
    glad_glTexStorage2D = (PFNGLTEXSTORAGE2DPROC)load("glTexStorage2D");
    glad_glTexStorage3D = (PFNGLTEXSTORAGE3DPROC)load("glTexStorage3D");
}

static void load_GL_VERSION_4_3(GLADloadproc load) {
    glad_glClearBufferData = (PFNGLCLEARBUFFERDATAPROC)load("glClearBufferData");
    glad_glClearBufferSubData = (PFNGLCLEARBUFFERSUBDATAPROC)load("glClearBufferSubData");
    glad_glDispatchCompute = (PFNGLDISPATCHCOMPUTEPROC)load("glDispatchCompute");
    glad_glDispatchComputeIndirect = (PFNGLDISPATCHCOMPUTEINDIRECTPROC)load("glDispatchComputeIndirect");
    glad_glCopyImageSubData = (PFNGLCOPYIMAGESUBDATAPROC)load("glCopyImageSubData");
    glad_glTextureView = (PFNGLTEXTUREVIEWPROC)load("glTextureView");
    glad_glBindVertexBuffer = (PFNGLBINDVERTEXBUFFERPROC)load("glBindVertexBuffer");
    glad_glVertexAttribFormat = (PFNGLVERTEXATTRIBFORMATPROC)load("glVertexAttribFormat");
    glad_glVertexAttribIFormat = (PFNGLVERTEXATTRIBIFORMATPROC)load("glVertexAttribIFormat");
    glad_glVertexAttribLFormat = (PFNGLVERTEXATTRIBLFORMATPROC)load("glVertexAttribLFormat");
    glad_glVertexBindingDivisor = (PFNGLVERTEXBINDINGDIVISORPROC)load("glVertexBindingDivisor");
}

static void load_GL_VERSION_4_4(GLADloadproc load) {
    glad_glBufferStorage = (PFNGLBUFFERSTORAGEPROC)load("glBufferStorage");
    glad_glClearTexImage = (PFNGLCLEARTEXIMAGEPROC)load("glClearTexImage");
    glad_glClearTexSubImage = (PFNGLCLEARTEXSUBIMAGEPROC)load("glClearTexSubImage");
    glad_glBindBuffersBase = (PFNGLBINDBUFFERSBASEPROC)load("glBindBuffersBase");
    glad_glBindBuffersRange = (PFNGLBINDBUFFERSRANGEPROC)load("glBindBuffersRange");
    glad_glBindTextures = (PFNGLBINDTEXTURESPROC)load("glBindTextures");
    glad_glBindSamplers = (PFNGLBINDSAMPLERSPROC)load("glBindSamplers");
    glad_glBindImageTextures = (PFNGLBINDIMAGETEXTURESPROC)load("glBindImageTextures");
}

static void load_GL_VERSION_4_5(GLADloadproc load) {
    glad_glClipControl = (PFNGLCLIPCONTROLPROC)load("glClipControl");
    glad_glCreateTransformFeedbacks = (PFNGLCREATETRANSFORMFEEDBACKSPROC)load("glCreateTransformFeedbacks");
    glad_glTransformFeedbackBufferBase = (PFNGLTRANSFORMFEEDBACKBUFFERBASEPROC)load("glTransformFeedbackBufferBase");
    glad_glTransformFeedbackBufferRange = (PFNGLTRANSFORMFEEDBACKBUFFERRANGEPROC)load("glTransformFeedbackBufferRange");
    glad_glCreateBuffers = (PFNGLCREATEBUFFERSPROC)load("glCreateBuffers");
    glad_glNamedBufferStorage = (PFNGLNAMEDBUFFERSTORAGEPROC)load("glNamedBufferStorage");
    glad_glNamedBufferData = (PFNGLNAMEDBUFFERDATAPROC)load("glNamedBufferData");
    glad_glNamedBufferSubData = (PFNGLNAMEDBUFFERSUBDATAPROC)load("glNamedBufferSubData");
    glad_glCopyNamedBufferSubData = (PFNGLCOPYNAMEDBUFFERSUBDATAPROC)load("glCopyNamedBufferSubData");
    glad_glClearNamedBufferData = (PFNGLCLEARNAMEDBUFFERDATAPROC)load("glClearNamedBufferData");
    glad_glClearNamedBufferSubData = (PFNGLCLEARNAMEDBUFFERSUBDATAPROC)load("glClearNamedBufferSubData");
    glad_glMapNamedBuffer = (PFNGLMAPNAMEDBUFFERPROC)load("glMapNamedBuffer");
    glad_glMapNamedBufferRange = (PFNGLMAPNAMEDBUFFERRANGEPROC)load("glMapNamedBufferRange");
    glad_glUnmapNamedBuffer = (PFNGLUNMAPNAMEDBUFFERPROC)load("glUnmapNamedBuffer");
    glad_glCreateFramebuffers = (PFNGLCREATEFRAMEBUFFERSPROC)load("glCreateFramebuffers");
    glad_glNamedFramebufferRenderbuffer = (PFNGLNAMEDFRAMEBUFFERRENDERBUFFERPROC)load("glNamedFramebufferRenderbuffer");
    glad_glNamedFramebufferTexture = (PFNGLNAMEDFRAMEBUFFERTEXTUREPROC)load("glNamedFramebufferTexture");
    glad_glNamedFramebufferTextureLayer = (PFNGLNAMEDFRAMEBUFFERTEXTURELAYERPROC)load("glNamedFramebufferTextureLayer");
    glad_glCheckNamedFramebufferStatus = (PFNGLCHECKNAMEDFRAMEBUFFERSTATUSPROC)load("glCheckNamedFramebufferStatus");
    glad_glClearNamedFramebufferiv = (PFNGLCLEARNAMEDFRAMEBUFFERIVPROC)load("glClearNamedFramebufferiv");
    glad_glClearNamedFramebufferuiv = (PFNGLCLEARNAMEDFRAMEBUFFERUIVPROC)load("glClearNamedFramebufferuiv");
    glad_glClearNamedFramebufferfv = (PFNGLCLEARNAMEDFRAMEBUFFERFVPROC)load("glClearNamedFramebufferfv");
    glad_glClearNamedFramebufferfi = (PFNGLCLEARNAMEDFRAMEBUFFERFIPROC)load("glClearNamedFramebufferfi");
    glad_glBlitNamedFramebuffer = (PFNGLBLITNAMEDFRAMEBUFFERPROC)load("glBlitNamedFramebuffer");
    glad_glCreateTextures = (PFNGLCREATETEXTURESPROC)load("glCreateTextures");
    glad_glTextureStorage1D = (PFNGLTEXTURESTORAGE1DPROC)load("glTextureStorage1D");
    glad_glTextureStorage2D = (PFNGLTEXTURESTORAGE2DPROC)load("glTextureStorage2D");
    glad_glTextureStorage3D = (PFNGLTEXTURESTORAGE3DPROC)load("glTextureStorage3D");
    glad_glTextureSubImage1D = (PFNGLTEXTURESUBIMAGE1DPROC)load("glTextureSubImage1D");
    glad_glTextureSubImage2D = (PFNGLTEXTURESUBIMAGE2DPROC)load("glTextureSubImage2D");
    glad_glTextureSubImage3D = (PFNGLTEXTURESUBIMAGE3DPROC)load("glTextureSubImage3D");
    glad_glGenerateTextureMipmap = (PFNGLGENERATETEXTUREMIPMAPPROC)load("glGenerateTextureMipmap");
    glad_glBindTextureUnit = (PFNGLBINDTEXTUREUNITPROC)load("glBindTextureUnit");
    glad_glCreateVertexArrays = (PFNGLCREATEVERTEXARRAYSPROC)load("glCreateVertexArrays");
    glad_glDisableVertexArrayAttrib = (PFNGLDISABLEVERTEXARRAYATTRIBPROC)load("glDisableVertexArrayAttrib");
    glad_glEnableVertexArrayAttrib = (PFNGLENABLEVERTEXARRAYATTRIBPROC)load("glEnableVertexArrayAttrib");
    glad_glVertexArrayVertexBuffer = (PFNGLVERTEXARRAYVERTEXBUFFERPROC)load("glVertexArrayVertexBuffer");
    glad_glVertexArrayAttribFormat = (PFNGLVERTEXARRAYATTRIBFORMATPROC)load("glVertexArrayAttribFormat");
    glad_glVertexArrayAttribIFormat = (PFNGLVERTEXARRAYATTRIBIFORMATPROC)load("glVertexArrayAttribIFormat");
    glad_glVertexArrayAttribLFormat = (PFNGLVERTEXARRAYATTRIBLFORMATPROC)load("glVertexArrayAttribLFormat");
    glad_glVertexArrayBindingDivisor = (PFNGLVERTEXARRAYBINDINGDIVISORPROC)load("glVertexArrayBindingDivisor");
    glad_glGetTextureImage = (PFNGLGETTEXTUREIMAGEPROC)load("glGetTextureImage");
}

static void load_GL_VERSION_4_6(GLADloadproc load) {
    glad_glSpecializeShader = (PFNGLSPECIALIZESHADERPROC)load("glSpecializeShader");
    glad_glMultiDrawArraysIndirectCount = (PFNGLMULTIDRAWARRAYSINDIRECTCOUNTPROC)load("glMultiDrawArraysIndirectCount");
    glad_glMultiDrawElementsIndirectCount = (PFNGLMULTIDRAWELEMENTSINDIRECTCOUNTPROC)load("glMultiDrawElementsIndirectCount");
    glad_glPolygonOffsetClamp = (PFNGLPOLYGONOFFSETCLAMPPROC)load("glPolygonOffsetClamp");
}

static int glad_load_gl_version(GLADloadproc load) {
    load_GL_VERSION_1_0(load);
    load_GL_VERSION_1_1(load);
    load_GL_VERSION_1_2(load);
    load_GL_VERSION_1_3(load);
    load_GL_VERSION_1_4(load);
    load_GL_VERSION_1_5(load);
    load_GL_VERSION_2_0(load);
    load_GL_VERSION_2_1(load);
    load_GL_VERSION_3_0(load);
    load_GL_VERSION_3_1(load);
    load_GL_VERSION_3_2(load);
    load_GL_VERSION_3_3(load);
    load_GL_VERSION_4_0(load);
    load_GL_VERSION_4_1(load);
    load_GL_VERSION_4_2(load);
    load_GL_VERSION_4_3(load);
    load_GL_VERSION_4_4(load);
    load_GL_VERSION_4_5(load);
    load_GL_VERSION_4_6(load);
    return 1;
}

int gladLoadGLLoader(GLADloadproc load) {
    glGetString = (PFNGLGETSTRINGPROC)load("glGetString");
    if(glGetString == NULL) return 0;
    if(find_GL_version() < 33) return 0;

    if (glad_load_gl_version(load)) {
        return 1;
    }
    return 0;
}

int gladLoadGL(void) {
    #ifdef _WIN32
        extern void* wglGetProcAddress(const char*);
        return gladLoadGLLoader((GLADloadproc)wglGetProcAddress);
    #else
        extern void* glXGetProcAddress(const char*);
        return gladLoadGLLoader((GLADloadproc)glXGetProcAddress);
    #endif
}

static int glad_load_gl_version(GLADloadproc load) {
    load_GL_VERSION_1_0(load);
    load_GL_VERSION_1_1(load);
    load_GL_VERSION_1_2(load);
    load_GL_VERSION_1_3(load);
    load_GL_VERSION_1_4(load);
    load_GL_VERSION_1_5(load);
    load_GL_VERSION_2_0(load);
    load_GL_VERSION_2_1(load);
    load_GL_VERSION_3_0(load);
    load_GL_VERSION_3_1(load);
    load_GL_VERSION_3_2(load);
    load_GL_VERSION_3_3(load);
    return 1;
}

int gladLoadGLLoader(GLADloadproc load) {
    glGetString = (PFNGLGETSTRINGPROC)load("glGetString");
    if(glGetString == NULL) return 0;
    if(find_GL_version() < 33) return 0; // Guard clause checking for Modern OpenGL 3.3 Core

    if (glad_load_gl_version(load)) {
        return 1;
    }
    return 0;
}

int gladLoadGL(void) {
    /* Safe fallback link step leveraging cross-platform context handlers */
    #ifdef _WIN32
        extern void* wglGetProcAddress(const char*);
        return gladLoadGLLoader((GLADloadproc)wglGetProcAddress);
    #else
        extern void* glXGetProcAddress(const char*);
        return gladLoadGLLoader((GLADloadproc)glXGetProcAddress);
    #endif
}
