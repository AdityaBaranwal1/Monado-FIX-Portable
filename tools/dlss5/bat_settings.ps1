# Applies the toggles at the top of the "Play Monado" bats to xcde.ini and ReShade.ini before the launcher starts
# (the launcher then shows them). An empty value leaves that setting as it is.
#   bat_settings.ps1 [-Upscaler dlss] [-DlssMode native|quality|balanced|performance|ultra] [-Dlss5 1|0|none] [-Dlss5Strength 0..1]
param([string]$Upscaler = '', [string]$DlssMode = '', [string]$Dlss5 = '', [string]$Dlss5Strength = '')
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot

# key = value in a file without sections (xcde.ini), or Key=Value inside [Section] (ReShade.ini); keeps the file's line ends
function Set-IniValue([string]$File, [string]$Section, [string]$Key, [string]$Value) {
    $t = if (Test-Path -LiteralPath $File) { [IO.File]::ReadAllText($File) } else { '' }
    $nl = if ($t -match "`r`n") { "`r`n" } else { "`n" }
    $k = [regex]::Escape($Key)
    if (-not $Section) {
        $line = "$Key = $Value"
        if ($t -match "(?m)^$k\s*=.*?(?=\r?$)") { $t = [regex]::Replace($t, "(?m)^$k\s*=.*?(?=\r?$)", $line) }
        else { $t = $t.TrimEnd() + $nl + $line + $nl }
    } else {
        $line = "$Key=$Value"
        $m = [regex]::Match($t, '(?ms)^\[' + [regex]::Escape($Section) + '\][ \t]*\r?$(.*?)(?=^\[|\z)')
        if (-not $m.Success) { $t = $t.TrimEnd() + $nl + $nl + "[$Section]" + $nl + $line + $nl }
        else {
            $body = $m.Groups[1].Value
            if ($body -match "(?m)^$k=.*?(?=\r?$)") { $body = [regex]::Replace($body, "(?m)^$k=.*?(?=\r?$)", $line) }
            else { $lead = [regex]::Match($body, '^\r?\n').Value; $body = $lead + $line + $nl + $body.Substring($lead.Length) }
            $t = $t.Substring(0, $m.Groups[1].Index) + $body + $t.Substring($m.Groups[1].Index + $m.Groups[1].Length)
        }
    }
    [IO.File]::WriteAllText($File, $t)
}

$ini = Join-Path $root 'xcde.ini'
$modes = 'native', 'quality', 'balanced', 'performance', 'ultra'
$upscalers = 'auto', 'bilinear', 'fsr', 'fsr3', 'dlss'
if ($Upscaler -and $upscalers -notcontains $Upscaler) { Write-Warning "UPSCALER '$Upscaler' is not one of: $($upscalers -join ', ') - left as it was"; $Upscaler = '' }
if ($Upscaler) {
    Set-IniValue $ini '' 'upscaler' $Upscaler
    Set-IniValue $ini '' 'resolution' 'match'  # the DLSS quality ratio only applies with resolution = match
}
if ($DlssMode) {
    if ($modes -notcontains $DlssMode) { Write-Warning "DLSS_MODE '$DlssMode' is not one of: $($modes -join ', ') - left as it was" }
    else { Set-IniValue $ini '' 'upscale_quality' $DlssMode }
}
$rs = Join-Path $PSScriptRoot 'ReShade.ini'
if ($Dlss5 -eq '1' -or $Dlss5 -eq '0') { Set-IniValue $rs 'RenoDX.DLSS5' 'NeuralUplift' $Dlss5 }
elseif ($Dlss5 -and $Dlss5 -ne 'none') { Write-Warning "DLSS5 '$Dlss5' is not 1, 0 or none - left as it was" }
if ($Dlss5Strength -and $Dlss5 -ne 'none') {
    $v = 0.0
    if ([double]::TryParse($Dlss5Strength, [Globalization.NumberStyles]::Float, [Globalization.CultureInfo]::InvariantCulture, [ref]$v) -and $v -ge 0 -and $v -le 1) {
        Set-IniValue $rs 'RenoDX.DLSS5' 'NRIntensity' $Dlss5Strength
    } else { Write-Warning "DLSS5_STRENGTH '$Dlss5Strength' is not between 0 and 1 - left as it was" }
}
