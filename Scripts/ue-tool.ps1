#Requires -Version 5.1
<#
ue-tool.ps1 - Token-efficient wrapper for the UnrealAI plugin HTTP API.

Usage:
  ue-tool.ps1 status                              Plugin health check
  ue-tool.ps1 list                                List all tool names
  ue-tool.ps1 help <tool>                         Show params for a tool
  ue-tool.ps1 call <tool> '<json>'                Call a tool (compact output)
  ue-tool.ps1 call -Raw <tool> '<json>'           Call a tool (raw JSON)
  ue-tool.ps1 call -Save <path> <tool> '<json>'   Call; save embedded image_base64 to <path>
  ue-tool.ps1 save                                Save all dirty assets

Options:
  -Port <int>   Override port (default 3000)

NOTE: This file is ASCII-only on purpose. PowerShell 5.1 reads scripts as
Windows-1252 without a BOM, so non-ASCII characters (em-dash, smart quotes,
etc.) corrupt the parse. Keep it ASCII.
#>

[CmdletBinding()]
param(
    [Parameter(Position = 0)][string]$Command,
    [Parameter(Position = 1, ValueFromRemainingArguments = $true)][string[]]$Rest,
    [int]$Port = 3000,
    [switch]$Raw,
    [string]$Save
)

$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'

$BaseUrl = "http://localhost:$Port"
$ToolsEndpoint = "$BaseUrl/mcp/tools"
$StatusEndpoint = "$BaseUrl/mcp/status"
$ToolEndpoint = "$BaseUrl/mcp/tool"

# ------------------------------------------------------------------
# Connectivity
# ------------------------------------------------------------------

function Test-Connectivity {
    try {
        $null = Invoke-WebRequest -Uri $StatusEndpoint -TimeoutSec 3 -UseBasicParsing
        return $true
    } catch { return $false }
}

function Require-Connectivity {
    if (-not (Test-Connectivity)) {
        Write-Error "Plugin not responding on $StatusEndpoint. Make sure the editor is running with the UnrealAI plugin loaded."
        exit 1
    }
}

# ------------------------------------------------------------------
# Compact formatter (port of json-compact.py)
#
# Rules:
#   - Strip success=true, unwrap single-key {"result": ...}
#   - Scalar:           key: value
#   - Flat dict (<=6):  key: a=1, b=2
#   - Scalar array:     key: a, b, c (truncated at 30)
#   - Object array:     "- " per item, flat-pair body when possible
#   - No quotes unless string contains a special character
# ------------------------------------------------------------------

function Test-Scalar {
    param($V)
    if ($null -eq $V) { return $true }
    if ($V -is [string] -or $V -is [bool]) { return $true }
    return ($V -is [ValueType] -and -not ($V -is [enum]))
}

function Format-Value {
    param($V)
    if ($null -eq $V) { return 'null' }
    if ($V -is [bool]) { if ($V) { return 'true' } else { return 'false' } }
    if ($V -is [ValueType]) { return [string]$V }
    if ($V -is [string]) {
        if ($V -eq '') { return '""' }
        if ($V -match '[\r\n:,{}\[\]]') { return (ConvertTo-Json $V -Compress) }
        return $V
    }
    return [string]$V
}

function Test-Object {
    param($D)
    return ($D -is [pscustomobject])
}

function Test-FlatDict {
    param($D)
    if (-not (Test-Object $D)) { return $false }
    foreach ($prop in $D.PSObject.Properties) {
        if (-not (Test-Scalar $prop.Value)) { return $false }
    }
    return $true
}

function Get-FlatPairs {
    param($D)
    $pairs = foreach ($prop in $D.PSObject.Properties) {
        "$($prop.Name)=$(Format-Value $prop.Value)"
    }
    return ($pairs -join ', ')
}

function Format-Body {
    param($D, [int]$Depth = 0)
    $ind = '  ' * $Depth
    $propsList = @($D.PSObject.Properties)
    if ($propsList.Count -eq 0) { return ($ind + '{}') }
    $lines = New-Object System.Collections.Generic.List[string]
    foreach ($prop in $propsList) {
        $k = $prop.Name
        $v = $prop.Value
        if (Test-Scalar $v) {
            $lines.Add(($ind + $k + ': ' + (Format-Value $v)))
        } elseif ((Test-FlatDict $v) -and @($v.PSObject.Properties).Count -le 6) {
            $lines.Add(($ind + $k + ': ' + (Get-FlatPairs $v)))
        } elseif ($v -is [array] -and $v.Count -eq 0) {
            $lines.Add(($ind + $k + ': []'))
        } elseif ($v -is [array] -and -not ($v | Where-Object { -not (Test-Scalar $_) })) {
            $items = @($v) | Select-Object -First 30 | ForEach-Object { Format-Value $_ }
            $line = $items -join ', '
            if ($v.Count -gt 30) { $line += (' ... +' + ($v.Count - 30) + ' more') }
            $lines.Add(($ind + $k + ': ' + $line))
        } else {
            $lines.Add(($ind + $k + ':'))
            $lines.Add((Format-Anything $v ($Depth + 1)))
        }
    }
    return ($lines -join "`n")
}

