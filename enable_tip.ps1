$langList = Get-WinUserLanguageList
$zhTW = $langList | Where-Object LanguageTag -eq "zh-Hant-TW"
if (-not $zhTW) {
    $zhTW = New-WinUserLanguageList "zh-Hant-TW"
    $langList += $zhTW
}
$tip = "0404:{810B8D97-0DAB-4E87-9551-76A3D49D0E76}{A3668853-2ED4-4D4B-A951-DE1C8B4C0A29}"
if ($zhTW.InputMethodTips -notcontains $tip) {
    $zhTW.InputMethodTips.Add($tip)
    Set-WinUserLanguageList $langList -Force
    Write-Host "Added Win-McBopomofo to language list."
} else {
    Write-Host "Win-McBopomofo already in language list."
}
