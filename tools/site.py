#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
"""MANIFOLD's site (GitHub Pages), from this repository and a release package:

  index.html, fonts/, img/        the front page (site/), {{VERSION}}, {{PKG}} and {{REPO}} filled in
  manual/index.html, manual/img/  the user guide (docs/MANUAL.md, rendered by fm1-base's tools/manual_html.py)
  emu/                            the firmware in the browser (emu/ in this repository), restyled
  install/index.html              fm1-base's web installer (web/installer.html), as tools/make_site.py makes it
  firmware/                       the package, m-bootloader (MIT) and the licence texts that ship with the firmware

  python3 tools/site.py FM1_BASE build/manifold-X.Y.fwsc X.Y OUT_DIR

fm1-base's own tools/make_site.py does much the same with Emscripten's emulator build; this one takes the emulator
built into emu/ (by tools/emcc-zig, no Emscripten needed) and keeps MANIFOLD's own front page."""
import hashlib
import json
import re
import shutil
import subprocess
import sys
from pathlib import Path

APP = Path(__file__).resolve().parents[1]

# the manual's and the installer's pages in the firmware's look: its screen's black, three greys, the theme's two
# accents (the one picked on the front page), its face (Barlow Semi Condensed)
DARK = """<style>
@font-face { font-family: "Barlow SC"; src: url(../fonts/BarlowSemiCondensed-Medium.ttf); font-weight: 400 500; }
@font-face { font-family: "Barlow SC"; src: url(../fonts/BarlowSemiCondensed-Bold.ttf); font-weight: 600 700; }
:root { --paper: #000; --sheet: #111116; --rule: #222228; --ink: #f4f4f8; --muted: #9c9ca6;
  --hot: rgb(64,168,232); --hot-t: #10202c; --hot-ink: rgb(168,226,255); --cool: rgb(64,168,232); --cool-t: #0c1820;
  color-scheme: dark; }
body { font-family: "Barlow SC", "Barlow Semi Condensed", system-ui, sans-serif; font-weight: 500; color: #c8c8d0; }
h1, h2, h3, strong, th { color: var(--ink); }
h2 { border-top-color: var(--rule); }
img { max-width: 100%; height: auto; }
main img { width: 300px; margin: 4px 8px 4px 0; border: 2px solid var(--rule); }
th { border-bottom-color: var(--rule); }
a { color: var(--hot-ink); }
</style>
<script>
try { const T = {Glacier:["64,168,232","168,226,255"],Ember:["232,128,32","255,196,96"],Moss:["88,196,108","176,248,184"],
  Rose:["228,88,132","255,168,196"],Violet:["148,112,240","208,184,255"],Sunset:["240,100,64","255,208,128"],
  Lagoon:["40,168,168","128,236,220"],Gold:["204,164,40","255,228,128"],Mono:["184,184,192","244,244,248"]}[localStorage.getItem("manifold-theme")];
  if (T) { const r = document.documentElement.style; r.setProperty("--hot", `rgb(${T[0]})`); r.setProperty("--cool", `rgb(${T[0]})`);
    r.setProperty("--hot-ink", `rgb(${T[1]})`); } } catch (e) {}
</script>
"""


# the installer's own variables (web/installer.html), the same look
DARK_INSTALL = """<style>
@font-face { font-family: "Barlow SC"; src: url(../fonts/BarlowSemiCondensed-Medium.ttf); font-weight: 400 500; }
@font-face { font-family: "Barlow SC"; src: url(../fonts/BarlowSemiCondensed-Bold.ttf); font-weight: 600 700; }
:root, :root[data-theme], :root:not([data-theme="light"]) { --bg: #000; --fg: #f4f4f8; --dim: #9c9ca6;
  --accent: rgb(64,168,232); --on-accent: #000; --bad: #ff8a73; --focus: rgb(168,226,255); color-scheme: dark; }
body { font-family: "Barlow SC", "Barlow Semi Condensed", system-ui, sans-serif; font-weight: 500; }
button { border-radius: 2px; font-weight: 700; }
</style>
<script>
try { const T = {Glacier:"64,168,232",Ember:"232,128,32",Moss:"88,196,108",Rose:"228,88,132",Violet:"148,112,240",
  Sunset:"240,100,64",Lagoon:"40,168,168",Gold:"204,164,40",Mono:"184,184,192"}[localStorage.getItem("manifold-theme")];
  if (T) document.documentElement.style.setProperty("--accent", `rgb(${T})`); } catch (e) {}
</script>
"""


# the emulator's page (fm1-base web/emu/index.html): the same black and accents, the device drawn as it is
DARK_EMU = """<style>
@font-face { font-family: "Barlow SC"; src: url(../fonts/BarlowSemiCondensed-Medium.ttf); font-weight: 400 500; }
@font-face { font-family: "Barlow SC"; src: url(../fonts/BarlowSemiCondensed-Bold.ttf); font-weight: 600 700; }
:root, :root[data-theme], :root:not([data-theme="light"]) { --ground: #000; --ink: #f4f4f8; --muted: #9c9ca6;
  --rule: #222228; --accent: rgb(64,168,232); --teal: rgb(168,226,255); --card: #111116; --focus: rgb(168,226,255);
  color-scheme: dark; }
body { font-family: "Barlow SC", "Barlow Semi Condensed", system-ui, sans-serif; }
.start button { background: var(--accent); color: #000; border-radius: 2px; font-weight: 700; }
</style>
<script>
try { const T = {Glacier:["64,168,232","168,226,255"],Ember:["232,128,32","255,196,96"],Moss:["88,196,108","176,248,184"],
  Rose:["228,88,132","255,168,196"],Violet:["148,112,240","208,184,255"],Sunset:["240,100,64","255,208,128"],
  Lagoon:["40,168,168","128,236,220"],Gold:["204,164,40","255,228,128"],Mono:["184,184,192","244,244,248"]}[localStorage.getItem("manifold-theme")];
  if (T) { const r = document.documentElement.style; r.setProperty("--accent", `rgb(${T[0]})`); r.setProperty("--teal", `rgb(${T[1]})`); } } catch (e) {}
</script>
"""


