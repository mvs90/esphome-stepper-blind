#!/usr/bin/env python3
"""Host regression tests: actual component and ESPHome Stepper; mocked cover/storage/time."""
import pathlib
import subprocess
import tempfile
import esphome

root = pathlib.Path(__file__).resolve().parent.parent
esphome_root = pathlib.Path(esphome.__file__).resolve().parent.parent
with tempfile.TemporaryDirectory(prefix="stepper-blind-test-") as tmp:
    binary = str(pathlib.Path(tmp) / "test")
    subprocess.run([
        "c++", "-std=c++17", "-Wall", "-Wextra", "-Wno-unused-variable", "-Werror",
        "-fsanitize=address,undefined", "-g",
        "-I" + str(root / "tests/stubs"), "-I" + str(esphome_root),
        "-I" + str(root / "components/stepper_blind"),
        str(root / "components/stepper_blind/stepper_blind.cpp"),
        str(esphome_root / "esphome/components/stepper/stepper.cpp"),
        str(root / "tests/test_stepper_blind.cpp"), "-o", binary,
    ], check=True)
    subprocess.run([binary], check=True)
