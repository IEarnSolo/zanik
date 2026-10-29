from pathlib import Path

page = (Path(__file__).resolve().parents[1] / "src/web_page.h").read_text(encoding="utf-8")

assert "document.createElement('a')" in page
assert "link.href='/recipe-unlocks?q='+encodeURIComponent(group.name)" in page
assert "link.target='_blank'" in page
assert "link.rel='noopener'" in page
assert "for(const childId of target.recipes" not in page
assert "function routeLines(entry){" in page
assert "link.onclick=closeRecipeTooltip" in page
assert 'id="recipeCompletion"' in page
assert "recipes unlocked';recipeCounts.textContent" in page
assert "snapshot.compatibility==='changed'" in page
assert "Catalog changed; matching unlocks are shown until the next scan" in page
assert "step.targetText||step.text" in page
assert "grid-template-columns:2.35rem minmax(0,1fr)" in page
assert ".activity-marker img{width:1.75rem;height:1.75rem" in page

print("player recipe card links and direct tooltips: ok")
