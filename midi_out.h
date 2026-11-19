/* MIDI out (1.0.6): the layers' notes, the keys, the drums, the knobs and the clock, over USB and Bluetooth MIDI.
 * Included once by app.c (it uses its settings, its Synth and its helpers).
 *
 * Nothing is made unless something listens: a computer (or another USB host) that has set USB MIDI up, or a Bluetooth
 * MIDI link. With none, all of this is idle and holds nothing. A link that arrives is given what sounds; one that goes
 * is forgotten.
 *
 * Where it runs: the 1 kHz tick (app_tick, the timer interrupt) does the clock, the drums, the late note-offs and,
 * every fifth tick, the layers' notes; the main loop's keys, knobs and panic take the tick's lock for what they share.
 * The audio render does nothing for it but count each drum's hits (dsp/drums.c).
 *
 * A layer's notes: a note starts when its level rises above the floor (Setup, -12 .. -36 dB) under the layer's peak
 * (its loudest note, held and let down over about 1.5 s), and ends 12 dB under that; it lasts 30 ms at least. New notes
 * are capped per layer: no more than the cap (24 by default: 32nd notes at 180 BPM) in any second; a note over the
 * floor that the cap holds back waits for its turn, the loudest first.
 *
 * Standard: each note as its nearest equal-tempered pitch, on its source's channel; two sources on the same note and
 * channel share it (the last to let go sends the note-off). MPE: one lower zone, channel 1 the master and 2..15 its
 * 14 members; each note its own member channel, a pitch bend (+-48 semitones, MPE's default: 0.6 cents a step) just
 * before its note-on puts it at its just pitch. Full: a key, or a layer note louder than the quietest sounding one,
 * takes the quietest's channel (its note-off first); a member channel is reused in the order they were let go. The
 * drums go on channel 16 (outside the zone) in MPE.
 *
 * No stuck notes: every note sent is kept (its channel and note, and the links it went to) until its note-off; a link
 * takes a note-on only while its queue has room for every note-off it may owe. Panic sends them all and All Notes Off. */

#define MO_D 3                           /* the links: USB, Bluetooth to a computer, Bluetooth to a device */
static const uint32_t MO_TO[MO_D] = {TO_USB, TO_BLE_HOST, TO_BLE_DEV};
#define MO_V 40                          /* notes held out at once (14 in MPE, plus the drums' short ones) */
enum { MS_L1, MS_L2, MS_KEY, MS_DRUM };
typedef struct {
    uint8_t used, src, id, ch, note, dests, vel, pr;   /* pr: the level last sent (Level on) */
    uint32_t t_on, t_off;                /* t_off: a note-off due then (0: none yet) */
} mo_voice_t;
static mo_voice_t mo_v[MO_V];
static struct {
    uint8_t up[MO_D];                    /* the link listens */
    uint8_t open[MO_D];                  /* notes it may owe a note-off */
    uint32_t gone_t[MO_D];               /* when its queue was last seen empty-handed (0: it was not) */
    uint32_t usb_link;                   /* plat_midi_out_link at the last look */
    uint8_t any;                         /* any link up */
    int8_t ln[2][MAX_NODES];             /* each layer note's voice (-1 off, -2 over the floor, not sent) */
    float peak[2];                       /* each layer's held peak */
    uint32_t rt[2][96];                  /* each layer's last note-ons' times (ms + 1000): the cap's window */
    uint8_t rw[2];
    float pitch[MAX_NODES];              /* the nodes' pitches in semitones (MIDI note numbers, fractional) */
    volatile uint32_t gen;               /* bumped when they change (the main loop) */
    uint32_t gen_seen;
    uint32_t ms, sub;
    uint16_t ch_free_t[16];              /* when each member channel was let go (ms, low bits) */
    uint8_t dr_seen[NUM_DRUMS];
    uint8_t clk_run[MO_D], clk_pend[MO_D];
    uint32_t clk_sent, clk_bar;
    uint8_t was_running;
    uint8_t knob_dirty;
    uint32_t knob_t;
    uint32_t ons, offs, ccs;             /* counted (the tests) */
} mo;

