# SPDX-License-Identifier: MPL-2.0
"""The pattern corpus: the genre knowledge the pattern rolls are trained on (train.py).

Each entry is one kit pattern of a genre: the skeleton a drummer or a drum machine of that style plays, written in
16ths, one bar (16 steps) or two ('|' between the bars). 'x' a hit, 'o' a ghost note (soft), '.' nothing.

    K kick, S snare or clap, H hat, T a fourth part (an open hat, a shaker, a rim, congas, a tom fill), or None: that
    drum stays on the linear rhythm (MANIFOLD's own drum choice from the stream)

and its tempo range (BPM), swing range (0 straight .. 1 hard; 0.67 the triplet), the level the swing works at (16:
the 16ths, 8: the 8ths) and a weight (how typical: how often the roll starts from it).

These are skeletons in the manner of each genre and of well-known breaks, simplified to four parts, not
transcriptions of recordings. MANIFOLD plays them without a grid: swing and the stream bend the timing, Density thins
them, the linear layer adds its ghosts, the long form brings fills.
"""

H, B, D, P = "house", "breakbeat", "dnb", "hiphop"   # (drum'n'bass: a breakbeat at its own tempo)

def E(genre, name, bpm, sw, lev, K, S, H_, T=None, w=1.0):
    return dict(genre=genre, name=name, bpm=bpm, sw=sw, lev=lev, K=K, S=S, H=H_, T=T, w=w)

FOUR = "x...x...x...x..."
BACK = "....x.......x..."
OFF = "..x...x...x...x."
EIGHT = "x.x.x.x.x.x.x.x."
SIXT = "xxxxxxxxxxxxxxxx"

