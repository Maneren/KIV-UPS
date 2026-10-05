# Pinned to the previously used submodule commit (post-v5.5.0 master).
# Tarball URL instead of a git clone: shallow-fetching an arbitrary commit
# SHA fails.
FetchContent_Declare(
    raylib_cpp
    URL
        https://github.com/RobLoach/raylib-cpp/archive/8e01c1dd8fbbe12199fc1256c3bc801d5745ad48.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP ON
)
