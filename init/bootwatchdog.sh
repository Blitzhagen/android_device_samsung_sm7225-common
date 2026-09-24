#!/vendor/bin/sh
# DEBUG-BRINGUP: rebootet in die Recovery wenn der Boot nicht fertig wird,
# damit das Geraet per adb debuggbar bleibt - vor produktivem Einsatz entfernen!
/vendor/bin/sleep 900
if [ "$(/vendor/bin/getprop sys.boot_completed)" != "1" ]; then
    /vendor/bin/setprop sys.powerctl reboot,recovery
fi
