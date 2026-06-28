#!/usr/bin/env python3
"""Generate simulation figures for the LightString course-design paper.

All plots are *labeled simulations* of the firmware's signal chain, produced
by re-implementing the exact fixed-point algorithms from
firmware/App/light_sense.c and scale_quantizer.c, so the figures faithfully
show what the firmware computes.
"""
import numpy as np
import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt

rng = np.random.default_rng(42)
OUT = "../paper/images/"

FS = 100  # 100 Hz sampling, as in firmware
T = 8.0
t = np.arange(0, T, 1 / FS)

# --- synthetic "hand wave" trajectory over the light sensor -----------------
# hand absent (0-1 s), slow rise, two held notes, fast vibrato-ish sweep, exit
clean = np.zeros_like(t)
seg = lambda a, b: (t >= a) & (t < b)
clean[seg(1.0, 2.5)] = np.linspace(5, 70, seg(1.0, 2.5).sum())
clean[seg(2.5, 3.5)] = 70
clean[seg(3.5, 4.2)] = np.linspace(70, 40, seg(3.5, 4.2).sum())
clean[seg(4.2, 5.2)] = 40
clean[seg(5.2, 6.8)] = 55 + 18 * np.sin(2 * np.pi * 1.2 * (t[seg(5.2, 6.8)] - 5.2))
clean[seg(6.8, 7.4)] = np.linspace(55, 3, seg(6.8, 7.4).sum())

# measurement noise: gaussian + occasional impulse spikes (mains flicker)
raw = clean + rng.normal(0, 2.0, t.size)
spikes = rng.random(t.size) < 0.02
raw[spikes] += rng.normal(0, 14, spikes.sum())
raw = np.clip(raw, 0, 100)

# --- firmware filter chain: median-5 then EMA alpha=0.3 (fixed point x10) ---
def firmware_filter(x):
    out = np.zeros_like(x)
    buf = [x[0]] * 5
    ema_x10 = int(x[0] * 10)
    for i, v in enumerate(x):
        buf[i % 5] = v
        med = sorted(buf)[2]
        ema_x10 += (3 * (int(med * 10) - ema_x10)) // 10
        out[i] = (ema_x10 + 5) // 10
    return out

filt = firmware_filter(raw)

# --- normalization with calibration [cal_lo, cal_hi] ------------------------
cal_lo, cal_hi = 5, 78
norm = np.clip((filt - cal_lo) * 1000 / (cal_hi - cal_lo), 0, 1000).astype(int)

# --- quantizer: pentatonic, 2 octaves, hysteresis + stable count ------------
PENTA = [0, 2, 4, 7, 9]
BASE = 60
N = len(PENTA) * 2
W = 1000 // N
HYST = 18
STABLE_N = 3
HAND_ON, HAND_OFF = 80, 40


def quantize(norm_seq):
    notes = np.full(norm_seq.size, np.nan)
    events = []  # (idx, note)
    hand = False
    cur_step, cand, cnt = -1, -1, 0
    active = False
    for i, nv in enumerate(norm_seq):
        if not hand and nv > HAND_ON:
            hand = True
        if hand and nv < HAND_OFF:
            hand = False
        if not hand:
            if active:
                events.append((i, None))
            active, cur_step, cand, cnt = False, -1, -1, 0
            continue
        step = min(nv // W, N - 1)
        if active and step != cur_step:
            center = cur_step * W + W // 2
            if abs(int(nv) - center) <= W // 2 + HYST:
                step = cur_step
        if step == cur_step and active:
            cand, cnt = -1, 0
        else:
            if step == cand:
                cnt += 1
            else:
                cand, cnt = step, 1
            if cnt >= STABLE_N:
                cur_step, active = cand, True
                cand, cnt = -1, 0
                note = BASE + 12 * (cur_step // len(PENTA)) + PENTA[cur_step % len(PENTA)]
                events.append((i, note))
        if active:
            notes[i] = BASE + 12 * (cur_step // len(PENTA)) + PENTA[cur_step % len(PENTA)]
    return notes, events


notes, events = quantize(norm)

plt.rcParams.update({"font.size": 9, "figure.dpi": 150})

# ---------------------------------------------------------------- figure 1 --
fig, ax = plt.subplots(2, 1, figsize=(5.2, 3.4), sharex=True)
ax[0].plot(t, raw, lw=0.5, color="#bbbbbb", label="raw ADC (Lsens 0-100)")
ax[0].plot(t, clean, lw=1.0, color="#2a7", ls="--", label="true hand trajectory")
ax[0].legend(loc="upper right", fontsize=7)
ax[0].set_ylabel("light value")
ax[1].plot(t, filt, lw=1.0, color="#06c", label="median-5 + EMA (firmware)")
ax[1].plot(t, clean, lw=1.0, color="#2a7", ls="--", label="true")
ax[1].legend(loc="upper right", fontsize=7)
ax[1].set_xlabel("time (s)")
ax[1].set_ylabel("light value")
fig.tight_layout()
fig.savefig(OUT + "filter_pipeline.pdf")
plt.close(fig)

# ---------------------------------------------------------------- figure 2 --
fig, ax = plt.subplots(2, 1, figsize=(5.2, 3.6), sharex=True,
                       gridspec_kw={"height_ratios": [1, 1.2]})
ax[0].plot(t, norm, lw=1.0, color="#06c")
for k in range(N + 1):
    ax[0].axhline(k * W, lw=0.3, color="#ccc", zorder=0)
ax[0].axhline(HAND_ON, lw=0.8, color="#e60", ls=":", label="hand-on threshold")
ax[0].set_ylabel("normalized light (0-1000)")
ax[0].legend(loc="upper right", fontsize=7)

ax[1].step(t, notes, where="post", lw=1.4, color="#c33")
on_i = [i for i, n in events if n is not None]
on_n = [n for _, n in events if n is not None]
ax[1].plot(t[on_i], on_n, "o", ms=3.5, color="#c33", mfc="white",
           label="note-on event")
ax[1].set_xlabel("time (s)")
ax[1].set_ylabel("MIDI note")
yt = [BASE + 12 * (s // 5) + PENTA[s % 5] for s in range(N)]
names = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]
ax[1].set_yticks(yt)
ax[1].set_yticklabels([f"{names[n % 12]}{n // 12 - 1}" for n in yt], fontsize=6)
ax[1].legend(loc="lower right", fontsize=7)
fig.tight_layout()
fig.savefig(OUT + "quantize_demo.pdf")
plt.close(fig)

# ---------------------------------------------------------------- figure 3 --
# step mapping with hysteresis band illustration
fig, ax = plt.subplots(figsize=(5.0, 2.6))
x = np.arange(0, 1001)
step = np.minimum(x // W, N - 1)
ax.step(x, step, where="post", color="#06c", lw=1.2)
for k in range(N):
    c = k * W + W // 2
    ax.axvspan(max(c - W // 2 - HYST, 0), min(c + W // 2 + HYST, 1000),
               ymin=k / N, ymax=(k + 1) / N, color="#fc3", alpha=0.25, lw=0)
ax.set_xlabel("normalized light (0-1000)")
ax.set_ylabel("scale step index")
ax.set_title("step mapping; shaded = hysteresis hold band of each step",
             fontsize=8)
fig.tight_layout()
fig.savefig(OUT + "mapping_hysteresis.pdf")
plt.close(fig)

n_on = len(on_i)
print(f"figures written to {OUT}; simulated note-on events: {n_on}")