static uint32_t mo_pkt(uint32_t st, uint32_t d1, uint32_t d2)   /* a USB-MIDI packet, cable 0 */
{
    return (st >> 4) | st << 8 | (d1 & 0x7Fu) << 16 | (d2 & 0x7Fu) << 24;
}
static uint32_t mo_room(int d) { return plat_midi_out_room(MO_TO[d]); }
static void mo_send(uint32_t pkt, uint32_t dests)
{
    int d;
    for (d = 0; d < MO_D; d++)
        if (dests >> d & 1u)
            plat_midi_send(pkt, MO_TO[d]);
}
static uint32_t mo_spare(void)           /* the links with room for something extra beyond what they owe */
{
    uint32_t m = 0;
    int d;
    for (d = 0; d < MO_D; d++)
        if (mo.up[d] && mo_room(d) >= (uint32_t)mo.open[d] + 4u)
            m |= 1u << d;
    return m;
}
static int mo_ch(int src)                /* Standard: a source's channel (0-based) */
{
    return src == MS_L1 ? set.mo_ch[0] : src == MS_L2 ? set.mo_ch[1] : src == MS_KEY ? set.mo_ch[2] : set.mo_ch[3];
}

static void mo_free(int i)
{
    mo_voice_t *v = &mo_v[i];
    int d;
    for (d = 0; d < MO_D; d++)
        if ((v->dests >> d & 1u) && mo.open[d])
            mo.open[d]--;
    if (set.mo_mpe && v->ch >= 1 && v->ch <= 14)
        mo.ch_free_t[v->ch] = (uint16_t)mo.ms;
    v->used = 0;
    v->dests = 0;
}
static void mo_off_now(int i)            /* the note-off, where it went; a note shared on a channel waits for its last */
{
    mo_voice_t *v = &mo_v[i];
    int j;
    if (v->dests) {
        for (j = 0; j < MO_V; j++)
            if (j != i && mo_v[j].used && mo_v[j].ch == v->ch && mo_v[j].note == v->note) {
                uint32_t give = v->dests & ~mo_v[j].dests;   /* (the other keeps it sounding there: it owes the
                                                                note-off now) */
                mo_v[j].dests |= (uint8_t)give;
                v->dests &= (uint8_t)~give;
                break;
            }
        if (v->dests) {
            mo_send(mo_pkt(0x80u | v->ch, v->note, 0), v->dests);
            mo.offs++;
        }
    }
    mo_free(i);
}
static void mo_off(int i)                /* a note-off, or at 30 ms if the note is younger */
{
    if (i < 0 || !mo_v[i].used)
        return;
    if (mo.ms - mo_v[i].t_on < 30u)
        mo_v[i].t_off = mo_v[i].t_on + 30u;
    else
        mo_off_now(i);
}

static int mo_mpe_ch(int prio, int vel)  /* a member channel (1..14, 0-based: MIDI 2..15) for a new note; -1: none */
{
    uint32_t busy = 0;
    int i, c, best = -1, quiet = -1;
    uint16_t oldest = 0;
    for (i = 0; i < MO_V; i++)
        if (mo_v[i].used && mo_v[i].ch >= 1 && mo_v[i].ch <= 14)
            busy |= 1u << mo_v[i].ch;
    for (c = 1; c <= 14; c++)
        if (!(busy >> c & 1u)) {
            uint16_t age = (uint16_t)((uint16_t)mo.ms - mo.ch_free_t[c]);
            if (best < 0 || age > oldest) {
                best = c;
                oldest = age;
            }
        }
    if (best >= 0)
        return best;
    for (i = 0; i < MO_V; i++)           /* full: the quietest layer note gives way (a key always wins) */
        if (mo_v[i].used && mo_v[i].src <= MS_L2 && mo_v[i].ch >= 1 && mo_v[i].ch <= 14 &&
            (quiet < 0 || mo_v[i].vel < mo_v[quiet].vel))
            quiet = i;
    if (quiet < 0 || (!prio && mo_v[quiet].vel >= vel))
        return -1;
    c = mo_v[quiet].ch;
    if (mo_v[quiet].src <= MS_L2 && mo.ln[mo_v[quiet].src][mo_v[quiet].id] >= 0)
        mo.ln[mo_v[quiet].src][mo_v[quiet].id] = -2;   /* (it still sounds: it may come back when it rises again) */
    mo_off_now(quiet);
    return c;
}

