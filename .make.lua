-- datapod's build, as recipes. This replaced the Makefile; there is no other.
--
--   make            the recipes, with what each of them says it does
--   make build      the library
--   make test       the suite
--
-- At an oslo prompt in this directory `make` is enough; everywhere else it is `oslo make`.
-- The dev shell's toolchain comes from `.env.lua`'s `nix_develop()`, so recipes call `cargo`
-- directly rather than wrapping every command in `nix develop -c`.

local make = oslo.make

local PYTHON = os.getenv("PYTHON") or "python3"
local CC = os.getenv("CC") or "cc"

local function need(tool, why)
  assert(oslo.run{ "sh", "-c", "command -v " .. tool, capture = true }.ok, why)
end

-- name = ... from Cargo.toml — the one place every tool reads it from.
local function project_name()
  local content = oslo.fs.read("Cargo.toml") or ""
  local name = content:match('\nname%s*=%s*"([^"]+)"') or content:match('^name%s*=%s*"([^"]+)"')
  assert(name, "Cargo.toml package name not found or invalid")
  return name
end

local NAME = project_name()
local TOP_DIR = oslo.sys.pwd()

-- The wheel bind-py or a benchmark/test just built, installed into a scratch
-- directory and cleaned up on the caller's own terms.
local function with_wheel(script)
  local tmp = oslo.run{ "mktemp", "-d", capture = true }
  assert(tmp.ok, "mktemp failed")
  local dir = (tmp.out or ""):gsub("%s+$", "")
  local wheel = oslo.fs.glob("target/wheels/*.whl")[1]
  assert(wheel, "no wheel found in target/wheels; run make bind-py first")
  assert(oslo.run{ PYTHON, "-m", "pip", "install", "--no-deps", "--force-reinstall",
                    "--target", dir, wheel }.ok, "pip install failed")
  local ok = oslo.run{ "sh", "-c", ("PYTHONPATH=%q %s"):format(dir, script) }.ok
  oslo.run{ "rm", "-rf", dir }
  assert(ok, script .. " failed")
end

make.recipe{ name = "build", desc = "the library",
             run = function() sh.cargo("build", "--lib") end }
make.alias("b", "build")

make.recipe{ name = "compile", desc = "clean, then build", deps = { "clean", "build" } }
make.alias("c", "compile")

make.recipe{
  name = "run",
  desc = "run a development example (if examples exist)",
  params = { { "--example", desc = "which example to run", default = "main" } },
  run = function(a) sh.cargo("run", "--example", a.example or "main") end,
}
make.alias("r", "run")

make.recipe{ name = "test", desc = "run all tests",
             run = function() sh.cargo("test", "--all-targets") end }
make.alias("t", "test")

make.recipe{ name = "check", desc = "cargo check on all targets",
             run = function() sh.cargo("check", "--all-targets") end }

make.recipe{ name = "fmt", desc = "format the workspace",
             run = function() sh.cargo("fmt", "--all") end }

make.recipe{ name = "bench", desc = "run Rust/C/Python owned-vs-borrowed wire benchmarks",
             deps = { "bench-rust", "bench-c", "bench-py" } }

make.recipe{ name = "bench-rust", desc = "the release-mode Rust wire benchmark",
             run = function() sh.cargo("run", "--release", "--example", "wire_bench_scaffold") end }

make.recipe{
  name = "bench-c",
  desc = "the release-mode C wire benchmark",
  run = function()
    sh.cargo("build", "--release", "--lib", "--no-default-features")
    local out = "/tmp/" .. NAME .. "_c_wire_bench"
    assert(oslo.run{
      CC, "-I.", "tests/c_wire_bench.c", "-L", "target/release", "-l" .. NAME,
      "-Wl,-rpath," .. TOP_DIR .. "/target/release", "-o", out,
    }.ok, "C wire bench build failed")
    assert(oslo.run{ out }.ok, "C wire bench failed")
  end,
}

make.recipe{
  name = "bench-py",
  desc = "the Python wire benchmark",
  deps = { "bind-py" },
  run = function() with_wheel(PYTHON .. " tests/python_wire_bench.py") end,
}

make.recipe{ name = "clean", desc = "remove Cargo build artifacts",
             run = function() sh.cargo("clean") end }

make.recipe{ name = "bind", desc = "generate both C and Python bindings",
             deps = { "bind-c", "bind-py" } }

make.recipe{
  name = "bind-c",
  desc = "generate the C header",
  run = function()
    sh.cargo("build", "--lib")
    sh.cbindgen("--config", "cbindgen.toml", "--crate", NAME, "--output", "include/" .. NAME .. ".h")
  end,
}

make.recipe{
  name = "bind-py",
  desc = "generate the Python bindings",
  run = function() sh.maturin("build", "--features", "python") end,
}

make.recipe{
  name = "test-py",
  desc = "build/install the Python wheel and run generic wire smoke",
  deps = { "bind-py" },
  run = function()
    sh.cargo("build", "--lib", "--example", "wire_fixture")
    with_wheel(PYTHON .. " tests/python_generic_wire_smoke.py")
  end,
}

make.recipe{
  name = "test-c-abi",
  desc = "build the C ABI library and run C wire runtime smoke",
  run = function()
    sh.cargo("build", "--lib", "--no-default-features")
    local out = "/tmp/" .. NAME .. "_c_wire_runtime_smoke"
    assert(oslo.run{
      CC, "-I.", "tests/c_wire_runtime_smoke.c", "-L", "target/debug", "-l" .. NAME,
      "-Wl,-rpath," .. TOP_DIR .. "/target/debug", "-o", out,
    }.ok, "C ABI build failed")
    assert(oslo.run{ out }.ok, "C wire runtime smoke failed")
  end,
}

make.recipe{ name = "test-bindings", desc = "run bind + Python/C binding smoke tests",
             deps = { "bind", "test-py", "test-c-abi" } }

make.recipe{
  name = "docs",
  desc = "build the mdbook site into docs/, and commit it",
  run = function()
    need("mdbook", "mdbook is not installed; install it first")
    sh.mdbook("build", TOP_DIR .. "/book", "--dest-dir", TOP_DIR .. "/docs")
    sh.git("add", "--all")
    sh.git("commit", "-m", "docs: building website/mdbook")
  end,
}

make.recipe{
  name = "release",
  desc = "cut a version: --type patch | minor | major | M.m.p",
  params = { { "--type", desc = "patch | minor | major | M.m.p" } },
  run = function(a)
    need("git-rel", "git-rel is not installed; install it first")
    assert(type(a.type) == "string",
           "which release? make release --type patch|minor|major|M.m.p")
    sh.git("rel", a.type)
  end,
}
