#!/bin/sh
#
# TiniLinux A/B Slot Manager
# Manages A/B boot slot metadata and switching logic
#

set -e

# Metadata file location (on overlay partition)
AB_METADATA="/mnt/overlayfs/ab-metadata"
AB_METADATA_BACKUP="/mnt/overlayfs/ab-metadata.bak"

# Default values
DEFAULT_SLOT="a"
MAX_BOOT_ATTEMPTS=3

# Initialize metadata file with defaults
init_metadata() {
    echo "Initializing A/B metadata..."
    cat > "$AB_METADATA" <<EOF
# TiniLinux A/B Boot Slot Metadata
ACTIVE_SLOT=a
BOOT_COUNT_A=0
BOOT_COUNT_B=0
BOOT_SUCCESS_A=1
BOOT_SUCCESS_B=0
FALLBACK_ENABLED=1
PENDING_SLOT=
EOF
    cp "$AB_METADATA" "$AB_METADATA_BACKUP"
}

# Load metadata into shell variables
load_metadata() {
    if [ ! -f "$AB_METADATA" ]; then
        init_metadata
    fi
    . "$AB_METADATA"
}

# Save metadata to file
save_metadata() {
    cat > "$AB_METADATA" <<EOF
# TiniLinux A/B Boot Slot Metadata
ACTIVE_SLOT=${ACTIVE_SLOT}
BOOT_COUNT_A=${BOOT_COUNT_A}
BOOT_COUNT_B=${BOOT_COUNT_B}
BOOT_SUCCESS_A=${BOOT_SUCCESS_A}
BOOT_SUCCESS_B=${BOOT_SUCCESS_B}
FALLBACK_ENABLED=${FALLBACK_ENABLED}
PENDING_SLOT=${PENDING_SLOT}
EOF
    cp "$AB_METADATA" "$AB_METADATA_BACKUP"
    sync
}

# Get the current active slot
get_active_slot() {
    load_metadata
    echo "$ACTIVE_SLOT"
}

# Get the inactive slot
get_inactive_slot() {
    load_metadata
    if [ "$ACTIVE_SLOT" = "a" ]; then
        echo "b"
    else
        echo "a"
    fi
}

# Check if slot is marked as successful
is_slot_successful() {
    local slot=$1
    load_metadata
    if [ "$slot" = "a" ]; then
        [ "$BOOT_SUCCESS_A" = "1" ]
    else
        [ "$BOOT_SUCCESS_B" = "1" ]
    fi
}

# Mark current boot as successful
mark_boot_successful() {
    load_metadata
    echo "Marking slot $ACTIVE_SLOT as successful..."
    if [ "$ACTIVE_SLOT" = "a" ]; then
        BOOT_SUCCESS_A=1
        BOOT_COUNT_A=0
    else
        BOOT_SUCCESS_B=1
        BOOT_COUNT_B=0
    fi
    PENDING_SLOT=""
    save_metadata
    echo "Boot marked successful for slot $ACTIVE_SLOT"
}

# Increment boot count for current slot
increment_boot_count() {
    local slot=$1
    load_metadata
    if [ "$slot" = "a" ]; then
        BOOT_COUNT_A=$((BOOT_COUNT_A + 1))
    else
        BOOT_COUNT_B=$((BOOT_COUNT_B + 1))
    fi
    save_metadata
}

# Get boot count for a slot
get_boot_count() {
    local slot=$1
    load_metadata
    if [ "$slot" = "a" ]; then
        echo "$BOOT_COUNT_A"
    else
        echo "$BOOT_COUNT_B"
    fi
}

# Check if we should fallback to other slot
should_fallback() {
    load_metadata
    [ "$FALLBACK_ENABLED" = "1" ] || return 1
    
    local count
    if [ "$ACTIVE_SLOT" = "a" ]; then
        count=$BOOT_COUNT_A
    else
        count=$BOOT_COUNT_B
    fi
    
    [ "$count" -ge "$MAX_BOOT_ATTEMPTS" ]
}

