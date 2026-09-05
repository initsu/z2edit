load(
    "@bazel_tools//tools/cpp:cc_toolchain_config_lib.bzl",
    "artifact_name_pattern",
    "feature",
    "flag_group",
    "flag_set",
    "tool_path",
)

ACTION_NAMES = struct(
    assemble = "assemble",
    c_compile = "c-compile",
    cpp_compile = "c++-compile",
    cpp_header_parsing = "cpp-header-parsing",
    link = "link",
    preprocess_assemble = "preprocess-assemble",
)

SYSTEMROOT = "external/llvm_mingw"
ABSROOT = "/home/marcus/z2bazel/execroot/__main__/external/llvm_mingw"

STDINCLUDE_DIRS = [
    SYSTEMROOT + "/x86_64-w64-mingw32/include/c++/v1",
    SYSTEMROOT + "/x86_64-w64-mingw32/include",
    SYSTEMROOT + "/generic-w64-mingw32/include",
    SYSTEMROOT + "/lib/clang/23/include",
]

# All execroot-relative so depfiles record sandbox-visible paths (the toolchain
# itself lives outside the workspace via new_local_repository).  libc++ first,
# then the target CRT, the generic CRT, and finally clang's builtin headers.
def _stdinc_flags():
    flags = []
    for d in STDINCLUDE_DIRS:
        flags += ["-isystem", d.replace(SYSTEMROOT, ABSROOT)]
    return flags

STDINC_FLAGS = _stdinc_flags()

def _impl(ctx):
    tool_paths = [
        # gcc/g++ are llvm-mingw's clang driver binaries.  g++ is used for
        # compiling and as the linker so the C++ runtime is pulled in
        # automatically.  The driver locates the sysroot relative to itself.
        ("ar", "bin/x86_64-w64-mingw32-ar"),
        ("as", "bin/x86_64-w64-mingw32-as"),
        ("cpp", "bin/x86_64-w64-mingw32-gcc"),
        ("gcc", "bin/x86_64-w64-mingw32-g++"),
        ("gcov", "bin/x86_64-w64-mingw32-gcc"),
        ("ld", "bin/x86_64-w64-mingw32-g++"),
        ("nm", "bin/x86_64-w64-mingw32-nm"),
        ("objcopy", "bin/x86_64-w64-mingw32-objcopy"),
        ("objdump", "bin/x86_64-w64-mingw32-objdump"),
        ("strip", "bin/x86_64-w64-mingw32-strip"),
    ]
    tool_paths = [tool_path(name = name, path = path) for name, path in tool_paths]

    features = [
        feature(
            name = "std_versions",
            enabled = True,
            flag_sets = [
                flag_set(
                    actions = [
                        ACTION_NAMES.c_compile,
                        ACTION_NAMES.cpp_header_parsing,
                    ],
                    flag_groups = [flag_group(flags = ["-x", "c", "-std=gnu11"])],
                ),
                flag_set(
                    actions = [
                        ACTION_NAMES.cpp_compile,
                        ACTION_NAMES.cpp_header_parsing,
                    ],
                    flag_groups = [flag_group(flags = ["-x", "c++", "-std=gnu++14"])],
                ),
            ],
        ),
        feature(
            name = "opt",
            flag_sets = [
                flag_set(
                    actions = [
                        ACTION_NAMES.c_compile,
                        ACTION_NAMES.cpp_compile,
                    ],
                    flag_groups = [flag_group(flags = ["-O2"])],
                ),
            ],
        ),
        feature(
            name = "dbg",
            flag_sets = [
                flag_set(
                    actions = [
                        ACTION_NAMES.c_compile,
                        ACTION_NAMES.cpp_compile,
                    ],
                    flag_groups = [flag_group(flags = ["-g", "-O0"])],
                ),
            ],
        ),
        feature(name = "fastbuild"),
        # absl (2019-era) on MinGW selects the POSIX-setspecific thread identity
        # path whose pthread_key code needs <pthread.h>, which the _WIN32 guard
        # never includes.  Force the C++11 mode like MSVC does.
        feature(
            name = "abs_force_cpp11_tls",
            enabled = True,
            flag_sets = [
                flag_set(
                    actions = [
                        ACTION_NAMES.c_compile,
                        ACTION_NAMES.cpp_compile,
                        ACTION_NAMES.cpp_header_parsing,
                    ],
                    flag_groups = [flag_group(flags = ["-DABSL_FORCE_THREAD_IDENTITY_MODE=2"])],
                ),
            ],
        ),
        # Point clang at the sysroot via a path relative to the execroot so
        # depfiles contain sandbox-visible paths instead of the absolute
        # location of the toolchain (new_local_repository under /opt/...).
        feature(
            name = "sysroot",
            enabled = True,
            flag_sets = [
                flag_set(
                    actions = [
                        ACTION_NAMES.assemble,
                        ACTION_NAMES.preprocess_assemble,
                        ACTION_NAMES.c_compile,
                        ACTION_NAMES.cpp_compile,
                        ACTION_NAMES.cpp_header_parsing,
                    ],
                    flag_groups = [flag_group(
                        flags = ["--sysroot", ABSROOT] + STDINC_FLAGS,
                    )],
                ),
                flag_set(
                    actions = [ACTION_NAMES.link],
                    flag_groups = [flag_group(flags = ["--sysroot", ABSROOT])],
                ),
            ],
        ),
    ]

    return cc_common.create_cc_toolchain_config_info(
        ctx = ctx,
        toolchain_identifier = "llvm-mingw-x86_64",
        host_system_name = "x86_64-unknown-linux-gnu",
        target_system_name = "x86_64-w64-mingw32",
        target_cpu = "x86_64",
        target_libc = "mingw",
        compiler = "clang",
        abi_version = "unknown",
        abi_libc_version = "unknown",
        tool_paths = tool_paths,
        cxx_builtin_include_directories = [
            ABSROOT + "/generic-w64-mingw32/include",
            ABSROOT + "/x86_64-w64-mingw32/include",
            ABSROOT + "/x86_64-w64-mingw32/include/c++/v1",
            ABSROOT + "/lib/clang/23/include",
        ],
        artifact_name_patterns = [
            # The clang driver appends .exe itself, so tell bazel our
            # executables are named <target>.exe.
            artifact_name_pattern(
                category_name = "executable",
                prefix = "",
                extension = ".exe",
            ),
        ],
        features = features,
    )

llvm_mingw_cc_toolchain_config = rule(
    implementation = _impl,
    attrs = {},
    provides = [CcToolchainConfigInfo],
)