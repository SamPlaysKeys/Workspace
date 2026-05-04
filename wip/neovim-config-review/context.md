# Storm Session: Neovim Config Review

**Goal**: Review and potentially improve the Neovim configuration located at `~/Src/dotfiles/configs/nvim/`.

> **Session graduated.** Documentation now lives in:
> - `~/Src/dotfiles/configs/nvim/README.md` — migration guide, plugin decisions
> - `docs/troubleshooting/neovim/treesitter-configs-module-not-found.md` — error symptoms and fixes

**Current state**: 
- Pre-Mortem complete: plugin bloat identified.
- Removed: `comment.lua`, `mini-files.lua`, `csvview.lua`, `notify.lua`
- Re-added: `rainbow_csv.nvim` (cameron-wags fork) for CSV highlighting
- Consolidated: mini plugins → `mini.lua`, git plugins → `git.lua`
- Added descriptions to all plugin files
- Cleaned up cruft files (lazyvim.json, map.txt, IDEAS.md, PLUGIN_GUIDE.md)
- Customized lualine (powerline separators, rearranged sections)
- Added `<S-Tab>` for file info in nvim-tree
- Fixed trouble.lua modes config
- Created AI_DISCLAIMER.md
- **Migrated treesitter.lua to new nvim-treesitter API** (main branch)
  - Uses `vim.treesitter.start()` for highlighting
  - Uses built-in indentexpr
  - Removed nvim-ts-autotag dependency (wasn't being used)
  - Incremental selection: use Neovim's built-in `an`/`in` in visual mode
- Added `<leader>gd` keymap for Snacks dashboard

**Open questions**:
- None. Ready for merge to main after user testing.

**Key constraints**:
- Adhere to user's existing dotfile management patterns.
