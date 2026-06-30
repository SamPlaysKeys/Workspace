# Blog Outline: Resetting the BIOS/UEFI on Lenovo ThinkMinis

## Introduction
- What is the BIOS/UEFI and why does it matter?
- Common scenarios for resetting:
    - Forgotten administrator passwords.
    - Erroneous configuration causing boot failures.
    - Troubleshooting hardware-related boot issues.
    - Preparing a device for resale or repurposing.

## Prerequisites
- Physical access to the ThinkMini chassis.
- A technician's toolkit (precision screwdriver, etc.).
- External keyboard/mouse (if OS/BIOS is inaccessible).
- Power adapter/unplugging the machine.

## Method 1: Software-Based Reset (Within UEFI Menu)
*Best for: Misconfigurations where you still have BIOS access.*
- **How to enter UEFI/BIOS:** (e.g., pressing `F1` during startup).
- **Navigating the Menu:**
    - Locating the "Configuration" or "Exit" tab.
    - Finding the "Load Setup Defaults" or "Restore Factory Defaults" option.
- **Saving Changes:** The importance of "Save and Exit" (F10).

## Method 2: Hardware Reset (Clearing CMOS)
*Best for: Forgotten passwords or when the system won't boot into BIOS.*
- **Preparation:** Disconnecting power and grounding yourself.
- **The CMOS Battery Method:**
    - Identifying the coin-cell battery on the ThinkMini motherboard.
    - Removing the battery and waiting (and why: capacitor discharge).
    - Reinstalling the battery.
- **The Jumper Method (if applicable):**
    - Locating the CLR_CMOS jumper on the motherboard.
    - The process of shorting/moving the jumper.

## Method 3: Dealing with BIOS Supervisor Passwords
- Note: Clearing CMOS often *does not* remove a Supervisor Password on modern ThinkPads/ThinkMinis due to security features.
- Mention Lenovo's official support/recovery paths for enterprise-level security.

## Troubleshooting Common Reset Issues
- **Issue:** System still prompts for password after CMOS reset.
- **Issue:** Boot order is lost after reset (How to fix: re-configure boot priority).
- **Issue:** System fails to POST after hardware reset (Troubleshooting power/RAM).

## Conclusion
- Summary of which method to use when.
- Reminder to document important settings before performing a reset.
