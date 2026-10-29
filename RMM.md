# Vyrja RMM — Architecture & Phased Implementation Plan

Status: Phase 1 (discovery) and Phase 2 (blueprint) complete.
Phase 3A scaffold landed: runtime feature gates (`RMM/*` config group),
build option `WITH_RMM` (default OFF) and the reserved plugin slot
`plugins/rmmagent/`. **Everything RMM is disabled by default; shipped
binaries behave exactly like stock Vyrja Classroom until an operator
explicitly enables it.**

The RMM capabilities are strictly additive. Nothing in the VNC /
screen-rendering path, the master↔client protocol, or the existing
service supervision is modified by this plan.

---

## Editions (two release tracks)

Vyrja Classroom ships in two editions built from the same source:

- **Standard edition** — tags like `v4.11.4`. Built with the default
  flags (`WITH_RMM=OFF`); the RMM plugin is not part of the installer.
- **RMM edition** — tags like `v4.11.4-rmm`. The `-rmm` tag suffix
  makes the build force `WITH_RMM=ON` (root CMakeLists detects the
  tag via `GITHUB_REF_NAME`), the `rmmagent` plugin is included, and
  the Windows installer file is named
  `vyrja-classroom-rmm-<version>-win64-setup.exe`.

Even in the RMM edition the runtime gates stay default-off
(`RMM/Enabled=false` etc.); an operator enables them explicitly per
machine, e.g. `vyrja-cli config set RMM/Enabled true`. Phase 3B widens
the functional difference between the editions (bridge + telemetry +
signed actions exist only in the RMM edition).

## Phase 1 — Current architecture (verified in source)

```
SCM ──► vyrja-service.exe (SYSTEM, session 0)
          service/src/main.cpp → VeyonCore(Component::Service)
          → PlatformServiceFunctions::runAsService("VeyonService", …)
          Windows: plugins/platform/windows/WindowsServiceCore.{h,cpp}
          │
          ├─ QLocalServer named pipe "VeyonServiceDataManager"
          │    (ServiceDataManager, own QThread, per-boot bearer token
          │     handed to children via VEYON_SERVICE_DATA_TOKEN env var)
          │    plugins/platform/common/ServiceDataManager.{h,cpp}
          │
          └─ per active WTS session: VeyonServerProcess
               launches vyrja-server.exe into the session via
               WindowsCoreFunctions::runProgramInSession()
               (winlogon token of that session)

        vyrja-server.exe (one per session, runs as the logged-on user)
          server/src/ComputerControlServer.cpp
          ├─ VncProxyServer   TCP 11100+sessionId  ◄── Master connects
          ├─ VncServer        TCP 11200+sessionId  (bundled LibVNC)
          └─ FeatureWorkerManager  TCP loopback 11300+sessionId
               spawns vyrja-worker.exe "<feature-uid>" per feature

        vyrja-master.exe (teacher PC), vyrja-configurator.exe,
        vyrja-cli.exe / vyrja-wcli.exe (service control & config CLI)
```

Facts that shape the design:

- The Windows service **already runs as SYSTEM**; its SCM name is the
  literal `VeyonService` (branding-independent). Today it only
  supervises per-session server processes (with a restart watchdog).
  It executes no admin commands and talks to no network endpoint.
- Master↔client authentication already exists
  (`ServerAuthenticationManager`): key-file challenge–response with
  RSA signatures (CryptoCore), logon credentials, or shared token.
  There is **no TLS** in the LAN control path.
- Config is a typed property table
  (`core/src/VeyonConfigurationProperties.h`) stored in the registry
  under the historical `Veyon` root — intentionally unchanged so
  existing data and student-PC installs stay compatible.
- A native feature kill switch already exists:
  `Features/DisabledFeatures` (list of feature UUIDs). Plugins are
  QPluginLoader modules; new plugin = new directory under `plugins/`
  (auto-globbed) using `build_veyon_plugin`, or excluded at build
  time with `-DSKIP_PLUGINS="…;rmmagent"`.
- Process execution helpers already exist in the Windows platform
  plugin (`runProgramAsAdmin`, `runProgramAsUser`,
  `runProgramInSession`) — but there is deliberately **no**
  remotely-triggerable generic SYSTEM executor. This plan keeps it
  that way (see Phase 2, signed allow-listed actions only).
- Consent/visibility hooks already exist: tray notifications for
  failed authentication and remote connections, and the
  DesktopAccessDialog approval flow via access-control rules.

## Phase 2 — Integration blueprint

### IPC: SYSTEM service ↔ user-session components

- A **new, separate** named pipe (`VyrjaRmmBridge`) hosted by a new
  `RmmServiceBridge` thread inside the service process (same hosting
  pattern as `ServiceDataManager`). The existing
  `VeyonServiceDataManager` pipe is **not** extended: its
  `UserAccessOption` + environment-variable bearer token is acceptable
  for logon-data shuttling, not for privileged operations.
- Pipe DACL: SYSTEM + Administrators full access; the interactive
  session user's SID gets status/query access only. The service calls
  `ImpersonateNamedPipeClient` + `GetTokenInformation(TokenUser)` to
  authenticate the caller per message, validates the caller's session
  against its own WTS session table, and never executes while
  impersonating. Every pipe client — including the genuine server
  process — is treated as untrusted (confused-deputy rule).
