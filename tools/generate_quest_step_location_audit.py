"""Generate the human- and machine-readable quest step marker audit."""
from __future__ import annotations

import json
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
STEPS = ROOT / 'web/quest-step-locations.json'
CATALOG = ROOT / 'web/quest-catalog.json'
ANALYSIS = ROOT / 'dist/quest-step-analysis.json'
OUTPUT_JSON = ROOT / 'dist/quest-step-location-audit.json'
OUTPUT_MD = ROOT / 'docs/quest-step-location-audit.md'


def marker_confidence(marker):
    evidence = marker.get('evidence', '')
    if evidence.startswith('exact direct objective-parent'):
        return 100, 'Direct cooked quest-graph parent link to an exact quest-owned location.'
    if evidence.startswith('exact quest ID and QuestLocationName'):
        return 98, 'Exact quest identity and cooked QuestLocationName relationship.'
    if evidence.startswith('reviewed objective relationship'):
        score = 96
        if 'order is free' in evidence or 'cannot identify which' in evidence:
            score = 88
        elif 'dungeon entrance' in evidence or 'overworld entrance' in evidence:
            score = 97
        return score, 'Reviewed relationship to a current-build identity; coordinates still come from extracted game placement data.'
    if evidence.startswith('exact quest-scoped'):
        return 96, 'Exact quest-scoped registry identity named by a cooked location operation.'
    if evidence.startswith('objective flow subtree'):
        return 94, 'Cooked objective-flow subtree identifies an interior whose overworld entrance was extracted independently.'
    if evidence.startswith('authored objective explicitly names exact'):
        return 93, 'Authored objective text exactly identifies a quest-scoped world-registry landmark.'
    if evidence.startswith('authored objective text names dungeon'):
        return 91, 'Authored objective names a dungeon with an extracted overworld entrance.'
    if evidence.startswith('quest-owned BP_QuestLocation explicitly named'):
        return 88, 'Objective wording matches a marker owned by the same quest asset.'
    if evidence.startswith('quest-scoped world-registry landmark'):
        return 86, 'Objective wording matches a landmark scoped to the same quest data.'
    if evidence.startswith('same authored objective-text family'):
        return 80, 'Destination is inherited from another objective state with identical normalized authored text.'
    return 65, 'Published evidence is valid but does not match a stronger recognized evidence class.'


def step_confidence(step):
    scores = []
    reasons = []
    for marker in step['markers']:
        score, reason = marker_confidence(marker)
        scores.append(score)
        if reason not in reasons:
            reasons.append(reason)
    score = min(scores) if scores else 0
    notes = []
    if step['destinationMode'] == 'area':
        score -= 5
        notes.append('The game authors an area rather than one exact interaction point.')
    if step['destinationMode'] == 'multiple':
        score -= 3
        notes.append('Several destinations are valid or runtime progress cannot identify which one remains.')
    if len(step['markers']) > 10:
        score -= 5
        notes.append('The state exposes a large candidate set, reducing route precision.')
    if step.get('semanticWarnings'):
        score = min(score, 70)
        notes.extend(step['semanticWarnings'])
    score = max(0, min(100, score))
    grade = 'High' if score >= 90 else 'Medium' if score >= 75 else 'Low'
    return score, grade, reasons + notes


