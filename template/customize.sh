#!/system/bin/sh

SKIPUNZIP=0

ui_print() { echo "$1"; }

ui_print "======================================"
ui_print "           MODULE INFORMATION          "
ui_print "======================================"
ui_print " Name       : PingPimp"
sleep 0.1
ui_print " Version    : $(grep_prop version "${TMPDIR}/module.prop" 2>/dev/null || echo "v1.0")"
sleep 0.1
ui_print " Author     : $(grep_prop author "${TMPDIR}/module.prop" 2>/dev/null || echo "Unknown")"
sleep 0.2
ui_print ""
ui_print "======================================"
ui_print "           DEVICE INFORMATION          "
ui_print "======================================"
ui_print " Model      : $(getprop ro.product.model 2>/dev/null || echo "Unknown")"
sleep 0.1
ui_print " Brand      : $(getprop ro.product.manufacturer 2>/dev/null || echo "Unknown")"
sleep 0.1
ui_print " Board      : $(getprop ro.product.board 2>/dev/null || echo "Unknown")"
sleep 0.1
ui_print " Android    : $(getprop ro.build.version.release 2>/dev/null || echo "Unknown")"
sleep 0.1
ui_print " Kernel     : $(uname -r 2>/dev/null || echo "Unknown")"
sleep 0.1
ui_print " CPU        : $(getprop ro.hardware 2>/dev/null || echo "Unknown")"
sleep 0.1
ui_print " RAM        : $(free 2>/dev/null | awk '/Mem:/ {print $2}' || echo "Unknown") kB"
sleep 0.2
ui_print ""
ui_print "======================================"
ui_print "           ROOT ENVIRONMENT            "
ui_print "======================================"
if [ "$APATCH" ]; then
    ROOT_METHOD="APatch"
    ROOT_VERSION="$APATCH_VER ($APATCH_VER_CODE)"
elif [ "$KSU" ]; then
    if [ "$KSU_NEXT" ]; then
        ROOT_METHOD="KernelSU Next"
        ROOT_VERSION="$KSU_KERNEL_VER_CODE ($KSU_VER_CODE)"
    else
        ROOT_METHOD="KernelSU"
        ROOT_VERSION="$KSU_KERNEL_VER_CODE ($KSU_VER_CODE)"
    fi
elif [ "$MAGISK_VER_CODE" ]; then
    ROOT_METHOD="Magisk"
    ROOT_VERSION="$MAGISK_VER ($MAGISK_VER_CODE)"
else
    ROOT_METHOD="Unknown"
    ROOT_VERSION="N/A"
fi
ui_print " Method     : ${ROOT_METHOD}"
ui_print " Version    : ${ROOT_VERSION}"
sleep 0.3
ui_print ""
ui_print "======================================"
ui_print "           INSTALLING BINARY           "
ui_print "======================================"

# Detect architecture and copy appropriate binary
ARCH=$(getprop ro.product.cpu.abi)
case "$ARCH" in
    arm64-v8a|arm64)
        BIN_SRC="PingPimp_64"
        ui_print " Architecture: ARM64 (64-bit)"
        ;;
    armeabi-v7a|armeabi)
        BIN_SRC="PingPimp_32"
        ui_print " Architecture: ARMv7 (32-bit)"
        ;;
    x86_64)
        BIN_SRC="PingPimp_x64"
        ui_print " Architecture: x86_64 (64-bit)"
        ;;
    x86|i686|i586|i486|i386)
        BIN_SRC="PingPimp_x86"
        ui_print " Architecture: x86 (32-bit)"
        ;;
    *)
        # Fallback: try to find any binary
        if [ -f "$MODPATH/system/bin/PingPimp_64" ]; then
            BIN_SRC="PingPimp_64"
            ui_print " Architecture: ARM64 (fallback)"
        elif [ -f "$MODPATH/system/bin/PingPimp_32" ]; then
            BIN_SRC="PingPimp_32"
            ui_print " Architecture: ARMv7 (fallback)"
        else
            ui_print " ! No compatible binary found!"
            abort
        fi
        ;;
esac

# Copy binary
cp "$MODPATH/system/bin/$BIN_SRC" "$MODPATH/system/bin/PingPimp"
chmod 755 "$MODPATH/system/bin/PingPimp"

# Remove unused binaries to save space
rm -f "$MODPATH/system/bin/PingPimp_32" \
      "$MODPATH/system/bin/PingPimp_64" \
      "$MODPATH/system/bin/PingPimp_x86" \
      "$MODPATH/system/bin/PingPimp_x64"

ui_print " Binary installed: $(ls -lh "$MODPATH/system/bin/PingPimp" | awk '{print $5}')"
sleep 0.3
ui_print ""
ui_print "======================================"
ui_print "           SETTING PERMISSIONS         "
ui_print "======================================"

# Set permissions
set_perm_recursive "$MODPATH" 0 0 0755 0644
set_perm "$MODPATH/system/bin/PingPimp" 0 0 0755
set_perm "$MODPATH/service.sh" 0 0 0755
set_perm "$MODPATH/action.sh" 0 0 0755
set_perm "$MODPATH/uninstall.sh" 0 0 0755
set_perm_recursive "$MODPATH/webroot" 0 0 0755 0644

ui_print " Permissions set successfully"
sleep 0.3
ui_print ""
ui_print "======================================"
ui_print "             FINALIZATION              "
ui_print "======================================"

# Create config directory
mkdir -p /data/adb/modules/PingPimp 2>/dev/null

# Create default config files if not exist
[ -f /data/adb/modules/PingPimp/preset.txt ] || echo "default" > /data/adb/modules/PingPimp/preset.txt
[ -f /data/adb/modules/PingPimp/state.txt ] || echo "0" > /data/adb/modules/PingPimp/state.txt
[ -f /data/adb/modules/PingPimp/saver.txt ] || echo "0" > /data/adb/modules/PingPimp/saver.txt
[ -f /data/adb/modules/PingPimp/ipv6_state.txt ] || echo "0" > /data/adb/modules/PingPimp/ipv6_state.txt
[ -f /data/adb/modules/PingPimp/tcp.txt ] || echo "cubic" > /data/adb/modules/PingPimp/tcp.txt
touch /data/adb/modules/PingPimp/boost_apps.txt 2>/dev/null
touch /data/adb/modules/PingPimp/isolate_apps.txt 2>/dev/null
chmod 755 /data/adb/modules/PingPimp
chmod 644 /data/adb/modules/PingPimp/*.txt 2>/dev/null

ui_print " Config directory: /data/adb/modules/PingPimp/"
ui_print " WebUI available in KernelSU/Magisk Manager"

sleep 1
ui_print ""
ui_print " Installation completed successfully!"
ui_print " Completed at : $(date '+%d %b %Y - %H:%M %Z')"
ui_print "======================================"