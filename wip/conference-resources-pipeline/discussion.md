# Running Discussion: Conference Resources Pipeline

## 2026-10-20 — Kickoff

Sam wants to scope a pipeline that processes all the resources from presenting at a conference. Not homelab-specific — applicable to live streams, meetups, etc. Runs on the homelab but portable.

**Confirmed scope (from clarify):**
- Slug: `conference-resources-pipeline`
- Outputs: show notes + blog post + social clips + archive

**Grounding observations:**
- The manual workflow already exists in the workspace:
  - `docs/notes/events/2026 - CKC Session Resources.md` — 51-row speaker resource table, hand-curated, mostly TBD
  - `docs/notes/events/TEMPLATE.md` — event note template (session index, links, takeaways, follow-ups)
  - `docs/talks/README.md` — talk proposals (Sam has 2 ideas, and presented "How to Talk About MFA" at CKC 2026)
- Blog output already has a home: `SamPlaysKeys/blog` Hugo leaf bundles + `blog-publishing` Hermes skill
- Homelab has Ollama on openclaw-host (local LLM option), fedora-dev runs services

**First-pass pipeline shape (to validate):**
```
INPUTS: slides (PDF/PPTX), VOD/recording URL, transcript (auto-downloaded or provided), repo links, event metadata
  → ingest/normalize (fetch, OCR/convert, audio extract)
  → transcribe (if no transcript: whisper/local or API)
  → analyze (summarize into show notes, pull quotes, timestamps for clips)
  → emit (show notes md, blog post leaf bundle, clip list, archive manifest)
```

Open questions to work through — see context.md.