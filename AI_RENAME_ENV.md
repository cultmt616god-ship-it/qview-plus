# AI Rename Environment Variables

You can configure the AI Rename feature using environment variables instead of or in addition to the settings dialog. This is useful for:

- Keeping API keys secure
- Using different configurations for different environments
- Automating deployments

## Available Environment Variables

| Variable | Description | Default Value |
|----------|-------------|---------------|
| `QVIEW_AI_API_KEY` | API key for the AI service | (empty) |
| `QVIEW_AI_BASE_URL` | Base URL for the AI service | `https://api.openai.com/v1` |
| `QVIEW_AI_MODEL` | Model name to use | `gpt-4-vision-preview` |

## Usage Examples

### Linux/macOS
```bash
export QVIEW_AI_API_KEY="your-api-key-here"
export QVIEW_AI_BASE_URL="https://api.openai.com/v1"
export QVIEW_AI_MODEL="gpt-4-vision-preview"
./qview
```

### Windows Command Prompt
```cmd
set QVIEW_AI_API_KEY=your-api-key-here
set QVIEW_AI_BASE_URL=https://api.openai.com/v1
set QVIEW_AI_MODEL=gpt-4-vision-preview
qview.exe
```

### Windows PowerShell
```powershell
$env:QVIEW_AI_API_KEY="your-api-key-here"
$env:QVIEW_AI_BASE_URL="https://api.openai.com/v1"
$env:QVIEW_AI_MODEL="gpt-4-vision-preview"
./qview.exe
```

## Precedence

Environment variables take precedence over settings stored in the application. If an environment variable is set, it will be used instead of the corresponding setting from the settings dialog.

## Security Notes

- Environment variables are generally more secure than storing API keys in application settings
- Be careful not to expose environment variables in logs or process lists
- Consider using a `.env` file in combination with tools like `dotenv` for local development