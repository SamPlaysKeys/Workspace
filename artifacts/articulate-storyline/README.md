---
type: README-Note
---
# Articulate Storyline Artifacts

Web Objects and interactive HTML designed for embedding in Articulate Storyline courses.

## Licensing Note

Some artifacts in this directory are derived from third-party work. Check individual file headers for attribution and usage restrictions.

## Contents

### `time-allocation-tool.html`

Interactive time allocation calculator with Chart.js visualization and PDF export.

**Original Author:** Ashley Skow (cleverfoxdesigns.net)  
**License:** Do not reuse without permission. Included for learning/reference only.

**Key Features:**
- Day/week toggle for time budgeting
- Pie chart visualization (Chart.js)
- PDF export that works inside Storyline iframes

**Storyline-Specific Fixes Applied:**
- PDF export uses `window.open()` + Blob URL instead of `jsPDF.save()` (bypasses iframe download restrictions)
- Chart.js animations disabled for stable canvas capture
- CSS syntax cleaned (no HTML comments in `<style>` blocks)

See `docs/troubleshooting/articulate-storyline/pdf-export-iframe.md` for the full troubleshooting writeup.

## Embedding in Storyline

1. Add as Web Object (Insert → Web Object → local file)
2. Ensure "Display in: New browser window" is NOT selected (embed inline)
3. Test PDF export — should open in new browser tab
4. If blocked, check that pop-ups are allowed for the LMS domain