/* a note out: src, its id (node, key, drum), its pitch (semitones), velocity 1..127; prio: may take a channel. Returns
   the voice, or -1 (no room) */
static int mo_on(int src, int id, float pitch, int vel, int prio)
{
    int i, j, ch, note, bend;
    uint32_t dests = 0;
    mo_voice_t *v;
    for (i = 0; i < MO_V && mo_v[i].used; i++)
        ;
    if (i == MO_V)
        return -1;
    {
        int d;
        for (d = 0; d < MO_D; d++)
            if (mo.up[d] && mo_room(d) >= (uint32_t)mo.open[d] + 4u)
                dests |= 1u << d;
    }
    if (!dests)
        return -1;
    if (pitch < 0.0f)
        pitch = 0.0f;
    if (pitch > 127.0f)
        pitch = 127.0f;
    note = (int)(pitch + 0.5f);
    vel = clamp(vel, 1, 127);
    if (set.mo_mpe && (src != MS_DRUM || set.mo_drums == 2)) {   /* (pitched drums: tuned, on a member) */
        ch = mo_mpe_ch(prio, vel);
        if (ch < 0)
            return -1;
    } else if (set.mo_mpe)
        ch = 15;                         /* (the kit: channel 16, outside the zone) */
    else
        ch = mo_ch(src);
    v = &mo_v[i];
    v->used = 1;
    v->src = (uint8_t)src;
    v->id = (uint8_t)id;
    v->ch = (uint8_t)ch;
    v->note = (uint8_t)note;
    v->vel = (uint8_t)vel;
    v->pr = 0;
    v->t_on = mo.ms;
    v->t_off = 0;
    v->dests = 0;
    if (!set.mo_mpe || ch == 15)         /* Standard: a note already out on this channel carries this one too */
        for (j = 0; j < MO_V; j++)
            if (j != i && mo_v[j].used && mo_v[j].dests && mo_v[j].ch == ch && mo_v[j].note == note)
                return i;
    if (set.mo_mpe && ch >= 1 && ch <= 14) {
        bend = 8192 + (int)((pitch - (float)note) * (8192.0f / 48.0f) + (pitch >= (float)note ? 0.5f : -0.5f));
        bend = clamp(bend, 0, 16383);
        mo_send(mo_pkt(0xE0u | ch, (uint32_t)bend & 0x7Fu, (uint32_t)bend >> 7), dests);
    }
    mo_send(mo_pkt(0x90u | ch, note, vel), dests);
    mo.ons++;
    v->dests = (uint8_t)dests;
    {
        int d;
        for (d = 0; d < MO_D; d++)
            if (dests >> d & 1u)
                mo.open[d]++;
    }
    return i;
}

static void mo_mcm(uint32_t m, int members)   /* MPE Configuration: RPN 6 on the master (a zone of members, its
                                                bend range 48; 0: no zone) */
{
    mo_send(mo_pkt(0xB0u, 101, 0), m);
    mo_send(mo_pkt(0xB0u, 100, 6), m);
    mo_send(mo_pkt(0xB0u, 6, (uint32_t)members), m);
    mo_send(mo_pkt(0xB0u, 101, 127), m);
    mo_send(mo_pkt(0xB0u, 100, 127), m);
}
static void mo_greet(int d)              /* a link that arrived: what sounds, the MPE zone, the knobs */
{
    int i;
    uint32_t m = 1u << d;
    if (set.mo_mpe)
        mo_mcm(m, 14);
    if (set.mo_sync & 1u)
        mo.knob_dirty = 0x0Fu;
    for (i = 0; i < MO_V; i++) {
        mo_voice_t *v = &mo_v[i];
        if (!v->used || (v->dests & m) || !v->dests || mo_room(d) < (uint32_t)mo.open[d] + 6u)
            continue;
        if (set.mo_mpe && v->ch >= 1 && v->ch <= 14) {
            float p = v->src <= MS_L2 ? mo.pitch[v->id] : (float)v->note;
            int bend = clamp(8192 + (int)((p - (float)v->note) * (8192.0f / 48.0f)), 0, 16383);
            if (v->src == MS_KEY && key_node[v->id] >= 0)
                bend = clamp(8192 + (int)((mo.pitch[key_node[v->id]] - (float)v->note) * (8192.0f / 48.0f)), 0, 16383);
            mo_send(mo_pkt(0xE0u | v->ch, (uint32_t)bend & 0x7Fu, (uint32_t)bend >> 7), m);
        }
        mo_send(mo_pkt(0x90u | v->ch, v->note, v->vel), m);
        v->dests |= (uint8_t)m;
        mo.open[d]++;
    }
}