- Protocol: length-prefixed `VariantArrayMessage` (codebase
  convention), hard size caps, strict per-command schema validation,
  structured verb + typed arguments only — never raw command lines.

### Threading & concurrency

- Telemetry and pipe handling run in dedicated `QThread`s inside the
  service and inside a feature **worker process** on the session side.
  They never run in `ComputerControlServer`'s thread and never touch
  the VNC server / framebuffer path.
- Bounded queues with explicit overflow policy: drop-oldest for
  telemetry samples, reject-new for admin actions. Collectors run on
  coarse timers (default 60 s) with jitter and a per-cycle CPU/time
  budget. Cloud upload is a single outbound queue with a capped disk
  spool and exponential backoff — a cloud outage can never block IPC
  or the desktop session.

### Feature flags, fallback, watchdog (three independent gates)

1. Build: `-DWITH_RMM=ON` (default OFF) or `-DSKIP_PLUGINS` exclusion.
2. Config: `RMM/Enabled` (default `false`) plus sub-gates
   `RMM/TelemetryEnabled`, `RMM/AdminActionsEnabled` (both default
   `false`) and `RMM/RequireLocalConsent` (default `true`).
3. Runtime: the feature UUID can be added to
   `Features/DisabledFeatures` at any time without reinstalling.

Any initialisation failure disables the RMM module only; the service
falls back to pure session supervision. A heartbeat with a circuit
breaker (self-disable after N consecutive failures) prevents crash
loops, mirroring the existing server watchdog pattern.

### Admin execution — safety rails

- **No generic shell, ever.** Fixed allow-list of action verbs
  (restart service, collect inventory, run an approved package
  identified by hash, power actions mirroring the existing
  powercontrol plugin).
- Every action request is a signed descriptor (nonce + expiry ≤ 60 s),
  verified with CryptoCore against a pinned admin public key, using
  the same key-store pattern as key-file authentication.
- Execution only via the existing platform helpers, with explicit
  environment block, no inherited handles, job-object limits, timeout
  and capped output capture.
- Append-only audit log in the data directory (ACL: SYSTEM +
  Administrators), written **before** execution (intent) and after
  (result + exit code).

### Telemetry scope (privacy boundary)

Inventory and health only: device ID (generated once, stored in
`RMM/DeviceID`), timestamps, agent/OS versions, hostname, session
count, service uptime, CPU/memory/disk aggregates, network interfaces,
AMT capability flag. **No keystrokes, no screen capture, no file
contents, no per-user content.** Screen/input capabilities remain
exclusively in the existing interactive, notified Veyon features.
A visible tray indicator shows when RMM is enabled, and when an admin
action executes.

### Control plane: PocketBase (self-hosted, open source)

The control plane is **PocketBase** — a single self-hostable binary
(MIT licence) with an embedded database, REST/realtime API, admin UI
and built-in authentication. It replaces the earlier Vercel + Supabase
design: no separate frontend hosting, no Postgres to operate, no
vendor lock-in. One small VPS (or any always-on machine) runs the
whole control plane.

- The agent never holds privileged database credentials and never
  accepts commands from an unauthenticated poll. Flow:
  agent -> HTTPS (TLS 1.2+, pinned certificate) -> PocketBase API
  (per-device authenticated account, collection API rules restrict
  each device to its own records) -> signed action descriptors are
  picked up from the device's command queue collection.
- Per-device keypair enrolled out-of-band (Configurator / Quick Setup
  wizard); the server keeps the public keys and a revocation list,
  checked on every request.
- Backups are a single data directory copy; scaling limits of the
  embedded database are acceptable for classroom / small-fleet sizes.
- The LAN master<->client protocol stays fully independent: control
  plane outage or compromise must not affect classroom control.

### Intel AMT / vPro — external gateway (Phase 3C)

AMT (WS-MAN, ports 16992/16993) is integrated **server-side**, never
inside the desktop agent: the agent is offline exactly when
out-of-band management is needed, and AMT credentials must never be
stored on endpoints. A standalone Node.js gateway next to the
PocketBase control plane authenticates operators, keeps per-device AMT
credentials in a secrets vault, issues power/boot/KVM-redirect
commands, and writes every OOB action to the same audit trail. The
agent's only AMT role is reporting provisioning capability as one
telemetry field.

## Phase 3 — Implementation steps

- **3A (scaffold, this change):** `RMM/*` config gates (default off),
  `WITH_RMM` build option (default OFF), reserved `plugins/rmmagent/`
  slot, this document. No behavioural change in shipped binaries.
- **3B:** `RmmServiceBridge` thread + `VyrjaRmmBridge` pipe with DACL
  and caller-token validation; telemetry collector worker with bounded
  queue; signed allow-listed admin actions with audit log.
- **3C:** PocketBase collections + API rules; device enrolment;
  AMT gateway service.

Open items to audit before 3B: `plugins/webapi` authentication model,
DesktopServices run-program surface, and the FeatureWorkerManager
worker-socket handshake details.
