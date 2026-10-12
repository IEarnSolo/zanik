#!/usr/bin/env python3
"""Inspect Dragonwilds SPUD saves for resource state without modifying them."""

import argparse, json, struct
from pathlib import Path


class Reader:
    def __init__(self, data, pos=0, end=None):
        self.data, self.pos = data, pos
        self.end = len(data) if end is None else end
    def take(self, n):
        if n < 0 or self.pos + n > self.end: raise ValueError(f"read past boundary at {self.pos:#x} + {n:#x} > {self.end:#x}")
        b = self.data[self.pos:self.pos+n]; self.pos += n; return b
    def u16(self): return struct.unpack('<H', self.take(2))[0]
    def u32(self): return struct.unpack('<I', self.take(4))[0]
    def i32(self): return struct.unpack('<i', self.take(4))[0]
    def i64(self): return struct.unpack('<q', self.take(8))[0]
    def fstring(self):
        n = self.i32()
        if n == 0: return ''
        if n > 0: return self.take(n).rstrip(b'\0').decode('utf-8', 'replace')
        return self.take(-n * 2).decode('utf-16le', 'replace').rstrip('\0')
    def chunk(self):
        start = self.pos; magic = self.take(4).decode('ascii', 'replace'); length = self.u32()
        end = self.pos + length
        if end > self.end: raise ValueError(f"bad {magic} chunk at {start:#x}")
        return magic, Reader(self.data, self.pos, end), end


def chunks(r):
    while r.pos + 8 <= r.end:
        magic, child, end = r.chunk(); yield magic, child; r.pos = end


def string_array(r):
    n = r.i32()
    if n < 0 or n > 1000000: raise ValueError(f"bad string array count {n}")
    return [r.fstring() for _ in range(n)]


def parse_metadata(r):
    out = {'classes': [], 'properties': [], 'defs': []}
    for magic, c in chunks(r):
        if magic == 'CNIX': out['classes'] = string_array(c)
        elif magic == 'PNIX': out['properties'] = string_array(c)
        elif magic == 'CLST':
            for cm, d in chunks(c):
                # Dragonwilds' SPUD fork wraps each class definition in a
                # versioned CDVE chunk.  Its first byte is the class-def
                # version and the ordinary upstream CDEF follows it.
                if cm == 'CDVE':
                    if d.pos >= d.end:
                        continue
                    d.take(1)
                    if d.pos + 8 > d.end:
                        continue
                    cm, d, _ = d.chunk()
                if cm == 'CDEF':
                    name = d.fstring(); count = d.u16(); defs = []
                    for _ in range(count): defs.append((d.u32(), d.u32(), d.u16()))
                    out['defs'].append({'name': name, 'properties': defs})
    return out


def parse_prop(r):
    n = r.i32(); offsets = [r.u32() for _ in range(n)]
    size = r.i32(); data = r.take(size)
    return offsets, data


def parse_actor(r, version):
    class_id = r.u32() if version >= 4 else None
    name = r.fstring()
    # Dragonwilds' World Partition additions follow the actor name.
    if version >= 6:
        class_versions = r.i32()
        if class_versions < 0 or class_versions > 10000: raise ValueError(f'bad class-version count {class_versions}')
        r.take(class_versions * 4)
    if version != 6: r.take(8)
    props = None; custom = None; core = None
    for magic, c in chunks(r):
        if magic == 'PROP': props = parse_prop(c)
        elif magic == 'CUST': custom = c.take(c.end-c.pos)
        elif magic == 'CORA': core = c.take(c.end-c.pos)
    return {'class_id': class_id, 'name': name, 'props': props, 'custom': custom, 'core': core}


def decode_value(dtype, b):
    base = dtype & 0xfff
    if dtype & 0x1000:
        return {'array_bytes': b.hex(), 'storage_type': dtype}
    fmts = {0:'<B',1:'<H',2:'<I',3:'<Q',4:'<b',5:'<h',6:'<i',7:'<q',8:'<f',9:'<d'}
    if base in fmts and struct.calcsize(fmts[base]) <= len(b): return struct.unpack_from(fmts[base], b)[0]
    if base == 23 and len(b) >= 16: return b[:16].hex()
    return {'bytes': b.hex(), 'storage_type': dtype}


