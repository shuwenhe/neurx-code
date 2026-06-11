#include "SkillMCPBuilder.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonArray>

SkillMCPBuilder::SkillMCPBuilder()
{
    initializeBestPractices();
}

void SkillMCPBuilder::initializeBestPractices()
{
    m_bestPractices = {
        {
            "API Coverage vs Workflow Tools",
            "Balance comprehensive API endpoint coverage with specialized workflow tools",
            "Prioritize comprehensive API coverage to give agents flexibility. Workflow tools should handle common multi-step operations.",
            "Include both list_items (comprehensive) and create_and_configure_item (workflow)",
            "design"
        },
        {
            "Tool Naming",
            "Clear, descriptive tool names help agents find the right tools quickly",
            "Use consistent prefixes (e.g., github_create_issue, github_list_repos) and action-oriented naming",
            "github_create_issue, github_list_repos, github_update_issue",
            "design"
        },
        {
            "Error Handling",
            "Error messages should guide agents toward solutions",
            "Provide specific suggestions and next steps in error messages",
            "\"Invalid API key. Please set GITHUB_API_KEY environment variable.\"",
            "reliability"
        },
        {
            "Context Management",
            "Agents benefit from concise tool descriptions and result filtering",
            "Design tools that return focused, relevant data. Support pagination and filtering.",
            "Add limit, offset, filter parameters to list operations",
            "performance"
        },
        {
            "Authentication Patterns",
            "Choose authentication that matches the service's capabilities",
            "Support bearer tokens, OAuth, API keys. Store securely.",
            "Use environment variables with clear naming conventions",
            "security"
        },
        {
            "Pagination Support",
            "Large result sets should be paginated",
            "Use consistent pagination patterns (limit/offset or cursor-based)",
            "list_items(limit=50, offset=0), list_items(limit=50, cursor='next-page-id')",
            "performance"
        },
        {
            "Rate Limiting Awareness",
            "Be aware of API rate limits and implement backoff",
            "Implement exponential backoff, cache responses when possible",
            "Catch 429 errors, wait with exponential backoff",
            "reliability"
        }
    };
}

QString SkillMCPBuilder::generateProjectStructure(
    const MCPServerProject &project,
    const QString &outputPath)
{
    QString structure = QString(
        "MCP Server Project: %1\n"
        "Location: %2\n\n"
        "Generated Structure:\n"
        "%1/\n"
        "├── README.md\n"
        "├── .env.example\n"
        "├── package.json (TypeScript) or pyproject.toml (Python)\n"
        "├── src/\n"
        "│   ├── index.ts / main.py\n"
        "│   ├── tools/\n"
        "│   │   └── <tool-name>.ts/py\n"
        "│   ├── resources/\n"
        "│   │   └── <resource-handler>.ts/py\n"
        "│   ├── auth/\n"
        "│   │   └── authenticator.ts/py\n"
        "│   └── utils/\n"
        "│       ├── api-client.ts/py\n"
        "│       ├── errors.ts/py\n"
        "│       └── logger.ts/py\n"
        "├── tests/\n"
        "│   ├── tools.test.ts/py\n"
        "│   └── integration.test.ts/py\n"
        "└── .gitignore\n"
    ).arg(project.name, outputPath);

    return structure;
}

QString SkillMCPBuilder::generatePackageJson(const MCPServerProject &project) const
{
    QJsonObject packageJson;
    packageJson["name"] = project.name;
    packageJson["version"] = project.version;
    packageJson["description"] = project.description;
    packageJson["type"] = "module";
    packageJson["main"] = "build/index.js";

    QJsonObject scripts;
    scripts["build"] = "tsc";
    scripts["dev"] = "tsx src/index.ts";
    scripts["test"] = "jest";
    scripts["start"] = "node build/index.js";
    packageJson["scripts"] = scripts;

    QJsonArray dependencies;
    for (const auto &dep : project.dependencies) {
        dependencies.append(dep);
    }
    
    QJsonObject devDeps;
    devDeps["@types/node"] = "^20.0.0";
    devDeps["typescript"] = "^5.0.0";
    devDeps["@typescript-eslint/eslint-plugin"] = "^6.0.0";
    devDeps["@typescript-eslint/parser"] = "^6.0.0";
    
    QJsonDocument doc(packageJson);
    return QString::fromUtf8(doc.toJson(QJsonDocument::Indented));
}

