## 2026-06-01: Removing Tolaria References

**Action**: Searched for and removed references to "Tolaria" from the workspace rules to ensure it remains clean and decoupled from local machine tools.

**Changes**:
- **`workstyle/working_style.md`**: Verified this file is completely clean. The recent rewrite naturally stripped any legacy references.
- **`AGENTS.md`**: Removed the reference to "Tolaria vault" in the "What agents should do" section.

**Current Status**: 
The portable behaviors and the root `AGENTS.md` are now fully decoupled from Tolaria, ensuring that when the framework is ported to other repositories, no local tool context leaks over.