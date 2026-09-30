#!/usr/bin/env python3
"""stage_sd.py -- put YOUR copy of PvZ TV Touch where the wrapper looks.

Optional: the wrapper does all of this itself on its first start, from
game.apk alone (source/dcr_setup.c). It needs, from the Android version you
own (com.trans.pvztv 1.1.5, armeabi-v7a):

    <root>/game.apk            the APK itself: the engine reads its data out of the zip
    <root>/libnative_code.so   \\
    <root>/libGameMain.so       > lib/armeabi-v7a/ from that APK
    <root>/libHomura.so        /
    <root>/classes.txt   the NAMES of the Java classes the APK defines (from its
                         classes*.dex, or apktool's smali/ folders), so the
                         wrapper's JNI FindClass answers as the phone would

with <root> = /switch/pvz_touch_nx on the SD card. (The wrapper accepts the
APK under any name; this copies it as game.apk.)

    python3 tools/stage_sd.py PvZ-TV-v1.1.5-260924-release.apk -o /Volumes/SWITCH
    python3 tools/stage_sd.py path/to/apktool-output -o /Volumes/SWITCH

An apktool output folder is repacked into a zip: its original/AndroidManifest.xml
(the binary manifest), assets/ and lib/, stored uncompressed with 4-byte data
alignment as zipalign would. Split APKs (.xapk/.apks) are not supported: pass
the base APK.
"""
import argparse
import os
import shutil
import sys
import zipfile

LIBS = ["libnative_code.so", "libGameMain.so", "libHomura.so"]
ABI_DIR = "lib/armeabi-v7a/"
ROOT = "switch/pvz_touch_nx"



def build_id(data):
    """GNU build-id of an ELF image in memory (None if absent)."""
    import struct
    if data[:4] != b"\x7fELF":
        return None
    shoff, = struct.unpack_from("<I", data, 0x20)
    shentsize, shnum, shstrndx = struct.unpack_from("<HHH", data, 0x2E)
    for i in range(shnum):
        off = shoff + i * shentsize
        sh_type, = struct.unpack_from("<I", data, off + 4)
        if sh_type == 7:  # SHT_NOTE
            noff, nsize = struct.unpack_from("<II", data, off + 16)
            p, end = noff, noff + nsize
            while p + 12 <= end:
                namesz, descsz, ntype = struct.unpack_from("<III", data, p)
                name = data[p + 12:p + 12 + namesz]
                desc_off = p + 12 + ((namesz + 3) & ~3)
                if ntype == 3 and name.startswith(b"GNU"):
                    return data[desc_off:desc_off + descsz].hex()
                p = desc_off + ((descsz + 3) & ~3)
    return None


def dex_class_names(data):
    """Class descriptors defined by one .dex image, as JNI names (a/b/C$D)."""
    import struct
    if data[:4] != b"dex\n":
        return []
    string_ids_off, = struct.unpack_from("<I", data, 0x3C)
    type_ids_off, = struct.unpack_from("<I", data, 0x44)
    class_defs_size, class_defs_off = struct.unpack_from("<II", data, 0x60)
    out = []
    for k in range(class_defs_size):
        type_idx, = struct.unpack_from("<I", data, class_defs_off + 32 * k)
        str_idx, = struct.unpack_from("<I", data, type_ids_off + 4 * type_idx)
        p, = struct.unpack_from("<I", data, string_ids_off + 4 * str_idx)
        while data[p] & 0x80:  # skip the uleb128 UTF-16 length
            p += 1
        p += 1
        desc = data[p:data.index(b"\0", p)].decode("utf-8", "replace")  # MUTF-8; names are ASCII
        if desc.startswith("L") and desc.endswith(";"):
            out.append(desc[1:-1])
    return out


def class_names(game):
    """Every class the app defines: from the APK's dex files, or apktool's smali."""
    names = set()
    if os.path.isdir(game):
        for top in sorted(os.listdir(game)):
            if not (top == "smali" or top.startswith("smali_")):
                continue
            base = os.path.join(game, top)
            for d, _, files in os.walk(base):
                for fn in files:
                    if fn.endswith(".smali"):
                        rel = os.path.relpath(os.path.join(d, fn), base)
                        names.add(rel[:-len(".smali")].replace(os.sep, "/"))
    else:
        with zipfile.ZipFile(game) as z:
            for n in z.namelist():
                if n.startswith("classes") and n.endswith(".dex") and "/" not in n:
                    names.update(dex_class_names(z.read(n)))
    return sorted(names)


