adb shell am force-stop com.beatgames.beatsaber
adb shell am start com.beatgames.beatsaber/com.unity3d.player.UnityPlayerActivity

& adb logcat -c
& $PSScriptRoot/start-logging.ps1
