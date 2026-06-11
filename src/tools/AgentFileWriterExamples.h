#pragma once

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <memory>

/**
 * @file AgentFileWriterExamples.h
 * @brief Real-world examples of using AgentFileWriterTool
 * 
 * This file demonstrates practical usage patterns for the agent file writing system.
 */

// ═══════════════════════════════════════════════════════════════════════════════
// Example 1: Create Python Project Structure
// ═══════════════════════════════════════════════════════════════════════════════

class Example1_CreatePythonProject {
public:
    /// Agent creates a complete Python project with CLI tool
    static QJsonObject agentCreatePythonProject()
    {
        return QJsonObject({
            {"operation", "write_batch"},
            {"atomic", true},
            {"checkpoint", true},
            {"files", QJsonArray({
                // Main CLI file
                QJsonObject({
                    {"path", "src/main.py"},
                    {"content", R"(#!/usr/bin/env python3
"""GitHub MCP Server - Interact with GitHub API"""

import argparse
import sys
from pathlib import Path

# Add src to path
sys.path.insert(0, str(Path(__file__).parent))

import github_handler
import config

def main():
    parser = argparse.ArgumentParser(
        description="GitHub MCP Server for Claude",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python main.py --org anthropics  # Get org repos
  python main.py --user feifei     # Get user repos
        """
    )
    
    parser.add_argument("--org", help="GitHub organization")
    parser.add_argument("--user", help="GitHub user")
    parser.add_argument("--repo", help="Repository name")
    parser.add_argument("--config", default=".env", help="Config file")
    parser.add_argument("--debug", action="store_true", help="Debug mode")
    
    args = parser.parse_args()
    
    # Load config
    cfg = config.load_config(args.config)
    
    # Process commands
    if args.org:
        repos = github_handler.get_org_repos(cfg, args.org)
        print(f"Found {len(repos)} repositories in {args.org}")
    elif args.user:
        repos = github_handler.get_user_repos(cfg, args.user)
        print(f"Found {len(repos)} repositories for user {args.user}")
    elif args.repo:
        repo = github_handler.get_repo_info(cfg, args.repo)
        print(f"Repository: {repo['name']}")
    else:
        parser.print_help()

if __name__ == "__main__":
    main()
)"}
                }),
                
                // Config module
                QJsonObject({
                    {"path", "src/config.py"},
                    {"content", R"("""Configuration management"""

import os
from pathlib import Path
from dotenv import load_dotenv

def load_config(config_file: str = ".env") -> dict:
    """Load configuration from environment file"""
    if Path(config_file).exists():
        load_dotenv(config_file)
    
    return {
        "github_token": os.getenv("GITHUB_TOKEN"),
        "github_api_url": os.getenv("GITHUB_API_URL", "https://api.github.com"),
        "debug": os.getenv("DEBUG", "false").lower() == "true",
    }

def validate_config(config: dict) -> bool:
    """Validate configuration"""
    return bool(config.get("github_token"))
)"}
                }),
                
                // GitHub handler module
                QJsonObject({
                    {"path", "src/github_handler.py"},
                    {"content", R"("""GitHub API handler"""

import requests
from typing import List, Dict, Any

def get_org_repos(config: dict, org: str) -> List[Dict[str, Any]]:
    """Get all repositories for an organization"""
    headers = {
        "Authorization": f"token {config['github_token']}",
        "Accept": "application/vnd.github.v3+json"
    }
    
    url = f"{config['github_api_url']}/orgs/{org}/repos"
    response = requests.get(url, headers=headers)
    response.raise_for_status()
    
    return response.json()

def get_user_repos(config: dict, user: str) -> List[Dict[str, Any]]:
    """Get all repositories for a user"""
    headers = {
        "Authorization": f"token {config['github_token']}",
        "Accept": "application/vnd.github.v3+json"
    }
    
    url = f"{config['github_api_url']}/users/{user}/repos"
    response = requests.get(url, headers=headers)
    response.raise_for_status()
    
    return response.json()

def get_repo_info(config: dict, repo: str) -> Dict[str, Any]:
    """Get repository information"""
    headers = {
        "Authorization": f"token {config['github_token']}",
        "Accept": "application/vnd.github.v3+json"
    }
    
    url = f"{config['github_api_url']}/repos/{repo}"
    response = requests.get(url, headers=headers)
    response.raise_for_status()
    
    return response.json()
)"}
                }),
                
                // Test file
                QJsonObject({
                    {"path", "tests/test_main.py"},
                    {"content", R"("""Tests for GitHub MCP Server"""

import unittest
from unittest.mock import patch, MagicMock
import sys
from pathlib import Path

# Add src to path
sys.path.insert(0, str(Path(__file__).parent.parent / "src"))

import github_handler
import config

class TestGitHubHandler(unittest.TestCase):
    def setUp(self):
        self.config = {
            "github_token": "test_token",
            "github_api_url": "https://api.github.com"
        }
    
    @patch('requests.get')
    def test_get_org_repos(self, mock_get):
        """Test getting organization repositories"""
        mock_response = MagicMock()
        mock_response.json.return_value = [
            {"name": "repo1", "url": "https://github.com/org/repo1"},
            {"name": "repo2", "url": "https://github.com/org/repo2"},
        ]
        mock_get.return_value = mock_response
        
        repos = github_handler.get_org_repos(self.config, "anthropics")
        
        self.assertEqual(len(repos), 2)
        self.assertEqual(repos[0]["name"], "repo1")

class TestConfig(unittest.TestCase):
    def test_load_config(self):
        """Test configuration loading"""
        config_data = config.load_config()
        self.assertIsInstance(config_data, dict)

if __name__ == "__main__":
    unittest.main()
)"}
                }),
                
                // Requirements file
                QJsonObject({
                    {"path", "requirements.txt"},
                    {"content", R"(requests==2.31.0
python-dotenv==1.0.0
pytest==7.4.0
pytest-cov==4.1.0
)"}
                }),
                
                // README
                QJsonObject({
                    {"path", "README.md"},
                    {"content", R"(# GitHub MCP Server

A Model Context Protocol (MCP) server for interacting with GitHub API through Claude.

## Features

- 🚀 Query GitHub repositories
- 👤 Get user information
- 📊 Organization management
- 🔐 Secure token authentication

## Installation

```bash
pip install -r requirements.txt
```

## Usage

```bash
# Get organization repositories
python src/main.py --org anthropics

# Get user repositories
python src/main.py --user torvalds

# Get repository information
python src/main.py --repo torvalds/linux
```

## Configuration

Create a `.env` file:

```
GITHUB_TOKEN=your_github_token_here
GITHUB_API_URL=https://api.github.com
DEBUG=false
```

## Testing

```bash
pytest tests/ -v --cov=src
```

## License

MIT
)"}
                }),
                
                // .gitignore
                QJsonObject({
                    {"path", ".gitignore"},
                    {"content", R"(__pycache__/
*.py[cod]
*$py.class
*.so
.Python
build/
develop-eggs/
dist/
downloads/
eggs/
.eggs/
lib/
lib64/
parts/
sdist/
var/
wheels/
*.egg-info/
.installed.cfg
*.egg

# Virtual environments
venv/
ENV/
env/

# IDE
.vscode/
.idea/
*.swp
*.swo

# OS
.DS_Store
Thumbs.db

# Environment
.env
.env.local

# Testing
.pytest_cache/
htmlcov/
.coverage

# Logs
*.log
)"}
                }),
                
                // GitHub Actions workflow
                QJsonObject({
                    {"path", ".github/workflows/test.yml"},
                    {"content", R"(name: Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    strategy:
      matrix:
        python-version: ['3.9', '3.10', '3.11']
    
    steps:
    - uses: actions/checkout@v3
    
    - name: Set up Python
      uses: actions/setup-python@v4
      with:
        python-version: ${{ matrix.python-version }}
    
    - name: Install dependencies
      run: |
        python -m pip install --upgrade pip
        pip install -r requirements.txt
    
    - name: Run tests
      run: pytest tests/ -v --cov=src
    
    - name: Upload coverage
      uses: codecov/codecov-action@v3
      with:
        file: ./coverage.xml
)"}
                })
            })}
        });
    }
};

// ═══════════════════════════════════════════════════════════════════════════════
// Example 2: Generate Web Application
// ═══════════════════════════════════════════════════════════════════════════════

class Example2_GenerateWebApp {
public:
    /// Agent creates a complete React + Tailwind web application
    static QJsonObject agentCreateWebApp()
    {
        return QJsonObject({
            {"operation", "write_batch"},
            {"atomic", true},
            {"files", QJsonArray({
                // HTML entry point
                QJsonObject({
                    {"path", "index.html"},
                    {"content", R"(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Task Management App</title>
    <script src="https://cdn.tailwindcss.com"></script>
    <script src="https://unpkg.com/react@18/umd/react.production.min.js"></script>
    <script src="https://unpkg.com/react-dom@18/umd/react-dom.production.min.js"></script>
</head>
<body class="bg-gray-50">
    <div id="root"></div>
    <script src="app.js"></script>
</body>
</html>
)"}
                }),
                
                // React app component
                QJsonObject({
                    {"path", "app.js"},
                    {"content", R"(const { useState } = React;

function App() {
    const [tasks, setTasks] = useState([]);
    const [input, setInput] = useState("");
    
    const addTask = () => {
        if (input.trim()) {
            setTasks([...tasks, { id: Date.now(), text: input, done: false }]);
            setInput("");
        }
    };
    
    const toggleTask = (id) => {
        setTasks(tasks.map(t => t.id === id ? {...t, done: !t.done} : t));
    };
    
    const deleteTask = (id) => {
        setTasks(tasks.filter(t => t.id !== id));
    };
    
    return (
        <div className="min-h-screen bg-gradient-to-br from-blue-50 to-indigo-100 py-8 px-4">
            <div className="max-w-md mx-auto bg-white rounded-lg shadow-lg p-6">
                <h1 className="text-3xl font-bold text-gray-800 mb-6">Task Manager</h1>
                
                <div className="flex gap-2 mb-6">
                    <input
                        type="text"
                        value={input}
                        onChange={(e) => setInput(e.target.value)}
                        onKeyPress={(e) => e.key === 'Enter' && addTask()}
                        placeholder="Add a new task..."
                        className="flex-1 px-4 py-2 border border-gray-300 rounded-lg focus:outline-none focus:ring-2 focus:ring-blue-500"
                    />
                    <button
                        onClick={addTask}
                        className="bg-blue-500 text-white px-4 py-2 rounded-lg hover:bg-blue-600 transition"
                    >
                        Add
                    </button>
                </div>
                
                <div className="space-y-2">
                    {tasks.map(task => (
                        <div key={task.id} className="flex items-center gap-2 p-3 bg-gray-50 rounded-lg">
                            <input
                                type="checkbox"
                                checked={task.done}
                                onChange={() => toggleTask(task.id)}
                                className="w-5 h-5 text-blue-500"
                            />
                            <span className={task.done ? "line-through text-gray-400" : "text-gray-800"}>
                                {task.text}
                            </span>
                            <button
                                onClick={() => deleteTask(task.id)}
                                className="ml-auto text-red-500 hover:text-red-700 transition"
                            >
                                ✕
                            </button>
                        </div>
                    ))}
                </div>
                
                {tasks.length === 0 && (
                    <p className="text-center text-gray-400 mt-8">No tasks yet. Add one to get started!</p>
                )}
            </div>
        </div>
    );
}

ReactDOM.createRoot(document.getElementById('root')).render(<App />);
)"}
                }),
                
                // CSS styles
                QJsonObject({
                    {"path", "styles.css"},
                    {"content", R"(/* Tailwind CSS is used - this file can contain custom styles */

/* Smooth transitions */
* {
    transition: all 0.3s ease;
}

/* Custom scrollbar */
::-webkit-scrollbar {
    width: 8px;
}

::-webkit-scrollbar-track {
    background: #f1f1f1;
}

::-webkit-scrollbar-thumb {
    background: #888;
    border-radius: 4px;
}

::-webkit-scrollbar-thumb:hover {
    background: #555;
}
)"}
                })
            })}
        });
    }
};

// ═══════════════════════════════════════════════════════════════════════════════
// Example 3: Update Existing File
// ═══════════════════════════════════════════════════════════════════════════════

class Example3_UpdateExistingFile {
public:
    /// Agent adds documentation section to existing README
    static QJsonObject agentAppendToReadme()
    {
        return QJsonObject({
            {"operation", "update_file"},
            {"path", "README.md"},
            {"mode", "append"},
            {"content", R"(

## Architecture

### Components

- **Main Server** - FastAPI application
- **GitHub Client** - Async GitHub API integration
- **MCP Protocol** - Model Context Protocol implementation

### Directory Structure

```
.
├── src/
│   ├── main.py          # Main server
│   ├── github_client.py # GitHub integration
│   └── models/          # Data models
├── tests/               # Test suite
├── docs/                # Documentation
└── README.md           # This file
```

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Run tests
5. Submit a pull request

## Support

For issues and questions, please open a GitHub issue.
)"}
        });
    }
};

// ═══════════════════════════════════════════════════════════════════════════════
// Example 4: Template-Based Generation
// ═══════════════════════════════════════════════════════════════════════════════

class Example4_TemplateGeneration {
public:
    /// Agent generates CLI application from template
    static QJsonObject agentGenerateFromTemplate()
    {
        return QJsonObject({
            {"operation", "write_template"},
            {"path", "src/cli_app.py"},
            {"template_name", "python-cli"},
            {"variables", QJsonObject({
                {"PROJECT_NAME", "DataProcessor"},
                {"PROJECT_DESC", "Process and analyze large datasets"}
            })}
        });
    }
};

// ═══════════════════════════════════════════════════════════════════════════════
// Example 5: Create Directory Structure
// ═══════════════════════════════════════════════════════════════════════════════

class Example5_CreateDirectoryStructure {
public:
    /// Agent creates complete project structure
    static QJsonObject agentCreateProjectStructure()
    {
        return QJsonObject({
            {"operation", "create_structure"},
            {"structure", QJsonObject({
                {"src", QJsonObject({
                    {"main.py", "# Main entry point\nif __name__ == '__main__':\n    pass"},
                    {"utils.py", "# Utility functions\ndef helper():\n    pass"},
                    {"config.py", "# Configuration"}
                })},
                {"tests", QJsonObject({
                    {"test_main.py", "import unittest\n\nclass Tests(unittest.TestCase):\n    pass"}
                })},
                {"docs", QJsonObject({
                    {"README.md", "# Documentation"}
                })},
                {"requirements.txt", "# Add dependencies here"},
                {".gitignore", "__pycache__/\n*.pyc\n.env"}
            })}
        });
    }
};
