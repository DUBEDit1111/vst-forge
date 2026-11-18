# DSP design and algorithm reference

Talk to the user in sonic terms: offer 2–3 options with the trade-off, recommend one. Put the DSP in `Source/Engine.h` (no JUCE) so the engine tests can drive it. Conventions: `prepare(sr)` computes rate-dependent values, `reset()` clears state, setters store targets, `processSample()` runs per sample; real-time safe after `prepare`.

## How to think

- Everything is a stream at sample resolution; a plugin is a small network of feedback loops around state.
- Minimal, exact DSP: curves from maths, every coefficient from the sample rate, clamp and denormal-guard every feedback loop. Ranges and curves are designed, not defaulted.
- What is the ONE idea? Few, deep, musically mapped controls; deterministic and stable.
- Measure, explain, don't mythologise: sine, sweep, impulse, spectrogram, null test. Know what is audible (zipper noise always; aliasing more than ~40 dB down usually not). Name the trade-offs.
- Finishable beats perfect: wire parameters early, listen in a real session, ship the 80%.
- Reduction pass: once it works, try removing a third of the parameters; keep the cut if the sound survives.

## Conversions

dB→gain `10^(dB/20)`; gain→dB `20·log10(g)` (floor −120). MIDI→Hz `440·2^((n−69)/12)`; semitones→ratio `2^(s/12)`. Beats→s `beats·60/bpm` (dotted ×1.5, triplet ×2/3). One-pole coefficient for time constant τ: `c = exp(−1/(τ·fs))`; to reach −60 dB in t: `exp(−6.9078/(t·fs))`; smoother `y = target + (y − target)·c`. Equal-power crossfade `cos/sin(t·π/2)`. Pan laws −3 dB (constant power), −4.5, −6. Q from bandwidth in octaves: `Q = 1/(2·sinh(ln2/2·BW))`.

## Filters: which for what

| Job | Use |
|---|---|
| synth filter, anything modulated, DJ filter | TPT state-variable filter (SVF): stable under audio-rate modulation |
| "analog" 24 dB synth | Moog-style ladder (tanh per stage, feedback k·y4, k 0..4), 2× oversample when swept hard |
| static EQ bands | RBJ "Audio EQ Cookbook" biquad (TDF-II). Direct-form biquads click when coefficients change per sample; SVF does not |
| tone/tilt, DC block, smoothing | one-pole TPT: `g = tan(π fc/fs)`, `G = g/(1+g)` |
| crossover | Linkwitz-Riley 4 = two Butterworth 2nd-order Q 0.7071 in series. LR4 sums flat but each band droops slightly: use Butterworth where a band's passband must be exact |
| phaser | 4–12 first-order allpasses, LFO 0.1–3 Hz, feedback, stereo via LFO phase offset |
| plucked string | Karplus-Strong: loop delay + lowpass damping; decay gain `g = 10^(−3L/(T60·fs))` |
| formants | bandpass bank; vowel F1–F3 (male, Peterson-Barney) a 730/1090/2440, e 530/1840/2480, i 270/2290/3010, o 570/840/2410, u 300/870/2240 Hz |

**SVF core** (`g = tan(π fc/fs)`, `k = 1/Q`):
`a1 = 1/(1+g(g+k)), a2 = g·a1, a3 = g·a2; v3 = x − ic2; v1 = a1·ic1 + a2·v3; v2 = ic2 + a2·ic1 + a3·v3; ic1 = 2v1 − ic1; ic2 = 2v2 − ic2; out = m0·x + m1·v1 + m2·v2`

| Output | (m0, m1, m2) |
|---|---|
| lowpass | 0, 0, 1 |
| bandpass | 0, 1, 0 (peak gain Q; m1 = k for unity) |
| highpass | 1, −k, −1 |
| notch | 1, −k, 0 |
| peak | 1, −k, −2 |
| allpass | 1, −2k, 0 |
| bell | 1, k(A²−1), 0 with k = 1/(Q·A) |
| low shelf | 1, k(A−1), A²−1 with g/√A |
| high shelf | A², k(1−A)A, 1−A² with g·√A |

A = 10^(dB/40). Q 0.707 is flat; a resonance knob maps well as Q = 0.5 + r²·29.5; self-oscillation near Q 30+. Clamp fc < 0.49·fs.

## Oscillators and synthesis

