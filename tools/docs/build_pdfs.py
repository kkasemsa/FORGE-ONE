#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 the Felucca-FORGE contributors
"""Felucca-FORGE: the user documents as PDFs, from their Markdown.

    python3 tools/docs/build_pdfs.py            (from the repository root)

Markdown -> HTML (python-markdown: tables, fenced code, attribute lists) -> PDF (Chromium through Playwright), in
the warm amber / cream booklet style: Lora headings (tools/docs/fonts, SIL OFL), Liberation Sans body, Liberation
Mono for values, Loma for Thai (system fonts; Helvetica / Menlo where they are missing). Chromium leaves the page margins white,
so each page is laid over a cream page (pypdf). Relative links become links to the repository on GitHub.
Out: docs/pdf/*.pdf.  Needs: pip install markdown playwright pypdf (and a Chromium for Playwright).
"""
import datetime
import html
import re
import sys
from pathlib import Path

import markdown
from playwright.sync_api import sync_playwright
from pypdf import PdfReader, PdfWriter

ROOT = Path(__file__).resolve().parents[2]
FONTS = Path(__file__).resolve().parent / "fonts"
OUT = ROOT / "docs" / "pdf"
REPO = "https://github.com/kkasemsa/Felucca-FORGE/blob/forge/"

DOCS = [   # (source, output, cover kicker, cover subtitle)
    ("README.md", "Felucca-FORGE-Read-Me-First.pdf", "Read me first",
     "What it is, the risks, installing, and getting back"),
    ("docs/FORGE-USER-GUIDE.md", "FORGE-User-Guide.pdf", "User guide",
     "The seeded motif generator on SEQ TOOLS F5"),
    ("docs/USER-GUIDE.md", "Felucca-FORGE-User-Guide.pdf", "Complete user guide",
     "Felucca 1.6.1 and FORGE 0.5.1 on the M-VAVE FM-1"),
    ("docs/TUTORIAL-TH.md", "Felucca-FORGE-Tutorial-TH.pdf", "Tutorial (ภาษาไทย)",
     "แจม 4 แทร็ก: FORGE บนแทร็ก 1–3 และกลองบนแทร็ก 4"),
]

CSS = """
@font-face { font-family: 'Lora'; font-weight: 400; font-style: normal; src: url('FONTS/lora-latin-400-normal.woff2'); }
@font-face { font-family: 'Lora'; font-weight: 400; font-style: italic; src: url('FONTS/lora-latin-400-italic.woff2'); }
@font-face { font-family: 'Lora'; font-weight: 600; font-style: normal; src: url('FONTS/lora-latin-600-normal.woff2'); }
@font-face { font-family: 'Lora'; font-weight: 700; font-style: normal; src: url('FONTS/lora-latin-700-normal.woff2'); }
:root {
  --paper: #fbf6ec; --ink: #2b2118; --soft: #6b5a48; --amber: #b5651d; --amber-deep: #8a4a12;
  --rule: #e2cfae; --band: #f3e5c8; --band-2: #f8eedb; --code: #f1e4c9;
}
@page { size: A4; margin: 20mm 18mm 20mm 18mm; }
html { background: transparent; }
body { font-family: 'Liberation Sans', 'Loma', 'Helvetica Neue', Helvetica, Arial, sans-serif; font-size: 10.2pt;
       line-height: 1.5; color: var(--ink); background: transparent; margin: 0; }
h1, h2, h3, h4 { font-family: 'Lora', 'Loma', Georgia, serif; color: var(--ink); line-height: 1.25; break-after: avoid; }
h1 { font-size: 24pt; font-weight: 700; margin: 0 0 4mm; }
h2 { font-size: 15.5pt; font-weight: 600; margin: 9mm 0 3mm; padding-bottom: 1.6mm; border-bottom: 1.4pt solid var(--amber); }
h3 { font-size: 12pt; font-weight: 600; margin: 6mm 0 2mm; color: var(--amber-deep); }
p { margin: 0 0 2.6mm; }
a { color: var(--amber-deep); text-decoration: none; border-bottom: 0.5pt solid var(--rule); }
strong { font-weight: 700; }
ul, ol { margin: 0 0 3mm; padding-left: 6mm; }
li { margin: 0 0 1.2mm; }
code { font-family: 'Liberation Mono', Menlo, Consolas, monospace; font-size: 8.9pt; background: var(--code);
       padding: 0.2mm 1.1mm; border-radius: 1mm; }
pre { background: var(--code); border-left: 2.5pt solid var(--amber); padding: 2.5mm 4mm; border-radius: 1mm;
      break-inside: avoid; }
pre code { background: none; padding: 0; }
hr { border: 0; border-top: 0.6pt solid var(--rule); margin: 7mm 0; }
table { width: 100%; border-collapse: collapse; margin: 2mm 0 4.5mm; font-size: 9pt; line-height: 1.38;
        break-inside: auto; }
thead { display: table-header-group; }
tr { break-inside: avoid; }
th { font-family: 'Lora', 'Loma', Georgia, serif; font-weight: 600; text-align: left; background: var(--band);
     color: var(--ink); padding: 1.6mm 2.2mm; border-bottom: 1pt solid var(--amber); }
td { padding: 1.5mm 2.2mm; border-bottom: 0.5pt solid var(--rule); vertical-align: top; }
tbody tr:nth-child(even) td { background: var(--band-2); }
td strong { color: var(--amber-deep); }
/* the cover */
.cover { height: 247mm; display: flex; flex-direction: column; justify-content: space-between; break-after: page; }
.cover .top { border-top: 5pt solid var(--amber); padding-top: 9mm; }
.cover .kicker { font-family: 'Lora', 'Loma', serif; font-style: italic; color: var(--amber-deep); font-size: 13pt; }
.cover .title { font-family: 'Lora', 'Loma', serif; font-weight: 700; font-size: 36pt; line-height: 1.1; margin: 5mm 0 4mm; }
.cover .sub { font-size: 12.5pt; color: var(--soft); max-width: 140mm; }
.cover .grid { display: grid; grid-template-columns: repeat(16, 1fr); gap: 1.4mm; margin: 0 0 6mm; }
.cover .grid span { height: 13mm; border-radius: 1.2mm; background: var(--band); }
.cover .grid span.on { background: var(--amber); }
.cover .grid span.tie { background: #d9a66a; }
.cover .meta { font-size: 9pt; color: var(--soft); border-top: 0.6pt solid var(--rule); padding-top: 3mm; line-height: 1.6; }
.cover .meta b { color: var(--ink); }
.doc > h1:first-child { display: none; }        /* (the cover holds the title) */
.doc > h1:first-child + p { font-family: 'Lora', 'Loma', serif; font-style: italic; color: var(--soft); }
"""

