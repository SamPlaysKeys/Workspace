# Context: Conference Resources Pipeline

**Goal**: A pipeline that ingests all resources produced by presenting at a conference (slides, recording/VOD, transcript, code repos, links) and emits show notes, a blog post, social clips, and a searchable archive. Built on the homelab but not homelab-specific — portable enough to run anywhere (laptop, cloud, homelab). Also applies to live streams and meetups.

**Current State**:
- Storm session just started (2026-10-20). Nothing decided yet.
- Confirmed outputs: show notes + blog post + social clips + archive.
- Grounding found: the manual version of this workflow already exists in the workspace:
  - `docs/notes/events/` — event notes with template + the CackalackyCon 2026 "Session Resources" table (51 rows of speaker/slides/repo links, hand-curated, mostly "TBD")
  - `docs/talks/` — talk proposals/outlines
  - Blog lives in `SamPlaysKeys/blog` (Hugo, `content/post/<slug>/index.md`)

**Open Questions**:
- What are the actual inputs? (slides PDF/PPTX, VOD URL, transcript file, repo URL, event schedule)
- Where does the pipeline live — separate repo, artifact in Workspace, GitHub Action?
- What's the trigger: manual run per talk, event-level batch, watcher on a folder?
- How much AI involvement (local LLM vs API) and where (transcription, summarization, clip selection)?
- What does "archive" mean: local file store, searchable index, public page?
- Social clips: how are they cut (whisper timestamps + ffmpeg?), which platforms?
- Relation to existing blog-publishing skill / Hugo leaf bundles

**Key Constraints**:
- Runs on homelab (openclaw-host has Ollama; fedora-dev runs Leantime) but must stay portable
- Not homelab-specific — keep infra coupling out of the core pipeline
- Workspace conventions: no nginx unless required (homelab), artifacts vs docs split