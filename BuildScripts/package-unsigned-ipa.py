#!/usr/bin/env python3
"""Validate a device build, strip signatures, package a re-signable (not installable) IPA."""
import argparse
import hashlib
import json
import plistlib
import re
import shutil
import subprocess
import tempfile
import zipfile
from pathlib import Path


def run(*args):
    return subprocess.check_output(args, text=True, stderr=subprocess.STDOUT).strip()


def package(app, output):
    app = app.resolve()
    output = output.resolve()
    with (app / "Info.plist").open("rb") as source:
        info = plistlib.load(source)
    if info.get("CFBundleSupportedPlatforms") != ["iPhoneOS"]:
        raise ValueError("Expected a real iPhoneOS build, not an iOS Simulator app")
    bundle_id = info["CFBundleIdentifier"]
    if bundle_id == "com.moonlight-stream.Moonlight" or not bundle_id.endswith(".moonlight.simcity"):
        raise ValueError(f"Unexpected fork Bundle Identifier: {bundle_id}")
    executable_name = info["CFBundleExecutable"]
    if Path(executable_name).name != executable_name:
        raise ValueError("Invalid executable name")
    if 2 not in info.get("UIDeviceFamily", []):
        raise ValueError("iPad device family missing")

    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory() as temp:
        staging = Path(temp)
        copied_app = staging / "Payload" / app.name
        copied_app.parent.mkdir()
        # ditto preserves executable modes, symlinks and framework layout.
        subprocess.run(["ditto", str(app), str(copied_app)], check=True)
        executable = copied_app / executable_name
        architectures = run("xcrun", "lipo", "-archs", str(executable)).split()
        if architectures != ["arm64"]:
            raise ValueError(f"Expected arm64-only device binary, got {architectures}")
        platform = run("xcrun", "vtool", "-show-build", str(executable))
        if not re.search(r"platform\s+IOS\b", platform) or "IOSSIMULATOR" in platform:
            raise ValueError(f"Mach-O platform is not IOS:\n{platform}")
        # Linkers or prebuilt dependencies may still carry ad-hoc signatures.
        # Remove every Mach-O signature, so Sideloadly can sign the entire bundle.
        for path in copied_app.rglob("*"):
            if path.is_file() and not path.is_symlink():
                if "Mach-O" in run("file", "-b", str(path)):
                    signed = subprocess.run(["codesign", "-d", str(path)], capture_output=True)
                    if signed.returncode == 0:
                        subprocess.run(["codesign", "--remove-signature", str(path)], check=True)
                    check = subprocess.run(["codesign", "-d", str(path)], capture_output=True, text=True)
                    if check.returncode == 0 or "not signed at all" not in check.stderr:
                        raise ValueError(f"Unable to verify unsigned binary: {path}: {check.stderr}")
        for signature in copied_app.rglob("_CodeSignature"):
            shutil.rmtree(signature)
        for profile in copied_app.rglob("embedded.mobileprovision"):
            profile.unlink()
        subprocess.run(["ditto", "-c", "-k", "--keepParent", "--norsrc",
                        str(copied_app.parent), str(output)], check=True)

    with zipfile.ZipFile(output) as ipa:
        names = ipa.namelist()
        expected = f"Payload/{app.name}/"
        if not any(name == expected + executable_name for name in names):
            raise ValueError("IPA is missing Payload/<app>.app/<executable>")
        if any("_CodeSignature/" in name or name.endswith("embedded.mobileprovision") for name in names):
            raise ValueError("IPA unexpectedly contains signing material")
        if ipa.testzip() is not None:
            raise ValueError("IPA ZIP integrity check failed")

    checksum = hashlib.sha256(output.read_bytes()).hexdigest()
    output.with_suffix(".ipa.sha256").write_text(f"{checksum}  {output.name}\n")
    manifest = {
        "bundle_identifier": bundle_id, "display_name": info.get("CFBundleDisplayName"),
        "architectures": architectures, "minimum_os": info.get("MinimumOSVersion"),
        "sdk": info.get("DTSDKName"), "platform": "iPhoneOS", "signed": False,
        "sha256": checksum, "source_commit": run("git", "rev-parse", "HEAD"),
        "xcode": run("xcodebuild", "-version"), "macho_build": platform,
        "device_input_tested": False,
    }
    output.with_suffix(".json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps(manifest, indent=2))
    print(f"Created {output} ({output.stat().st_size} bytes)")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("app", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    package(args.app, args.output)
