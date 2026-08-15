---
type: Outline
status: draft
---

# Talk Outline — How To Plan Your Perfect Homelab

20-minute talk. Slide-by-slide skeleton. Status: draft.

The outcome is documentation, but the talk is about **planning** — a 20-minute, four-section walk through the iterative planning loop and a live example, ending with the deliverable you can take home.

## 1. Introduction
- The accretion problem: a homelab grows one impulse-buy and one "I'll write this down later" at a time.
- The lab you can't explain is the lab you can't recover (or right-size).
- We're here to plan, not to admire gear. By the end you'll have a method and a kit.

## 2. Explanation of the Iterative Planning Concept
- Documentation is a *byproduct*; planning is the loop: Goal → Achievable → How to → Fit.
- You can enter the loop at any phase (existing lab = start at Fit or How to).
- Goal — visualize what you *want* (Ideal State). Achievable — what's realistic given hardware, time, skill, risk. How to — architecture, ADRs, inventory, runbooks. Fit — gap analysis: Ideal vs Current/Plan, side by side.
- The AI assistant's real job: structured interviewer that forces you to confront the gaps you'd skip alone — not a doc ghostwriter.
- Pattern reminder: the IaC omitted the "why" — this loop captures it up front, before the gear.

## 3. Walking through the example (`/docs/homelab`)
- Live walk of this repo's *populated* `docs/homelab/` through all four phases.
- Goal: the overview map + Ideal State. Achievable: ADRs and what was deferred. How to: architecture, environments, inventory. Fit: the gap matrix — growth roadmap.
- Shows the artifact most labs never produce: the Fit/gap view.

## 4. Closing & Delivery of `homelab-doc-kit`
- Recap: planning is a loop, documentation is what it leaves behind.
- Takeaway: the `homelab-doc-kit` — portable, dependency-free scaffolding for the whole loop.
  - Now at `artifacts/homelab-doc-kit/` (templates + AI session scripts per phase).
- Start with Goal, even if your "lab" is a wishlist. Q&A / resources.