- **PolyBLEP** saw/square (2-sample): `dt = f/fs`; correction for `t < dt`: `u = t/dt → 2u − u² − 1`; for `t > 1−dt`: `u = (t−1)/dt → u² + 2u + 1`. Saw = `2t−1 − blep(t)`; square = `±1 + blep(t) − blep(wrap(t+0.5))`. ~40–60 dB alias reduction; 2× oversample for hard sync and FM.
- **Wavetables**: 2048-sample tables, one band-limited table per octave (FFT), linear or cubic read.
- **FM** (phase modulation): `sin(2π f t + I·sin(2π fm t))`; Carson bandwidth ≈ 2(I+1)·fm.
- **Noise**: xorshift white (seeded); pink by Kellet's 3-pole filter.
- **Envelopes**: exponential; attack chases a 1.3 target (snappy), release to −80 dB then idle; retrigger from the current level (no click).
- **LFOs**: from a phasor; tempo-synced phase derived from host PPQ each block, never integrated.
- **Voices**: steal oldest/quietest with a ~5 ms fade; pitch bend 14-bit `(v−8192)/8192·range`; MPE per-note bend, CC74 slide, pressure.
- Pitch and audio-rate cutoff modulation per sample; everything else per block is fine.

## Effects

- **Tremolo / auto-pan / LFO to gain**: gain = `1 − depth·(1 − lfo)/2` with `lfo` in −1..1, so depth 0 is exactly unity. Sine LFO from a phasor (`phase += rate/fs`, wrap at 1). A click-free square: slew-limit the hard square, `hard = phase < 0.5 ? 1 : −1; y += clamp(hard − y, −step, step)` with `step = 2/(ramp·fs)` and a 1–3 ms ramp. Smooth depth (~5 ms) and rate (~50 ms); crossfade shape changes (10–20 ms). "Stereo tremolo" can mean L and R in phase (a pulsing tremolo, mono-safe) or 180° apart (auto-pan): ask, default in phase. Tempo-synced rate: phase from host PPQ.

- **Delay**: power-of-two buffer, read-then-write, fractional read by cubic Hermite (`c0 = y0; c1 = 0.5(y1 − y−1); c2 = y−1 − 2.5y0 + 2y1 − 0.5y2; c3 = 0.5(y2 − y−1) + 1.5(y0 − y1)`). Tape feel = smoothed delay time (100–200 ms glide), tanh in the loop, one-pole high-frequency loss, wow 0.5–2 Hz, flutter ~6 Hz. Chorus 5–30 ms ±1–5 ms at 0.1–3 Hz; flanger 0.1–10 ms with ± feedback.
- **Reverb**: Freeverb tunings at 44.1 kHz: combs 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617; allpasses 556, 441, 341, 225; stereo spread +23; scale by fs/44100. Dattorro plate. FDN with Householder/Hadamard mixing, mutually prime delay lengths, `g = 10^(−3L/(T60·fs))`. Slow (0.1–0.5 Hz) sub-millisecond modulation kills metallic ringing. Choosing delay ratios from "special" constants makes no measurable difference; mutually prime rounding does.
- **Saturation**: tanh, atan, asymmetric (even harmonics) + DC blocker, hard clip, foldback. Level compensation ~1/√drive. Oversample 2–4× or use first-order antiderivative anti-aliasing `(F(x) − F(x₁))/(x − x₁)`. Bitcrush `round(x·2^(b−1))/2^(b−1)`.
- **Dynamics** (Giannoulis, Massberg & Reiss 2012): peak detector for drums, RMS for bus; gain computer in dB; soft knee `y = x + (1/R − 1)(x − T + W/2)²/(2W)` for |x−T| ≤ W/2; attack/release one-poles on the gain reduction in dB; lookahead = delay + reported latency. Transient shaper = fast envelope minus slow envelope.
- **Limiter**: brickwall with lookahead; true-peak safety needs oversampled detection (8× in practice; 4× can let +0.3 dB through on noise).
- **Stereo**: M = (L+R)/2, S = (L−R)/2, width scales S; correlation = Σ LR / √(ΣL²·ΣR²). Analyse the power sum of both channels, never mid only (side-only problems become invisible).
- **Metering**: peak with 20 dB/s fall; RMS over 300 ms; LUFS = K-weighting + 400 ms blocks + gates −70 LUFS absolute / −10 LU relative.

## Hygiene and cost

- Denormals: `juce::ScopedNoDenormals` (FTZ/DAZ) or add 1e-18 in feedback. Sanitise NaN/inf on feedback outputs.
- Use double for filters with very low fc/fs and for long accumulators.
- Never assume block size. Deterministic renders (seeded noise, no wall clock) so bounce == playback.
- Rough costs: add/mul 1, divide 10–20, sqrt 15, sin/exp/log/tan 20–50, tanh 40+. Compute coefficients per block or on change. Padé tanh `x(135135 + x²(17325 + x²(378 + x²))) / (135135 + x²(62370 + x²(3150 + 28x²)))`, clamped to ±4.97 (< 1e-4 error).

## Measure, don't guess

Frequency response by sine RMS in/out after settling (−3 dB at fc; 6 dB/oct per pole). Stability: noise plus every parameter swept to extremes, all outputs finite. Impulse response for delays; T60 by Schroeder integration. THD by FFT of a sine. Aliasing: 5 kHz saw at 44.1 kHz, non-harmonic bins > 40 dB down. Null tests against a reference. Tolerances: ±0.5 dB response, ±2% envelope times, exact equality for bit-transparent bypass.
