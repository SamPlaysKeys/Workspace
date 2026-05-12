# Objective
Fix the "Export to PDF" functionality in `Export.html` when embedded in Articulate Storyline, and resolve minor CSS syntax errors.

# Key Files & Context
- `Export.html`: Contains the HTML, CSS, and JS for the Time Allocation Tool. When embedded in Storyline (likely as a Web Object/iframe), the default `jsPDF.save()` often fails due to sandbox restrictions or download blocking.

# Implementation Steps

1. **Fix CSS Syntax:**
   - Change invalid HTML comments `<!-- ... -->` inside the `<style>` block to valid CSS comments `/* ... */`.
   - Correct `border:0 4px solid #2e4a54;` to `border: 4px solid #2e4a54;`.

2. **Disable Chart Animations:**
   - Add `animation: false` to the Chart.js `options` object to ensure the canvas is fully rendered and ready for immediate capture by `html2canvas`.

3. **Enhance `exportPDF()` Function:**
   - Add a `try...catch` block to handle and alert errors.
   - Configure `html2canvas` with `{ scale: 2, useCORS: true }` for better quality and cross-origin safety.
   - Change the output method: Instead of `pdf.save()`, generate a Blob representation of the PDF (`pdf.output('bloburl')` or `URL.createObjectURL(pdf.output('blob'))`) and open it directly in a new browser tab using `window.open()`. This avoids iframe download restrictions.

# Verification & Testing
- Load `Export.html` locally in a browser to ensure it still works correctly.
- Review in Articulate Storyline to verify the PDF opens successfully in a new tab.