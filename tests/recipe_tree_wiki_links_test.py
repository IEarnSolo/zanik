from pathlib import Path

page = (Path(__file__).resolve().parents[1] / "src/web_page.h").read_text(encoding="utf-8")

assert "https://dragonwilds.runescape.wiki/w/" in page
assert "encodeURIComponent(String(name).trim().replaceAll(' ','_'))" in page
assert "label.target='_blank'" in page
assert "label.rel='noopener noreferrer'" in page
assert ".node-label.wiki-link{color:#fff;text-decoration:none" in page
assert ".node-label.wiki-link:hover,.node-label.wiki-link:focus-visible{text-decoration:underline" in page
assert "treeNode(step.text,'',step.kind.replaceAll('_',' '),false)" in page

print("recipe tree wiki links: ok")
