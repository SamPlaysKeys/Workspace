---
name: cross-link
description: Finds and fixes missing index links for newly created files to prevent orphans. Auto-triggers when completing documentation.
---

# Cross-Link — Anti-Orphan Convention

<objective>
Ensure no file is orphaned. Whenever a new document is graduated or created, it must be linked in the appropriate README or index file.
</objective>

## Triggers
- A new file is created in `docs/` or `library/`.
- User says "cross-link this" or "index this".
- Automatically runs at the end of the `document` skill.

## Process

1. **Identify the Target**: Determine what file was just created or moved.
2. **Find the Index**: Locate the relevant parent index. 
   - New troubleshooting doc -> `docs/troubleshooting/README.md` (create if missing)
   - New guide -> `docs/guides/README.md` (create if missing)
3. **Update Index**: Append a link to the new file in the index's Table of Contents.
4. **Update Related**: If the new doc heavily references another existing doc, add a "Related" link to the bottom of the older doc.

## Template (Index Addition)

When updating an index/README, use this format:
```markdown
- [Document Title](./path/to/doc.md) - [One sentence summary of the document's purpose]
```