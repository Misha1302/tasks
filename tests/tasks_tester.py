import subprocess
from pathlib import Path

tasks = ["task1.1", "task1.2", "task1.3", "task1.4"]
build = Path("build-tests")
build.mkdir(exist_ok=True)

subprocess.run(["cmake", "-S", ".", "-B", build], check=True)
subprocess.run(["cmake", "--build", build], check=True)

ok = 0
total = 0

for task in tasks:
    for input_file in sorted(Path("tests", task).glob("*.in")):
        output_file = input_file.with_suffix(".out")

        actual = subprocess.run(
            [str(build / task.replace(".", "_"))],
            input=input_file.read_text(),
            text=True,
            capture_output=True,
        ).stdout

        expected = output_file.read_text()
        total += 1

        if actual == expected:
            print("OK", input_file)
            ok += 1
        else:
            print("FAIL", input_file)
            print("expected:")
            print(repr(expected))
            print("actual:")
            print(repr(actual))

print(f"{ok}/{total} passed")