static void mo_reset(void)               /* no one listens: nothing held */
{
    int i, l;
    for (i = 0; i < MO_V; i++)
        mo_v[i].used = mo_v[i].dests = 0;
    for (l = 0; l < 2; l++)
        for (i = 0; i < MAX_NODES; i++)
            mo.ln[l][i] = -1;
    for (i = 0; i < MO_D; i++) {
        mo.open[i] = 0;
        mo.clk_run[i] = mo.clk_pend[i] = 0;
    }
}

static void mo_links(void)               /* who listens: every 10 ms */
{
    int d, any = 0;
    uint32_t link = plat_midi_out_link();
    if (link != mo.usb_link) {           /* a new USB connection: whatever was owed there is gone with the old one */
        mo.usb_link = link;
        if (mo.up[0]) {
            int i;
            for (i = 0; i < MO_V; i++)
                mo_v[i].dests &= ~1u;
            mo.up[0] = 0;
            mo.open[0] = 0;
            mo.clk_run[0] = 0;
        }
    }
    for (d = 0; d < MO_D; d++) {
        uint32_t r = mo_room(d);
        if (r) {
            mo.gone_t[d] = 0;
            if (!mo.up[d]) {
                mo.up[d] = 1;
                mo.open[d] = 0;
                mo_greet(d);             /* (the first link of all: nothing sounds out yet; the layers' come as
                                            they sound) */
                if (S.transport && (set.mo_sync & 2u))
                    mo.clk_pend[d] = 1;
            }
        } else if (mo.up[d]) {           /* no room: gone, or a full queue for a moment; gone after 500 ms of it */
            if (!mo.gone_t[d])
                mo.gone_t[d] = mo.ms | 1u;
            else if (mo.ms - mo.gone_t[d] > 500u) {
                int i;
                mo.up[d] = 0;
                mo.open[d] = 0;
                mo.clk_run[d] = mo.clk_pend[d] = 0;
                for (i = 0; i < MO_V; i++)
                    mo_v[i].dests &= (uint8_t)~(1u << d);
            }
        }
        any |= mo.up[d];
    }
    if (!any && mo.any)
        mo_reset();
    mo.any = (uint8_t)any;
}

