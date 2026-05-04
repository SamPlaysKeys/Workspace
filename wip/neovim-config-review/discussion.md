## Plugin Stack Pre-Mortem

**State**: We have ~35 plugin configuration files in `lua/plugins/`.
**Assumption of Failure**: It is 6 months from now. The config is sluggish, startup time is poor, and random UI bugs happen on Neovim updates. The cognitive load of remembering keybinds for everything is too high.

**Why did it fail? (Failure Modes):**
1.  **Overlapping Capabilities**: We installed multiple tools that do the same job.
    - *File Explorers*: `nvim-tree.lua`, `mini-files.lua`, and `yazi.lua`.
    - *CSV Handling*: `csvview.lua` and `rainbow_csv.lua`.
    - *Notifications*: `noice.lua` (often handles notifications) AND `notify.lua`.
2.  **Redundancy with Mega-Plugins**: We have `snacks.lua` installed. Snacks provides modern replacements for many things (dashboard, notifications, terminal, even picking), making standalone plugins redundant.
3.  **Redundancy with Core Neovim**: If running Neovim 0.10+, native commenting is built-in, potentially making `comment.lua` obsolete.
4.  **Maintenance Burden**: Niche plugins (`code-preview.lua`, `codediff.lua`, `opencode.lua`) might be unmaintained or rarely used, breaking on core API changes.

### Consolidation Phase 1
- **Decided**: Drop `comment.lua` (rely on native Neovim 0.10+ commenting).
- **Decided**: Keep `yazi.lua`, drop `mini-files.lua`.
- **Decided**: Keep `nvim-tree.lua` (useful as sidebar, many integrations).
- **Completed**: Removed `comment.lua` and `mini-files.lua` from `~/Src/nvim/lua/plugins/`.

---

## 2026-04-24

Path correction: config is at `~/Src/dotfiles/dotfiles/config/nvim/`.

**Remaining overlaps to resolve:**
- CSV: `csvview.lua` vs `rainbow_csv.lua`
- Notifications: `noice.lua` vs `notify.lua`

**Decided:** Remove both CSV plugins initially, but treesitter CSV highlighting was insufficient.

**Completed:** Re-added `cameron-wags/rainbow_csv.nvim` in `csv.lua`, disabled treesitter highlighting for CSV files.

**Note:** Earlier changes were made to `~/Src/nvim/` (a backup). All changes re-applied to actual config at `~/Src/dotfiles/dotfiles/config/nvim/`.

### nvim-tree enhancements
- Fixed inconsistent file colors — some `.lua` files had executable bit set, causing `NvimTreeExecFile` highlight (blue). Fixed with `chmod -x *.lua`.
- Added `<S-Tab>` keybinding to show file info popup (permissions, size, modified time).
- Attempted custom float styling (borders, shadows) but reverted to defaults — looked worse than stock Catppuccin.

### Notifications
- **Decided:** Remove `nvim-notify` — noice's built-in mini fallback is sufficient.
- **Completed:** User removed nvim-notify from noice dependencies and deleted `notify.lua`.