function Format-Anything {
    param($Obj, [int]$Depth = 0)
    $ind = '  ' * $Depth
    if (Test-Scalar $Obj) { return ($ind + (Format-Value $Obj)) }
    if ($Obj -is [array]) {
        if ($Obj.Count -eq 0) { return ($ind + '[]') }
        $allScalar = -not ($Obj | Where-Object { -not (Test-Scalar $_) })
        if ($allScalar) {
            $items = @($Obj) | Select-Object -First 30 | ForEach-Object { Format-Value $_ }
            $line = $items -join ', '
            if ($Obj.Count -gt 30) { $line += (' ... +' + ($Obj.Count - 30) + ' more') }
            return ($ind + $line)
        }
        $lines = New-Object System.Collections.Generic.List[string]
        $limit = 30
        $shown = 0
        foreach ($x in $Obj) {
            if ($shown -ge $limit) { break }
            if ((Test-FlatDict $x)) {
                $lines.Add(($ind + '- ' + (Get-FlatPairs $x)))
            } elseif (Test-Object $x) {
                $lines.Add(($ind + '-'))
                $lines.Add((Format-Body $x ($Depth + 1)))
            } else {
                $lines.Add(($ind + '- ' + (Format-Value $x)))
            }
            $shown++
        }
        if ($Obj.Count -gt $limit) {
            $lines.Add(($ind + '... +' + ($Obj.Count - $limit) + ' more'))
        }
        return ($lines -join "`n")
    }
    return Format-Body $Obj $Depth
}

# ------------------------------------------------------------------
# Image extraction (auto-save image_base64 to disk)
# ------------------------------------------------------------------

function Save-EmbeddedImage {
    param($Data, [string]$SavePath)
    if (-not (Test-Object $Data)) { return $Data }
    $defaultPath = Join-Path $env:TEMP 'ue-capture.jpg'
    $b64Prop = $Data.PSObject.Properties['image_base64']
    if ($b64Prop -and $b64Prop.Value) {
        $dest = if ($SavePath) { $SavePath } else { $defaultPath }
        $raw = [Convert]::FromBase64String($b64Prop.Value)
        [IO.File]::WriteAllBytes($dest, $raw)
        $sizeMsg = '[saved to ' + $dest + '] (' + $raw.Length + ' bytes)'
        $Data.image_base64 = $sizeMsg
        return $Data
    }
    foreach ($prop in $Data.PSObject.Properties) {
        if (Test-Object $prop.Value) {
            $null = Save-EmbeddedImage $prop.Value $SavePath
        }
    }
    return $Data
}

# ------------------------------------------------------------------
# Top-level: unwrap + format
# ------------------------------------------------------------------

function Format-Compact {
    param($Data, [string]$SavePath)
    if ($null -eq $Data) { return '' }
    $Data = Save-EmbeddedImage $Data $SavePath
    if (Test-Object $Data) {
        if ($Data.PSObject.Properties['success']) {
            $Data.PSObject.Properties.Remove('success')
        }
        $remaining = @($Data.PSObject.Properties.Name)
        if ($remaining.Count -eq 1 -and $remaining[0] -eq 'result') {
            $Data = $Data.result
        }
    }
    if (Test-Object $Data) {
        return Format-Body $Data 0
    }
    return Format-Anything $Data 0
}

# ------------------------------------------------------------------
# Commands
# ------------------------------------------------------------------

function Invoke-StatusCmd {
    if (Test-Connectivity) {
        try {
            $s = Invoke-RestMethod -Uri $StatusEndpoint -TimeoutSec 3
            '[STATUS] Plugin: responding | port=' + $Port + ' | tools=' + $s.toolCount + ' | version=' + $s.version
        } catch {
            '[STATUS] Plugin: responding | port=' + $Port
        }
    } else {
        '[STATUS] Plugin: not responding | port=' + $Port
        exit 1
    }
}

function Invoke-ListCmd {
    Require-Connectivity
    $data = Invoke-RestMethod -Uri $ToolsEndpoint -TimeoutSec 10
    $tools = if ($data -is [array]) { $data } elseif ($data.tools) { $data.tools } else { @() }
    $tools | Sort-Object name | ForEach-Object { $_.name }
}