static const uint8_t MO_RATE[] = {6, 12, 24, 48, 96, 255};   /* (255: no cap) */
#define MO_NRATE 6
static void mo_layers(void)              /* every 5 ms */
{
    int l, i, n = S.eng.n_nodes;
    if (n > MAX_NODES)
        n = MAX_NODES;
    if (mo.gen != mo.gen_seen || S.build_req) {   /* the notes changed: what was out lets go */
        for (i = 0; i < MO_V; i++)
            if (mo_v[i].used && mo_v[i].src <= MS_L2)
                mo_off_now(i);
        for (l = 0; l < 2; l++)
            for (i = 0; i < MAX_NODES; i++)
                mo.ln[l][i] = -1;
        if (S.build_req)
            return;
        mo.gen_seen = mo.gen;
    }
    for (l = 0; l < 2; l++) {
        const float *a = S.layer[l].g.a;
        float mx = 0.0f, ref, on_th, off_th, fl;
        int on = set.midi_out >= 2 && S.transport && S.val[l ? P_V2_LEVEL : P_V1_LEVEL] > 0.01f;
        int cand[MAX_NODES], nc = 0, k, cap = MO_RATE[set.mo_rate % MO_NRATE];
        for (i = 0; i < n; i++)
            if (a[i] > mx)
                mx = a[i];
        mo.peak[l] = mx > mo.peak[l] ? mx : mo.peak[l] * 0.99667f;   /* (let down over ~1.5 s) */
        ref = mo.peak[l] > 0.1f ? mo.peak[l] : 0.1f;
        fl = fm_exp2f(-(float)set.mo_floor * (1.0f / 6.0206f));
        on_th = ref * fl;
        off_th = on_th * 0.25119f;       /* (12 dB under) */
        for (i = 0; i < n; i++) {
            int8_t s = mo.ln[l][i];
            if (!on || a[i] < off_th) {
                if (s >= 0)
                    mo_off(s);
                mo.ln[l][i] = -1;
            } else if (s == -1 && a[i] >= on_th)
                cand[nc++] = i;
        }
        for (k = 0; k < nc; k++) {       /* the loudest first, while the bucket lasts */
            int b = k, j;
            for (j = k + 1; j < nc; j++)
                if (a[cand[j]] > a[cand[b]])
                    b = j;
            j = cand[k]; cand[k] = cand[b]; cand[b] = j;
            if (cap <= 96 && mo.ms + 1000u - mo.rt[l][(mo.rw[l] + 96 - cap) % 96] < 1000u)
                break;                   /* (the cap: no more than cap note-ons in any second) */
            {
                int nd = cand[k], vel, vi;
                float rel = fm_log2f(a[nd] / ref) * 6.0206f;            /* dB under the peak, <= 0 */
                vel = (int)(127.0f + rel * (97.0f / (float)set.mo_floor));   /* the floor: 30, the peak: 127 */
                vi = mo_on(l, nd, mo.pitch[nd], vel, 0);
                if (vi >= 0) {
                    mo.ln[l][nd] = (int8_t)vi;
                    mo.rt[l][mo.rw[l]] = mo.ms + 1000u;
                    mo.rw[l] = (uint8_t)((mo.rw[l] + 1u) % 96u);
                } else
                    mo.ln[l][nd] = -2;
            }
        }
        if (set.mo_level)                /* the level as it moves: pressure (MPE) or poly aftertouch, 20 a second */
            if (!(mo.sub % 50u))
                for (i = 0; i < n; i++) {
                    int8_t s = mo.ln[l][i];
                    int p;
                    if (s < 0 || !mo_v[s].dests)
                        continue;
                    p = clamp((int)(a[i] / ref * 127.0f), 0, 127);
                    if (p - mo_v[s].pr < 4 && mo_v[s].pr - p < 4)
                        continue;
                    {
                        uint32_t m = mo_spare() & mo_v[s].dests;
                        if (!m)
                            continue;
                        mo_v[s].pr = (uint8_t)p;
                        mo_send(set.mo_mpe ? mo_pkt(0xD0u | mo_v[s].ch, (uint32_t)p, 0)
                                           : mo_pkt(0xA0u | mo_v[s].ch, mo_v[s].note, (uint32_t)p), m);
                    }
                }
    }
}

/* the drums: the render counts each drum's hits and stamps where in its samples the last one lands; the block it
   rendered plays one block after its render began (app_render notes when, mo_blk_ms / mo_blk_s), so a hit goes out
   then, in time with the sound (within the tick's millisecond) */
static volatile uint32_t mo_blk_ms, mo_blk_s;
static struct { uint8_t pend; uint8_t vel; int8_t node; uint32_t due; } mo_dp[NUM_DRUMS];
static void mo_drums(void)               /* every tick */
{
    int d;
    for (d = 0; d < NUM_DRUMS; d++) {
        uint8_t h = S.dr.hn[d];
        if (h != mo.dr_seen[d]) {        /* a hit (two in a millisecond: the last) */
            const DrumV *v = &S.dr.v[d];
            uint32_t off = S.dr.hs[d] - mo_blk_s;
            mo.dr_seen[d] = h;
            if (set.mo_drums && off < 4u * APP_BLOCK) {
                mo_dp[d].pend = 1;
                mo_dp[d].vel = (uint8_t)clamp((int)(v->g * 127.0f + 0.5f), 1, 127);
                mo_dp[d].node = (int8_t)v->node;
                mo_dp[d].due = mo_blk_ms + (uint32_t)((float)(APP_BLOCK + off) * (1000.0f / SR));
            }
        }
        if (mo_dp[d].pend && (int32_t)(mo.ms - mo_dp[d].due) >= 0) {
            int vi, nd = mo_dp[d].node;
            float p = set.mo_drums == 2 && nd >= 0 && nd < MAX_NODES ? mo.pitch[nd] : (float)set.mo_kit[d];
            mo_dp[d].pend = 0;
            if (!set.mo_drums)
                continue;
            if (set.mo_drums == 2 && !set.mo_mpe)
                p = (float)(int)(p + 0.5f);
            {   /* the same drum's last note, if still out, lets go first */
                int i;
                for (i = 0; i < MO_V; i++)
                    if (mo_v[i].used && mo_v[i].src == MS_DRUM && mo_v[i].id == d)
                        mo_off_now(i);
            }
            vi = mo_on(MS_DRUM, d, p, mo_dp[d].vel, 0);
            if (vi >= 0)
                mo_v[vi].t_off = mo.ms + 20u;
        }
    }
}