# a FORGE-like row of 16 steps for the cover: on, tie, rest
COVER_STEPS = "1 1 0 1 2 0 1 1 1 0 1 2 2 0 1 0".split()


def absolute_links(md_text, src):
    """relative links -> the repository on GitHub (anchors and absolute URLs stay)"""
    base = (Path(src).parent.as_posix() + "/") if "/" in src else ""

    def fix(m):
        label, target = m.group(1), m.group(2)
        if re.match(r"[a-z]+:|#", target):
            return m.group(0)
        path, _, frag = target.partition("#")
        full = (Path(base) / path).as_posix() if path else src
        full = re.sub(r"[^/]+/\.\./", "", full)
        return f"[{label}]({REPO}{full}{'#' + frag if frag else ''})"
    return re.sub(r"\[([^\]]+)\]\(([^)\s]+)\)", fix, md_text)


def page(src, kicker, sub):
    text = (ROOT / src).read_text()
    title = re.search(r"^# (.+)$", text, re.M).group(1).split(" — ")[0]
    body = markdown.markdown(absolute_links(text, src), extensions=["tables", "fenced_code", "attr_list", "sane_lists"])
    steps = "".join(f'<span class="{ {"1": "on", "2": "tie"}.get(s, "") }"></span>' for s in COVER_STEPS)
    today = datetime.date.today().strftime("%d %B %Y").lstrip("0")
    cover = f"""<section class="cover">
  <div class="top"><div class="kicker">{html.escape(kicker)}</div>
  <div class="title">{html.escape(title)}</div><div class="sub">{html.escape(sub)}</div></div>
  <div><div class="grid">{steps}</div>
  <div class="meta"><b>Felucca-FORGE</b> · Felucca 1.6.1 + FORGE 0.5.1 · M-VAVE FM-1<br>
  github.com/kkasemsa/Felucca-FORGE (forge branch) · {today}<br>
  Felucca by Hügelton Instruments and contributors · GPL-3.0-only</div></div></section>"""
    css = CSS.replace("FONTS", FONTS.as_uri())
    return f"""<!doctype html><html><head><meta charset="utf-8"><title>{html.escape(title)}</title>
<style>{css}</style></head><body>{cover}<main class="doc">{body}</main></body></html>"""


FOOT = """<div style="font-family: Helvetica, Arial, sans-serif; font-size: 7.5pt; color: #8a7660; width: 100%;
  padding: 0 18mm; display: flex; justify-content: space-between;"><span>TITLE</span>
  <span><span class="pageNumber"></span> / <span class="totalPages"></span></span></div>"""


def underlay(pdf, paper):
    """every page of pdf over the cream page (its margins and footer included); keeps the links"""
    bg = PdfReader(paper).pages[0]
    rd, wr = PdfReader(pdf), PdfWriter()
    for p in rd.pages:
        base = wr.add_blank_page(width=p.mediabox.width, height=p.mediabox.height)
        base.merge_page(bg)
        base.merge_page(p)
        if "/Annots" in p:
            base[__import__("pypdf").generic.NameObject("/Annots")] = p["/Annots"]
    wr.add_metadata({"/Title": rd.metadata.title if rd.metadata and rd.metadata.title else pdf.stem,
                     "/Author": "Felucca-FORGE contributors"})
    with open(pdf, "wb") as f:
        wr.write(f)
    paper.unlink()


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    with sync_playwright() as pw:
        browser = pw.chromium.launch()
        for src, out, kicker, sub in DOCS:
            doc = page(src, kicker, sub)
            tmp = OUT / (Path(out).stem + ".html")
            tmp.write_text(doc)
            pg = browser.new_page()
            pg.goto(tmp.as_uri(), wait_until="networkidle")
            title = re.search(r"<title>(.*?)</title>", doc).group(1)
            pg.pdf(path=str(OUT / out), format="A4", print_background=True, display_header_footer=True,
                   header_template="<div></div>", footer_template=FOOT.replace("TITLE", title),
                   margin={"top": "18mm", "bottom": "18mm", "left": "18mm", "right": "18mm"})
            pg.set_content('<html style="margin:0;background:#fbf6ec"><body style="margin:0"></body></html>')
            pg.pdf(path=str(OUT / "_paper.pdf"), format="A4", print_background=True,
                   margin={"top": "0", "bottom": "0", "left": "0", "right": "0"})
            pg.close()
            tmp.unlink()
            underlay(OUT / out, OUT / "_paper.pdf")
            print("wrote", (OUT / out).relative_to(ROOT))
        browser.close()


if __name__ == "__main__":
    sys.exit(main())
