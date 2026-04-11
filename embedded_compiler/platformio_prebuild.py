Import("env")

import os
import subprocess
from pathlib import Path


def run_checked(command, cwd):
    print("[embedded-prebuild] running:", " ".join(command), "cwd=", cwd)
    subprocess.run(command, cwd=cwd, check=True)


project_dir = Path(env["PROJECT_DIR"])
compiler_dir = project_dir / "embedded_compiler"
input_dir = project_dir / "angular_app"
output_dir = project_dir / "src" / "generated"

output_dir.mkdir(parents=True, exist_ok=True)

run_checked(["make"], str(compiler_dir))
run_checked([str(compiler_dir / "bin" / "main.exe"), str(input_dir), str(output_dir)], str(compiler_dir))
