---
type: Note
---
# Fixing PDF Export in Articulate Storyline Web Objects

When embedding interactive HTML (with jsPDF/html2canvas) in Articulate Storyline as a Web Object, PDF export via `jsPDF.save()` fails silently due to iframe sandbox restrictions.

## Symptoms

- Clicking "Export PDF" does nothing (no download, no error visible to user)
- Console may show security or cross-origin errors
- Works fine when HTML is opened directly in browser, fails only when embedded in Storyline

## Why This Happens

Articulate Storyline embeds Web Objects in sandboxed iframes. The iframe sandbox blocks:
- Direct file downloads (`jsPDF.save()`)
- Certain cross-origin requests
- Pop-ups (unless explicitly allowed)

Additionally, if using canvas capture libraries like `html2canvas` with animated content (e.g., Chart.js), race conditions can cause incomplete captures.

## Investigation

### Check 1: CSS Syntax Errors

Invalid CSS can cause rendering inconsistencies that affect canvas capture.

Common issues:
- HTML comments (`<!-- -->`) inside `<style>` blocks — use `/* */` instead
- Malformed properties like `border: 0 4px solid #color` — should be `border: 4px solid #color`

### Check 2: Animation Race Conditions

If using Chart.js or similar:
```javascript
options: {
  animation: false  // Ensures canvas is ready immediately
}
```

### Check 3: iframe Download Restrictions

The core issue — `pdf.save()` triggers a download that the iframe blocks.

## Resolution

Replace `pdf.save()` with a Blob URL opened in a new tab:

```javascript
async function exportPDF() {
  try {
    const { jsPDF } = window.jspdf
    const exportArea = document.getElementById("exportArea")

    const canvas = await html2canvas(exportArea, {
      scale: 2,
      useCORS: true,
      logging: false
    })

    const img = canvas.toDataURL("image/png")
    const pdf = new jsPDF({
      orientation: "landscape",
      unit: "px",
      format: [canvas.width / 2, canvas.height / 2]
    })

    pdf.addImage(img, "PNG", 0, 0, 
      pdf.internal.pageSize.getWidth(), 
      pdf.internal.pageSize.getHeight()
    )

    // Bypass iframe restrictions: open PDF in new tab instead of downloading
    const blob = pdf.output('blob')
    const url = URL.createObjectURL(blob)
    const newTab = window.open(url, '_blank')

    if (!newTab || newTab.closed || typeof newTab.closed === 'undefined') {
      alert('PDF export failed. Please allow pop-ups for this site.')
    }
  } catch (error) {
    console.error('Export Error:', error)
    alert('Export failed: ' + error.message)
  }
}
```

### Key Changes

| Original | Fixed |
|----------|-------|
| `pdf.save('filename.pdf')` | `window.open(URL.createObjectURL(pdf.output('blob')))` |
| No error handling | `try/catch` with user feedback |
| Default html2canvas | `scale: 2, useCORS: true` for quality |
| Chart animations enabled | `animation: false` for stable capture |

## Verification

1. Open the HTML directly in a browser — confirm export works
2. Embed in Storyline as Web Object — confirm PDF opens in new tab
3. Test with pop-up blocker enabled — confirm user gets helpful error message

## Prevention

When building interactive HTML for LMS/iframe embedding:

1. **Never use direct downloads** — always use Blob + `window.open()`
2. **Disable animations** before canvas capture
3. **Validate CSS** — no HTML comments in `<style>` blocks
4. **Add error handling** — iframes fail silently, surface errors to users
5. **Test in context** — always test embedded, not just standalone
