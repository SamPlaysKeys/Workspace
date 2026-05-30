---
type: Reference
---
# VSCode Setup for Enterprise Development

This guide covers setting up Visual Studio Code (VSCode) as your primary development environment on enterprise devices for working with OpenShift, Ansible, Python, YAML, and Bash.

## Prerequisites

Before configuring VSCode, ensure the following core tools are installed on your system. While VSCode is assumed to be installed, these underlying dependencies are required for functionality.

### 1. Git (git-scm)
Download and install Git from the official [git-scm.com](https://git-scm.com/) site. 
- **Windows**: Use the Git for Windows installer. Ensure "Git from the command line and also from 3rd-party software" is selected during installation.
- **macOS**: Install via Homebrew (`brew install git`) or by installing Xcode Command Line Tools (`xcode-select --install`).

### 2. Python 3
Install Python 3.x from [python.org](https://www.python.org/) or your system's package manager.
- Ensure `pip` is included in the installation.
- Add Python to your system PATH.

### 3. Ansible Lint
Once Python is installed, install `ansible-lint` to enable playbook validation in VSCode:
```bash
pip install ansible-lint
```

---

## Authentication: Personal Access Tokens (PAT)

In an enterprise environment, you will typically use a Personal Access Token (PAT) instead of your account password for Git operations over HTTPS.

### Generating a PAT (GitHub Example)
1. Log in to your GitHub account.
2. Go to **Settings > Developer settings > Personal access tokens > Fine-grained tokens** (or Tokens classic).
3. Click **Generate new token**.
4. Give it a descriptive name (e.g., "VSCode Enterprise Desktop").
5. Select the required scopes (typically `repo` for full control of private repositories).
6. **Copy the token immediately**. You will not be able to see it again.

### Using the PAT
When you clone a repository or push changes for the first time, Git will prompt for your credentials. 
- **Username**: Your GitHub username.
- **Password**: Paste your **PAT** here.

*Note: On Windows and macOS, the system credential manager will securely store this token for future use.*

---

## Recommended Extensions

To get the best experience, install the following extensions from the VSCode Marketplace.

### Core Languages & Tools

| Technology | Extension Name | Publisher | Description |
|------------|----------------|-----------|-------------|
| **Python** | Python | Microsoft | Rich support for Python (IntelliSense, linting, debugging). |
| **YAML** | YAML | Red Hat | YAML Language Support with built-in Kubernetes/OpenShift syntax. |
| **Ansible** | Ansible | Red Hat | Language support, linting, and integration for Ansible playbooks. |
| **Bash** | Bash IDE | mads-hartmann | Bash language server for better autocomplete and linting. |
| **OpenShift** | OpenShift Connector | Red Hat | Interact with OpenShift clusters directly from VSCode. |

### Version Control & Collaboration

| Extension Name | Publisher | Description |
|----------------|-----------|-------------|
| **GitLens** | GitKraken | Supercharge Git capabilities, view line history, and repository heatmaps. |
| **GitHub PRs & Issues** | GitHub | Create, review, and manage GitHub Pull Requests and Issues within VSCode. |
| **Git Graph** | mhutchie | View a Git Graph of your repository and perform Git actions from the graph. |

### Utilities

- **Error Lens**: Highlights errors and warnings inline in your code.
- **Path Intellisense**: Autocompletes filenames.
- **Dotenv**: Support for `.env` file syntax highlighting.

---

## Installing Extensions

To install any of the recommended extensions:
1. Click the **Extensions** icon in the Activity Bar on the side of VSCode (or press `Ctrl + Shift + X`).
2. Type the name of the extension in the search bar.
3. Click **Install**.

---

## Recommended Settings

Add these to your user `settings.json` (`Ctrl + Shift + P` > "Open User Settings (JSON)") for a smoother experience:

```json
{
  "editor.formatOnSave": true,
  "editor.renderWhitespace": "all",
  "files.insertFinalNewline": true,
  "files.trimTrailingWhitespace": true,
  "git.autofetch": true,
  "git.confirmSync": false,
  "workbench.editor.restoreViewState": true,
  "yaml.format.enable": true,
  "ansible.validation.lint.enabled": true
}
```

---

## Setting Up Your Workspace

### 1. Pulling Repositories

Use the integrated terminal (`Ctrl + \``) or the Command Palette (`Ctrl + Shift + P`) to clone your repositories.

```bash
git clone https://github.com/your-org/your-repo.git
```

### 2. Creating a Workspace

If you work across multiple repositories (e.g., one for Ansible playbooks, one for Python tools), use **Multi-root Workspaces**:

1. Open your first repository folder.
2. Go to **File > Add Folder to Workspace...**
3. Select your second repository folder.
4. Save the workspace: **File > Save Workspace As...**

### 3. Version Control Workflow

#### Basic Git Usage
- **Stage Changes**: Click the `+` icon next to files in the **Source Control** tab.
- **Commit**: Enter a message in the text box and click **Commit**.
- **Push/Pull**: Use the sync icon in the status bar or the `...` menu in Source Control.

#### Creating Pull Requests (PRs)
With the **GitHub Pull Requests and Issues** extension:
1. Push your branch to the remote repository.
2. Click the **GitHub** icon in the Activity Bar.
3. Click the **+** (Create Pull Request) icon.
4. Follow the prompts to select base/head branches and enter a title/description.

---

## Plugin Configuration Tips

### Ansible & YAML
To ensure the YAML extension recognizes OpenShift/Kubernetes schemas:
1. Open Settings (`Ctrl + ,`).
2. Search for `yaml.schemas`.
3. Add entries for your Kubernetes/OpenShift manifest patterns.

### Python
Ensure you select the correct Python interpreter:
1. Press `Ctrl + Shift + P`.
2. Type `Python: Select Interpreter`.
3. Choose the version or virtual environment (`.venv`) relevant to your project.

---

## Related Ideas & Next Steps

- **Dev Containers**: Use the **Remote - Containers** extension to define your development environment in a `Dockerfile` or `docker-compose.yaml` within the repo.
- **Snippets**: Create custom snippets for common Ansible tasks or OpenShift manifest headers.
- **Tasks**: Define VSCode Tasks (`tasks.json`) to run your playbooks or tests with a single shortcut.
