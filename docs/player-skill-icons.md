# Player skill icons

Player pages render the game's skill-menu artwork from `web/skills`. The twelve required PNG files are served only through `/assets/skills/<name>.png`; filenames are allowlisted by the page and the server rejects path separators.

The assets are the cooked `T_Icon_Tag_Skill_*` menu emblems in `Art/UI/Skills/Icons/Tags`, matching the icons shown beside levels in the in-game skills list. Fishing uses the corresponding tag texture from its game-feature plugin.

`/api/diagnostics/player-skills` reports the expected, missing, and compatible icon counts. `/api/update-status` exposes the same capability as `playerSkillIconCompatibility`, `playerSkillIconAssets`, and `playerSkillMissingIconAssets`. A missing or invalid asset fails only that image request; skill names, levels, XP, player data, and unrelated website features remain available.

The normal update backup already copies the complete `/home/container/zanik/web` directory. For deployment, copy the rebuilt `dist/libdwstatus.so` to `/home/container/libdwstatus.so` and the complete local `web/skills` directory to `/home/container/zanik/web/skills`.