def properties_for_class(class_id, offsets, data, meta):
    if class_id is None or class_id >= len(meta['classes']): return {}
    class_name = meta['classes'][class_id]
    class_def = next((d for d in meta['defs'] if d['name'] == class_name), None)
    if not class_def: return {}
    defs = class_def['properties']; names = meta['properties']; out = {}
    for i, off in enumerate(offsets):
        if i >= len(defs) or off == 0xffffffff or off > len(data): continue
        later = [v for v in offsets[i+1:] if v != 0xffffffff and v >= off]
        end = min(later) if later else len(data)
        pid, prefix, dtype = defs[i]
        pn = names[pid] if pid < len(names) else f'#{pid}'
        px = '' if prefix == 0xffffffff else (names[prefix] if prefix < len(names) else f'#{prefix}') + '/'
        value = data[off:end]

        # A nested UObject first stores its uint32 class ID. Dragonwilds then
        # appends a uint8 ESpudObjectStoreFormat, a TArray<uint32> of offsets,
        # and a TArray<uint8> containing that object's properties.
        nested = None
        # Upstream SPUD uses UInt32 here; Dragonwilds' fork records nested
        # object references as storage type 40.
        if dtype in (2, 40) and len(value) >= 13:
            for has_format in (False, True):
                nr = Reader(value)
                nested_class_id = nr.u32()
                fmt = nr.take(1)[0] if has_format else 0
                if nested_class_id >= len(meta['classes']) or fmt != 0:
                    continue
                count = nr.i32()
                if 0 <= count <= 10000 and nr.pos + count * 4 + 4 <= nr.end:
                    nested_offsets = [nr.u32() for _ in range(count)]
                    size = nr.i32()
                    if 0 <= size <= nr.end - nr.pos:
                        nested_data = nr.take(size)
                        nested = {
                            'class': meta['classes'][nested_class_id],
                            'properties': properties_for_class(nested_class_id, nested_offsets, nested_data, meta),
                        }
                        break
        out[px+pn] = nested if nested is not None else decode_value(dtype, value)
    return out


def actor_properties(actor, meta):
    if not actor['props'] or actor['class_id'] is None: return {}
    offsets, data = actor['props']
    return properties_for_class(actor['class_id'], offsets, data, meta)


def parse_level(r, version):
    name = r.fstring()
    # Dragonwilds' SPUD fork stores the World Partition actor-container ID
    # after the level name (introduced in system version 4).
    if version >= 4: r.take(8)
    if version <= 6: r.take(8)
    meta = None; actors=[]
    for magic, c in chunks(r):
        if magic == 'META': meta = parse_metadata(c)
        elif magic == 'LATS':
            for am, a in chunks(c):
                if am == 'NOBJ': actors.append(parse_actor(a, version))
    return name, meta, actors


def main():
    ap=argparse.ArgumentParser(); ap.add_argument('save'); ap.add_argument('-o','--output'); args=ap.parse_args()
    data=Path(args.save).read_bytes(); root=Reader(data); magic, save, _=root.chunk()
    if magic != 'SAVE': raise SystemExit('not a SPUD SAVE')
    version=8; results=[]
    for magic, c in chunks(save):
        if magic == 'INFO':
            version=c.u16()
        elif magic == 'LVLS':
            for lm, l in chunks(c):
                if lm != 'LEVL': continue
                name, meta, actors=parse_level(l, version)
                if not meta: continue
                for a in actors:
                    props=actor_properties(a, meta)
                    wanted={k:v for k,v in props.items() if any(x in k for x in ('ResourcesAvailable','PersistedTimeOfLastResourceChange','ISMCGuid','/Index','TypeAndLocationID','SavedRedundantFoliageItems'))}
                    if 'InteractableFoliageManager' in a['name']:
                        wanted = props
                    if wanted or any(x in a['name'] for x in ('Spawner_', 'InteractableFoliageManager')):
                        results.append({'level':name,'class':meta['classes'][a['class_id']] if a['class_id'] < len(meta['classes']) else a['class_id'],'name':a['name'],'properties':wanted,'custom_bytes':len(a['custom'] or b'')})
    out={'system_version':version,'matching_actors':results}
    text=json.dumps(out, indent=2)
    if args.output: Path(args.output).write_text(text, encoding='utf-8')
    else: print(text)

if __name__ == '__main__': main()