function Invoke-HelpCmd {
    param([string]$ToolName)
    Require-Connectivity
    $data = Invoke-RestMethod -Uri $ToolsEndpoint -TimeoutSec 10
    $tools = if ($data -is [array]) { $data } elseif ($data.tools) { $data.tools } else { @() }
    $match = $tools | Where-Object { $_.name -eq $ToolName } | Select-Object -First 1
    if (-not $match) {
        Write-Error ("Tool '" + $ToolName + "' not found. Run 'ue-tool.ps1 list' to see available tools.")
        exit 1
    }
    $desc = if ($match.description) { $match.description } else { 'No description' }
    $descLines = $desc -split "`r?`n"
    ($ToolName + ' - ' + $descLines[0])
    if ($descLines.Count -gt 1) {
        $descLines[1..($descLines.Count - 1)] -join "`n"
    }
    ''
    $params = @()
    if ($match.parameters -and @($match.parameters).Count -gt 0) {
        $params = @($match.parameters)
    } elseif ($match.inputSchema -and $match.inputSchema.properties) {
        $required = @()
        if ($match.inputSchema.required) { $required = @($match.inputSchema.required) }
        foreach ($prop in $match.inputSchema.properties.PSObject.Properties) {
            $p = $prop.Value | Select-Object *
            $p | Add-Member -NotePropertyName name -NotePropertyValue $prop.Name -Force
            $p | Add-Member -NotePropertyName required -NotePropertyValue ($required -contains $prop.Name) -Force
            $params += $p
        }
    }
    if ($params.Count -eq 0) {
        'No parameters.'
        return
    }
    $req = @($params | Where-Object { $_.required } | Sort-Object name)
    $opt = @($params | Where-Object { -not $_.required } | Sort-Object name)
    if ($req.Count -gt 0) {
        'Required:'
        foreach ($p in $req) {
            $pdesc = $p.description
            if ($p.enum) { $pdesc += ' [' + (@($p.enum) -join ', ') + ']' }
            '  ' + $p.name + ' (' + $p.type + ') - ' + $pdesc
        }
        ''
    }
    if ($opt.Count -gt 0) {
        'Optional:'
        foreach ($p in $opt) {
            $pdesc = $p.description
            if ($p.enum) { $pdesc += ' [' + (@($p.enum) -join ', ') + ']' }
            if ($null -ne $p.default -and "$($p.default)" -ne '') { $pdesc += ' [default: ' + $p.default + ']' }
            '  ' + $p.name + ' (' + $p.type + ') - ' + $pdesc
        }
    }
}

function Invoke-CallCmd {
    param([string]$ToolName, [string]$JsonBody, [bool]$RawMode, [string]$SavePath)
    Require-Connectivity
    if (-not $JsonBody) { $JsonBody = '{}' }
    try {
        $response = Invoke-RestMethod -Uri ($ToolEndpoint + '/' + $ToolName) -Method Post -Body $JsonBody -ContentType 'application/json' -TimeoutSec 30
    } catch {
        $msg = $_.Exception.Message
        if ($_.ErrorDetails -and $_.ErrorDetails.Message) { $msg = $_.ErrorDetails.Message }
        Write-Error ("Call to '" + $ToolName + "' failed: " + $msg)
        exit 1
    }
    if ($RawMode) {
        $response | ConvertTo-Json -Depth 20
    } else {
        Format-Compact $response $SavePath
    }
    if ($response.PSObject.Properties['success'] -and $response.success -eq $false) {
        exit 1
    }
}

function Invoke-SaveCmd {
    Require-Connectivity
    '[SAVE] Saving all assets...'
    try {
        $resp = Invoke-RestMethod -Uri ($ToolEndpoint + '/asset') -Method Post -Body '{"operation":"save_all"}' -ContentType 'application/json' -TimeoutSec 60
    } catch {
        Write-Error ('Save failed: ' + $_.Exception.Message)
        exit 1
    }
    if ($resp.PSObject.Properties['success'] -and $resp.success -eq $false) {
        Write-Error ('Save failed: ' + ($resp | ConvertTo-Json -Compress -Depth 5))
        exit 1
    }
    '[SAVE] Done'
}

# ------------------------------------------------------------------
# Dispatch
# ------------------------------------------------------------------

switch ($Command) {
    'status' { Invoke-StatusCmd }
    'list'   { Invoke-ListCmd }
    'help' {
        if (-not $Rest -or $Rest.Count -eq 0) { Write-Error 'Usage: ue-tool.ps1 help <tool>'; exit 1 }
        Invoke-HelpCmd $Rest[0]
    }
    'call' {
        if (-not $Rest -or $Rest.Count -eq 0) { Write-Error "Usage: ue-tool.ps1 call [-Raw] [-Save <path>] <tool> '<json>'"; exit 1 }
        $tool = $Rest[0]
        $body = if ($Rest.Count -ge 2) { $Rest[1] } else { '{}' }
        Invoke-CallCmd -ToolName $tool -JsonBody $body -RawMode:$Raw.IsPresent -SavePath $Save
    }
    'save' { Invoke-SaveCmd }
    default {
        @'
ue-tool.ps1 - UnrealAI plugin wrapper (PowerShell)

  ue-tool.ps1 status                            Plugin health check
  ue-tool.ps1 list                              List all tool names
  ue-tool.ps1 help <tool>                       Show params for a tool
  ue-tool.ps1 call <tool> '<json>'              Call (compact output)
  ue-tool.ps1 call -Raw <tool> '<json>'         Call (raw JSON)
  ue-tool.ps1 call -Save <path> <tool> '<j>'    Call + save image_base64 to <path>
  ue-tool.ps1 save                              Save all dirty assets

Options:
  -Port <int>   Override port (default 3000)
'@
        if ($Command) { Write-Error ('Unknown command: ' + $Command); exit 1 }
    }
}
