---
type: Outline
status: draft
---

# Talk Outline — How to Document Your Homelab (even if you don't have one)

Slide-by-slide skeleton. Status: draft.

## 1. The problem
- Documentation is either never started or instantly stale.
- "I'll document it later" becomes "I have no idea why I set this up."
- The lab you can't explain is the lab you can't recover.

## 2. Reframe: documentation as a loop, not a deliverable
- Four phases, iterated: Goal → Achievable → How to → Fit.
- You can enter the loop at any phase (existing lab = start at Fit or How to).

## 3. Phase 1 — Goal (Ideal State)
- Visualize what you *want* before you own anything.
- Output: an Ideal-State doc (services, environments, intent).
- Demo hook: build one with no hardware at all.

## 4. Phase 2 — Achievable (Realistic scope)
- Constraints: hardware, budget, time, skill, risk tolerance.
- Output: a scoped plan that admits what's deferred.
- Pattern reminder: the IaC omitted the "why" — here we capture it up front.

## 5. Phase 3 — How to (Build/run docs)
- Architecture, ADRs, inventory, runbooks.
- Borrowed from real practice: environment maps, decision records.
- Output: the docs you'd actually need to rebuild or onboard someone.

## 6. Phase 4 — Fit (Gap analysis)
- Ideal-State vs Current/Plan-State, side by side.
- Output: a visual fit-gap matrix — the growth roadmap.
- This is the artifact most labs never produce.

## 7. The AI assistant's real job
- Structured interviewer, not doc ghostwriter.
- Per-phase conversation scripts drive the loop.
- Forces you to confront gaps you'd skip alone.

## 8. Live demo
- Empty-lab example through all four phases with the assistant.
- Then the same loop on a real, populated lab → reveal the Fit view.

## 9. Takeaways + how to start today
- Templates and session scripts are published (methodology dir).
- Start with Goal, even if your "lab" is a wishlist.

## 10. Q&A / resources
- Link to the reusable system: `../methodology/document-your-homelab/`
