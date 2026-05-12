# Articulate Storyline: PDF Export Failure

**Status:** resolved

**Environment:** Articulate Storyline Web Object (iframe), jsPDF, html2canvas, Chart.js

---

## Licensing

> **Note:** The original `Export.html` belongs to Ashley Skow (cleverfoxdesigns.net) and should not be reused without permission. This artifact is included for learning/troubleshooting purposes only, with express permission from the author.

---

## Symptoms

- `jsPDF.save()` fails when embedded in Articulate Storyline (sandbox/iframe restrictions).
- Invalid CSS syntax in `Export.html` (HTML comments inside `<style>`).
- Potential race condition: Chart.js animations might prevent full capture by `html2canvas`.

---

## Investigation

### 2026-05-12 10:15 - Fixes Applied

**Hypothesis:** The export fails because Storyline blocks file downloads from iframes, and CSS syntax errors are causing styling inconsistencies. Disabling animations will ensure stable canvas capture.

**Tried:**
1.  **CSS Refactor:** Replaced invalid HTML comments with `/* ... */` and corrected invalid border syntax.
2.  **Chart Stability:** Set `animation: false` in Chart.js options to ensure the canvas is ready for capture immediately.
3.  **PDF Export Refactor:**
    *   Added `scale: 2` and `useCORS: true` to `html2canvas` for higher resolution and safety.
    *   Implemented `try...catch` for error handling.
    *   Replaced `pdf.save()` with `URL.createObjectURL(pdf.output('blob'))` and `window.open()` to bypass iframe download restrictions common in Articulate Storyline.

**Result:** Files updated. Fixes applied to `Export.html`.

---

## Resolution

**Root cause:** Multiple issues combined:
1. Articulate Storyline sandboxes Web Objects in iframes, blocking `jsPDF.save()` file downloads
2. Invalid CSS syntax (HTML comments inside `<style>` block, malformed border property)
3. Chart.js animations created race condition with `html2canvas` capture

**Fix:**
1. **CSS cleanup:** Replaced `<!-- -->` with `/* */`, fixed `border:0 4px solid` → `border: 4px solid`
2. **Chart stability:** Added `animation: false` to Chart.js options
3. **PDF export bypass:** Replaced `pdf.save()` with Blob + `window.open()` to open PDF in new tab instead of downloading

**Prevention:** When embedding interactive HTML in Storyline or similar LMS platforms:
- Always use `window.open()` with Blob URLs instead of direct file downloads
- Disable animations before canvas capture
- Validate CSS syntax (no HTML comments in `<style>` blocks)

