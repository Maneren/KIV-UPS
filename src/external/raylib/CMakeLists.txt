# Pinned to the previously used submodule commit (post-5.5 master).
# Tarball URL instead of a git clone: shallow-fetching an arbitrary commit
# SHA fails, and a full raylib clone is hundreds of MB.
FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/dedf382f64bf7b8da6872063bda21e3a2dfffd19.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP ON
)
