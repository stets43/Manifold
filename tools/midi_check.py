#!/usr/bin/env python3
"""Checks a host run's midi_out.txt: every note-on has its note-off (per link, channel, note), no note is started twice
while it sounds, and how many new notes a second each channel sends at most."""
import sys, re, collections
openn = collections.Counter(); dup = 0; ons = collections.defaultdict(list); last_t = 0
for line in open(sys.argv[1]):
    m = re.match(r'(\d+) ms: ([0-9A-F]{2}) ([0-9A-F]{2}) ([0-9A-F]{2}) ->(.*)', line)
    if not m: continue
    t = int(m.group(1)); st, d1, d2 = (int(m.group(i), 16) for i in (2, 3, 4)); links = m.group(5).split(); last_t = t
    ch = st & 15
    for l in links:
        if st >> 4 == 9 and d2:
            if openn[(l, ch, d1)]: dup += 1
            openn[(l, ch, d1)] += 1; ons[(l, ch)].append(t)
        elif st >> 4 == 8 or (st >> 4 == 9 and not d2):
            if openn[(l, ch, d1)]: openn[(l, ch, d1)] -= 1
        elif st >> 4 == 11 and d1 == 123:
            for k in list(openn):
                if k[0] == l and k[1] == ch: openn[k] = 0
left = {k: v for k, v in openn.items() if v}
print(f"notes still sounding at the end: {len(left)} {sorted(left)[:8]}")
print(f"note-ons on a note already sounding: {dup}")
for k, ts in sorted(ons.items()):
    peak = 0; j = 0
    for i, t in enumerate(ts):
        while ts[j] <= t - 1000: j += 1
        peak = max(peak, i - j + 1)
    print(f"  {k[0]} ch {k[1]+1}: {len(ts)} note-ons, at most {peak} in a second")
