#!/usr/bin/env bash
#
# Fetch the latest CI-built firmware and flash it to a Charybdis half.
#
#   util/flash.sh            flash one half
#   util/flash.sh --both     flash both halves, prompting between them
#   util/flash.sh --fetch    download + verify only, don't wait for a board
#   util/flash.sh --cached   skip the download, use what's already cached
#
# The board is an RP2040: in bootloader mode it appears as a USB mass-storage
# device labelled RPI-RP2, and flashing is a file copy. Writing the .uf2 makes
# the drive disconnect mid-write and the board reboot -- that is success, not
# an error, and both are reported as such below.
set -euo pipefail

REPO=${FLASH_REPO:-cdub615/qmk_userspace}
ASSET=${FLASH_ASSET:-bastardkb_charybdis_3x5_splinktegrated_rev1_cdub.uf2}
CACHE_DIR=${XDG_DATA_HOME:-$HOME/.local/share}/qmk-builds
FIRMWARE=$CACHE_DIR/charybdis_cdub_latest.uf2
DRIVE_LABEL=RPI-RP2
KEYBOARD_USB_ID=a8f8:1832
WAIT_TIMEOUT=${FLASH_WAIT_TIMEOUT:-300}

TMP_WORKDIR=""
cleanup() { [[ -n $TMP_WORKDIR && -d $TMP_WORKDIR ]] && rm -rf "$TMP_WORKDIR"; }
trap cleanup EXIT

RED=$'\e[31m'; GRN=$'\e[32m'; YEL=$'\e[33m'; DIM=$'\e[2m'; OFF=$'\e[0m'
# UI goes to stderr so that functions can return data on stdout
# (wait_for_drive echoes the mount path, and its prompts must still be seen).
info() { printf '%s\n' "$*" >&2; }
ok()   { printf '%s✓%s %s\n' "$GRN" "$OFF" "$*" >&2; }
warn() { printf '%s!%s %s\n' "$YEL" "$OFF" "$*" >&2; }
die()  { printf '%s✗%s %s\n' "$RED" "$OFF" "$*" >&2; exit 1; }

# --- verify it's a real UF2 for an RP2040 before we hand it to a bootloader ---
verify_uf2() {
    python3 - "$1" <<'PY'
import struct, sys
data = open(sys.argv[1], 'rb').read()
MAGIC0, MAGIC1, MAGICEND = 0x0A324655, 0x9E5D5157, 0x0AB16F30
RP2040 = 0xE48BFF56
if not data or len(data) % 512:
    sys.exit(f"not a whole number of 512-byte UF2 blocks ({len(data)})")
n = len(data) // 512
fams, addrs = set(), []
for i in range(n):
    b = data[i*512:(i+1)*512]
    m0, m1, flags, addr, psize, bno, total, fam = struct.unpack('<8I', b[:32])
    if (m0, m1) != (MAGIC0, MAGIC1) or struct.unpack('<I', b[508:512])[0] != MAGICEND:
        sys.exit(f"block {i}: bad UF2 magic")
    if bno != i or total != n:
        sys.exit(f"block {i}: inconsistent block numbering")
    if flags & 0x2000:
        fams.add(fam)
    addrs.append(addr)
if fams and fams != {RP2040}:
    sys.exit("wrong chip family: " + ", ".join(hex(f) for f in fams) + " (expected RP2040)")
if min(addrs) != 0x10000000:
    sys.exit(f"unexpected flash base 0x{min(addrs):08X}")
print(f"{n} blocks, RP2040, 0x{min(addrs):08X}-0x{max(addrs)+256:08X}")
PY
}

fetch() {
    mkdir -p "$CACHE_DIR"
    if ! command -v gh >/dev/null 2>&1; then
        warn "gh not installed; using cached firmware"; return 1
    fi
    if ! gh auth status >/dev/null 2>&1; then
        warn "gh not authenticated; using cached firmware"; return 1
    fi
    TMP_WORKDIR=$(mktemp -d)
    info "${DIM}fetching latest release from $REPO...${OFF}"
    if ! gh release download latest --repo "$REPO" --pattern "$ASSET" --dir "$TMP_WORKDIR" >/dev/null 2>&1; then
        warn "download failed; using cached firmware"; return 1
    fi
    local desc
    desc=$(verify_uf2 "$TMP_WORKDIR/$ASSET") || die "downloaded firmware failed verification -- cache left untouched"
    # only replace the cache once the new file has verified
    mv -f "$TMP_WORKDIR/$ASSET" "$FIRMWARE"
    ok "fetched and verified: $desc"
    return 0
}