QString SkillMCPBuilder::generatePyprojectToml(const MCPServerProject &project) const
{
    QString toml = QString(
        "[project]\n"
        "name = \"%1\"\n"
        "version = \"%2\"\n"
        "description = \"%3\"\n"
        "requires-python = \">=3.10\"\n"
        "dependencies = [\n"
    ).arg(project.name, project.version, project.description);

    for (const auto &dep : project.dependencies) {
        toml += QString("    \"%1\",\n").arg(dep);
    }

    toml += QString(
        "]\n\n"
        "[build-system]\n"
        "requires = [\"setuptools\", \"wheel\"]\n"
        "build-backend = \"setuptools.build_meta\"\n"
    );

    return toml;
}

QString SkillMCPBuilder::generateServerMain(const MCPServerProject &project) const
{
    if (project.language == Language::TypeScript) {
        return QString(
            "import { Server } from \"@modelcontextprotocol/sdk/server/index.js\";\n"
            "import { StdioServerTransport } from \"@modelcontextprotocol/sdk/server/stdio.js\";\n"
            "import { Tool, TextContent } from \"@modelcontextprotocol/sdk/types.js\";\n\n"
            "const server = new Server({\n"
            "  name: \"%1\",\n"
            "  version: \"%2\"\n"
            "});\n\n"
            "// Register tools\n"
            "const tools: Tool[] = [\n"
        ).arg(project.name, project.version);
    }
    else {
        return QString(
            "from mcp.server import Server\n"
            "from mcp.server.stdio import stdio_server\n"
            "import logging\n\n"
            "logging.basicConfig(level=logging.DEBUG)\n"
            "logger = logging.getLogger(__name__)\n\n"
            "server = Server(\"%1\")\n\n"
            "@server.list_tools()\n"
            "async def list_tools():\n"
            "    return [\n"
        ).arg(project.name);
    }
}

QString SkillMCPBuilder::generateToolDefinition(const ToolDefinition &tool) const
{
    QJsonObject toolDef;
    toolDef["name"] = tool.name;
    toolDef["description"] = tool.description;
    toolDef["inputSchema"] = tool.inputSchema;

    QJsonDocument doc(toolDef);
    return QString::fromUtf8(doc.toJson(QJsonDocument::Indented));
}

QString SkillMCPBuilder::generateToolImplementation(
    const ToolDefinition &tool,
    Language language) const
{
    if (language == Language::TypeScript) {
        return QString(
            "/**\n"
            " * %1\n"
            " * %2\n"
            " */\n"
            "export async function %3(input: any): Promise<string> {\n"
            "    // TODO: Implement %1\n"
            "    // Input schema: %2\n"
            "    try {\n"
            "        // Call external API\n"
            "        // const result = await apiClient.%3(input);\n"
            "        // return JSON.stringify(result);\n"
            "        throw new Error('Not implemented');\n"
            "    } catch (error) {\n"
            "        throw new Error(`Failed to %3: ${error.message}`);\n"
            "    }\n"
            "}\n"
        ).arg(tool.name, tool.description, tool.name.toLower());
    }
    else {
        return QString(
            "async def %1(input: dict) -> str:\n"
            "    \"\"\"\n"
            "    %2\n"
            "    Input schema: %3\n"
            "    \"\"\"\n"
            "    try:\n"
            "        # TODO: Implement %1\n"
            "        # result = await api_client.%1(input)\n"
            "        # return json.dumps(result)\n"
            "        raise NotImplementedError('%1')\n"
            "    except Exception as e:\n"
            "        raise Exception(f'Failed to %1: {str(e)}')\n"
        ).arg(tool.name.toLower(), tool.description, tool.description);
    }
}