def restyle(path, css=DARK):
    html = path.read_text(encoding="utf-8")
    if "</head>" not in html:
        raise SystemExit(f"{path}: no </head>")
    path.write_text(html.replace("</head>", css + "</head>", 1), encoding="utf-8")


def main(base, pkg, version, out):
    base, pkg, out = Path(base).resolve(), Path(pkg).resolve(), Path(out).resolve()
    sys.path.insert(0, str(base / "tools"))
    import make_site as ms                                  # fm1-base: its helpers, the same installer
    conf = ms.read_conf(APP / "fm1.toml")
    raw = pkg.read_bytes()
    product = ms.product_of(raw)
    if not re.fullmatch(re.escape(conf.get("id_prefix", "FM-1_0")) + r"\d{6}", product):
        raise SystemExit(f"{pkg}: identity {product!r}: make a release build (tools/build.py APP --release X.Y)")
    name = f"{conf['name'].lower()}-{re.sub(r'[^A-Za-z0-9.-]', '-', version)}.fwsc"
    repo = conf.get("repo", "")
    if out.exists():
        shutil.rmtree(out)
    shutil.copytree(APP / "site", out)
    html = (out / "index.html").read_text(encoding="utf-8")
    for k, v in (("VERSION", version), ("PKG", name), ("REPO", repo)):
        html = html.replace("{{" + k + "}}", v)
    (out / "index.html").write_text(html, encoding="utf-8")
    # the firmware, m-bootloader, the licence texts
    fw = out / "firmware"
    fw.mkdir()
    shutil.copy(pkg, fw / name)
    boot = base / "web" / "m-bootloader" / f"fm1-uboot-{ms.MBOOT_VERSION}.fwsc"
    boot_name = f"m-bootloader-{ms.MBOOT_VERSION}.fwsc"
    shutil.copy(boot, fw / boot_name)
    shutil.copy(base / "web" / "m-bootloader" / "LICENSE", fw / "m-bootloader-LICENSE.txt")
    shutil.copy(base / "LICENSE", fw / "MPL-2.0.txt")
    for lic in sorted((base / "LICENSES").glob("*.txt")):
        shutil.copy(lic, fw / lic.name)
    # the installer, filled as make_site.py fills it
    page = (base / "web" / "installer.html").read_text(encoding="utf-8")
    lib = "\n".join(ms.strip_module((base / "web" / f).read_text(encoding="utf-8"))
                    for f in ("fm1ports.js", "fm1pkg.js", "fm1ota.js"))
    meta = json.dumps({"version": version, "product": product, "pkg": "../firmware/" + name,
                       "backup": bool(conf.get("backup")), "res": None,
                       "boot": {"pkg": "../firmware/" + boot_name, "version": ms.MBOOT_VERSION,
                                "sha256": hashlib.sha256(boot.read_bytes()).hexdigest()}})
    credits = conf.get("credits", "")
    for key, val in (("RES", conf.get("resources_what", "{{NAME}}'s data")),
                     ("RES_JA", conf.get("resources_what_ja", "{{NAME}} のデータ")),
                     ("NAME", conf["name"]), ("REPO", repo), ("REPO_SHORT", re.sub(r"^https?://", "", repo)),
                     ("LICENSE", conf.get("licence", "MPL-2.0")), ("CREDITS", " " + credits if credits else "")):
        page = page.replace("{{" + key + "}}", val)
    (out / "install").mkdir()
    (out / "install" / "index.html").write_text(page.replace("/*LIB*/", lib).replace("/*META*/", meta)
                                                .replace("/*APP*/", "null"), encoding="utf-8")
    # the manual
    (out / "manual").mkdir()
    subprocess.run([sys.executable, str(base / "tools" / "manual_html.py"), str(APP / "docs" / "MANUAL.md"),
                    str(out / "manual" / "index.html"), "--name", conf["name"], "--repo", repo,
                    "--tagline", conf.get("tagline", "")], check=True, capture_output=True)
    shutil.copytree(APP / "docs" / "img", out / "manual" / "img")
    restyle(out / "manual" / "index.html")
    restyle(out / "install" / "index.html", DARK_INSTALL)
    if (APP / "emu" / "app.wasm").exists():           # the firmware in the browser (emu/: tools/emcc-zig)
        shutil.copytree(APP / "emu", out / "emu", ignore=shutil.ignore_patterns("README.md"))
        restyle(out / "emu" / "index.html", DARK_EMU)
    (out / ".nojekyll").write_text("")
    print(f"site: {out}: index.html, manual/, install/ ({product}), firmware/{name} ({len(raw)} B)")


if __name__ == "__main__":
    if len(sys.argv) != 5:
        sys.exit(__doc__)
    main(*sys.argv[1:5])
