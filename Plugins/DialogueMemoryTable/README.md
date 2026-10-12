# Dialogue Memory Table

Runtime UE plugin that migrates the HTML demo's non-visual gameplay systems into UE:
three-layer NPC personas, DeepSeek-compatible chat requests, 22 major cards, four court
modifiers, nine commissions, vector evaluation, world-consequence prompt construction,
and compact dialogue-memory SaveGame tables.

The plugin also provides `DailyMushroomSubsystem`: a saved world-day counter and
deterministic daily availability/`FAlchemyVector` values for the existing mushroom
types. The subsystem is Blueprint-callable; the player's harvest Blueprint must call
`IsMushroomAvailable` and `GetMushroomValue` before hiding a mushroom.

## Install

Copy this folder to:

<YourProject>/Plugins/DialogueMemoryTable

Regenerate project files if the project uses C++, build the Editor target, then enable Dialogue Memory Table if UE does not enable it automatically.

## Blueprint flow

1. Get Game Instance Subsystem → DialogueMemoryTableSubsystem.
2. Before AI memory writes, call Set Authoritative NPC Progress with the real familiarity and secret flags.
3. At conversation end, call Apply Memory Patch JSON.
4. Pass the expected NPC ID, session ID, world day, and the raw JSON returned by the summarizer.
5. Check bSuccess, ErrorCode, and ErrorMessage.
6. Use Get Ledger Snapshot to inspect the four tables.
7. Use Export Ledger To CSV for debug spreadsheets.

### NPC three-layer dialogue flow

1. Get AlchemyGameplaySubsystem.
2. Read GetPersonaProfile for blacksmith, operator, or novelist.
3. After each player message, call AdvancePersonaFromPlayerText. It checks explicit
   persona phrases first, then stage-2 combinations of a commission topic and a
   natural follow-up intent. It advances familiarity in the authoritative SaveGame
   and never moves it backward. EvaluatePersonaTrigger is available when the caller
   wants a non-persistent preview.
4. Pass the returned level, compact memory JSON, commission order, and the last eight
   messages to BuildPersonaSystemPrompt.
5. Get DialogueAIServiceSubsystem, configure the DeepSeek gateway, then call
   RequestNpcDialogue.
6. At session end, send the JSON patch to ApplyMemoryPatchJson.

The three levels are compiled into the plugin and include each persona's voice, daily
cover identity, secret past, level summaries, level prompts, explicit trigger phrases,
stage-2 topic/intent pairs, and the no-regression rule.

### Complete non-visual commission gameplay

- GetMajorCards exposes all 22 major cards.
- GetCourtCards exposes Knight, Prince, Queen, and Princess modifiers.
- GetCommissions exposes all nine HTML commissions. Their CustomerOrderIndex maps the
  first, second, and third commission for each NPC to world-day slots 1, 2, and 3.
  Forest NPC dialogue resolves the current world day to that slot when it builds the
  AI prompt.
- EvaluateAlchemySequence reproduces the elemental vector operations and verdicts:
  Magnum Opus, deliverable, under-dosed, side-effect, over-dosed, reversed effect,
  empty bottle, caput mortuum, and contract breach.
- RequestWorldConsequence builds the outcome-sensitive next-day story request.

The HTML canvas trajectory renderer is intentionally not included.

The default save slot is DialogueMemoryLedger, stored by Unreal under Saved/SaveGames. Configure Save Slot can select another slot before loading or saving.

## Data ownership

- AI can suggest relationship summaries, facts, reports, inferences, and open loops.
- AI cannot write game_event authority.
- Familiarity and secret flags are set only by gameplay through SetAuthoritativeNpcProgress.
- Gameplay world facts are written through UpsertAuthoritativeWorldState.
- Duplicate or stale session patches are rejected or treated idempotently.

## Tables

- NPC state: one row per NPC.
- Character facts: up to 24 rows per NPC.
- World state: up to 64 rows globally.
- Open loops: up to 8 rows per NPC.

These are runtime USTRUCT rows in a SaveGame, not mutable UDataTable assets. This works in packaged builds and avoids asset writes at runtime.

The exact prompt and JSON schema are included in Resources.

### World day and mushrooms

- `GetCurrentDay`, `AdvanceDay`, and `SetCurrentDay` manage the saved day in the
  `WorldDayState` slot.
- `GetDailyMushrooms` returns all known types with that day's availability and value.
- `GetDailyMushroom`, `IsMushroomAvailable`, and `GetMushroomValue` query one type.
- Each mushroom type has a fixed fire/wind/water element. The day changes its local
  random pip value from 1 to 9, availability, and weight. The seeded random stream
  makes the row stable for a given day and type, so loading the same day reproduces
  the same harvest rules without LLM.
- `FDailyElementModifierRow` is the placeholder table for score-driven next-day
  consequences. `GetDailyElementModifierRuleTemplates` exposes block/reduce/bonus
  condition slots without assuming score thresholds. `AddElementModifier` persists
  a resolved row for a target day; `GetElementModifiersForDay` reads it back. These
  rows are not applied to mushroom generation until the scoring rules are defined.
- Call `ConfirmDryPotSynthesis` once after the dry-pot synthesis result is confirmed
  to enter the next day. `AdvanceDay` remains available for tests and debug flows.

Commission narrative exposes `ElementRequest` and prompt assembly describes fire,
wind, and water properties. Internal card definitions remain available to the
calculator, while NPC speech and player-facing task wording use element language.

## Package check

Run the dependency-free check from the plugin root:

    python3 Tools/validate_package.py

It verifies the descriptor, example JSON, runtime contract, and C++ delimiter
structure. It does not replace compiling the plugin with Unreal Build Tool in
the target project.

## Forest NPC integration (2026-10-07)

Custom three-layer profiles can be installed with RegisterPersonaProfile. Memory patches validate against registered profiles, including the original three personas. MagicForest registers forest_witch, natta and fawnia and owns the dialogue UI, request lifecycle and conversation saves. See [integration and test report](../../Docs/NPC_Dialogue_Integration_2026-10-07.md).

On world day 1, Fawnia leaves her patrol, finds the player, and opens the first
conversation with the fixed tutorial message. The delivery flag lives in the
conversation SaveGame, so loading the level does not replay the introduction.
