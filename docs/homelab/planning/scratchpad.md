
---
type: Idea
---

## Machine List

JONSBO NAS - 6th Gen i7, 16 GB RAM, NVIDIA GPU, 10TB of storage (8 usable)

Synology NAS - 4GB RAM, 6TB storage

Intel NUC - Celeron CPU, 8GB of RAM

Raspberry Pi 4 - 4GB RAM

LenovoMini 1 - 8th gen i7, NVIDIA GPU, 16GB RAM
LenovoMini 2 - 7th gen i5, NVIDIA GPU, 16GB RAM
LenovoMini 3 - i5, 16GB RAM
LenovoMini 4 - i5, 16GB RAM
LenovoMini 5 - i5, 16GB RAM

ProxMox Host - Ryzen 3700, 16GB RAM, NVIDIA GPU
Gaming PC - Ryzen 5700, 16GB RAM, NVIDIA GPU

## Machine Assignments

LenovoMini 1 - Prod, baremetal docker host, Fedora, tailscale installed, komodo periphery installed
LenovoMini 2 - Test, baremetal docker host, Fedora, tailscale installed, komodo periphery installed
LenovoMini (3,4,5) - Dev, OCP cluster, tailscale operator installed

JONSBO NAS - Prod, Tailscale container, mapped to LenovoMini 1

Synology NAS - Test, Tailscale installed, mapped to LenovoMini 2

ProxMox Host - Tailscale installed directly, several VMs available
 - "Remoting VM"
   - Running Fedora, used for remote work, has my dotfiles and tools installed. Tailscale installed directly
 - "Debian Test"
   - Test VM, running debian, tools installed, used only for testing packages on Debian. Mostly stays powered off.
 - Docker VM
   - mirror of the test and prod VMs, running Komodo periphery, testing docker containers and workloads
 - Remaining resources will be used for testing out VM ideas. No remote windows machine is needed.

Intel NUC - Technically part of the prod environment, this runs the komodo instance that does the management for everything else. Additionally, it will be the main host that the ansible and terraform commands will be run from. It will have a GitHub runner installed to handle executing the management tasks.

Gaming PC - Not technically part of any environment (it will be part of a separate "user" environment) it will still function as my remote windows machine when needed.

NOTE: At some point, it may become worth it to leverage on of my spare laptops as a stand-in device for running an image for work/testing, but that is a future problem. For now, I will not include a machine to use as a "daily work" machine in my plans.


