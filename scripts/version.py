Import("env")
import subprocess

def get_version():
    try:
        # On a tagged commit → "v1.0.4" → "1.0.4"
        tag = subprocess.check_output(
            ["git", "describe", "--tags", "--exact-match"],
            stderr=subprocess.DEVNULL
        ).decode().strip().lstrip("v")
        return tag
    except Exception:
        pass
    try:
        # On an untagged commit → "1.0.3-5-gabcdef"
        desc = subprocess.check_output(
            ["git", "describe", "--tags"],
            stderr=subprocess.DEVNULL
        ).decode().strip().lstrip("v")
        return desc
    except Exception:
        return "0.0.0-dev"

version = get_version()
print(f"[version] Firmware version: {version}")
env.Append(CPPDEFINES=[("FIRMWARE_VERSION", f'\\"{version}\\"')])
