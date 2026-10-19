include_guard(GLOBAL)

include(CheckCXXCompilerFlag)
include(CheckCXXSourceCompiles)
include(CheckLinkerFlag)

# All flags are probed with CMake's compile/link check to ensure compatibility
# with wide range of compilers.

# Filter out unsupported compiler flags
function(cxx_supported_flags OUT)
    set(supported "")
    foreach(flag IN LISTS ARGN)
        string(MAKE_C_IDENTIFIER "CXX_COMPILER_SUPPORTS${flag}" cache_var)
        if(NOT DEFINED ${cache_var})
            check_cxx_compiler_flag("${flag}" ${cache_var})
        endif()
        if(${cache_var})
            list(APPEND supported "${flag}")
        endif()
    endforeach()
    set(${OUT} "${supported}" PARENT_SCOPE)
endfunction()

# Filter out unsupported linker flags
function(cxx_supported_link_flags OUT)
    set(supported "")
    foreach(flag IN LISTS ARGN)
        string(MAKE_C_IDENTIFIER "CXX_LINKER_SUPPORTS${flag}" cache_var)
        if(NOT DEFINED ${cache_var})
            check_linker_flag(CXX "${flag}" ${cache_var})
        endif()
        if(${cache_var})
            list(APPEND supported "${flag}")
        endif()
    endforeach()
    set(${OUT} "${supported}" PARENT_SCOPE)
endfunction()

# Filter out unsupported compiler/linker flags
function(cxx_supported_combined_flags OUT)
    set(supported "")
    foreach(flag IN LISTS ARGN)
        string(
            MAKE_C_IDENTIFIER "CXX_COMPILER_AND_LINKER_SUPPORTS${flag}"
            cache_var
        )
        if(NOT DEFINED ${cache_var})
            set(CMAKE_REQUIRED_FLAGS "${flag}")
            set(CMAKE_REQUIRED_LINK_OPTIONS "${flag}")
            check_cxx_source_compiles("int main() { return 0; }" ${cache_var})
            unset(CMAKE_REQUIRED_FLAGS)
            unset(CMAKE_REQUIRED_LINK_OPTIONS)
        endif()
        if(${cache_var})
            list(APPEND supported "${flag}")
        endif()
    endforeach()
    set(${OUT} "${supported}" PARENT_SCOPE)
endfunction()

set_property(GLOBAL PROPERTY THIN_ARCHIVE ${THIN_ARCHIVE})

# Flag sets, used by set_compiler_and_linker_flags
if(MSVC)
    # `cl.exe` silently ignores unknown options, so no probing is needed
    set(CXX_WARNING_FLAGS /W4)
    set(CXX_WERROR_FLAGS /WX)
    set(CXX_HARDENING_FLAGS)
    set(CXX_DEBUG_INFO_FLAGS /Zi)
    set(CXX_DEBUG_OPT_FLAGS /Od)
    set(CXX_SECTION_FLAGS)
    set(CXX_LINK_HARDENING_FLAGS)
    set(CXX_LINK_GC_FLAGS)
    set(CXX_LINK_ICF_FLAGS)
else()
    # Enable as many warnings as possible without being too noisy
    cxx_supported_flags(
        CXX_WARNING_FLAGS
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Wcast-align
        -Wcast-qual
        -Wunused
        -Woverloaded-virtual
        -Wdouble-promotion
        -Wimplicit-fallthrough
        -Wundef
        -Wnull-dereference 
        -Wduplicated-cond 
        -Wlogical-op
        -Wuseless-cast
    )
    # Error on warnings
    cxx_supported_flags(CXX_WERROR_FLAGS -Werror)

    # Generate more debug information
    cxx_supported_flags(
        CXX_DEBUG_INFO_FLAGS
        -g3
        -gdwarf-5
        -fno-omit-frame-pointer
    )
    # Optimize at a minimal level required for tooling to work
    cxx_supported_flags(CXX_DEBUG_OPT_FLAGS -Og)
    # Reduce binary size
    cxx_supported_flags(CXX_SECTION_FLAGS -ffunction-sections -fdata-sections)
    cxx_supported_link_flags(CXX_LINK_GC_FLAGS -Wl,--gc-sections)

    # General hardening
    cxx_supported_flags(
        CXX_HARDENING_FLAGS
        -fstack-protector-strong
        -fcf-protection=full # x86 only, dropped elsewhere
        -ffile-prefix-map=${CMAKE_SOURCE_DIR}=. # reproducible paths
    )
    # ELF specific hardening
    cxx_supported_link_flags(CXX_LINK_HARDENING_FLAGS -Wl,-z,relro -Wl,-z,now)
    # Inter-module optimization
    cxx_supported_link_flags(CXX_LINK_ICF_FLAGS -Wl,--icf=all)
endif()

# _FORTIFY_SOURCE is not supported by MSVC.
if(NOT MSVC)
    cxx_supported_flags(FORTIFY_FLAGS -D_FORTIFY_SOURCE=3)
endif()

# Sanitizers
if(SANITIZERS)
    if(MSVC)
        add_compile_options(/fsanitize=address)
        add_link_options(/fsanitize=address)
    else()
        cxx_supported_combined_flags(
            SANITIZER_FLAGS
            -fsanitize=address
            -fsanitize=undefined
            -fsanitize=leak
            -fno-sanitize-recover=all # make UB abort instead of continuing
            -fno-omit-frame-pointer
        )
        if(SANITIZER_FLAGS)
            add_compile_options(${SANITIZER_FLAGS})
            add_link_options(${SANITIZER_FLAGS})
        else()
            message(
                WARNING
                "SANITIZERS requested but not supported by"
                "${CMAKE_CXX_COMPILER}"
            )
        endif()
    endif()
endif()

# Interprocedural optimisation (LTO)
if(USE_IPO)
    include(CheckIPOSupported)

    check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error LANGUAGES CXX)
    if(ipo_supported)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON)
    else()
        message(WARNING "IPO is not supported, disabling: ${ipo_error}")
    endif()
endif()

# Compiler launcher (ccache/sccache)
if(USE_CCACHE)
    find_program(CCACHE ccache)
    find_program(SCCACHE sccache)
    if(CCACHE)
        message(STATUS "Found ccache: ${CCACHE}")
        set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE})
    elseif(SCCACHE)
        message(STATUS "Found sccache: ${SCCACHE}")
        set(CMAKE_CXX_COMPILER_LAUNCHER ${SCCACHE})
    else()
        message(WARNING "ccache not found")
    endif()
endif()
