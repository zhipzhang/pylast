# pylast local integration notes

This copy of `pylast` is kept independent from the main LACT_sim build and
runtime. It is intended as a reference implementation for interface alignment
and result checking.

## Location

- Source: `/Users/yun/Downloads/LACT_sim/external/pylast`
- Python environment: `/Users/yun/Downloads/LACT_sim/external/pylast/.venv-py39`
- ROOT-backed test runtime found by Codex: `/private/tmp/lact-root-test`
- Older local ROOT setup noted previously: `/Users/yun/root_install/bin/thisroot.sh`

## Basic use

```bash
cd /Users/yun/root_install
source bin/thisroot.sh
cd /Users/yun/Downloads/LACT_sim/external/pylast
source .venv-py39/bin/activate
python -c "import pylast; print(pylast.__file__)"
```

If `.venv-py39` fails with `Library not loaded: @rpath/libCore.so`, use the
temporary ROOT-backed runtime for checks:

```bash
/private/tmp/lact-root-test/bin/python -c "import pylast; print(pylast.__file__)"
/private/tmp/lact-root-test/bin/root-config --cflags --libs
```

Installed command entry points include:

- `hillas_reco`
- `merge_source`
- `simplified_convert`
- `make_histogram`

Use them through the local venv, for example:

```bash
cd /Users/yun/root_install
source bin/thisroot.sh
cd /Users/yun/Downloads/LACT_sim/external/pylast
.venv-py39/bin/hillas_reco --help
```

## Build command used

```bash
cd /Users/yun/root_install
source bin/thisroot.sh
cd /Users/yun/Downloads/LACT_sim/external/pylast
PATH=/Users/yun/Downloads/LACT_sim/external/pylast/.venv-py39/bin:$PATH \
CMAKE_PREFIX_PATH=/Users/yun/Downloads/LACT_sim/external/pylast/.venv-py39/lib/python3.9/site-packages/nanobind/cmake \
.venv-py39/bin/python -m pip install . --config-settings=cmake.args="-DENABLE_OPENMP=OFF"
```

## Local compatibility patches

The upstream source needed small macOS/ROOT compatibility fixes before it would
build here:

- Use Python 3.9 venv to avoid Python 3.12 `numba`/`llvmlite` source builds.
- Disable muparser OpenMP auto-detection for this local build.
- Keep `PointingTelescope` aggregate-friendly while supporting explicit
  `(azimuth, altitude)` construction sites.
- Avoid a ROOT `TString` overload interfering with Boost.PFR field-name
  constexpr checks.
- Match `ReconstructedEnergy` nanobind init order to the aggregate field order.

Keep these changes contained in `external/pylast`; do not copy them into the
main LACT_sim runtime unless we intentionally vendor pylast later.

## Syntax checks used by Codex

```bash
c++ -std=c++20 -fsyntax-only \
  -I/Users/yun/Downloads/LACT_sim/external/pylast/include \
  -I/Users/yun/Downloads/LACT_sim/external/pylast/include/external \
  -I/Users/yun/Downloads/LACT_sim/external/pylast/root/include \
  -I/private/tmp/lact-root-test/include \
  /Users/yun/Downloads/LACT_sim/external/pylast/root/LactEventSource.cpp

python3 -m py_compile \
  /Users/yun/Downloads/LACT_sim/external/pylast/src/pylast/io/LactEventSource.py \
  /Users/yun/Downloads/LACT_sim/external/pylast/src/pylast/visualize/event_visualizer.py
```
