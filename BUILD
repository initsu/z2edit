package(default_visibility = ["//visibility:public"])

config_setting(
    name = "windows",
    constraint_values = [
        "@platforms//os:windows",
    ],
)

genrule(
    name = "make_version",
    outs = ["version.h"],
    cmd = """awk '{ printf "#define %s ", $$1; printf "\\""; s = ""; for (i = 2; i <= NF; i++) { printf "%s%s", s, $$i; s = " " } print "\\"" }' bazel-out/volatile-status.txt > $@""",
    stamp = 1,
)

cc_library(
    name = "app",
    srcs = [
        "app.cc",
    ],
    hdrs = [
        "app.h",
        "version.h",
    ],
    deps = [
        "//imwidget:base",
        "//imwidget:drops",
        "//imwidget:editor",
        "//imwidget:enemyattr",
        "//imwidget:hwpalette",
        "//imwidget:misc_hacks",
        "//imwidget:map_connect",
        "//imwidget:neschrview",
        "//imwidget:palace_gfx",
        "//imwidget:palette",
        "//imwidget:project",
        "//imwidget:rom_memory",
        "//imwidget:simplemap",
        "//imwidget:start_values",
        "//imwidget:text_table",
        "//imwidget:tile_transform",
        "//imwidget:item_effects",
        "//imwidget:object_table",
        "//imwidget:xptable",
        "//nes:cartridge",
        "//nes:chr_util",
        "//nes:cpu6502",
        "//nes:mappers",
        "//nes:text_encoding",
        "//proto:rominfo",
        "//util:browser",
        "//util:fpsmgr",
        "//util:imgui_sdl_opengl",
        "//util:os",
        "//util:logging",
        "@gflags//:gflags",
        "@sdl2_ext//:sdl2",
        "@sdl2_ext//:sdl2main",

        # TODO(cfrantz): on ubuntu 16 with MIR, there is a library conflict
        # between MIR (linked with protobuf 2.6.1) and this program,
        # which builds with protbuf 3.x.x.  A temporary workaround is to
        # not link with nfd (native-file-dialog).
        "@nativefiledialog_git//:nfd",
    ],
)

filegroup(
    name = "content",
    srcs = glob(["content/*.textpb"]),
)

genrule(
    name = "make_zelda2_config",
    srcs = [
        "zelda2.textpb",
        ":content",
    ],
    outs = ["zelda2_config.h"],
    cmd = "$(location //tools:pack_config) --config $(location zelda2.textpb)" +
          " --symbol kZelda2Cfg > $(@)",
    tools = ["//tools:pack_config"],
)

cc_binary(
    name = "z2edit",
    srcs = [
        "main.cc",
        "zelda2_config.h",
    ],
    linkopts = select({
        ":windows": [
            "-lpthread",
            "-lm",
            "-lopengl32",
            "-ldinput8",
            "-ldxguid",
            "-ldxerr8",
            "-luser32",
            "-lgdi32",
            "-lwinmm",
            "-limm32",
            "-lole32",
            "-loleaut32",
            "-lshell32",
            "-lversion",
            "-luuid",
            "-lsetupapi",
            "-ladvapi32",
            "-lhid",
        ],
        "//conditions:default": [
            "-lpthread",
            "-lm",
            "-lGL",
        ],
    }),
    deps = [
        ":app",
        "@gflags//:gflags",
        "//util:config",
    ],
)

genrule(
    name = "z2edit-windows",
    srcs = [
        ":z2edit",
        "@llvm_mingw//:x86_64-w64-mingw32/bin/libc++.dll",
        "@llvm_mingw//:x86_64-w64-mingw32/bin/libunwind.dll",
    ],
    outs = ["z2edit-windows.zip"],
    cmd = "cp $(location :z2edit) z2edit.exe && cp $(location @llvm_mingw//:x86_64-w64-mingw32/bin/libc++.dll) . && cp $(location @llvm_mingw//:x86_64-w64-mingw32/bin/libunwind.dll) . && zip $@ z2edit.exe libc++.dll libunwind.dll && rm -f z2edit.exe libc++.dll libunwind.dll",
    tags = ["manual"],
)