bool SkillMCPBuilder::validateToolDefinition(const ToolDefinition &tool, QString &errorMsg) const
{
    if (tool.name.isEmpty()) {
        errorMsg = "Tool name is required";
        return false;
    }

    if (tool.description.isEmpty()) {
        errorMsg = "Tool description is required";
        return false;
    }

    if (tool.inputSchema.isEmpty()) {
        errorMsg = "Tool input schema is required";
        return false;
    }

    // Additional validation could go here
    return true;
}

QString SkillMCPBuilder::generateToolRegistry(
    const QVector<ToolDefinition> &tools,
    Language language) const
{
    QString registry;

    if (language == Language::TypeScript) {
        registry = "export const toolRegistry = {\n";
        for (const auto &tool : tools) {
            registry += QString("  %1: %2,\n").arg(tool.name.toLower(), tool.name.toLower());
        }
        registry += "};\n";
    }
    else {
        registry = "tool_registry = {\n";
        for (const auto &tool : tools) {
            registry += QString("    '%1': %2,\n").arg(tool.name.toLower(), tool.name.toLower());
        }
        registry += "}\n";
    }

    return registry;
}

QString SkillMCPBuilder::generateResourceDefinition(const ResourceDefinition &resource) const
{
    QJsonObject resourceDef;
    resourceDef["uri"] = resource.uri;
    resourceDef["mimeType"] = resource.mimeType;
    resourceDef["description"] = resource.description;

    QJsonDocument doc(resourceDef);
    return QString::fromUtf8(doc.toJson(QJsonDocument::Indented));
}

QString SkillMCPBuilder::generateResourceHandler(
    const ResourceDefinition &resource,
    Language language) const
{
    if (language == Language::TypeScript) {
        return QString(
            "export async function readResource(uri: string): Promise<string> {\n"
            "    if (uri !== '%1') {\n"
            "        throw new Error('Resource not found');\n"
            "    }\n"
            "    // TODO: Load resource %2\n"
            "    return '';\n"
            "}\n"
        ).arg(resource.uri, resource.uri);
    }
    else {
        return QString(
            "async def read_resource(uri: str) -> str:\n"
            "    \"\"\"Read resource: %1\"\"\"\n"
            "    if uri != '%2':\n"
            "        raise Exception('Resource not found')\n"
            "    # TODO: Load resource\n"
            "    return ''\n"
        ).arg(resource.description, resource.uri);
    }
}

QString SkillMCPBuilder::generateAuthModule(
    const QString &authType,
    Language language) const
{
    if (language == Language::TypeScript) {
        if (authType == "bearer") {
            return R"(
export class BearerAuth {
    private token: string;
    
    constructor() {
        this.token = process.env.API_TOKEN || '';
    }
    
    getHeaders(): Record<string, string> {
        return {
            'Authorization': `Bearer ${this.token}`
        };
    }
}
            )";
        }
        else if (authType == "api_key") {
            return R"(
export class APIKeyAuth {
    private apiKey: string;
    
    constructor() {
        this.apiKey = process.env.API_KEY || '';
    }
    
    getHeaders(): Record<string, string> {
        return {
            'X-API-Key': this.apiKey
        };
    }
}
            )";
        }
    }
    else if (language == Language::Python) {
        if (authType == "bearer") {
            return R"(
import os

class BearerAuth:
    def __init__(self):
        self.token = os.getenv('API_TOKEN', '')
    
    def get_headers(self):
        return {
            'Authorization': f'Bearer {self.token}'
        }
            )";
        }
    }

    return "// Authentication not configured";
}

