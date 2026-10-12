from pathlib import Path

page = (Path(__file__).resolve().parents[1] / "src/web_page.h").read_text(encoding="utf-8")

assert "link.href='?q='+encodeURIComponent(entry.name)" in page
assert "history.pushState({view:'search'}" in page
assert "window.addEventListener('popstate'" in page
assert "details.dataset.section=name" in page
assert "details.open=open.has(name)" in page
assert "scrollY:window.scrollY" in page
assert "event.ctrlKey||event.metaKey||event.shiftKey||event.altKey" in page

print("recipe category navigation history: ok")