def main():
    steps_doc = json.loads(STEPS.read_text(encoding='utf-8'))
    catalog_doc = json.loads(CATALOG.read_text(encoding='utf-8'))
    analysis_doc = json.loads(ANALYSIS.read_text(encoding='utf-8'))
    catalog = {q['id']: q for q in catalog_doc['quests']}
    analysis = {(q['id'], o['key']): o for q in analysis_doc['quests'] for o in q['objectives']}

    coordinate_uses = defaultdict(list)
    records = []
    for quest_steps in steps_doc['quests']:
        quest = catalog[quest_steps['id']]
        for ordinal, step in enumerate(quest_steps['steps'], 1):
            identity = f"{quest['id']}:{step['key']}"
            score, grade, rationale = step_confidence(step)
            internal = analysis.get((quest['id'], step['key']), {})
            markers = []
            for marker_index, marker in enumerate(step['markers'], 1):
                coordinate = (marker['map']['x'], marker['map']['y'])
                coordinate_uses[coordinate].append({
                    'stepId': identity,
                    'quest': quest['name'],
                    'objectiveKey': step['key'],
                    'objective': step['text'],
                    'marker': marker.get('name', ''),
                    'markerIndex': marker_index,
                })
                marker_score, marker_rationale = marker_confidence(marker)
                markers.append({**marker, 'confidenceScore': marker_score, 'confidenceRationale': marker_rationale})
            records.append({
                'stepId': identity,
                'questId': quest['id'],
                'quest': quest['name'],
                'questType': quest['type'],
                'questRegion': quest['region'],
                'questSubregion': quest.get('subregion', ''),
                'ordinal': ordinal,
                'objectiveKey': step['key'],
                'objective': step['text'],
                'destinationMode': step['destinationMode'],
                'destinationNote': step.get('destinationNote', ''),
                'reviewedRelationship': step.get('reviewedRelationship', False),
                'semanticWarnings': step.get('semanticWarnings', []),
                'locationOperations': internal.get('locationOperations', []),
                'markers': markers,
                'confidenceScore': score,
                'confidence': grade,
                'confidenceRationale': rationale,
            })

    shared = []
    for coordinate, uses in sorted(coordinate_uses.items()):
        distinct_steps = sorted({use['stepId'] for use in uses})
        if len(distinct_steps) < 2:
            continue
        shared.append({
            'map': {'x': coordinate[0], 'y': coordinate[1]},
            'distinctStepCount': len(distinct_steps),
            'uses': uses,
        })
    shared_lookup = {(row['map']['x'], row['map']['y']): index + 1 for index, row in enumerate(shared)}
    for record in records:
        for marker in record['markers']:
            marker['sharedCoordinateGroup'] = shared_lookup.get((marker['map']['x'], marker['map']['y']))

    summary = {
        'quests': len(steps_doc['quests']),
        'steps': len(records),
        'markers': sum(len(row['markers']) for row in records),
        'confidence': {grade: sum(row['confidence'] == grade for row in records) for grade in ('High', 'Medium', 'Low')},
        'sharedCoordinateGroups': len(shared),
        'stepsUsingSharedCoordinates': len({use['stepId'] for row in shared for use in row['uses']}),
    }
    OUTPUT_JSON.write_text(json.dumps({
        'schemaVersion': 1,
        'catalogFingerprint': steps_doc['catalogFingerprint'],
        'summary': summary,
        'steps': records,
        'sharedCoordinates': shared,
    }, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')

    lines = [
        '# Quest step location confidence audit', '',
        f"Catalog fingerprint: `{steps_doc['catalogFingerprint']}`", '',
        'This report lists every public quest objective state and explains how each map marker was determined. Coordinates are normalized map coordinates extracted from the current cooked game build. Wiki guides may inform manual relationship review, but never supply coordinates.', '',
        f"- {summary['quests']} quests; {summary['steps']} objective states; {summary['markers']} marker uses",
        f"- Confidence: {summary['confidence']['High']} High, {summary['confidence']['Medium']} Medium, {summary['confidence']['Low']} Low",
        f"- {summary['sharedCoordinateGroups']} exact shared-coordinate groups across {summary['stepsUsingSharedCoordinates']} objective states", '',
        'Confidence is an evidence-quality and route-precision assessment, not a claim of certainty. High means a direct/exact/reviewed current-build relationship. Medium generally means text-scoped matching, inherited counter-state evidence, an authored area, or several possible destinations. Low means the available relationship is weak enough to require further reverse engineering.', '',
        '## Every quest step', ''
    ]
    for quest_steps in steps_doc['quests']:
        quest = catalog[quest_steps['id']]
        lines.extend([f"### {quest['name']}", '', f"{quest['type'].title()} quest · {quest['region']}" + (f" / {quest.get('subregion')}" if quest.get('subregion') else ''), ''])
        for record in (row for row in records if row['questId'] == quest['id']):
            lines.extend([
                f"#### {record['ordinal']}. {record['objective']}", '',
                f"- Objective key: `{record['objectiveKey']}`",
                f"- Destination mode: `{record['destinationMode']}`",
                f"- Confidence: **{record['confidence']} ({record['confidenceScore']}/100)**",
                f"- Assessment: {' '.join(record['confidenceRationale'])}",
            ])
            if record['destinationNote']:
                lines.append(f"- Display note: {record['destinationNote']}")
            lines.append(f"- Reviewed relationship: **{'yes' if record['reviewedRelationship'] else 'no'}**")
            for warning in record['semanticWarnings']:
                lines.append(f"- Semantic warning: **{warning}**")
            if record['locationOperations']:
                operations = '; '.join(f"`{op['name']}` ({'revealed' if op.get('revealed') else 'hidden/removed'}, `{Path(op['source']).name}` / `{op['node']}`)" for op in record['locationOperations'])
                lines.append(f"- Cooked location operations: {operations}")
            else:
                lines.append('- Cooked location operations: none directly attributed; marker relationship comes from the evidence below.')
            for index, marker in enumerate(record['markers'], 1):
                shared_ref = f"; shared-coordinate group [S{marker['sharedCoordinateGroup']}](#s{marker['sharedCoordinateGroup']})" if marker['sharedCoordinateGroup'] else ''
                world = marker.get('world') or {}
                lines.extend([
                    f"- Marker {index}: **{marker.get('name') or '(unnamed)'}** (`{marker.get('kind', 'location')}`) — map `{marker['map']['x']}, {marker['map']['y']}`; world `{world.get('x')}, {world.get('y')}, {world.get('z')}`{shared_ref}",
                    f"  - Determination: {marker.get('evidence', '')}",
                    f"  - Source: `{marker.get('source', '')}`",
                    f"  - Marker confidence: {marker['confidenceScore']}/100 — {marker['confidenceRationale']}",
                ])
            lines.append('')

    lines.extend(['## Exact shared map coordinates', '', 'These groups use numerically identical normalized `x` and `y` values. Most are intentional: counter variants, repeated return objectives, an NPC and its colocated quest target, or several states within the same dungeon.', ''])
    for index, row in enumerate(shared, 1):
        lines.extend([f"### S{index}", '', f"Map coordinate: `{row['map']['x']}, {row['map']['y']}` · {row['distinctStepCount']} distinct objective states", ''])
        for use in row['uses']:
            lines.append(f"- **{use['quest']}** — `{use['objectiveKey']}`: {use['objective']} — marker `{use['marker']}`")
        lines.append('')
    OUTPUT_MD.write_text('\n'.join(lines), encoding='utf-8')
    print(json.dumps(summary))


if __name__ == '__main__':
    main()