QString SkillMCPBuilder::generateConfigTemplate(const MCPServerProject &project) const
{
    QString config = "# Configuration for " + project.name + "\n\n";
    config += "# API Configuration\n";
    config += "API_BASE_URL=" + project.apiBaseUrl + "\n";
    config += "API_AUTH_TYPE=" + project.apiAuthType + "\n";
    config += "API_KEY=<your-api-key>\n";
    config += "API_TOKEN=<your-api-token>\n\n";
    config += "# Server Configuration\n";
    config += "SERVER_PORT=8000\n";
    config += "LOG_LEVEL=INFO\n";
    
    return config;
}

QString SkillMCPBuilder::generateAPIClient(
    const MCPServerProject &project,
    Language language) const
{
    if (language == Language::TypeScript) {
        return QString(
            "import axios from 'axios';\n\n"
            "export class APIClient {\n"
            "    private baseURL: string;\n"
            "    private headers: Record<string, string>;\n\n"
            "    constructor(baseURL: string, headers: Record<string, string>) {\n"
            "        this.baseURL = baseURL;\n"
            "        this.headers = headers;\n"
            "    }\n\n"
            "    async request(method: string, path: string, data?: any) {\n"
            "        try {\n"
            "            const response = await axios({\n"
            "                method,\n"
            "                url: `${this.baseURL}${path}`,\n"
            "                headers: this.headers,\n"
            "                data\n"
            "            });\n"
            "            return response.data;\n"
            "        } catch (error: any) {\n"
            "            throw new Error(`API request failed: ${error.message}`);\n"
            "        }\n"
            "    }\n"
            "}\n"
        );
    }
    else {
        return QString(
            "import aiohttp\n"
            "import json\n\n"
            "class APIClient:\n"
            "    def __init__(self, base_url: str, headers: dict):\n"
            "        self.base_url = base_url\n"
            "        self.headers = headers\n\n"
            "    async def request(self, method: str, path: str, data: dict = None):\n"
            "        try:\n"
            "            async with aiohttp.ClientSession() as session:\n"
            "                async with session.request(\n"
            "                    method,\n"
            "                    f'{self.base_url}{path}',\n"
            "                    headers=self.headers,\n"
            "                    json=data\n"
            "                ) as response:\n"
            "                    return await response.json()\n"
            "        except Exception as e:\n"
            "            raise Exception(f'API request failed: {str(e)}')\n"
        );
    }
}

QString SkillMCPBuilder::generateErrorHandling(Language language) const
{
    if (language == Language::TypeScript) {
        return R"(
export class MCPError extends Error {
    constructor(code: string, message: string) {
        super(message);
        this.name = 'MCPError';
    }
}

export class APIError extends MCPError {
    constructor(statusCode: number, message: string) {
        super('API_ERROR', `HTTP ${statusCode}: ${message}`);
    }
}

export class ValidationError extends MCPError {
    constructor(message: string) {
        super('VALIDATION_ERROR', message);
    }
}

export function handleError(error: any): void {
    if (error instanceof MCPError) {
        console.error(`[${error.name}] ${error.message}`);
    } else {
        console.error(`[UnknownError] ${error.message}`);
    }
}
        )";
    }
    else {
        return R"(
class MCPError(Exception):
    def __init__(self, code: str, message: str):
        self.code = code
        super().__init__(message)

class APIError(MCPError):
    def __init__(self, status_code: int, message: str):
        super().__init__('API_ERROR', f'HTTP {status_code}: {message}')

class ValidationError(MCPError):
    def __init__(self, message: str):
        super().__init__('VALIDATION_ERROR', message)

def handle_error(error: Exception) -> None:
    if isinstance(error, MCPError):
        print(f"[{error.code}] {error}")
    else:
        print(f"[UnknownError] {error}")
        )";
    }
}

