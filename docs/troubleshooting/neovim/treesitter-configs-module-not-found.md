# nvim-treesitter.configs Module Not Found

## Symptoms

- Neovim throws error on startup: `module 'nvim-treesitter.configs' not found`
- Other cascading errors may appear (mini.icons, telescope, bufferline)
- Treesitter highlighting doesn't work
- Happens after running `:Lazy sync` or updating plugins

## Cause

The nvim-treesitter plugin underwent a breaking rewrite in 2025-2026. The `main` branch removed the `nvim-treesitter.configs` module entirely. There is no migration path — it's a different plugin architecture.

Old API (no longer works):
```lua
require("nvim-treesitter.configs").setup({
  ensure_installed = { ... },
  highlight = { enable = true },
})
```

## Fix Options

### Option 1: Migrate to New API (Recommended)

The new API uses Neovim's built-in treesitter with nvim-treesitter just handling parser installation:

```lua
return {
  "nvim-treesitter/nvim-treesitter",
  build = function()
    require("nvim-treesitter").install({ "lua", "python", "markdown", ... })
  end,
  config = function()
    require("nvim-treesitter").setup({
      install_dir = vim.fn.stdpath("data") .. "/site",
    })
    
    vim.api.nvim_create_autocmd("FileType", {
      callback = function(args)
        pcall(vim.treesitter.start, args.buf)
      end,
    })
  end,
}
```

**Features lost with no replacement:**
- `incremental_selection` — use Neovim's built-in `an`/`in` in visual mode
- `autotag` — configure nvim-ts-autotag separately with its own setup()

### Option 2: Pin to Master Branch (Quick Fix)

Keep the old config and pin to the legacy branch:

```lua
{ "nvim-treesitter/nvim-treesitter", branch = "master" }
```

## Verification

After fixing, open a file and check:
- Syntax highlighting works
- `:InspectTree` shows the parsed AST
- No errors on startup

## Related

- Dotfiles nvim config: `~/Src/dotfiles/configs/nvim/lua/plugins/treesitter.lua`
- Full migration docs: `~/Src/dotfiles/configs/nvim/README.md`
