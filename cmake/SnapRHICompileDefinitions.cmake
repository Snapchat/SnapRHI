# ============================================================================
# SnapRHI Compile Definitions
# ============================================================================
# Configures compile definitions for the public-api target based on options.
# ============================================================================

include_guard(GLOBAL)

# Helper: Convert boolean option to 0/1 for compile definitions
function(snap_rhi_bool_to_int OPTION_NAME OUT_VAR)
    if(${OPTION_NAME})
        set(${OUT_VAR} 1 PARENT_SCOPE)
    else()
        set(${OUT_VAR} 0 PARENT_SCOPE)
    endif()
endfunction()

# Helper: Convert report level string to integer
function(snap_rhi_report_level_to_int LEVEL OUT_VAR)
    if(LEVEL STREQUAL "all")
        set(${OUT_VAR} 0 PARENT_SCOPE)
    elseif(LEVEL STREQUAL "debug")
        set(${OUT_VAR} 1 PARENT_SCOPE)
    elseif(LEVEL STREQUAL "info")
        set(${OUT_VAR} 2 PARENT_SCOPE)
    elseif(LEVEL STREQUAL "warning")
        set(${OUT_VAR} 3 PARENT_SCOPE)
    elseif(LEVEL STREQUAL "performance_warning")
        set(${OUT_VAR} 4 PARENT_SCOPE)
    elseif(LEVEL STREQUAL "error")
        set(${OUT_VAR} 5 PARENT_SCOPE)
    elseif(LEVEL STREQUAL "critical_error")
        set(${OUT_VAR} 6 PARENT_SCOPE)
    else()
        message(FATAL_ERROR "Unknown report level: ${LEVEL}")
    endif()
endfunction()