QString SkillMCPBuilder::generateValidation(Language language) const
{
    if (language == Language::TypeScript) {
        return R"(
import { z } from 'zod';

export function validateInput(schema: z.ZodSchema, input: any): any {
    try {
        return schema.parse(input);
    } catch (error) {
        if (error instanceof z.ZodError) {
            throw new Error(`Validation failed: ${error.errors.map(e => e.message).join(', ')}`);
        }
        throw error;
    }
}
        )";
    }
    else {
        return R"(
from pydantic import BaseModel, ValidationError

def validate_input(model_class: type[BaseModel], input_data: dict) -> BaseModel:
    try:
        return model_class(**input_data)
    except ValidationError as e:
        raise Exception(f"Validation failed: {e}")
        )";
    }
}

QString SkillMCPBuilder::generateLogging(Language language) const
{
    if (language == Language::TypeScript) {
        return R"(
export class Logger {
    static debug(message: string, meta?: any) {
        console.log(`[DEBUG] ${message}`, meta || '');
    }
    
    static info(message: string, meta?: any) {
        console.log(`[INFO] ${message}`, meta || '');
    }
    
    static warn(message: string, meta?: any) {
        console.warn(`[WARN] ${message}`, meta || '');
    }
    
    static error(message: string, error?: any) {
        console.error(`[ERROR] ${message}`, error || '');
    }
}
        )";
    }
    else {
        return R"(
import logging

logger = logging.getLogger(__name__)

def setup_logging(level=logging.INFO):
    handler = logging.StreamHandler()
    formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
    handler.setFormatter(formatter)
    logger.addHandler(handler)
    logger.setLevel(level)
    return logger
        )";
    }
}

QString SkillMCPBuilder::generateTestFile(
    const ToolDefinition &tool,
    Language language) const
{
    if (language == Language::TypeScript) {
        return QString(
            "import { describe, it, expect } from '@jest/globals';\n"
            "import { %1 } from '../src/tools/%2';\n\n"
            "describe('%1', () => {\n"
            "    it('should execute successfully', async () => {\n"
            "        const input = { /* test input */ };\n"
            "        const result = await %1(input);\n"
            "        expect(result).toBeDefined();\n"
            "    });\n\n"
            "    it('should handle errors', async () => {\n"
            "        const input = { /* invalid input */ };\n"
            "        await expect(%1(input)).rejects.toThrow();\n"
            "    });\n"
            "});\n"
        ).arg(tool.name, tool.name.toLower());
    }
    else {
        return QString(
            "import pytest\n"
            "from src.tools.%1 import %2\n\n"
            "pytest.mark.asyncio\n"
            "async def test_%1_success():\n"
            "    input_data = {}\n"
            "    result = await %2(input_data)\n"
            "    assert result is not None\n\n"
            "@pytest.mark.asyncio\n"
            "async def test_%1_error():\n"
            "    input_data = {}\n"
            "    with pytest.raises(Exception):\n"
            "        await %2(input_data)\n"
        ).arg(tool.name.toLower(), tool.name.toLower());
    }
}

QString SkillMCPBuilder::generateIntegrationTest(Language language) const
{
    if (language == Language::TypeScript) {
        return R"(
import { describe, it, expect, beforeAll, afterAll } from '@jest/globals';
import { Server } from '../src/index';

describe('MCP Server Integration', () => {
    let server: Server;
    
    beforeAll(async () => {
        server = new Server();
        await server.start();
    });
    
    afterAll(async () => {
        await server.stop();
    });
    
    it('should list tools', async () => {
        const tools = await server.listTools();
        expect(tools.length).toBeGreaterThan(0);
    });
});
        )";
    }
    else {
        return R"(
import pytest
from src.server import server

@pytest.mark.asyncio
async def test_list_tools():
    tools = await server.list_tools()
    assert len(tools) > 0

@pytest.mark.asyncio
async def test_tool_execution():
    result = await server.execute_tool('tool_name', {})
    assert result is not None
        )";
    }
}

