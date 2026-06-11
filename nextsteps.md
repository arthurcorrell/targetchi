# Next Steps: Tuning the HID Write Pipeline

Notes on choosing the trade-off between queue (mailbox) length and output
report frequency for the coalescing writer added in `raylib-gui/src/init.cpp`.

## Reframe

With the coalescing mailbox, this is **not** a classic buffering trade-off.
Coalescing makes the queue self-bounding (relative deltas merge, nothing is
lost), so the loss term drops out. The problem collapses to:

> Minimize latency subject to not overrunning the device's report rate.

## The three rates that bound everything

1. **Producer rate** `λ_p` — how often `queueMove` is called. Currently the aim
   loop, ~200 Hz (the 5 ms sleep). This is the arrival rate; measure it.
2. **USB OUT service rate** `μ` — how fast the writer can *complete* one
   `WriteFile` to the Leonardo. Bounded by the interrupt-OUT endpoint's
   `bInterval` (Full-Speed frames = 1 ms each → `bInterval=1` ≈ 1000/s,
   `bInterval=8` ≈ 125/s).
3. **Device upstream rate** `f_up` — how often the Arduino emits its *own* mouse
   report to the game (its HID descriptor's poll rate, 125–1000 Hz).

The game never sees motion faster than `f_up`. Sending OUT reports faster than
`f_up` gives zero benefit and pushes toward the endpoint saturation that caused
the original drops. That is the ceiling.

## The optimum is rate-matching

    f_out* = min(μ, f_up)

Send **one** report per upstream frame, carrying the delta accumulated during
that frame.

- **Faster than `f_up`:** intermediate reports are invisible to the game — pure
  waste + saturation risk.
- **Slower than `f_up`:** adds latency *and* forces large accumulated deltas
  that must be chunked (see below).

So don't hunt for a magic queue length — pin the send rate to the consumer and
let the queue fall out of it.

## Resulting queue length (Little's Law)

    L̄ = λ_p · W  ≈  λ_p / f_out

At `λ_p = 200 Hz` and `f_out = 1000 Hz`, average pending ≈ 0.2 — the mailbox
holds 0 or 1 delta almost always and coalescing essentially never fires. The
trade-off evaporates as long as utilization `ρ = λ_p / μ` stays comfortably
below 1. Aim for `ρ ≲ 0.7` to absorb USB completion jitter (the jitter, not the
mean, is what forces the headroom).

## The real coupling: the int8 step cap

The wire format is ±127 per axis per report. A flick of `D` pixels needs
`ceil(D / 127)` reports, so its delivery latency is:

    L_flick = ceil(D / 127) / f_out

A 500 px flick at 1 kHz = 4 reports = 4 ms of forced chunking latency. This —
not queue length — is the actual latency knob.

- **Best lever:** widen the axis in the firmware HID descriptor to 16-bit
  relative (`LOGICAL_MAXIMUM 32767`). Then step_cap ≫ any flick,
  `ceil(D/cap) = 1` always, queue depth ≡ 1, and chunking latency disappears.
- Otherwise raise `f_out` (bounded by `μ`).

## How to find the numbers (measure, don't guess)

1. In `writeReportBlocking`, timestamp before `WriteFile` and after
   `GetOverlappedResult` → distribution gives `μ` and its p99 jitter (and
   reveals if `bInterval` is secretly 8 = the real bottleneck).
2. In `queueMove`, log call rate (`λ_p`) and dx/dy magnitudes (the `D`
   distribution).
3. At each drain, log mailbox depth and accumulated delta.
4. Sweep `f_out ∈ {immediate, 2000, 1000, 500, 250} Hz` and record per setting:
   p99 write latency, max mailbox depth, write failures, end-to-end motion
   latency (high-speed camera, or in-game frame timing).

The optimum is the **lowest `f_out` where end-to-end latency stops improving** —
that is where you've hit `f_up`; going higher only re-adds USB load. If failures
appear before that knee, `μ` (the endpoint) is the binding constraint and the
fix is firmware-side (`bInterval`), not host-side tuning.

## Recommendation for this rig

- Set `f_out` = the Leonardo's upstream poll rate; make that 1000 Hz in the
  descriptor if it isn't already.
- Keep `ρ` under ~0.7.
- Switch the relative axes to **16-bit** so chunking latency vanishes and queue
  depth pins at 1.

## TODO

- [ ] Add timing/depth instrumentation to `writeReportBlocking` and `queueMove`
      behind a compile-time flag (zero-cost when off).
- [ ] Capture `μ`, `λ_p`, and the `D` distribution on hardware.
- [ ] Verify the Leonardo descriptor's `bInterval` and upstream poll rate.
- [ ] Evaluate moving the relative axes to 16-bit in firmware.