# Adds all the compile definitions for the public-api target based on options.
function(snap_rhi_configure_public_api_definitions TARGET_NAME)
    # Convert boolean options to 0/1
    snap_rhi_bool_to_int(SNAP_RHI_ENABLE_METAL _METAL)
    snap_rhi_bool_to_int(SNAP_RHI_ENABLE_VULKAN _VULKAN)
    snap_rhi_bool_to_int(SNAP_RHI_ENABLE_OPENGL _OPENGL)
    snap_rhi_bool_to_int(SNAP_RHI_ENABLE_NOOP _NOOP)
    snap_rhi_bool_to_int(SNAP_RHI_ENABLE_SLOW_SAFETY_CHECKS _SLOW_SAFETY)
    snap_rhi_bool_to_int(SNAP_RHI_ENABLE_SOURCE_LOCATION _SOURCE_LOC)
    snap_rhi_bool_to_int(SNAP_RHI_ENABLE_CUSTOM_PROFILING_LABELS _PROFILING)
    snap_rhi_bool_to_int(SNAP_RHI_ENABLE_DEBUG_LABELS _DEBUG_LABELS)
    snap_rhi_bool_to_int(SNAP_RHI_ENABLE_LOGS _LOGS)
    snap_rhi_bool_to_int(SNAP_RHI_ENABLE_API_DUMP _API_DUMP)
    snap_rhi_bool_to_int(SNAP_RHI_THROW_ON_LIFETIME_VIOLATION _THROW_LIFETIME)

    snap_rhi_report_level_to_int(${SNAP_RHI_REPORT_LEVEL} _REPORT_LEVEL)

    # Backend and main definitions
    target_compile_definitions(${TARGET_NAME} PUBLIC
        SNAP_RHI_ENABLE_BACKEND_VULKAN=${_VULKAN}
        SNAP_RHI_ENABLE_BACKEND_METAL=${_METAL}
        SNAP_RHI_ENABLE_BACKEND_OPENGL=${_OPENGL}
        SNAP_RHI_ENABLE_BACKEND_NOOP=${_NOOP}
        SNAP_RHI_ENABLE_SLOW_SAFETY_CHECKS=${_SLOW_SAFETY}
        SNAP_RHI_ENABLE_SOURCE_LOCATION=${_SOURCE_LOC}
        SNAP_RHI_ENABLE_CUSTOM_PROFILING_LABELS=${_PROFILING}
        SNAP_RHI_ENABLE_DEBUG_LABELS=${_DEBUG_LABELS}
        SNAP_RHI_ENABLE_LOGS=${_LOGS}
        SNAP_RHI_ENABLED_REPORT_LEVEL=${_REPORT_LEVEL}
        SNAP_RHI_ENABLE_API_DUMP=${_API_DUMP}
        SNAP_RHI_THROW_ON_LIFETIME_VIOLATION=${_THROW_LIFETIME}
    )

    # Validation tag definitions
    snap_rhi_bool_to_int(SNAP_RHI_VALIDATION_CREATE_OP _V_CREATE)
    snap_rhi_bool_to_int(SNAP_RHI_VALIDATION_DESTROY_OP _V_DESTROY)
    snap_rhi_bool_to_int(SNAP_RHI_VALIDATION_DEVICE_CONTEXT_OP _V_DEV_CTX)
    snap_rhi_bool_to_int(SNAP_RHI_VALIDATION_COMMAND_QUEUE_OP _V_CMD_QUEUE)
    snap_rhi_bool_to_int(SNAP_RHI_VALIDATION_COMMAND_BUFFER_OP _V_CMD_BUF)
    snap_rhi_bool_to_int(SNAP_RHI_VALIDATION_RENDER_COMMAND_ENCODER_OP _V_RENDER_ENC)
    snap_rhi_bool_to_int(SNAP_RHI_VALIDATION_COMPUTE_COMMAND_ENCODER_OP _V_COMPUTE_ENC)
    snap_rhi_bool_to_int(SNAP_RHI_VALIDATION_BLIT_COMMAND_ENCODER_OP _V_BLIT_ENC)
    snap_rhi_bool_to_int(SNAP_RHI_VALIDATION_RENDER_PASS_OP _V_RENDER_PASS)
    snap_rhi_bool_to_int(SNAP_RHI_VALIDATION_FRAMEBUFFER_OP _V_FRAMEBUF)
    snap_rhi_bool_to_int(SNAP_RHI_VALIDATION_RENDER_PIPELINE_OP _V_RENDER_PIPE)
    snap_rhi_bool_to_int(SNAP_RHI_VALIDATION_COMPUTE_PIPELINE_OP _V_COMPUTE_PIPE)
    snap_rhi_bool_to_int(SNAP_RHI_VALIDATION_SHADER_MODULE_OP _V_SHADER_MOD)
    snap_rhi_bool_to_int(SNAP_RHI_VALIDATION_SHADER_LIBRARY_OP _V_SHADER_LIB)
    snap_rhi_bool_to_int(SNAP_RHI_VALIDATION_SAMPLER_OP _V_SAMPLER)
    snap_rhi_bool_to_int(SNAP_RHI_VALIDATION_TEXTURE_OP _V_TEXTURE)
    snap_rhi_bool_to_int(SNAP_RHI_VALIDATION_BUFFER_OP _V_BUFFER)
    snap_rhi_bool_to_int(SNAP_RHI_VALIDATION_DESCRIPTOR_SET_LAYOUT_OP _V_DESC_LAYOUT)
    snap_rhi_bool_to_int(SNAP_RHI_VALIDATION_PIPELINE_LAYOUT_OP _V_PIPE_LAYOUT)
    snap_rhi_bool_to_int(SNAP_RHI_VALIDATION_DESCRIPTOR_POOL_OP _V_DESC_POOL)
    snap_rhi_bool_to_int(SNAP_RHI_VALIDATION_DESCRIPTOR_SET_OP _V_DESC_SET)
    snap_rhi_bool_to_int(SNAP_RHI_VALIDATION_FENCE_OP _V_FENCE)
    snap_rhi_bool_to_int(SNAP_RHI_VALIDATION_SEMAPHORE_OP _V_SEMAPHORE)
    snap_rhi_bool_to_int(SNAP_RHI_VALIDATION_DEVICE_OP _V_DEVICE)
    snap_rhi_bool_to_int(SNAP_RHI_VALIDATION_PIPELINE_CACHE_OP _V_PIPE_CACHE)
    snap_rhi_bool_to_int(SNAP_RHI_VALIDATION_QUERY_POOL_OP _V_QUERY_POOL)
    snap_rhi_bool_to_int(SNAP_RHI_VALIDATION_GL_COMMAND_QUEUE_EXTERNAL_ERROR _V_GL_EXT_ERR)
    snap_rhi_bool_to_int(SNAP_RHI_VALIDATION_GL_COMMAND_QUEUE_INTERNAL_ERROR _V_GL_INT_ERR)
    snap_rhi_bool_to_int(SNAP_RHI_VALIDATION_GL_STATE_CACHE_OP _V_GL_STATE)
    snap_rhi_bool_to_int(SNAP_RHI_VALIDATION_GL_PROGRAM_VALIDATION_OP _V_GL_PROG)
    snap_rhi_bool_to_int(SNAP_RHI_VALIDATION_GL_PROFILE_OP _V_GL_PROF)

    target_compile_definitions(${TARGET_NAME} PUBLIC
        SNAP_RHI_ENABLE_VALIDATION_TAG_CreateOp=${_V_CREATE}
        SNAP_RHI_ENABLE_VALIDATION_TAG_DestroyOp=${_V_DESTROY}
        SNAP_RHI_ENABLE_VALIDATION_TAG_DeviceContextOp=${_V_DEV_CTX}
        SNAP_RHI_ENABLE_VALIDATION_TAG_CommandQueueOp=${_V_CMD_QUEUE}
        SNAP_RHI_ENABLE_VALIDATION_TAG_CommandBufferOp=${_V_CMD_BUF}
        SNAP_RHI_ENABLE_VALIDATION_TAG_RenderCommandEncoderOp=${_V_RENDER_ENC}
        SNAP_RHI_ENABLE_VALIDATION_TAG_ComputeCommandEncoderOp=${_V_COMPUTE_ENC}
        SNAP_RHI_ENABLE_VALIDATION_TAG_BlitCommandEncoderOp=${_V_BLIT_ENC}
        SNAP_RHI_ENABLE_VALIDATION_TAG_RenderPassOp=${_V_RENDER_PASS}
        SNAP_RHI_ENABLE_VALIDATION_TAG_FramebufferOp=${_V_FRAMEBUF}
        SNAP_RHI_ENABLE_VALIDATION_TAG_RenderPipelineOp=${_V_RENDER_PIPE}
        SNAP_RHI_ENABLE_VALIDATION_TAG_ComputePipelineOp=${_V_COMPUTE_PIPE}
        SNAP_RHI_ENABLE_VALIDATION_TAG_ShaderModuleOp=${_V_SHADER_MOD}
        SNAP_RHI_ENABLE_VALIDATION_TAG_ShaderLibraryOp=${_V_SHADER_LIB}
        SNAP_RHI_ENABLE_VALIDATION_TAG_SamplerOp=${_V_SAMPLER}
        SNAP_RHI_ENABLE_VALIDATION_TAG_TextureOp=${_V_TEXTURE}
        SNAP_RHI_ENABLE_VALIDATION_TAG_BufferOp=${_V_BUFFER}
        SNAP_RHI_ENABLE_VALIDATION_TAG_DescriptorSetLayoutOp=${_V_DESC_LAYOUT}
        SNAP_RHI_ENABLE_VALIDATION_TAG_PipelineLayoutOp=${_V_PIPE_LAYOUT}
        SNAP_RHI_ENABLE_VALIDATION_TAG_DescriptorPoolOp=${_V_DESC_POOL}
        SNAP_RHI_ENABLE_VALIDATION_TAG_DescriptorSetOp=${_V_DESC_SET}
        SNAP_RHI_ENABLE_VALIDATION_TAG_FenceOp=${_V_FENCE}
        SNAP_RHI_ENABLE_VALIDATION_TAG_SemaphoreOp=${_V_SEMAPHORE}
        SNAP_RHI_ENABLE_VALIDATION_TAG_DeviceOp=${_V_DEVICE}
        SNAP_RHI_ENABLE_VALIDATION_TAG_PipelineCacheOp=${_V_PIPE_CACHE}
        SNAP_RHI_ENABLE_VALIDATION_TAG_QueryPoolOp=${_V_QUERY_POOL}
        SNAP_RHI_ENABLE_VALIDATION_TAG_GLCommandQueueExternalError=${_V_GL_EXT_ERR}
        SNAP_RHI_ENABLE_VALIDATION_TAG_GLCommandQueueInternalError=${_V_GL_INT_ERR}
        SNAP_RHI_ENABLE_VALIDATION_TAG_GLStateCacheOp=${_V_GL_STATE}
        SNAP_RHI_ENABLE_VALIDATION_TAG_GLProgramValidationOp=${_V_GL_PROG}
        SNAP_RHI_ENABLE_VALIDATION_TAG_GLProfileOp=${_V_GL_PROF}
    )
endfunction()