QString SkillMCPBuilder::generateMockServer(Language language) const
{
    if (language == Language::TypeScript) {
        return R"(
export class MockServer {
    private tools = new Map();
    
    registerTool(name: string, handler: Function) {
        this.tools.set(name, handler);
    }
    
    async executeTool(name: string, input: any) {
        const handler = this.tools.get(name);
        if (!handler) throw new Error(`Tool not found: ${name}`);
        return await handler(input);
    }
    
    async listTools() {
        return Array.from(this.tools.keys());
    }
}
        )";
    }
    else {
        return R"(
class MockServer:
    def __init__(self):
        self.tools = {}
    
    def register_tool(self, name: str, handler):
        self.tools[name] = handler
    
    async def execute_tool(self, name: str, input_data: dict):
        if name not in self.tools:
            raise Exception(f'Tool not found: {name}')
        return await self.tools[name](input_data)
    
    async def list_tools(self):
        return list(self.tools.keys())
        )";
    }
}

QString SkillMCPBuilder::generateREADME(const MCPServerProject &project) const
{
    QString readme = QString(
        "# %1\n\n"
        "%2\n\n"
        "## Getting Started\n\n"
        "### Installation\n\n"
        "```bash\n"
    ).arg(project.name, project.description);

    if (project.language == Language::TypeScript) {
        readme += "npm install\nnpm run build\n";
    }
    else {
        readme += "pip install -r requirements.txt\n";
    }

    readme += QString(
        "```\n\n"
        "### Configuration\n\n"
        "1. Copy `.env.example` to `.env`\n"
        "2. Add your API credentials\n\n"
        "### Running\n\n"
        "```bash\n"
    );

    if (project.language == Language::TypeScript) {
        readme += "npm start\n";
    }
    else {
        readme += "python main.py\n";
    }

    readme += QString(
        "```\n\n"
        "## Available Tools\n\n"
    );

    for (const auto &tool : project.tools) {
        readme += QString("- **%1**: %2\n").arg(tool.name, tool.description);
    }

    readme += QString(
        "\n## API Documentation\n\n"
        "See [docs/API.md](docs/API.md) for detailed API documentation.\n\n"
        "## Testing\n\n"
        "```bash\n"
    );

    if (project.language == Language::TypeScript) {
        readme += "npm test\n";
    }
    else {
        readme += "pytest\n";
    }

    readme += "```\n";

    return readme;
}

QString SkillMCPBuilder::generateAPIDocumentation(const MCPServerProject &project) const
{
    QString docs = "# API Documentation\n\n";
    docs += "Base URL: " + project.apiBaseUrl + "\n";
    docs += "Auth Type: " + project.apiAuthType + "\n\n";
    docs += "## Endpoints\n\n";

    for (const auto &tool : project.tools) {
        docs += QString("### %1\n\n%2\n\n").arg(tool.name, tool.description);
    }

    return docs;
}

QString SkillMCPBuilder::generateToolDocumentation(const QVector<ToolDefinition> &tools) const
{
    QString docs = "# Tool Reference\n\n";

    for (const auto &tool : tools) {
        docs += QString("## %1\n\n%2\n\n**Input Schema:**\n```json\n%3\n```\n\n")
            .arg(tool.name, tool.description, 
                 QString::fromUtf8(QJsonDocument(tool.inputSchema).toJson(QJsonDocument::Indented)));
    }

    return docs;
}

QVector<SkillMCPBuilder::BestPractice> SkillMCPBuilder::getBestPractices(
    const QString &category) const
{
    if (category.isEmpty()) {
        return m_bestPractices;
    }

    QVector<BestPractice> filtered;
    for (const auto &practice : m_bestPractices) {
        if (practice.category == category) {
            filtered.append(practice);
        }
    }
    return filtered;
}