/* the clock: 24 a quarter note. The render moves the meter a block (5.8 ms) at a time, so the pulses come from a
   phase that runs at the tempo every millisecond and is drawn gently toward the meter's count (snapped to it after a
   jump). Start waits for a bar to begin; Stop goes when the meter stops. */
static float mo_clk_ph;
static void mo_clock(void)               /* every tick */
{
    int d, nb = (int)(S.meter.beats + 0.5f);
    uint32_t per = (uint32_t)(nb > 0 ? nb : 4) * 24u, m = 0, n, due;
    float md;
    if (!(set.mo_sync & 2u)) {
        for (d = 0; d < MO_D; d++)
            mo.clk_run[d] = mo.clk_pend[d] = 0;
        return;
    }
    if (S.transport && !mo.was_running)
        for (d = 0; d < MO_D; d++)
            if (mo.up[d])
                mo.clk_pend[d] = 1;
    if (!S.transport && mo.was_running)
        for (d = 0; d < MO_D; d++)
            if (mo.clk_run[d]) {
                mo.clk_run[d] = 0;
                mo_send(0x0Fu | 0xFCu << 8, 1u << d);
            }
    mo.was_running = S.transport;
    if (!S.transport || S.meter.bar_sec <= 0.0f)
        return;
    md = (float)(S.meter.bar % 1024u) * (float)per + S.meter.tau * (float)per;   /* the meter's count (mod 1024 bars) */
    mo_clk_ph += (float)per / (S.meter.bar_sec * 1000.0f);
    {
        float e = md - mo_clk_ph, wrap = 1024.0f * (float)per;
        if (e > wrap * 0.5f) e -= wrap;
        if (e < -wrap * 0.5f) e += wrap;
        if (e > 12.0f || e < -12.0f)
            mo_clk_ph = md;              /* (a jump: no burst) */
        else
            mo_clk_ph += e * 0.01f;
        if (mo_clk_ph >= wrap)
            mo_clk_ph -= wrap;
        if (mo_clk_ph < 0.0f)
            mo_clk_ph += wrap;
    }
    due = (uint32_t)mo_clk_ph;
    if (S.meter.bar != mo.clk_bar || S.meter.tau < 0.01f) {   /* a bar begins: a pending Start goes now */
        mo.clk_bar = S.meter.bar;
        for (d = 0; d < MO_D; d++)
            if (mo.clk_pend[d] && mo.up[d]) {
                mo.clk_pend[d] = 0;
                mo.clk_run[d] = 1;
                mo_send(0x0Fu | 0xFAu << 8, 1u << d);
                mo.clk_sent = (due / per) * per - 1u;   /* (the bar's first pulse goes now: a Start's downbeat) */
            }
    }
    for (d = 0; d < MO_D; d++)
        if (mo.clk_run[d] && mo.up[d])
            m |= 1u << d;
    if ((uint32_t)(due - mo.clk_sent) > 8u)
        mo.clk_sent = due;               /* (a jump, or the start: no burst) */
    for (n = 0; mo.clk_sent != due && n < 3u; n++) {
        mo.clk_sent++;
        if (m)
            mo_send(0x0Fu | 0xF8u << 8, m);
    }
}

void app_tick(void)                      /* 1 kHz, the timer interrupt */
{
    int i;
    mo.ms = plat_ms();
    mo.sub++;
    if (!(mo.sub % 10u))
        mo_links();
    if (!mo.any)
        return;
    for (i = 0; i < MO_V; i++)           /* the late note-offs (30 ms at least; the drums' 20 ms) */
        if (mo_v[i].used && mo_v[i].t_off && (int32_t)(mo.ms - mo_v[i].t_off) >= 0)
            mo_off_now(i);
    mo_clock();
    mo_drums();
    if (!(mo.sub % 5u))
        mo_layers();
}

