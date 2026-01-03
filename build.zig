const std = @import("std");

// ==================================================================================================
// Project configuration
// ==================================================================================================
const PROJECT_NAME = "datapod";

// Dependencies are defined in build.zig.zon:
//   .dependencies = .{
//       .dep_name = .{ .url = "...", .hash = "...", .lazy = true/false },
//   }
// Library deps: always fetched
// Test deps: use .lazy = true (only fetched when -Dtests=true)

const LIB_DEPS: []const []const u8 = &.{
    // "dep_name",
};
const EXAMPLE_DEPS: []const []const u8 = &.{
    // additional deps for examples (lib deps are auto-included)
};
const TEST_DEPS: []const []const u8 = &.{
    "doctest",
};

// Common C++ flags
const CPP_FLAGS: []const []const u8 = &.{
    "-std=c++20",
    "-Wall",
    "-Wextra",
    "-Wpedantic",
    "-Wno-reorder",
    "-Wno-narrowing",
    "-Wno-array-bounds",
    "-Wno-unused-variable",
    "-Wno-unused-parameter",
    "-Wno-unused-but-set-variable",
    "-Wno-gnu-line-marker",
};

// ==================================================================================================
// Build
// ==================================================================================================
pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    // Build options
    const short_namespace = b.option(bool, "short-namespace", "Enable SHORT_NAMESPACE compile definition") orelse true;
    const build_examples = b.option(bool, "examples", "Build example programs") orelse false;
    const build_tests = b.option(bool, "tests", "Build test executables") orelse false;

    // Main library
    const lib_mod = createCppModule(b, target, optimize, short_namespace);
    addIncludePaths(lib_mod, b, LIB_DEPS);
    addSourcesFromDir(lib_mod, b, "src/" ++ PROJECT_NAME);

    const lib = b.addLibrary(.{
        .name = PROJECT_NAME,
        .root_module = lib_mod,
    });
    b.installArtifact(lib);
    lib.installHeadersDirectory(b.path("include"), "", .{});

    // Examples
    if (build_examples) {
        const all_example_deps = LIB_DEPS ++ EXAMPLE_DEPS;
        buildBinaries(b, target, optimize, short_namespace, "examples", all_example_deps, .{});
    }

    // Tests
    if (build_tests) {
        const all_test_deps = LIB_DEPS ++ EXAMPLE_DEPS ++ TEST_DEPS;
        buildBinaries(b, target, optimize, short_namespace, "test", all_test_deps, .{
            .recursive = true,
            .defines = &.{"DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN"},
            .is_test = true,
        });
    }
}

// ==================================================================================================
// Helper functions
// ==================================================================================================
const BinaryOptions = struct {
    recursive: bool = false,
    defines: []const []const u8 = &.{},
    is_test: bool = false,
};

fn createCppModule(
    b: *std.Build,
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    short_namespace: bool,
) *std.Build.Module {
    const mod = b.createModule(.{
        .target = target,
        .optimize = optimize,
        .link_libc = true,
        .link_libcpp = true,
    });

    mod.addIncludePath(b.path("include"));

    if (short_namespace) {
        mod.addCMacro("SHORT_NAMESPACE", "");
    }

    return mod;
}

fn addIncludePaths(mod: *std.Build.Module, b: *std.Build, deps: []const []const u8) void {
    for (deps) |dep_name| {
        if (b.lazyDependency(dep_name, .{})) |dep| {
            // Add package root (for headers like <dep_name/header.h>)
            mod.addIncludePath(dep.path(""));
            // Also try common include subdirectory
            mod.addIncludePath(dep.path("include"));
        }
    }
}

fn addSourcesFromDir(mod: *std.Build.Module, b: *std.Build, dir_path: []const u8) void {
    var dir = std.fs.cwd().openDir(dir_path, .{ .iterate = true }) catch return;
    defer dir.close();

    var it = dir.iterate();
    while (it.next() catch null) |entry| {
        if (entry.kind == .file and std.mem.endsWith(u8, entry.name, ".cpp")) {
            const path = std.fmt.allocPrint(b.allocator, "{s}/{s}", .{ dir_path, entry.name }) catch continue;
            mod.addCSourceFile(.{
                .file = b.path(path),
                .flags = CPP_FLAGS,
            });
        }
    }
}

fn buildBinaries(
    b: *std.Build,
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    short_namespace: bool,
    dir_path: []const u8,
    deps: []const []const u8,
    opts: BinaryOptions,
) void {
    var files: std.ArrayListUnmanaged([]const u8) = .empty;
    defer files.deinit(b.allocator);

    if (opts.recursive) {
        collectFilesRecursive(b, dir_path, &files);
    } else {
        collectFiles(b, dir_path, &files);
    }

    const test_step = if (opts.is_test) b.step("test", "Run all tests") else null;

    for (files.items) |file_path| {
        const basename = std.fs.path.basename(file_path);
        const name = basename[0 .. basename.len - 4]; // Remove .cpp
        const name_z = b.allocator.dupeZ(u8, name) catch continue;

        const mod = createCppModule(b, target, optimize, short_namespace);
        addIncludePaths(mod, b, deps);

        // Add extra defines
        for (opts.defines) |define| {
            mod.addCMacro(define, "");
        }

        mod.addCSourceFile(.{
            .file = b.path(file_path),
            .flags = CPP_FLAGS,
        });

        const exe = b.addExecutable(.{
            .name = name_z,
            .root_module = mod,
        });

        b.installArtifact(exe);

        // Add to test step if this is a test
        if (test_step) |step| {
            const run_cmd = b.addRunArtifact(exe);
            step.dependOn(&run_cmd.step);
        }
    }
}

fn collectFiles(b: *std.Build, dir_path: []const u8, files: *std.ArrayListUnmanaged([]const u8)) void {
    var dir = std.fs.cwd().openDir(dir_path, .{ .iterate = true }) catch return;
    defer dir.close();

    var it = dir.iterate();
    while (it.next() catch null) |entry| {
        if (entry.kind == .file and std.mem.endsWith(u8, entry.name, ".cpp")) {
            const path = std.fmt.allocPrint(b.allocator, "{s}/{s}", .{ dir_path, entry.name }) catch continue;
            files.append(b.allocator, path) catch continue;
        }
    }
}

fn collectFilesRecursive(b: *std.Build, dir_path: []const u8, files: *std.ArrayListUnmanaged([]const u8)) void {
    var dir = std.fs.cwd().openDir(dir_path, .{ .iterate = true }) catch return;
    defer dir.close();

    var it = dir.iterate();
    while (it.next() catch null) |entry| {
        const full_path = std.fmt.allocPrint(b.allocator, "{s}/{s}", .{ dir_path, entry.name }) catch continue;

        if (entry.kind == .directory) {
            collectFilesRecursive(b, full_path, files);
        } else if (entry.kind == .file and std.mem.endsWith(u8, entry.name, ".cpp")) {
            files.append(b.allocator, full_path) catch continue;
        }
    }
}
