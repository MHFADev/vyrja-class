# Vyrja RMM Server — run it on your own PC

The RMM control plane is **PocketBase**: one small open-source program
(MIT licence). You run it on the **teacher/admin PC**, in the same
network as the student computers. No VPS, no cloud account, no
subscription — and it works without internet.

> **Bahasa Indonesia (ringkas):** Unduh PocketBase, taruh di folder
> ini, klik dua kali `start-rmm-server.bat`, buat akun admin di
> `http://localhost:8090/_/`, impor `pb_schema.json`, lalu arahkan
> agen di PC siswa ke `http://<IP-PC-guru>:8090`. Selesai — semua
> berjalan lokal di jaringan sekolah.

## What you need

- The admin/teacher PC (Windows or Linux) — the same PC that runs
  Vyrja Master is perfect.
- The PocketBase program for your system (free, ~15 MB):
  download it from <https://pocketbase.io> (Downloads section) and
  unzip `pocketbase.exe` (Windows) or `pocketbase` (Linux/macOS)
  into this folder.

## Setup (about 5 minutes)

1. **Start the server**
   - Windows: double-click `start-rmm-server.bat`
   - Linux/macOS: run `./start-rmm-server.sh`
   - The first time, Windows Firewall will ask — click **Allow**
     (private network), otherwise student PCs cannot reach it.
2. **Create your admin account**
   Open <http://localhost:8090/_/> in a browser and follow the
   first-run setup. Use a strong password — this account controls
   the fleet.
3. **Import the database structure**
   In the admin panel: **Settings → Import collections →** select
   `pb_schema.json` from this folder **→ Confirm**.
   This creates four collections: `devices`, `telemetry`,
   `commands`, `audit`. Their API rules stay locked (superuser only)
   until Phase 3C wires up per-device agent access.
4. **Find this PC's network address**
   Windows: open Command Prompt, type `ipconfig`, note the IPv4
   address (something like `192.168.1.10`).
   Tip: give this PC a fixed address (router DHCP reservation) so
   the address does not change between lessons.
5. **Point the agents at this PC** (on each managed computer, RMM
   edition only):
   ```
   vyrja-cli config set RMM/CloudEndpoint "http://192.168.1.10:8090"
   vyrja-cli config set RMM/Enabled true
   ```
   (replace the address with your PC's address from step 4.)

That is all. The server keeps its data in a `pb_data` folder next to
the program — **backing up = copying that folder**.

## Good to know

- The PC must be on and the server running for telemetry/commands to
  flow. Classroom control itself (Vyrja Master) never depends on it.
- Moving to the internet later is easy: copy this whole folder
  (program + `pb_data`) to any VPS and start it the same way. If you
  expose it to the internet, put it behind HTTPS (e.g. a Caddy
  reverse proxy) — on a trusted school LAN the built-in setup plus
  signed commands (Phase 3B) is the designed baseline.
- Status: the agent-side sync (telemetry upload, command pickup) is
  implemented in Phase 3B/3C of `../RMM.md`. This kit is the server
  half, ready to run today.
