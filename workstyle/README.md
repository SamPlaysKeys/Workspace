# Portable Agent Working Style

## What is this?
This directory contains a **Portable Agent Working Style**. It is designed to be dropped into any new repository or project to instantly give an AI agent (Cursor, Claude Code, etc.) a set of mature, structured behaviors and conventions without needing a complex framework or plugin installation.

## How to use it
You have two modes of operation: **Adopt** (temporary) and **Install** (persistent).

### Mode 1: Adopt (Default)
1. Copy this `workstyle/` directory into the root of your project.
2. Tell your AI Agent: *"Read `workstyle/working_style.md` and adopt its behaviors for this session."*
3. The agent will immediately understand how to scaffold projects, handle troubleshooting, and capture handoffs **in memory**, without altering the target repo's configuration.

### Mode 2: Install
If you want to permanently convert a target repository to use this skill-based architecture:
1. Tell your AI Agent: *"Read `workstyle/working_style.md` and **install** its behaviors."*
2. The agent will extract the behaviors and generate an `.agents/skills/` directory and `AGENTS.md` configuration in the target repository.

## Why portable?
Agent frameworks and specific tool directories (like `.agents/skills/`) are great for established, permanent workspaces. However, when you need to move fast, jump into a foreign repository, or spin up a quick prototype, you still want your agent to behave predictably. 

This portable markdown file encodes the entire "Skill Suite" (Ideate, Troubleshoot, Handoff, etc.) into plain text instructions that any modern LLM can read and follow dynamically.