# Perform fallback to other slot
perform_fallback() {
    load_metadata
    echo "Boot failed $MAX_BOOT_ATTEMPTS times, falling back..."
    
    local old_slot=$ACTIVE_SLOT
    if [ "$ACTIVE_SLOT" = "a" ]; then
        ACTIVE_SLOT=b
        BOOT_SUCCESS_A=0
    else
        ACTIVE_SLOT=a
        BOOT_SUCCESS_B=0
    fi
    
    echo "Switched from slot $old_slot to slot $ACTIVE_SLOT"
    save_metadata
}

# Set a slot as pending (for next boot)
set_pending_slot() {
    local slot=$1
    load_metadata
    PENDING_SLOT=$slot
    save_metadata
    echo "Pending slot set to: $slot"
}

# Switch to pending slot if one is set
switch_to_pending() {
    load_metadata
    if [ -n "$PENDING_SLOT" ]; then
        local old_slot=$ACTIVE_SLOT
        ACTIVE_SLOT=$PENDING_SLOT
        PENDING_SLOT=""
        
        # Reset boot counters and success flags for new slot
        if [ "$ACTIVE_SLOT" = "a" ]; then
            BOOT_COUNT_A=0
            BOOT_SUCCESS_A=0
        else
            BOOT_COUNT_B=0
            BOOT_SUCCESS_B=0
        fi
        
        save_metadata
        echo "Switched from slot $old_slot to slot $ACTIVE_SLOT"
        return 0
    fi
    return 1
}

# Get slot info
get_slot_info() {
    local slot=$1
    load_metadata
    
    local boot_count success
    if [ "$slot" = "a" ]; then
        boot_count=$BOOT_COUNT_A
        success=$BOOT_SUCCESS_A
    else
        boot_count=$BOOT_COUNT_B
        success=$BOOT_SUCCESS_B
    fi
    
    echo "Slot: $slot"
    echo "Boot count: $boot_count"
    echo "Boot success: $success"
    [ "$ACTIVE_SLOT" = "$slot" ] && echo "Status: ACTIVE" || echo "Status: INACTIVE"
}

# Show all metadata
show_status() {
    load_metadata
    echo "=== A/B Boot Slot Status ==="
    echo ""
    echo "Active slot: $ACTIVE_SLOT"
    echo "Pending slot: ${PENDING_SLOT:-none}"
    echo "Fallback enabled: $FALLBACK_ENABLED"
    echo ""
    echo "--- Slot A ---"
    echo "Boot count: $BOOT_COUNT_A"
    echo "Boot successful: $BOOT_SUCCESS_A"
    echo ""
    echo "--- Slot B ---"
    echo "Boot count: $BOOT_COUNT_B"
    echo "Boot successful: $BOOT_SUCCESS_B"
}

# Main command dispatcher
case "${1:-}" in
    init)
        init_metadata
        ;;
    get-active)
        get_active_slot
        ;;
    get-inactive)
        get_inactive_slot
        ;;
    mark-successful)
        mark_boot_successful
        ;;
    increment-boot-count)
        increment_boot_count "${2:-$ACTIVE_SLOT}"
        ;;
    get-boot-count)
        get_boot_count "${2:-$ACTIVE_SLOT}"
        ;;
    should-fallback)
        should_fallback
        ;;
    perform-fallback)
        perform_fallback
        ;;
    set-pending)
        set_pending_slot "$2"
        ;;
    switch-to-pending)
        switch_to_pending
        ;;
    get-slot-info)
        get_slot_info "${2:-$ACTIVE_SLOT}"
        ;;
    status)
        show_status
        ;;
    *)
        echo "Usage: $0 {init|get-active|get-inactive|mark-successful|increment-boot-count [slot]|get-boot-count [slot]|should-fallback|perform-fallback|set-pending <slot>|switch-to-pending|get-slot-info [slot]|status}"
        exit 1
        ;;
esac
