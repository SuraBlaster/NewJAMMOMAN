import argparse
import codecs
import hashlib
import json
import zipfile
from collections import Counter
from datetime import datetime, timezone
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
DIRECTORIES = ["Source", "Shader", "Tests", "Tools/StageConverter/Source"]
EXTENSIONS = {".cpp", ".h", ".hpp", ".inl", ".hlsl", ".hlsli"}


def decode_source(data):
    if data.startswith(codecs.BOM_UTF8):
        return data.decode("utf-8-sig"), "utf-8-bom"

    if data.startswith((codecs.BOM_UTF16_LE, codecs.BOM_UTF16_BE)):
        return data.decode("utf-16"), "utf-16"

    try:
        return data.decode("utf-8"), "utf-8"
    except UnicodeDecodeError:
        return data.decode("cp932"), "cp932"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--apply", action="store_true")
    arguments = parser.parse_args()
    files = sorted(path for directory in DIRECTORIES for path in (ROOT / directory).rglob("*")
                   if path.is_file() and path.suffix.lower() in EXTENSIONS)
    plan = []
    failures = []

    for path in files:
        original = path.read_bytes()

        try:
            text, encoding = decode_source(original)
        except UnicodeDecodeError as error:
            failures.append({"path": path.relative_to(ROOT).as_posix(), "error": str(error)})
            continue

        prefix = b"" if path.suffix.lower() in {".hlsl", ".hlsli"} else codecs.BOM_UTF8
        converted = prefix + text.encode("utf-8")
        assert converted.decode("utf-8-sig") == text
        plan.append((path, original, converted, encoding))

    print(json.dumps({"files": len(files), "encodings": dict(Counter(item[3] for item in plan)),
                      "changes": sum(before != after for _, before, after, _ in plan),
                      "failures": failures}, ensure_ascii=False, indent=2))

    if failures:
        raise SystemExit("変換できないファイルがあるため、書き込みは行っていません")

    if not arguments.apply:
        return

    stamp = datetime.now(timezone.utc).strftime("%Y%m%d-%H%M%S")
    backup = ROOT / "output" / "encoding-backup" / stamp
    backup.mkdir(parents=True, exist_ok=False)
    configuration = [ROOT / name for name in ["Game.vcxproj", ".editorconfig", "CODING_STYLE.md",
                     "Tools/build_laboratory.cjs", "Tools/StageConverter/StageConverter.vcxproj"]]

    with zipfile.ZipFile(backup / "original-files.zip", "w", zipfile.ZIP_DEFLATED) as archive:
        for path, original, _, _ in plan:
            archive.writestr(path.relative_to(ROOT).as_posix(), original)

        for path in configuration:
            if path.exists():
                archive.writestr(path.relative_to(ROOT).as_posix(), path.read_bytes())

    manifest = []

    for path, original, converted, encoding in plan:
        if original != converted:
            path.write_bytes(converted)

        assert path.read_bytes().decode("utf-8-sig") == decode_source(original)[0]
        manifest.append({"path": path.relative_to(ROOT).as_posix(), "previousEncoding": encoding,
                         "beforeSha256": hashlib.sha256(original).hexdigest(),
                         "afterSha256": hashlib.sha256(converted).hexdigest()})

    (backup / "manifest.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2), encoding="utf-8")
    print("バックアップ:", backup)


if __name__ == "__main__":
    main()
