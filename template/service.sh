#!/system/bin/sh

MODDIR=${0%/*}

# Wait for system to boot
while [ -z "$(getprop sys.boot_completed)" ]; do
    sleep 5
done

# Wait a bit more for network to stabilize
sleep 10

# Run PingPimp in boot mode
if [ -f "$MODDIR/system/bin/PingPimp" ]; then
    "$MODDIR/system/bin/PingPimp" --boot > /dev/null 2>&1 &
else
    echo "PingPimp binary not found!" > /data/local/tmp/pingpimp_error.log
fi

# Flush route cache
ip route flush cache 2>/dev/null

echo "PingPimp service started at $(date)" > /data/local/tmp/pingpimp_service.log