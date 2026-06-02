SHELL := /bin/bash

PROJECT_NAME := $(shell if [ -f PROJECT ]; then sed -n '/^[[:space:]]*[^#\[[:space:]]/p' PROJECT | head -1 | tr -d '[:space:]'; else sed -n 's/^[[:space:]]*name[[:space:]]*=[[:space:]]*"\([^"]*\)".*/\1/p' Cargo.toml | head -1; fi)
PROJECT_VERSION := $(shell if [ -f PROJECT ]; then sed -n '/^[[:space:]]*[^#\[[:space:]]/p' PROJECT | sed -n '2p' | tr -d '[:space:]'; else sed -n 's/^[[:space:]]*version[[:space:]]*=[[:space:]]*"\([^"]*\)".*/\1/p' Cargo.toml | head -1; fi)
ifeq ($(PROJECT_NAME),)
    $(error Error: PROJECT file not found or invalid)
endif

TOP_DIR := $(CURDIR)
CARGO := cargo
PYTHON ?= python3
CC ?= cc
EXAMPLE ?= main

HAS_REL := $(shell command -v git-rel 2>/dev/null)

$(info ------------------------------------------)
$(info Project: $(PROJECT_NAME) v$(PROJECT_VERSION))
$(info ------------------------------------------)

.PHONY: build b compile c run r test t check fmt bench clean bind bind-c bind-py test-py test-c-abi test-bindings help h

build:
	@$(CARGO) build --lib

b: build

compile:
	@$(CARGO) clean
	@$(MAKE) build

c: compile

run:
	@$(CARGO) run --example $(EXAMPLE)

r: run

test:
	@$(CARGO) test --all-targets

t: test

check:
	@$(CARGO) check --all-targets

fmt:
	@$(CARGO) fmt --all

clean:
	@$(CARGO) clean

bind: bind-c bind-py

bind-c:
	@$(CARGO) build --lib
	@cbindgen --config cbindgen.toml --crate $(PROJECT_NAME) \
		--output include/$(PROJECT_NAME).h

bind-py:
	@maturin build --features python

test-py: bind-py
	@$(CARGO) build --lib --example wire_fixture
	@tmp=$$(mktemp -d); \
	trap 'rm -rf "$$tmp"' EXIT; \
	$(PYTHON) -m pip install --no-deps --force-reinstall --target "$$tmp" target/wheels/$(PROJECT_NAME)-$(PROJECT_VERSION)-*.whl >/dev/null; \
	PYTHONPATH="$$tmp" $(PYTHON) tests/python_generic_wire_smoke.py

test-c-abi:
	@$(CARGO) build --lib --no-default-features
	@$(CC) -I. tests/c_wire_runtime_smoke.c -L target/debug -ldatapod \
		-Wl,-rpath,$(TOP_DIR)/target/debug \
		-o /tmp/$(PROJECT_NAME)_c_wire_runtime_smoke
	@/tmp/$(PROJECT_NAME)_c_wire_runtime_smoke

test-bindings: bind test-py test-c-abi

docs:
	@command -v mdbook >/dev/null 2>&1 || { echo "mdbook is not installed. Please install it first."; exit 1; }
	@mdbook build $(TOP_DIR)/book --dest-dir $(TOP_DIR)/docs
	@git add --all && git commit -m "docs: building website/mdbook"

release:
	@if [ -z "$(HAS_REL)" ]; then \
		echo "git-rel is not installed. Please install it first."; \
		exit 1; \
	fi
	@if [ -z "$(TYPE)" ]; then \
		echo "Release type not specified. Use 'make release TYPE=[patch|minor|major|m.m.p]'"; \
		exit 1; \
	fi
	@git rel $(TYPE)

help:
	@echo
	@echo "Usage: make [target]"
	@echo
	@echo "Available targets:"
	@echo "  build        Build the library"
	@echo "  compile      Clean and rebuild"
	@echo "  run          Run a development example (if examples exist)"
	@echo "  test         Run all tests"
	@echo "  bind         Generate both C and Python bindings"
	@echo "  test-py      Build/install the Python wheel and run generic wire smoke"
	@echo "  test-c-abi   Build the C ABI library and run C wire runtime smoke"
	@echo "  test-bindings Run bind + Python/C binding smoke tests"
	@echo "  check        Run cargo check on all targets"
	@echo "  fmt          Format the workspace"
	@echo "  clean        Remove Cargo build artifacts"
	@echo "  docs         Build the documentation"
	@echo "  release      Release a new version"
	@echo
	@echo "Examples:"
	@echo "  make run"
	@echo "  make run EXAMPLE=main"
	@echo

h: help