# After a successful write the board reboots and its RPI-RP2 drive disappears.
# Wait for that before looking for the next one, so we can't re-detect the half
# we just finished.
wait_for_drive_gone() {
    local deadline=$((SECONDS + 30))
    while (( SECONDS < deadline )); do
        findmnt -no TARGET -S LABEL=$DRIVE_LABEL >/dev/null 2>&1 || return 0
        sleep 1
    done
    warn "the previous drive is still mounted; continuing anyway"
    return 0
}

wait_for_drive() {
    local half=$1 deadline=$((SECONDS + WAIT_TIMEOUT)) mnt=""
    info ""
    info "${YEL}Put the ${half:+$half }half into bootloader mode now:${OFF}"
    info "  unplug USB from it, hold the top-outer key, plug USB back in, release"
    info "  ${DIM}(left half: top-LEFT key. right half: top-RIGHT key, or QK_BOOT on Media)${OFF}"
    info "${DIM}waiting up to ${WAIT_TIMEOUT}s for the $DRIVE_LABEL drive...${OFF}"
    while (( SECONDS < deadline )); do
        mnt=$(findmnt -no TARGET -S LABEL=$DRIVE_LABEL 2>/dev/null | head -1 || true)
        if [[ -n $mnt && -d $mnt && -w $mnt ]]; then
            printf '%s\n' "$mnt"; return 0
        fi
        sleep 1
    done
    return 1
}

flash_one() {
    local half=$1 mnt
    mnt=$(wait_for_drive "$half") || die "timed out waiting for the $DRIVE_LABEL drive"
    ok "found bootloader drive at $mnt"

    local free_kb need_kb
    free_kb=$(df -Pk "$mnt" | awk 'NR==2{print $4}')
    need_kb=$(( ($(stat -c%s "$FIRMWARE") + 1023) / 1024 ))
    (( free_kb >= need_kb )) || die "not enough space on the drive (${free_kb}K free, need ${need_kb}K)"

    info "${DIM}writing $(basename "$FIRMWARE") ...${OFF}"
    # The board disconnects mid-write by design, so cp/sync "failing" here is
    # expected. Success is judged by the board coming back, below.
    cp "$FIRMWARE" "$mnt"/ 2>/dev/null || true
    sync "$mnt" 2>/dev/null || true

    info "${DIM}waiting for the board to reboot...${OFF}"
    local deadline=$((SECONDS + 30))
    while (( SECONDS < deadline )); do
        if ! findmnt -no TARGET -S LABEL=$DRIVE_LABEL >/dev/null 2>&1; then
            sleep 2
            if lsusb 2>/dev/null | grep -qi "$KEYBOARD_USB_ID"; then
                ok "${half:+$half }half flashed -- board re-enumerated as the keyboard"
                return 0
            fi
            warn "${half:+$half }half: drive went away but the keyboard hasn't appeared yet"
            warn "give it a moment; if it stays dark, re-enter bootloader and retry"
            return 0
        fi
        sleep 1
    done
    die "the drive never disconnected -- the write may not have taken. Do NOT unplug; retry."
}

main() {
    local mode=one do_fetch=1
    while (( $# )); do
        case "$1" in
            --both)   mode=both ;;
            --fetch)  mode=fetch ;;
            --cached) do_fetch=0 ;;
            -h|--help)
                sed -n '3,12p' "$0" | sed 's/^# \?//'
                return 0 ;;
            *)        die "unknown option: $1" ;;
        esac
        shift
    done

    if (( do_fetch )); then
        fetch || true
    fi
    [[ -f $FIRMWARE ]] || die "no firmware at $FIRMWARE and nothing could be fetched"
    verify_uf2 "$FIRMWARE" >/dev/null || die "cached firmware at $FIRMWARE is not a valid RP2040 UF2"

    info "firmware: $FIRMWARE"
    info "          $(stat -c%s "$FIRMWARE") bytes, sha256 $(sha256sum "$FIRMWARE" | cut -c1-16)..."
    [[ $mode == fetch ]] && { ok "fetch-only: nothing flashed"; return 0; }

    if [[ $mode == both ]]; then
        flash_one "FIRST"
        info ""
        info "${YEL}Now the other half.${OFF} Move the USB cable over."
        info "${DIM}(leave the TRRS cable connected; nothing to press here)${OFF}"
        wait_for_drive_gone
        flash_one "SECOND"
        info ""
        ok "both halves done -- leave USB on the right half"
    else
        flash_one ""
        info ""
        info "${DIM}run again for the other half (or use --both next time)${OFF}"
    fi
}

main "$@"
