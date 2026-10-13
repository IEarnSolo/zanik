# Zanik QoL module (integration pending)

This directory is independent of the status API and website. Its compost
selection core chooses the first filled bucket of the same type in inventory
slot order. The packaged setting is `FEATURE_QOL_AUTO_COMPOST=false` and must
remain false: the feature is incomplete and has no deployable server integration.

**This is not yet a deployable server mod.** Do not upload these files or add a
`libzanikqol.so` preload to Pterodactyl yet. No server hook or Linux library has
been produced. The Linux executable and matching `.sym` file are in
`reference/linux-server/`. The `.sym` file maps function names to addresses;
it is a crash-symbol map, not the full `.debug` file named in the ELF. The
relevant methods have been located, but their call behavior, item
identity/charge representation, inventory order, and authoritative equip
operation have not yet been verified. The Windows object dump shows
`HeldContainerEquipmentItem.TryDispenseCharge`,
`InventoryController`, `InventoryComponent`, and `PlayerEquipmentComponent`,
but it cannot establish their Linux addresses or that the same path runs on
the dedicated server.

Once verified, the adapter must run on the server game thread, observe a
successful transition of the equipped compost bucket to empty, snapshot the
player's personal inventory in slot order, call `next_compost_bucket`, and use
the game's normal authoritative equip operation for the selected instance.
It must validate the game build and required objects before registering the
hook and disable the feature with a console message when validation fails.

Run `make -C qol test` on a Linux system with `g++` to check the selection
logic. This does not test game integration.

`tools/find_sym.py` queries the matching `.sym` file. Symbol addresses are
relative to a 0x200000 image base; ELF virtual addresses add 0x200000. This was
checked against the ELF `.text` section start.

The Windows object dump identifies the content assets
`/Game/Gameplay/LiquidsAndOtherContent/CC_Compost_Regular` and
`/Game/Gameplay/LiquidsAndOtherContent/CC_Compost_Super`; these are distinct
from the bucket equipment asset
`/Game/Gameplay/Character/Player/Equipment/Held/Bucket/ITEM_Bucket_Compost`.
The Linux symbol map shows the compost spell's removal module calls
`UContainerFunctionLibrary::DispenseAmountFromContentHeldByPlayer`, which
receives the player and content asset and is a candidate for a post-use hook.
That candidate has not been proven against a live server. The candidate's
`.sym` address is `0xa6b7ad0` (`0xa6d7ad0` ELF virtual address) for the
current reference executable only.

**Loading constraint:** the panel currently exposes only an `Extra arguments`
startup field. It cannot set `LD_PRELOAD`, which is an environment variable
that must be present before the game process starts. A standalone native `.so`
will require the panel administrator to expose an environment or startup
setting. Do not place `LD_PRELOAD=...` into `Extra arguments`; that would pass
it to the game as text rather than load the library.