CORPUS = [
    # ---------------- house and techno: the kick on every beat ----------------
    E(H, "classic house", (120, 126), (0, .12), 16, FOUR, BACK, OFF, w=3),
    E(H, "house, 16th hats, open off-beat", (121, 127), (0, .15), 16, FOUR, BACK, SIXT, OFF, w=3),
    E(H, "deep house shuffle", (114, 122), (.35, .6), 16, FOUR, BACK, "x.xxx.xxx.xxx.xx", OFF, w=2),
    E(H, "house, a pickup into the bar", (122, 126), (0, .2), 16, "x...x...x...x..x", BACK, OFF, w=1.5),
    E(H, "house, an and-of-4 kick", (120, 125), (0, .2), 16, "x...x...x...x.x.", BACK, OFF, SIXT, w=1),
    E(H, "Chicago jack", (122, 128), (.05, .25), 16, FOUR, "....x...o...x...", EIGHT, OFF, w=1.5),
    E(H, "acid house, 3-3-2 percussion", (120, 128), (0, .15), 16, FOUR, BACK, OFF, "x..x..x...x..x..", w=1.5),
    E(H, "tech house", (123, 127), (.05, .2), 16, FOUR, BACK, OFF, "...x..x....x..x.", w=2),
    E(H, "disco house, a 16th shaker", (118, 124), (.1, .25), 16, FOUR, BACK, OFF, SIXT, w=1.5),
    E(H, "French filter house", (120, 126), (0, .1), 16, FOUR, BACK, SIXT, w=1),
    E(H, "lo-fi house", (116, 122), (.2, .4), 16, FOUR, "....x.......x..o", OFF, w=1),
    E(H, "afro house", (118, 124), (.1, .3), 16, FOUR, BACK, OFF, "x..x..x...x.x...", w=1.5),
    E(H, "latin house, congas", (122, 126), (.05, .2), 16, FOUR, BACK, OFF, "......x...x..x..", w=1),
    E(H, "progressive house", (124, 128), (0, .1), 16, FOUR, BACK, EIGHT, OFF, w=1),
    E(H, "electro house", (126, 130), (0, .1), 16, FOUR, BACK, EIGHT, "...x...x...x...x", w=1),
    E(H, "Detroit techno", (124, 132), (0, .15), 16, FOUR, BACK, SIXT, "..x..x....x..x..", w=1.5),
    E(H, "techno, no snare", (128, 136), (0, .05), 16, FOUR, "................", OFF, SIXT, w=1.5),
    E(H, "techno rumble", (128, 136), (0, .05), 16, "x..ox..ox..ox..o", BACK, OFF, w=1.5),
    E(H, "hard techno", (135, 145), (0, .05), 16, FOUR, "................", SIXT, BACK, w=1),
    E(H, "dub techno, sparse", (118, 126), (0, .1), 16, FOUR, "................", OFF, ".......x.......x", w=1),
    E(H, "minimal, ghosted clap", (122, 128), (.05, .2), 16, FOUR, "....x.......x..o", "..xx..x...xx..x.", w=1),
    E(H, "house, doubled clap", (122, 126), (0, .15), 16, FOUR, "....x.......x.x.", OFF, w=1),
    E(H, "broken house kick", (122, 126), (.05, .2), 16, "x...x...x..xx...", BACK, OFF, w=1),
    E(H, "house, the fourth bar drops", (120, 126), (0, .15), 16, FOUR + "|x...x...x.......", BACK, OFF, w=1),
    E(H, "house, call and response", (122, 126), (0, .15), 16, FOUR, BACK, SIXT, "..x...x...x...x.|..x...x.x.x...x.", w=1),
    E(H, "shuffle house, 8th swing", (118, 124), (.5, .7), 8, FOUR, BACK, EIGHT, OFF, w=1),
    E(H, "tribal techno", (126, 134), (0, .1), 16, FOUR, "................", OFF, "x.x..x.x..x.x..x", w=1),
    E(H, "UK garage, 2-step", (130, 136), (.5, .7), 16, "x.....x...x.....", BACK, "..x.x.x...x.x.x.", w=1.5),
    E(H, "speed garage", (128, 134), (.3, .55), 16, "x...x...x..x..x.", BACK, OFF, SIXT, w=1),
    E(H, "house, rim on the and-of-2", (122, 126), (0, .15), 16, FOUR, BACK, OFF, "......x.......x.", w=1),
    E(H, "warehouse", (124, 130), (0, .1), 16, FOUR, "....x...o...x...", "x.xxx.xxx.xxx.xx", OFF, w=1),
    E(H, "techno, offbeat clap", (128, 134), (0, .05), 16, FOUR, "..x...x...x...x.", SIXT, w=.7),
    E(H, "jackin' house", (124, 128), (.2, .35), 16, FOUR, "....x..o....x...", "x.x.x.xxx.x.x.xx", OFF, w=1),
    E(H, "deep house, 2-bar kick", (116, 122), (.3, .5), 16, FOUR + "|x...x...x...x.x.", BACK, "x.xxx.xxx.xxx.xx", w=1),
    E(H, "Balearic house", (112, 120), (.15, .3), 16, FOUR, BACK, OFF, "x..x..x.x..x..x.", w=.8),
    E(H, "acid techno", (132, 140), (0, .05), 16, FOUR, BACK, SIXT, OFF, w=1),

    # ---------------- breakbeat: the classic breaks and their kin ----------------
    E(B, "the six-second break", (132, 140), (0, .1), 16, "x.x.......xx....|..xx......x.....", "....x..o.o..x..o|o...x..o.o....x.", EIGHT, w=3),
    E(B, "funky drummer kind", (98, 108), (.15, .35), 16, "x.x...x...x..x..", "....x..o.o.ox..o", "xxxxxxxoxxxxxxxx", w=2),
    E(B, "think-style break", (112, 120), (.05, .2), 16, "x.........x.x...", "....x..o.o..x...", EIGHT, w=2),
    E(B, "apache-style break", (110, 118), (.1, .25), 16, "x.....x...x.....", "....x.......x..o", SIXT, "..........x.x..x", w=1.5),
    E(B, "impeach-style break", (94, 102), (.1, .25), 16, "x......xx.......", "....x.......x...", EIGHT, w=1.5),
    E(B, "skull-snaps kind", (96, 104), (.1, .3), 16, "x.x.......x.....", "....x..o....x..o", EIGHT, w=1.5),
    E(B, "big beat", (125, 135), (0, .15), 16, "x...x.x...x.....", BACK, EIGHT, w=2),
    E(B, "big beat, double kick", (128, 136), (0, .1), 16, "xx......x.x.....", "....x.......x.o.", SIXT, w=1.5),
    E(B, "breaks, the kick before 3", (128, 138), (0, .15), 16, "x.x......x......", BACK, EIGHT, w=1.5),
    E(B, "nu-skool breaks", (130, 138), (0, .1), 16, "x..x......x.....", "....x..o.o..x...", "x.xxx.xxx.xxx.xx", w=1.5),
    E(B, "breaks, syncopated", (128, 136), (.05, .2), 16, "x.x.......xx....", BACK, "xxx.xxx.xxx.xxx.", w=1.5),
    E(B, "hot-pants kind", (100, 110), (.15, .3), 16, "x......x.x......", ".o..x..o.o..x...", SIXT, w=1),
    E(B, "cold-sweat kind", (104, 112), (.1, .25), 16, "x.....x.x.......", "....x.......x.o.", EIGHT, ".......x.......x", w=1),
    E(B, "the levee kind", (70, 78), (.05, .2), 16, "xx......x.x.....", "....x.......x...", EIGHT, w=1),
    E(B, "breaks, 2-bar answer", (130, 138), (0, .15), 16, "x.x.......x.....|x.x....x..x.....", "....x..o....x..o|....x..o.o..x...", EIGHT, w=1.5),
    E(B, "breaks, open hat lift", (128, 136), (0, .1), 16, "x.....x...x.....", BACK, "x.x.x.x.x.x.x.x.", "..............x.", w=1),
    E(B, "breaks, ghosted snare", (126, 134), (.05, .2), 16, "x.......xx......", "....x..o.o..x..o", "x.x.x.x.x.x.x.xx", w=1.5),
    E(B, "the long break", (120, 128), (.1, .25), 16, "x.x...x...x..x..|x.x.......x.....", "....x..o.o.ox..o|....x..o....x...", "xxxxxxxoxxxxxxxx", w=1),
    E(B, "breakbeat house", (126, 132), (0, .15), 16, "x...x.x...x.....", BACK, OFF, SIXT, w=1),
    E(B, "funk break, 16th hats", (100, 112), (.2, .4), 16, "x..x...x..x.....", ".o..x..o.o..x..o", SIXT, w=1),
    E(B, "breaks, tom fill", (128, 136), (0, .1), 16, "x.x.......x.....", BACK, EIGHT, "............x.xx", w=1),
    E(B, "electro breaks", (126, 134), (0, .05), 16, "x.....x...x.....", "....x.......x...", SIXT, w=1),
    E(B, "the substitution kind", (96, 104), (.1, .3), 16, "x.x.......x..x..", "....x..o....x...", EIGHT, w=1),
    E(B, "breaks, broken hats", (130, 138), (0, .1), 16, "x.x.......xx....", "....x..o.o..x..o", "x.x.x.xxx.x.x.x.", w=1),
    # drum'n'bass and jungle: the same breaks, near 170
    E(D, "two-step", (168, 176), (0, .05), 16, "x.........x.....", BACK, EIGHT, w=2),
    E(D, "amen jungle", (162, 172), (0, .1), 16, "x.x.......xx....|..xx......x.....", "....x..o.o..x..o|o...x..o.o....x.", EIGHT, w=2),
    E(D, "techstep", (170, 176), (0, .05), 16, "x.........x.....", BACK, OFF, SIXT, w=1),
    E(D, "liquid", (170, 176), (0, .1), 16, "x.........x.....", "....x..o....x..o", SIXT, w=1),
    E(D, "jungle, ghosts", (160, 170), (0, .1), 16, "x.x.......x.....", ".o..x..o.o..x..o", "x.xxx.xxx.xxx.xx", w=1),
    E(D, "neurofunk", (172, 178), (0, .05), 16, "x.........xx....", BACK, EIGHT, w=1),
    E(D, "drumfunk", (164, 172), (.05, .15), 16, "x.x....x..x.....", "....x..o.o..x.o.", SIXT, w=.8),
    E(D, "halftime", (170, 176), (0, .05), 16, "x.........x.....", "........x.......", EIGHT, w=.8),

    # ---------------- hip-hop: boom bap and its family, heavily swung ----------------
    E(P, "boom bap", (86, 94), (.5, .75), 16, "x......x..x.....", BACK, EIGHT, w=3),
    E(P, "boom bap, and-of-2 kick", (84, 92), (.5, .75), 16, "x.....x...x.....", BACK, EIGHT, w=3),
    E(P, "boom bap, the 'e' of 1", (86, 94), (.55, .8), 16, "x..x......x.....", BACK, SIXT, w=2),
    E(P, "dusty loop", (82, 90), (.6, .85), 16, "x......xx.....x.", "....x.......x..o", EIGHT, w=2),
    E(P, "East Coast", (88, 96), (.5, .7), 16, "x.x.......x..x..", BACK, "x.xxx.xxx.xxx.xx", w=2),
    E(P, "the kick after 2", (84, 92), (.55, .8), 16, "x......x.x......", BACK, EIGHT, w=1.5),
    E(P, "three kicks", (86, 94), (.5, .75), 16, "x.....x.x.x.....", BACK, "x.x.x.xxx.x.x.x.", w=1.5),
    E(P, "doubled kick", (88, 96), (.45, .7), 16, "xx........x..x..", BACK, EIGHT, w=1.5),
    E(P, "late kick", (80, 90), (.6, .85), 16, "x.......x.x....x", "....x..o....x...", SIXT, w=1.5),
    E(P, "lo-fi", (70, 82), (.65, .9), 16, "x..x...x..x.....", "....x.......x...", "x.x.x.x.x.x.x.x.", w=2),
    E(P, "lo-fi, lazy", (64, 76), (.7, .9), 16, "x......x...x.x..", BACK, "x...x...x...x...", w=1.5),
    E(P, "jazzy", (84, 92), (.6, .8), 16, "x.x....x..x.....", "....x.......x.o.", "x.xxx.xxx.xxx.xx", w=1.5),
    E(P, "ghosted snare", (86, 94), (.5, .75), 16, "x......x..x.....", "....x..o....x..o", EIGHT, w=1.5),
    E(P, "rim on the and", (84, 92), (.55, .75), 16, "x.....x...x.....", BACK, EIGHT, "......x.......x.", w=1),
    E(P, "2-bar boom bap", (86, 94), (.5, .75), 16, "x......x..x.....|x.x....x..x..x..", BACK, EIGHT, w=2),
    E(P, "2-bar, the turnaround", (84, 92), (.55, .8), 16, "x.....x...x.....|x.....x.x...x.x.", "....x.......x...|....x.......x.o.", SIXT, w=1.5),
    E(P, "West Coast bounce", (90, 98), (.35, .55), 16, "x.....x...x..x..", BACK, SIXT, w=1.5),
    E(P, "G-funk", (88, 96), (.4, .6), 16, "x......x..x...x.", BACK, "xxxxxxxoxxxxxxxx", "..............x.", w=1),
    E(P, "the and-of-4 kick", (86, 94), (.5, .7), 16, "x.........x...x.", BACK, EIGHT, w=1),
    E(P, "Dilla-ish, drunk", (82, 90), (.75, .95), 16, "x..x...x..x...x.", "....x...o...x...", "x.xxx.xxx.xxx.xx", w=1.5),
    E(P, "sparse head-nod", (78, 88), (.55, .8), 16, "x.........x.....", BACK, "x...x...x...x...", w=1),
    E(P, "shaker groove", (86, 94), (.55, .75), 16, "x......x..x.....", BACK, EIGHT, "..x...x...x...x.", w=1),
    E(P, "rolling kicks", (84, 92), (.5, .7), 16, "x.x....x.x......", "....x.......x..o", EIGHT, w=1),
    E(P, "16th hats, syncopated kick", (88, 96), (.45, .7), 16, "x..x...x..x.....", BACK, SIXT, w=1),
    E(P, "underground", (90, 96), (.45, .65), 16, "x.......xx......", "....x..o....x...", "x.x.x.x.x.x.x.xx", w=1),
    E(P, "backpack", (88, 94), (.5, .7), 16, "x......x..x.x...", BACK, EIGHT, w=1),
    E(P, "trap-ish halftime", (68, 76), (.1, .3), 16, "x......x..x.....", "........x.......", SIXT, w=1),
    E(P, "kick on 3-and", (84, 92), (.55, .75), 16, "x.........x.x...", BACK, EIGHT, w=1),
    E(P, "swung 16ths", (86, 94), (.65, .9), 16, "x......x..x.....", BACK, "xoxoxoxoxoxoxoxo", w=1),
    E(P, "two-bar lo-fi", (72, 82), (.65, .9), 16, "x..x...x..x.....|x......x...x.x..", "....x.......x...|....x..o....x..o", EIGHT, w=1),
    E(P, "the stop", (86, 94), (.5, .75), 16, "x......x..x.....", BACK, "x.x.x.x.x.x.....", w=.8),
    E(P, "conga loop", (88, 96), (.5, .7), 16, "x.....x...x.....", BACK, EIGHT, "......x...x..x..", w=.8),
    E(P, "the and-of-1 kick", (86, 94), (.5, .75), 16, "x.x.......x.....", BACK, SIXT, w=1),
    E(P, "heavy, slow", (66, 76), (.6, .85), 16, "x......x..x...x.", "....x.......x...", "x.x.x.x.x.x.x.x.", w=1),
]

# (1.0.8) the families (families.py): Funk, Latin and Indie, and the first genres grown, each to a hundred entries or more
from families import FAMILIES, expand
for _f in FAMILIES:
    CORPUS.extend(expand(_f))