QString SkillMCPBuilder::getPerformanceGuidelines() const
{
    return R"(
MCP Server Performance Guidelines:

1. **Response Times**
   - Aim for <100ms for simple tools
   - <1s for API calls
   - <5s for heavy operations

2. **Memory Management**
   - Cache API clients and connections
   - Implement connection pooling
   - Clean up resources properly

3. **Concurrency**
   - Use async/await for I/O operations
   - Implement rate limiting
   - Handle concurrent requests

4. **Optimization**
   - Batch API requests when possible
   - Use pagination for large results
   - Cache frequently accessed data
    )";
}

QString SkillMCPBuilder::getSecurityGuidelines() const
{
    return R"(
MCP Server Security Guidelines:

1. **Authentication**
   - Store credentials in environment variables
   - Never commit secrets to version control
   - Use secure token storage

2. **Input Validation**
   - Validate all user inputs
   - Use schema validation
   - Sanitize strings before use

3. **API Security**
   - Use HTTPS for all communications
   - Implement rate limiting
   - Set appropriate timeouts

4. **Error Handling**
   - Don't expose internal errors to users
   - Log security-relevant events
   - Implement audit trails
    )";
}

QString SkillMCPBuilder::getScalabilityPatterns() const
{
    return R"(
MCP Server Scalability Patterns:

1. **Horizontal Scaling**
   - Stateless server design
   - Use message queues
   - External session management

2. **Caching Strategy**
   - Cache API responses
   - Implement cache invalidation
   - Use CDN for static content

3. **Database Design**
   - Use indexed queries
   - Implement connection pooling
   - Plan for data growth

4. **Monitoring**
   - Track response times
   - Monitor resource usage
   - Set up alerts
    )";
}

QString SkillMCPBuilder::generatePaginationPattern(Language language) const
{
    if (language == Language::TypeScript) {
        return R"(
export interface PaginationParams {
    limit: number;
    offset: number;
}

export interface PaginatedResponse<T> {
    items: T[];
    total: number;
    limit: number;
    offset: number;
    hasMore: boolean;
}

export async function paginate<T>(
    fetcher: (limit: number, offset: number) => Promise<T[]>,
    limit: number = 50,
    offset: number = 0
): Promise<PaginatedResponse<T>> {
    const items = await fetcher(limit, offset);
    return {
        items,
        total: items.length + offset,
        limit,
        offset,
        hasMore: items.length === limit
    };
}
        )";
    }
    else {
        return R"(
from dataclasses import dataclass
from typing import List, TypeVar

T = TypeVar('T')

@dataclass
class PaginationParams:
    limit: int = 50
    offset: int = 0

@dataclass
class PaginatedResponse:
    items: List[T]
    total: int
    limit: int
    offset: int
    has_more: bool

async def paginate(fetcher, limit=50, offset=0):
    items = await fetcher(limit, offset)
    return PaginatedResponse(
        items=items,
        total=len(items) + offset,
        limit=limit,
        offset=offset,
        has_more=len(items) == limit
    )
        )";
    }
}

QString SkillMCPBuilder::generateRetryLogic(Language language) const
{
    if (language == Language::TypeScript) {
        return R"(
export async function retry<T>(
    fn: () => Promise<T>,
    maxRetries: number = 3,
    delayMs: number = 1000
): Promise<T> {
    for (let i = 0; i < maxRetries; i++) {
        try {
            return await fn();
        } catch (error) {
            if (i === maxRetries - 1) throw error;
            await new Promise(r => setTimeout(r, delayMs * Math.pow(2, i)));
        }
    }
    throw new Error('Retry failed');
}
        )";
    }
    else {
        return R"(
import asyncio

async def retry(fn, max_retries=3, delay_ms=1000):
    for i in range(max_retries):
        try:
            return await fn()
        except Exception as e:
            if i == max_retries - 1:
                raise
            await asyncio.sleep(delay_ms * (2 ** i) / 1000)
        )";
    }
}

