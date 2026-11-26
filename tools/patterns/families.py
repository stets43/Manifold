# SPDX-License-Identifier: MPL-2.0
"""The corpus's families (1.0.8): each a genre's era or sub-style written as the choices its drummers make, part by part.

A family lists, for each part (K kick or its kin, S snare / clap / rim / clave, H hat / shaker / cascara / guira,
T the fourth: an open hat, congas, a tamborim, a cowbell, a tom, or None to leave that drum to the stream), the lines a
player of that style reaches for, each with how typical it is. Any one line of a part goes with any line of the
others: that is what makes them one family (a James Brown kick under a James Brown snare under a James Brown hat).
`expand` draws n distinct combinations from a family (by weight; a fixed seed per family, so the corpus is the same
every run), each an entry of the corpus with the family's tempo, swing and the swing's level. A family's entries share
its weight: families are weighted, not entries, so a genre's eras or sub-styles come up equally often (or as set).

Lines are 16ths, one bar (16) or two (16|16); 'x' a hit, 'o' a ghost (soft), '.' nothing. The fast Afro-Cuban styles
(son, salsa, mambo, songo, rumba) are written a step to an 8th of their cut time: one line is the clave's two bars, and
their tempos are half the dancers' count. The 6/8 and 12/8 feels (bembe, the 6/8 ones) are written in 8ths over
quarters and played with triplet swing at the 8th level (0.62..0.72): each beat's two 8ths land on the first and third
triplet, the nearest the 16-step grid comes to three against two. These are skeletons in the manner of each style,
simplified to four parts, not transcriptions of recordings.
"""
import random, zlib

def F(genre, name, bpm, sw, lev, K, S, H, T, n=15, w=6.0):
    return dict(genre=genre, name=name, bpm=bpm, sw=sw, lev=lev, K=K, S=S, H=H, T=T, n=n, w=w)

def _opts(lst):
    return [(o, 1.0) if not isinstance(o, tuple) else o for o in lst]

def expand(fam):
    rng = random.Random(zlib.crc32(fam["name"].encode()))
    parts = [_opts(fam[k]) for k in "KSHT"]
    total = 1
    for p in parts:
        total *= len(p)
    n = min(fam["n"], total)
    seen, out, tries = set(), [], 0
    while len(out) < n and tries < 20000:
        tries += 1
        pick = []
        for p in parts:
            u = rng.random() * sum(wt for _, wt in p)
            for line, wt in p:
                u -= wt
                if u <= 0:
                    break
            pick.append(line)
        key = tuple(pick)
        if key in seen:
            continue
        seen.add(key)
        out.append(dict(genre=fam["genre"], fam=fam["name"], name=f"{fam['name']} {len(out) + 1}", bpm=fam["bpm"], sw=fam["sw"],
                        lev=fam["lev"], K=pick[0], S=pick[1], H=pick[2], T=pick[3], w=fam["w"] / n))
    assert len(out) == n, fam["name"]
    return out

FOUR = "x...x...x...x..."
BACK = "....x.......x..."
OFF = "..x...x...x...x."
EIGHT = "x.x.x.x.x.x.x.x."
SIXT = "xxxxxxxxxxxxxxxx"
SIXA = "x.xxx.xxx.xxx.xx"           # 16ths, the 'e' left out: a shuffle's lilt
QUAR = "x...x...x...x..."
HALF = "........x......."

HO, BR, DB, HH, FU, LA, IN = "house", "breakbeat", "dnb", "hiphop", "funk", "latin", "indie"