/* ---- the main loop's side: under the tick's lock ---- */
static void mo_pitches(void)             /* the nodes' pitches, after a build (and a root that moved) */
{
    int i, n = S.eng.n_nodes, ch = 0;
    float root = S.val[P_ROOT], p[MAX_NODES];
    if (n > MAX_NODES)
        n = MAX_NODES;
    for (i = 0; i < n; i++) {
        p[i] = root + 12.0f * fm_log2f(S.eng.node[i].ratio);
        if (p[i] - mo.pitch[i] > 0.001f || mo.pitch[i] - p[i] > 0.001f)
            ch = 1;
    }
    if (ch) {
        uint32_t f = plat_lock();
        for (i = 0; i < n; i++)
            mo.pitch[i] = p[i];
        mo.gen++;
        plat_unlock(f);
    }
}
static void mo_key(int k, int node, int vel, int on)   /* a key's note (Notes: Keys, or Keys + layers) */
{
    uint32_t f;
    int i;
    if (k < 0 || k >= (int)NKEYS)
        return;
    f = plat_lock();
    if (on) {
        if (set.midi_out && mo.any && node >= 0 && node < MAX_NODES)
            mo_on(MS_KEY, k, mo.pitch[node], vel, 1);
    } else
        for (i = 0; i < MO_V; i++)
            if (mo_v[i].used && mo_v[i].src == MS_KEY && mo_v[i].id == k)
                mo_off(i);
    plat_unlock(f);
}
static void mo_panic(void)               /* every note out let go, and All Notes Off on each channel used */
{
    uint32_t f = plat_lock(), all = 0, chs = 0;
    int i, c, d;
    for (i = 0; i < MO_V; i++)
        if (mo_v[i].used) {
            all |= mo_v[i].dests;
            chs |= 1u << mo_v[i].ch;
            mo_off_now(i);
        }
    for (d = 0; d < MO_D; d++)
        if (mo.up[d])
            all |= 1u << d;
    if (set.mo_mpe)
        chs |= 0xFFFFu;
    else
        for (i = 0; i < 4; i++)
            chs |= 1u << mo_ch(i);
    for (c = 0; c < 16; c++)
        if (chs >> c & 1u)
            mo_send(mo_pkt(0xB0u | (uint32_t)c, 123, 0), all);
    for (i = 0; i < 2; i++)
        for (c = 0; c < MAX_NODES; c++)
            if (mo.ln[i][c] >= 0)
                mo.ln[i][c] = -2;   /* (sounding still: they come back when they fall and rise again) */
    plat_unlock(f);
}
static void mo_knobs(void)               /* the knobs you turn, as CC 21..24, 30 a second at most */
{
    uint32_t now = plat_ms(), f, m;
    int k;
    if (!mo.knob_dirty || now - mo.knob_t < 33u)
        return;
    f = plat_lock();
    m = (set.mo_sync & 1u) && mo.any ? mo_spare() : 0u;
    if (m) {
        int ch = set.mo_mpe ? 0 : (set.midi_ch ? set.midi_ch - 1 : 0);
        for (k = 0; k < 4; k++)
            if (mo.knob_dirty >> k & 1u) {
                int v = clamp((int)((S.macro[k] + 1.0f) * 63.5f + 0.5f), 0, 127);
                mo_send(mo_pkt(0xB0u | (uint32_t)ch, 21u + (uint32_t)k, (uint32_t)v), m);
                mo.ccs++;
            }
    }
    mo.knob_dirty = 0;
    mo.knob_t = now;
    plat_unlock(f);
}
static void mo_settings_changed(void)    /* a change of mode or channels: what is out lets go first */
{
    mo_panic();
}
static void mo_mode_set(int mpe)         /* Tuning changed: the zone set up, or taken down, where they listen */
{
    uint32_t f = plat_lock(), m = 0;
    int d;
    for (d = 0; d < MO_D; d++)
        if (mo.up[d])
            m |= 1u << d;
    if (m)
        mo_mcm(m, mpe ? 14 : 0);
    plat_unlock(f);
}