def write_class_list(game, root):
    names = class_names(game)
    dst = os.path.join(root, "classes.txt")
    if os.path.lexists(dst):
        os.unlink(dst)
    if not names:
        print("  WARNING: no Java classes found (no classes*.dex / smali folders); the\n"
              "           wrapper will fall back to its built-in guesses")
        return
    with open(dst, "w", newline="\n") as f:
        f.write("# Java classes defined by the game's APK (names only). The wrapper's JNI\n"
                "# FindClass/Class.forName report exactly these, plus the Android framework.\n")
        for n in names:
            f.write(n + "\n")
    print("  classes.txt  %d Java class names" % len(names))


def write_aligned_zip(dst, entries):
    """entries: list of (arcname, source path). Stored, data 4-byte aligned."""
    with zipfile.ZipFile(dst, "w", zipfile.ZIP_STORED, allowZip64=True) as z:
        for arc, src in entries:
            zi = zipfile.ZipInfo.from_file(src, arc)
            zi.compress_type = zipfile.ZIP_STORED
            # local header = 30 + name + extra; pad extra so the data is aligned
            hdr = z.fp.tell() + 30 + len(arc.encode("utf-8"))
            zi.extra = b"\0" * ((4 - hdr % 4) % 4)
            with open(src, "rb") as f, z.open(zi, "w") as out:
                shutil.copyfileobj(f, out, 1 << 20)


def from_apktool_dir(src, apk_out):
    manifest = os.path.join(src, "original", "AndroidManifest.xml")
    if not os.path.isfile(manifest):
        sys.exit("error: %s has no original/AndroidManifest.xml (the binary manifest apktool "
                 "keeps); pass the APK itself instead" % src)
    entries = [("AndroidManifest.xml", manifest)]
    for top in ("assets", "lib"):
        base = os.path.join(src, top)
        for d, _, files in os.walk(base):
            for fn in sorted(files):
                p = os.path.join(d, fn)
                arc = os.path.relpath(p, src).replace(os.sep, "/")
                entries.append((arc, p))
    print("repacking %d files into %s ..." % (len(entries), apk_out))
    write_aligned_zip(apk_out, entries)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("game", help="your PvZ TV Touch APK, or an apktool output folder")
    ap.add_argument("-o", "--out", required=True, help="SD card root (or a staging folder)")
    ap.add_argument("--nsp", help="also copy the built pvz_nx.nsp next to the game files")
    ap.add_argument("--classes-only", action="store_true",
                    help="only (re)write classes.txt; leave game.apk and the libraries alone")
    a = ap.parse_args()

    root = os.path.join(a.out, ROOT)
    os.makedirs(root, exist_ok=True)
    if a.classes_only:
        write_class_list(a.game, root)
        return
    apk_dst = os.path.join(root, "game.apk")
    if os.path.lexists(apk_dst):
        os.unlink(apk_dst)

    if os.path.isdir(a.game):
        from_apktool_dir(a.game, apk_dst)
    else:
        print("copying %s -> %s" % (a.game, apk_dst))
        shutil.copyfile(a.game, apk_dst)

    with zipfile.ZipFile(apk_dst) as z:
        names = set(z.namelist())
        if "AndroidManifest.xml" not in names:
            sys.exit("error: not an APK (no AndroidManifest.xml)")
        if not any(n.startswith("assets/files/") for n in names):
            sys.exit("error: no assets/files in the APK -- is this PvZ TV Touch?")
        for lib in LIBS:
            arc = ABI_DIR + lib
            if arc not in names:
                sys.exit("error: %s missing from the APK (an armeabi-v7a build is required)" % arc)
            data = z.read(arc)
            dst = os.path.join(root, lib)
            if os.path.lexists(dst):
                os.unlink(dst)  # never write through a symlink into someone's files
            with open(dst, "wb") as f:
                f.write(data)
            bid = build_id(data)
            print("  %-18s %8d KB  build-id %s" % (lib, len(data) >> 10, bid))
    write_class_list(a.game, root)
    if a.nsp:
        shutil.copyfile(a.nsp, os.path.join(root, "pvz_nx.nsp"))
        print("  pvz_nx.nsp copied")
    print("\nstaged in %s" % root)
    print("\nTo launch it from its own HOME-screen icon, see README.md (Run on a Switch):\n"
          "    python3 tools/make_exefs_override.py <forwarder title id> -o %s" % a.out)


if __name__ == "__main__":
    main()