FAMILIES = [
    # ======================================= FUNK: six eras, equal ========================================
    F(FU, "James Brown funk", (100, 122), (0, .2), 16,
      K=[("x.x...x...x..x..", 2), "x..x..x...x.....", "x.x.......x..x..", "x......x..x..x..", "x.....x.x...x...",
         "x.x...x.x.....x.", "x..x...x..x.....", "x.x....x...x.x.."],
      S=[("....x..o.o.ox..o", 2), "....x..o..o.x..o", "o...x..o.o..x.o.", "..o.x..o.o..x...", ".o..x..o..o.x..o",
         "....x.o.o..ox...", "....x..o.o.ox..o|....x..o.o..x.o."],
      H=[("x.xxx.xxx.xxx.xx", 2), SIXT, EIGHT, "xoxoxoxoxoxoxoxo", "xxxxxxxoxxxxxxxx"],
      T=[(None, 2), "......x.......x.", "..............x.", QUAR, "...x.......x...."], n=18),
    F(FU, "New Orleans second-line funk", (88, 108), (.3, .6), 16,
      K=["x..x..x...x..x..", "x.....x.x..x....", "x..x....x.x..x..", "x.x..x....x..x..", "x......xx..x....",
         "x..x..x.x.....x."],
      S=["..x..x.o..x...x.", "o.x.ox.o..x.o.x.", "....x..o.ox..o.x", "..o.x.ox.o..x.o.", ".x..x..x.o..x..o",
         "o..xo..xo.x.o..x", "..x..x.x..x..x.x"],
      H=["x.x.xxx.x.x.xxx.", SIXA, EIGHT, "x.xxx.x.x.xxx.x."],
      T=["x..x..x...x.x...", (None, 1.5), "...x..x....x..x.", "x.x..x.x..x.x...", "x..x..x.x..x..x."], n=18),
    F(FU, "P-Funk", (94, 112), (.1, .3), 16,
      K=["x.....x...x..x..", "x..x....x.x.....", "x.....x.x.....x.", "x.x.....x..x....", "x......x.x......",
         "x..x..x...x.x..."],
      S=[(BACK, 2), "....x..o....x...", "....x.......x..o", "....x..o.o..x...", "....x.......xo.o"],
      H=[EIGHT, SIXA, SIXT, "x.x.x.x.x.x.x.xx"],
      T=[(None, 2), BACK, OFF, "..............x."], n=18),
    F(FU, "go-go", (92, 112), (.35, .6), 16,
      K=["x.....x...x.x...", "x......x..x...x.", "x..x......x.x...", "x.....x.x...x...", "x.......x.x..x.."],
      S=["....x..x....x...", "....x.......x..x", "....x..o....x.o.", "....x...o..ox...", "...ox...o..ox..."],
      H=[SIXA, SIXT, EIGHT, "x.x.xxx.x.x.xxx."],
      T=[("..x..x...xx.x...", 2), "x..x..x...x.x...", "o.xxo.x.o.xxo.x.", "..xx..x...xx..x.", "x..x..x...x..x.."], n=18),
    F(FU, "Minneapolis funk", (110, 130), (0, .1), 16,
      K=["x.....x...x.....", "x..x..x...x.....", "x.x...x...x.....", "x.....x.x.x.....", "x..x....x.x....."],
      S=[(BACK, 2), "....x.......x..x", "....x.....x.x...", "....x..x....x..."],
      H=[(SIXT, 2), EIGHT, SIXA, "xxx.xxx.xxx.xxx."],
      T=[(None, 2), "......x.......x.", "..........x.x.xx", "...x...x...x...x"], n=18),
    F(FU, "modern funk and neo-soul", (82, 110), (.15, .7), 16,
      K=["x......x..x.....", "x..x...x..x.....", "x.x....x...x....", "x.....xx..x..x..", "x........xx.....",
         "x......x.x...x.."],
      S=["....x..o....x...", "o...x..o.o..x..o", "....x...o..ox..o", "..o.x..o....x.o.", "....x.....o.x..."],
      H=[SIXA, "xoxoxoxoxoxoxoxo", EIGHT, "x.xxx.x.x.xxx.x."],
      T=[(None, 2), "..........x...x.", "......x.......x."], n=18),

    # ======================================= LATIN: seventeen styles, equal ================================
    # Afro-Cuban, in 8ths of cut time (one line the clave's two bars; the tempo half the count)
    F(LA, "son montuno", (82, 98), (0, .1), 16,
      K=["...x..x....x..x.", "...x...........x", "......x.......x.", "...x..x....x...."],
      S=[("x..x..x...x.x...", 2), "..x.x...x..x..x."],
      H=[EIGHT, "x.x.x.x.x.x.x.xx", "x.xx.x.xx.x.xx.x"],
      T=["ooxoooxxooxoooxx", "o.x.o.xxo.x.o.xx", "......xx......xx", "ooxo.oxxooxo.oxx"], n=7),
    F(LA, "salsa", (88, 104), (0, .08), 16,
      K=["...x..x....x..x.", "...x............|...........x....", "...x...x...x...x", "......x.......x."],
      S=[("x..x..x...x.x...", 2), ("..x.x...x..x..x.", 2), "x..x...x..x.x..."],
      H=["x.xx.x.xx.x.xx.x", EIGHT, "x.x.xx.xx.xx.x.x"],
      T=["ooxoooxxooxoooxx", "o.x.o.xxo.x.o.xx", "ooxo.oxxooxo.oxx"], n=7),
    F(LA, "mambo", (90, 108), (0, .08), 16,
      K=["...x..x....x..x.", "x..x..x...x..x..", "...x.......x...."],
      S=["x..x..x...x.x...", "..x.x...x..x..x."],
      H=["x.x.xx.xx.xx.x.x", "x.xx.x.xx.x.xx.x", EIGHT],
      T=["ooxoooxxooxoooxx", "..xx..x...xx..x.", "o.x.o.xxo.x.o.xx", "x.xx.x.xx.x.xx.x"], n=7),
    F(LA, "cha-cha-cha", (112, 128), (0, .08), 16,
      K=["x.......x.......", "x.....x.x.......", "x.......x...x.x."],
      S=["....x...x.......|x.....x.....x...", "x.....x.....x...|....x...x......."],
      H=[QUAR, SIXA, "x.xxx.xxx.xxx.xx"],
      T=["o.x.o.o.o.x.xx..", "............xx..", "o.x.o.x.o.x.o.xx", "x...o.x.x...o.xx"], n=7),
    F(LA, "songo", (90, 108), (0, .06), 16,
      K=["...x......x...x.", "...x.......x..x.", "......x...x...x."],
      S=["..x..x.x..x..x.x", "x..x...x..x.x...", "....x..x....x..x"],
      H=[EIGHT, "x.x.x.xxx.x.x.xx", SIXA],
      T=["oox.ooxxoox.ooxx", "x.xx.x..x.xx.x..", "ooxoooxxooxoooxx"], n=7),
    F(LA, "rumba guaguanco", (92, 112), (0, .1), 16,
      K=["......xx......xx", "......x.......x.", "...x..xx....x.xx"],
      S=[("x..x...x..x.x...", 2), "..x.x...x..x...x"],
      H=["x.xx.x.xx.x.xx.x", "x.x.xx.xx.xx.x.x"],
      T=["..x.x..x..xx.x..", "x...x.x.x...x.xx", "o.xx.o.xo.x.xo.x"], n=7),
    F(LA, "bembe (6/8)", (96, 118), (.62, .72), 8,
      K=["x...........x...", "x.......x.......", "x...x...x...x..."],
      S=["x.x.x.x...x.x.x.", "x.x.xx.x.x.x....", "x...x.x.x...x.x."],
      H=["x.x.x.x.x.x.x.x.", "x.xxx.xxx.xxx.xx"],
      T=["o.x.o.xxo.x.o.xx", "..x.x...x.x.x.x.", "x.o.x.o.x.o.x.xx"], n=7),
    # Brazil
    F(LA, "samba", (94, 108), (0, .15), 16,
      K=["o...x...o...x...", "o.o.x...o.o.x..x", "....x.......x...", "o...x..xo...x..x"],
      S=["xoxxxoxxxoxxxoxx", "xoxoxoxxxoxoxoxx", "x.xxx.xxx.xxx.xx"],
      H=["oxoxoxoxoxoxoxox", SIXT, "xoxxxoxxxoxxxoxx"],
      T=["x.x.x..x.x.x..x.|x.x.x..x.x..x.x.", "x.x..x.x.x.x..x.", "x..x.x.x..x.x..."], n=7),
    F(LA, "bossa nova", (104, 132), (0, .1), 16,
      K=["x..xx..xx..xx..x", "x..x....x..x....", "x..xx..x........"],
      S=["x..x..x...x..x..|..x..x...x..x...", "x..x..x...x..x..", "..x..x...x..x..."],
      H=[EIGHT, "xoxoxoxoxoxoxoxo", QUAR],
      T=[(None, 2), "......x.......x.", "..x...x...x...x."], n=7),
    F(LA, "baiao", (88, 110), (0, .1), 16,
      K=["x..x....x..x....", "x..x...xx..x...x", "x..x..x.x..x..x."],
      S=["....x.......x...", "..x...x...x...x.", "....x..x....x..x"],
      H=["x.xxx.xxx.xxx.xx", "xoxxxoxxxoxxxoxx", SIXT],
      T=["x..x..x.x..x..x.", (None, 1), "...x...x...x...x"], n=7),
    F(LA, "partido alto", (94, 110), (0, .15), 16,
      K=["x...x...x...x...", "o...x...o...x...", "x..x....x..x...."],
      S=["..x..x.x..x..x.x", "x..x..x.x..x..x.", ".x.x..x..x.x..x."],
      H=["xoxxxoxxxoxxxoxx", SIXT],
      T=["x.x..x.x.x.x..x.", "x.x.x..x.x.x..x.", (None, 1)], n=7),
    F(LA, "samba-reggae", (96, 112), (0, .1), 16,
      K=["....x.......x...|..x.....x...x..x", "x...x...x...x...", "o...x.o.o...x..x"],
      S=["..x..x...x..x.x.", "x..x..x...x..x..", "..x...x.x.x...x."],
      H=[SIXT, "x.xxx.xxx.xxx.xx"],
      T=["x.xx.x.xx.x.xx.x", "..x.x..xx.x.x..x", (None, 1)], n=7),
    # the Caribbean
    F(LA, "reggaeton (dembow)", (88, 100), (0, .05), 16,
      K=[(FOUR, 3), "x..xx...x..xx...", "x...x...x...x..x"],
      S=[("...x..x....x..x.", 3), "...x..x...x..x..", "...x..x....xx.x."],
      H=[EIGHT, SIXT, "x.xxx.xxx.xxx.xx"],
      T=[(None, 2), "..........x.x.xx", "...x...x...x...x", "x..x..x...x..x.."], n=7),
    F(LA, "merengue", (118, 140), (0, .08), 16,
      K=["x.......x...x.x.", "x...x..xx...x..x", "x.......x......."],
      S=["...x...x.x.x..xx", "..x...x.xx.x..xx", "....x..x....x.xx"],
      H=["x.xxx.xxx.xxx.xx", SIXT],
      T=["....x..x....x.xx", "o.x.o.xxo.x.o.xx", "..x...x...x..xx."], n=7),
    F(LA, "bachata", (120, 140), (0, .08), 16,
      K=["x.....x.x.......", "x.......x.....x.", "x..x....x..x...."],
      S=["x.x.x.x.x.x.x.x.", "x.xxx.x.x.xxx.x.", "..x...x...x...x."],
      H=["x.xxx.xxx.xxx.xx", "x...x.xxx...x.xx", QUAR],
      T=["......x.......xx", "....x.x.....x.x.", (None, 1)], n=7),
    F(LA, "cumbia", (86, 104), (0, .12), 16,
      K=["x.......x.......", "x.......x.....x.", "x..x....x..x...."],
      S=[(OFF, 2), "..x...x...x..xx.", "..x...x...x...xx"],
      H=["x.xxx.xxx.xxx.xx", "xoxxxoxxxoxxxoxx", SIXT],
      T=["....x..x....x..x", "o..x.o.xo..x.o.x", "..x...xx..x...xx"], n=7),
    # the River Plate
    F(LA, "tango and habanera", (96, 124), (0, .05), 16,
      K=["x..xx...x..xx...", "x..x.x..x..x.x..", "x...x...x...x..."],
      S=["....x.......x...", "x..x..x...x.x...", "...x.......x...."],
      H=[EIGHT, QUAR, "x.xx.x.xx.xx.x.x"],
      T=[(None, 2), "x..xx...x..xx...", "...xx......xx..."], n=7),

    # ======================================= INDIE: seven styles, equal ===================================
    F(IN, "indie, straight 8ths", (112, 150), (0, .05), 16,
      K=[("x.......x.x.....", 2), "x.....x.x.......", "x.x.....x.......", "x.......x.....x.", "x..x....x.x....."],
      S=[(BACK, 3), "....x.......x..x", "....x......xx..."],
      H=[(EIGHT, 3), SIXT, "x.x.x.x.x.x.x.xx"],
      T=[(None, 3), "x...............", "x...............|................"], n=15),
    F(IN, "motorik", (120, 150), (0, .03), 16,
      K=["x.....x.x.....x.", "x.x...x.x.x...x.", "x...x...x...x...", "x.....x.x.x....."],
      S=[(BACK, 3), "....x.......x..o"],
      H=[(EIGHT, 2), SIXT, "x.xxx.xxx.xxx.xx"],
      T=[(None, 2), BACK, OFF], n=15),
    F(IN, "dance-punk", (118, 132), (0, .15), 16,
      K=[(FOUR, 3), "x...x...x...x.x.", "x...x...x..xx..."],
      S=[(BACK, 2), "....x.......x..o", "....x..x....x..."],
      H=[SIXT, SIXA, "xxx.xxx.xxx.xxx."],
      T=[(OFF, 2), EIGHT, "x.x.x.x.x.x.x.x.|x.x.x.x.x.x.xxxx", None], n=15),
    F(IN, "post-punk", (128, 160), (0, .05), 16,
      K=["x.....x.x.......", "x.x...x...x.x...", "x...x...x...x...", "x.x.....x.x....."],
      S=[(BACK, 3), "....x..x....x...", "........x......."],
      H=[EIGHT, None, "x...x...x...x..."],
      T=["x.xxx.xxx.xxx.xx", "x.x.x.x.x.x.x.x.", "xx.xxx.xxx.xxx.x", "x..x..x.x..x..x."], n=15),
    F(IN, "garage rock", (130, 170), (0, .45), 8,
      K=["x.....x.x.......", "x..x..x.x.......", "x.x...x...x.....", "x.......x.x....."],
      S=[(BACK, 3), "....x..x....x...", "....x.......x.x."],
      H=[EIGHT, "x.x.x.x.x.x.xxxx", QUAR],
      T=[(None, 3), "x...............", "............x.xx"], n=15),
    F(IN, "dream pop and shoegaze", (66, 92), (0, .1), 16,
      K=["x.........x.....", "x......x..x.....", "x.x.......x.....", "x.........x..x.."],
      S=[(HALF, 3), "........x......o", "........x.....x."],
      H=[EIGHT, QUAR, "xoxoxoxoxoxoxoxo"],
      T=[(None, 2), BACK, "x...............|................"], n=15),
    F(IN, "punk", (150, 180), (0, .02), 16,
      K=["x.......x.x.....", "x...x...x...x...", "x.x.x.x.x.x.x.x.", "x..x.x..x..x.x.."],
      S=[(BACK, 2), OFF, "....x...x...x..."],
      H=[(EIGHT, 2), QUAR],
      T=[(None, 3), "x...............", "x.......x......."], n=15),

    # ============== the first genres, grown (1.0.8): families beside their hand-written entries ==============
    F(HO, "classic and Chicago house", (118, 128), (0, .25), 16,
      K=[(FOUR, 4), "x...x...x...x..x", "x...x...x..xx...", "x...x...x...x.x."],
      S=[(BACK, 3), "....x..o....x...", "....x.......x.x.", "....x.......x..o"],
      H=[(OFF, 3), SIXT, EIGHT, "x.xxx.xxx.xxx.xx"],
      T=[(None, 2), OFF, SIXT, "......x.......x.", "x..x..x...x..x.."], n=16, w=8),
    F(HO, "deep and lo-fi house", (112, 124), (.15, .55), 16,
      K=[(FOUR, 4), "x...x...x...x.x.", "x...x...x..x...."],
      S=[(BACK, 3), "....x.......x..o", "....x..o....x..."],
      H=["x.xxx.xxx.xxx.xx", OFF, "x.x.x.x.x.x.x.xx"],
      T=[(None, 2), OFF, "x..x..x.x..x..x.", "......x...x..x.."], n=16, w=8),
    F(HO, "tech house and minimal", (122, 130), (0, .2), 16,
      K=[(FOUR, 4), "x...x...x...x..x"],
      S=[(BACK, 2), "....x.......x..o", "..x...x...x...x.", "................"],
      H=[OFF, "..xx..x...xx..x.", SIXT],
      T=["...x..x....x..x.", "x..x..x...x..x..", "..x.......x.x...", None], n=16, w=8),
    F(HO, "techno", (126, 142), (0, .05), 16,
      K=[(FOUR, 4), "x..ox..ox..ox..o"],
      S=[("................", 2), BACK, OFF],
      H=[OFF, SIXT, "x.xxx.xxx.xxx.xx"],
      T=[SIXT, BACK, "x.x..x.x..x.x..x", "..x..x....x..x..", None], n=16, w=8),
    F(HO, "UK garage and 2-step", (128, 136), (.45, .7), 16,
      K=["x.....x...x.....", "x.........x..x..", "x......x..x.....", "x.....x.x.......|x.........x....."],
      S=[(BACK, 2), "....x.......x..o", "....x..o....x..."],
      H=["..x.x.x...x.x.x.", "x.xxx.xxx.xxx.xx", OFF],
      T=[(None, 2), OFF, "...x..x....x..x."], n=16, w=6),
    F(HO, "disco and nu-disco", (112, 124), (.05, .25), 16,
      K=[(FOUR, 3), "x...x...x...x..x"],
      S=[(BACK, 3), "....x..o....x..."],
      H=[SIXT, OFF, "xxx.xxx.xxx.xxx."],
      T=[(OFF, 2), SIXT, "x.x.x.x.x.x.x.x.", None], n=16, w=6),

    F(BR, "classic breaks", (96, 120), (.05, .3), 16,
      K=["x.x.......xx....", "x.........x.x...", "x.....x...x.....", "x......xx.......", "x.x.......x.....",
         "x.....x.x......."],
      S=["....x..o.o..x..o", "....x.......x..o", "....x..o....x...", ".o..x..o.o..x...", "....x..o.o.ox..o"],
      H=[EIGHT, SIXT, "x.xxx.xxx.xxx.xx"],
      T=[(None, 3), "..........x.x..x", ".......x.......x"], n=16, w=8),
    F(BR, "big beat", (122, 136), (0, .15), 16,
      K=["x...x.x...x.....", "xx......x.x.....", "x.x......x......", "x...x.x...x..x.."],
      S=[(BACK, 2), "....x.......x.o.", "....x..o....x..x"],
      H=[EIGHT, SIXT, "x.x.x.x.x.x.x.xx"],
      T=[(None, 2), OFF, "............x.xx"], n=16, w=8),
    F(BR, "nu-skool and electro breaks", (126, 138), (0, .12), 16,
      K=["x..x......x.....", "x.x.......xx....", "x.....x...x.....", "x..x....x.x....."],
      S=["....x..o.o..x...", BACK, "....x..o....x..o"],
      H=["x.xxx.xxx.xxx.xx", SIXT, EIGHT],
      T=[(None, 2), "..............x.", "...x...x...x...x"], n=16, w=8),
    F(DB, "jungle and drum'n'bass", (162, 178), (0, .1), 16,
      K=["x.........x.....", "x.x.......xx....", "x.........xx....", "x.x....x..x.....",
         "x.x.......xx....|..xx......x....."],
      S=[(BACK, 2), "....x..o.o..x..o", ".o..x..o.o..x..o", "....x..o....x..o"],
      H=[EIGHT, SIXT, "x.xxx.xxx.xxx.xx", OFF],
      T=[(None, 3), SIXT, "..........x.x..x"], n=20, w=8),

    F(HH, "boom bap", (84, 96), (.45, .8), 16,
      K=["x......x..x.....", "x.....x...x.....", "x..x......x.....", "x.x.......x..x..", "x......x.x......",
         "x.........x...x."],
      S=[(BACK, 3), "....x..o....x..o", "....x.......x.o."],
      H=[EIGHT, "x.xxx.xxx.xxx.xx", SIXT],
      T=[(None, 3), "......x.......x.", "..x...x...x...x."], n=16, w=8),
    F(HH, "lo-fi and Dilla", (64, 88), (.6, .95), 16,
      K=["x..x...x..x.....", "x......x...x.x..", "x..x...x..x...x.", "x.......x.x....x"],
      S=[BACK, "....x...o...x...", "....x.......x..o"],
      H=[EIGHT, QUAR, "x.xxx.xxx.xxx.xx"],
      T=[(None, 3), "..........x...x.", "x..x..x...x.x..."], n=16, w=8),
    F(HH, "West Coast and G-funk", (88, 100), (.3, .6), 16,
      K=["x.....x...x..x..", "x......x..x...x.", "x.....x.x.....x.", "x..x..x...x....."],
      S=[(BACK, 3), "....x..o....x..."],
      H=[SIXT, "xxxxxxxoxxxxxxxx", "x.xxx.xxx.xxx.xx"],
      T=[(None, 2), "..............x.", "......x.......x."], n=16, w=6),
    F(HH, "trap and drill (halftime)", (66, 76), (0, .3), 16,
      K=["x......x..x.....", "x.........x..x..", "x.....x....x....", "x......x...x.x.."],
      S=[(HALF, 3), "........x......o", "......x.x......."],
      H=[SIXT, "x.x.x.x.xxxxx.x.", "xxx.x.x.xxx.xxxx", "x.xxx.x.x.xxx.x."],
      T=[(None, 3), "..........x.x...", "x..x..x...x..x.."], n=16, w=6),
    F(HH, "Southern bounce and crunk", (90, 102), (.05, .35), 16,
      K=["x.....x...x.....", "x..x..x...x..x..", "x.x...x...x....."],
      S=[(BACK, 2), "....x.......x.x.", "..x.x...x.x.x..."],
      H=[EIGHT, SIXT],
      T=[(None, 2), OFF, "x..x..x.x..x..x."], n=16, w=6),
]