QString SkillMCPBuilder::generateRateLimiting(Language language) const
{
    if (language == Language::TypeScript) {
        return R"(
export class RateLimiter {
    private queue: (() => Promise<any>)[] = [];
    private active = 0;
    private maxConcurrent: number;
    
    constructor(maxConcurrent: number = 10) {
        this.maxConcurrent = maxConcurrent;
    }
    
    async run<T>(fn: () => Promise<T>): Promise<T> {
        while (this.active >= this.maxConcurrent) {
            await new Promise(r => setTimeout(r, 100));
        }
        this.active++;
        try {
            return await fn();
        } finally {
            this.active--;
        }
    }
}
        )";
    }
    else {
        return R"(
import asyncio
from typing import Coroutine, TypeVar

T = TypeVar('T')

class RateLimiter:
    def __init__(self, max_concurrent=10):
        self.semaphore = asyncio.Semaphore(max_concurrent)
    
    async def run(self, fn: Coroutine[None, None, T]) -> T:
        async with self.semaphore:
            return await fn
        )";
    }
}

QString SkillMCPBuilder::generateCaching(Language language) const
{
    if (language == Language::TypeScript) {
        return R"(
export class Cache<T> {
    private data = new Map<string, T>();
    private ttl: number;
    
    constructor(ttlMs: number = 60000) {
        this.ttl = ttlMs;
    }
    
    set(key: string, value: T): void {
        this.data.set(key, value);
        setTimeout(() => this.data.delete(key), this.ttl);
    }
    
    get(key: string): T | undefined {
        return this.data.get(key);
    }
    
    has(key: string): boolean {
        return this.data.has(key);
    }
}
        )";
    }
    else {
        return R"(
import time
from typing import Optional, TypeVar, Generic

T = TypeVar('T')

class Cache(Generic[T]):
    def __init__(self, ttl_seconds: int = 60):
        self.data = {}
        self.ttl = ttl_seconds
    
    def set(self, key: str, value: T) -> None:
        self.data[key] = (value, time.time())
    
    def get(self, key: str) -> Optional[T]:
        if key not in self.data:
            return None
        value, timestamp = self.data[key]
        if time.time() - timestamp > self.ttl:
            del self.data[key]
            return None
        return value
        )";
    }
}

QString SkillMCPBuilder::detectServiceType(const QString &apiDescription) const
{
    if (apiDescription.toLower().contains("github")) return "GitHub";
    if (apiDescription.toLower().contains("slack")) return "Slack";
    if (apiDescription.toLower().contains("jira")) return "Jira";
    if (apiDescription.toLower().contains("stripe")) return "Stripe";
    if (apiDescription.toLower().contains("openai")) return "OpenAI";
    return "Generic";
}

QVector<QString> SkillMCPBuilder::getRecommendedToolsForService(
    const QString &serviceType) const
{
    QMap<QString, QVector<QString>> serviceTools = {
        {"GitHub", {"list_repos", "create_issue", "list_issues", "update_issue", "list_pulls"}},
        {"Slack", {"list_channels", "send_message", "list_messages", "create_channel", "update_topic"}},
        {"Jira", {"list_projects", "create_issue", "list_issues", "update_issue", "search_issues"}},
        {"Generic", {"list_items", "get_item", "create_item", "update_item", "delete_item"}}
    };

    return serviceTools.value(serviceType, serviceTools["Generic"]);
}

QVector<QString> SkillMCPBuilder::generateToolNamesForService(
    const QString &serviceType,
    int count) const
{
    auto tools = getRecommendedToolsForService(serviceType);
    QVector<QString> result;
    for (int i = 0; i < std::min(count, static_cast<int>(tools.size())); ++i) {
        result.append(tools[i]);
    }
    return result;
}

QString SkillMCPBuilder::languageToString(Language lang) const
{
    return (lang == Language::TypeScript) ? "TypeScript" : "Python";
}
