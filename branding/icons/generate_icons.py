#!/usr/bin/env python3
"""
Vyrja icon generator - draws the complete original icon set of Vyrja Classroom.

Every icon is described below as simple SVG shapes (24x24 grid) and rendered to PNG with cairosvg:
    pip install cairosvg pillow
    python3 branding/icons/generate_icons.py        (run from the repository root)

Styles
  tile   - colored rounded-square tile with a white glyph (feature buttons)
  plain  - single-color line glyph (UI icons); "-dark" variants use a light color
  badge  - round colored badge (toasts)
  state  - monitor illustration with a status badge (computer states)
Edit colors/glyphs here and re-run to restyle the whole application.
"""
import os, sys, io
import cairosvg
from PIL import Image

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))

NAVY, INDIGO, ORANGE = '#26236B', '#5B56D6', '#FF6B4A'
GREEN, RED, AMBER, SKY = '#2E9E5B', '#E5484D', '#F5A524', '#3B82F6'
LIGHT, MUTED, PLUM = '#ECEBFA', '#9A97B5', '#8E4EC6'
TILE_NAVY = '#3B37A0'   # navy tile that stays readable on dark backgrounds
MID = '#5B56D6'         # line color for icons that have no dark twin

G = {}  # glyph library (24x24, stroke based, currentColor)
G['monitor'] = '<rect x="3" y="4" width="18" height="12" rx="2"/><path d="M9 20h6M12 16v4"/>'
G['monitor_eye'] = ('<rect x="3" y="4" width="18" height="12" rx="2"/><path d="M9 20h6M12 16v4"/>'
                    '<path d="M7.5 10c1.200-1.700 2.800-2.600 4.500-2.600s3.300.9 4.500 2.600c-1.200 1.700-2.800 2.600-4.500 2.600S8.700 11.700 7.500 10z"/><circle cx="12" cy="10" r="1" fill="currentColor"/>')
G['monitor_cursor'] = ('<path d="M21 12V6a2 2 0 0 0-2-2H5a2 2 0 0 0-2 2v8a2 2 0 0 0 2 2h6"/><path d="M8 20h4"/>'
                       '<path d="M14 11l7 3-3 1.500L16.500 19z" fill="currentColor"/>')
G['monitor_play'] = '<rect x="3" y="4" width="18" height="12" rx="2"/><path d="M9 20h6M12 16v4"/><path d="M10.500 7.200v5.600l4.500-2.800z" fill="currentColor"/>'
G['monitor_off'] = '<rect x="3" y="4" width="18" height="12" rx="2"/><path d="M9 20h6M12 16v4M4 3l16 16"/>'
G['lock'] = '<rect x="5" y="11" width="14" height="9" rx="2"/><path d="M8 11V8a4 4 0 0 1 8 0v3"/><circle cx="12" cy="15.500" r="1.100" fill="currentColor"/>'
G['message'] = '<path d="M4 5h16a1 1 0 0 1 1 1v9a1 1 0 0 1-1 1h-8l-5 4v-4H4a1 1 0 0 1-1-1V6a1 1 0 0 1 1-1z"/><path d="M8 9.500h8M8 12.500h5"/>'
G['window'] = '<rect x="3" y="4" width="18" height="16" rx="2"/><path d="M3 8.500h18"/><circle cx="6" cy="6.300" r=".6" fill="currentColor"/><circle cx="8.500" cy="6.300" r=".6" fill="currentColor"/>'
G['rocket'] = '<path d="M5 19c0-3 1-4 3-5l-2-2c2-1 3-1 4-1 3-5 7-7 10-7 0 3-2 7-7 10 0 1 0 2-1 4l-2-2c-1 2-2 3-5 3z"/><circle cx="15" cy="9" r="1.200" fill="currentColor"/>'
G['globe'] = '<circle cx="12" cy="12" r="9"/><path d="M3 12h18M12 3c3.500 3 3.500 15 0 18M12 3c-3.500 3-3.500 15 0 18"/>'
G['upload'] = '<path d="M4 15v4h16v-4"/><path d="M12 15V4M8 8l4-4 4 4"/>'
G['download'] = '<path d="M4 15v4h16v-4"/><path d="M12 4v11M8 11l4 4 4-4"/>'
G['camera'] = '<path d="M4 8h3l1.500-2h7L17 8h3a1 1 0 0 1 1 1v9a1 1 0 0 1-1 1H4a1 1 0 0 1-1-1V9a1 1 0 0 1 1-1z"/><circle cx="12" cy="13" r="3.500"/>'
G['power'] = '<path d="M12 3v8"/><path d="M6.500 6.500a8 8 0 1 0 11 0"/>'
G['reboot'] = '<path d="M20 12a8 8 0 1 1-2.500-5.800"/><path d="M20 4v5h-5"/>'
G['bolt'] = '<path d="M13 3L5 13.500h6L10 21l8-10.500h-6z"/>'
G['login'] = '<path d="M10 4H6a2 2 0 0 0-2 2v12a2 2 0 0 0 2 2h4"/><path d="M9 12h11M16 8l4 4-4 4"/>'
G['logout'] = '<path d="M14 4h4a2 2 0 0 1 2 2v12a2 2 0 0 1-2 2h-4"/><path d="M15 12H4M8 8l-4 4 4 4"/>'
G['users'] = '<circle cx="9" cy="8" r="3.500"/><path d="M2.500 20a6.500 6.500 0 0 1 13 0"/><circle cx="17" cy="9" r="2.500"/><path d="M17.500 14a5 5 0 0 1 4 5"/>'
G['user_add'] = '<circle cx="10" cy="8" r="3.500"/><path d="M3 20a7 7 0 0 1 14 0M19 8v6M16 11h6"/>'
G['search'] = '<circle cx="10.500" cy="10.500" r="6.500"/><path d="M15.500 15.500L21 21"/>'
G['folder'] = '<path d="M3 7a2 2 0 0 1 2-2h4l2 2h8a2 2 0 0 1 2 2v9a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2z"/>'
G['folder_open'] = '<path d="M3 18V6a2 2 0 0 1 2-2h4l2 2h7a2 2 0 0 1 2 2v2"/><path d="M3 18l2.500-7h16.500l-2.500 7a2 2 0 0 1-2 1.500H5a2 2 0 0 1-2-1.500z"/>'
G['save'] = '<path d="M5 3h11l4 4v13a1 1 0 0 1-1 1H5a1 1 0 0 1-1-1V4a1 1 0 0 1 1-1z"/><path d="M8 3v5h7V3M8 21v-7h8v7"/>'
G['add'] = '<path d="M12 5v14M5 12h14"/>'
G['trash'] = '<path d="M4 7h16M9 7V4h6v3M6 7l1 13h10l1-13M10 11v6M14 11v6"/>'
G['info'] = '<circle cx="12" cy="12" r="9"/><path d="M12 11v6"/><circle cx="12" cy="7.600" r="1.100" fill="currentColor"/>'
G['check_circle'] = '<circle cx="12" cy="12" r="9"/><path d="M8 12.500l3 3 5-6"/>'
G['check'] = '<path d="M5 12.500l4.500 4.500L19 7"/>'
G['x'] = '<path d="M6 6l12 12M18 6L6 18"/>'
G['x_circle'] = '<circle cx="12" cy="12" r="9"/><path d="M9 9l6 6M15 9l-6 6"/>'
G['minus_circle'] = '<circle cx="12" cy="12" r="9"/><path d="M8 12h8"/>'
G['warning'] = '<path d="M12 3.500l10 17.500H2z"/><path d="M12 10v5"/><circle cx="12" cy="18" r=".9" fill="currentColor"/>'
G['chev_down'] = '<path d="M6 9l6 6 6-6"/>'
G['chev_up'] = '<path d="M6 15l6-6 6 6"/>'
G['chev_left'] = '<path d="M15 6l-6 6 6 6"/>'
G['chev_right'] = '<path d="M9 6l6 6-6 6"/>'
G['fullscreen'] = '<path d="M4 9V4h5M20 9V4h-5M4 15v5h5M20 15v5h-5"/>'
G['pause'] = '<path d="M8.500 5v14M15.500 5v14"/>'
G['play'] = '<path d="M7 4.500v15l13-7.500z" fill="currentColor"/>'
G['stop'] = '<rect x="6" y="6" width="12" height="12" rx="1.800" fill="currentColor"/>'
G['record'] = '<circle cx="12" cy="12" r="6" fill="currentColor"/>'
G['grid'] = '<rect x="4" y="4" width="7" height="7" rx="1.500"/><rect x="13" y="4" width="7" height="7" rx="1.500"/><rect x="4" y="13" width="7" height="7" rx="1.500"/><rect x="13" y="13" width="7" height="7" rx="1.500"/>'
G['swap'] = '<path d="M4 8h14M14 4l4 4-4 4M20 16H6M10 12l-4 4 4 4"/>'
G['zoom_fit'] = '<circle cx="10.500" cy="10.500" r="6.500"/><path d="M15.500 15.500L21 21M8 10.500h5M10.500 8v5"/>'
G['refresh'] = '<path d="M20 11a8 8 0 0 0-14-4M4 5v4h4M4 13a8 8 0 0 0 14 4M20 19v-4h-4"/>'
G['refresh_off'] = '<path d="M20 11a8 8 0 0 0-14-4M4 5v4h4M4 13a8 8 0 0 0 14 4M20 19v-4h-4M3 3l18 18"/>'
G['slideshow'] = '<rect x="2.500" y="6" width="14" height="10" rx="2"/><path d="M7 3h13a1.500 1.500 0 0 1 1.500 1.500V13M5 20h8M9 16v4"/>'
G['spotlight'] = '<circle cx="12" cy="9" r="4"/><path d="M12 13v4M9 20h6M5 5L3 3M19 5l2-2M3.500 10H1.500M22.500 10h-2"/>'
G['key'] = '<circle cx="8" cy="15" r="4"/><path d="M11 12l9-9M16 7l3 3M14 9l2 2"/>'
G['globe_text'] = '<circle cx="12" cy="12" r="9"/><path d="M3 12h18M12 3c3.500 3 3.500 15 0 18M12 3c-3.500 3-3.500 15 0 18"/>'
G['document'] = '<path d="M6 3h9l4 4v14H6z"/><path d="M15 3v4h4M9 12h6M9 16h6"/>'
G['edit'] = '<path d="M4 20l1-4L16 5l3 3L8 19z"/><path d="M14 7l3 3"/>'
G['help'] = '<circle cx="12" cy="12" r="9"/><path d="M9.500 9.500a2.500 2.500 0 1 1 3.500 2.300c-.7.400-1 .9-1 1.700"/><circle cx="12" cy="17" r=".9" fill="currentColor"/>'
G['keyboard'] = '<rect x="2.500" y="6" width="19" height="12" rx="2"/><path d="M6 10h.01M10 10h.01M14 10h.01M18 10h.01M7 14h10"/>'
G['dup'] = '<rect x="3" y="7" width="13" height="13" rx="2"/><path d="M8 7V5a2 2 0 0 1 2-2h9a2 2 0 0 1 2 2v9a2 2 0 0 1-2 2h-3"/>'
G['present_full'] = '<rect x="3" y="4" width="18" height="12" rx="2"/><path d="M9 20h6M12 16v4M7 8v-1h1M17 8v-1h-1M7 12v1h1M17 12v1h-1"/>'
G['present_win'] = '<rect x="3" y="4" width="18" height="12" rx="2"/><path d="M9 20h6M12 16v4"/><rect x="8" y="7.500" width="8" height="5" rx="1"/>'
G['network'] = '<circle cx="12" cy="5" r="2"/><circle cx="5" cy="19" r="2"/><circle cx="19" cy="19" r="2"/><path d="M12 7v5M12 12l-6 5.500M12 12l6 5.500"/>'
G['tag'] = '<path d="M3 12V4h8l10 10-8 8z"/><circle cx="7.500" cy="8.500" r="1.100" fill="currentColor"/>'
G['gear'] = '<circle cx="12" cy="12" r="3"/><circle cx="12" cy="12" r="6.800"/><path d="M12 2v3M12 19v3M2 12h3M19 12h3M4.900 4.900l2.100 2.100M17 17l2.100 2.100M19.100 4.900L17 7M7 17l-2.100 2.100"/>'
G['shield'] = '<path d="M12 3l8 3v6c0 5-3.500 8-8 9-4.500-1-8-4-8-9V6z"/>'
G['shield_q'] = '<path d="M12 3l8 3v6c0 5-3.500 8-8 9-4.500-1-8-4-8-9V6z"/><path d="M9.800 10a2.200 2.200 0 1 1 3 2c-.6.300-.8.700-.8 1.300"/><circle cx="12" cy="16" r=".8" fill="currentColor"/>'
G['hourglass'] = '<path d="M6 3h12M6 21h12M7 3c0 5 5 6 5 9s-5 4-5 9M17 3c0 5-5 6-5 9s5 4 5 9"/>'
G['clock'] = '<circle cx="12" cy="12" r="9"/><path d="M12 7v5l3.500 2"/>'
G['sliders'] = '<path d="M4 7h9M17 7h3M4 17h3M11 17h9"/><circle cx="15" cy="7" r="2"/><circle cx="9" cy="17" r="2"/>'
G['book'] = '<path d="M5 4h12a2 2 0 0 1 2 2v14H7a2 2 0 0 1-2-2z"/><path d="M9 8h6M9 12h6"/>'
G['code'] = '<path d="M8 8l-5 4 5 4M16 8l5 4-5 4M14 5l-4 14"/>'
G['puzzle'] = '<path d="M10 4a2 2 0 1 1 4 0v2h4v4h-2a2 2 0 1 0 0 4h2v6H4v-6h2a2 2 0 1 0 0-4H4V6h6z"/>'
G['file_ok'] = '<path d="M6 3h9l4 4v14H6z"/><path d="M15 3v4h4M9 14l2 2 4-4"/>'
G['file_arrows'] = '<path d="M6 3h9l4 4v14H6z"/><path d="M15 3v4h4M12 17v-6M9.500 13.500L12 11l2.500 2.500"/>'
G['terminal'] = '<rect x="3" y="4" width="18" height="16" rx="2"/><path d="M7 9l3 3-3 3M12 15h5"/>'
G['table'] = '<rect x="3" y="4" width="18" height="16" rx="2"/><path d="M3 10h18M9 10v10"/>'
G['star'] = '<path d="M12 3l2.800 5.800 6.200.9-4.500 4.400 1.100 6.200L12 17.300 6.400 20.300l1.100-6.200L3 9.700l6.200-.9z"/>'
G['mark'] = '<path d="M4 5l8 15 8-15"/>'

def svg_plain(name, color, size, sw=1.8):
    body = G[name].replace('currentColor', color)
    return (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="{size}" height="{size}">'
            f'<g fill="none" stroke="{color}" stroke-width="{sw}" stroke-linecap="round" stroke-linejoin="round">{body}</g></svg>')

def shade(hexcol, f):
    h = hexcol.lstrip('#'); r, g, b = [int(h[i:i+2], 16) for i in (0, 2, 4)]
    return '#%02x%02x%02x' % tuple(max(0, min(255, int(c * f))) for c in (r, g, b))

def svg_tile(name, color, size):
    body = G[name].replace('currentColor', '#ffffff')
    lighter, darker = shade(color, 1.18), shade(color, 0.86)
    s = size / 128
    return (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 128 128" width="{size}" height="{size}">'
            f'<defs><linearGradient id="g" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="{lighter}"/><stop offset="1" stop-color="{darker}"/></linearGradient></defs>'
            f'<rect x="4" y="4" width="120" height="120" rx="30" fill="url(#g)"/>'
            f'<rect x="4.500" y="4.500" width="119" height="119" rx="29.500" fill="none" stroke="#ffffff" stroke-opacity=".25"/>'
            f'<g transform="translate(34,34) scale(2.500)" fill="none" stroke="#ffffff" stroke-width="1.700" stroke-linecap="round" stroke-linejoin="round">{body}</g></svg>')

def svg_badge(name, color, size):
    body = G[name].replace('currentColor', '#ffffff')
    return (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 128 128" width="{size}" height="{size}">'
            f'<circle cx="64" cy="64" r="60" fill="{color}"/>'
            f'<g transform="translate(32,32) scale(2.670)" fill="none" stroke="#ffffff" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">{body}</g></svg>')

def svg_state(color, badge_glyph, size, dim=False):
    sc = MUTED if dim else NAVY
    fill = '#E9E7F7' if dim else '#EEF0FF'
    badge = ''
    if badge_glyph:
        body = G[badge_glyph].replace('currentColor', '#ffffff')
        badge = (f'<circle cx="152" cy="140" r="38" fill="{color}" stroke="#ffffff" stroke-width="6"/>'
                 f'<g transform="translate(130,118) scale(1.830)" fill="none" stroke="#ffffff" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">{body}</g>')
    return (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 200 200" width="{size}" height="{size}">'
            f'<rect x="14" y="26" width="150" height="104" rx="14" fill="{fill}" stroke="{sc}" stroke-width="8"/>'
            f'<rect x="26" y="38" width="126" height="80" rx="6" fill="{"#F6F5FB" if dim else "#FFFFFF"}"/>'
            f'<path d="M89 130v22M60 156h58" stroke="{sc}" stroke-width="8" stroke-linecap="round"/>{badge}</svg>')

def save(svg, path, size=None):
    full = os.path.join(ROOT, path)
    cairosvg.svg2png(bytestring=svg.encode(), write_to=full)
    return path

# ------------------------------------------------------------------ mapping
PLAIN = {  # file: (glyph, size handled from existing file)
 'core/resources/application-x-pem-key.png':'key', 'core/resources/dialog-ok-apply.png':'check',
 'core/resources/document-edit.png':'edit', 'core/resources/document-open.png':'folder_open',
 'core/resources/document-save.png':'save', 'core/resources/edit-delete.png':'trash',
 'core/resources/edit-find.png':'search', 'core/resources/go-down.png':'chev_down',
 'core/resources/go-next.png':'chev_right', 'core/resources/go-previous.png':'chev_left',
 'core/resources/go-up.png':'chev_up', 'core/resources/help-about.png':'info',
 'core/resources/languages.png':'globe_text', 'core/resources/license.png':'document',
 'core/resources/list-add.png':'add', 'core/resources/media-playback-pause.png':'pause',
 'core/resources/media-playback-start.png':'play', 'core/resources/media-playback-stop.png':'stop',
 'core/resources/media-record.png':'record',
 'core/resources/user-group-new.png':'user_add', 'core/resources/view-fullscreen.png':'fullscreen',
 'master/resources/align-grid.png':'grid', 'master/resources/camera-photo.png':'camera',
 'master/resources/computer-slideshow.png':'slideshow', 'master/resources/exchange-positions-zorder.png':'swap',
 'master/resources/powered-on.png':'bolt', 'master/resources/spotlight.png':'spotlight',
 'master/resources/update-realtime-disabled.png':'refresh_off', 'master/resources/update-realtime-enabled.png':'refresh',
 'master/resources/zoom-fit-best.png':'zoom_fit',
 'configurator/resources/access-rule-ask.png':'shield_q', 'configurator/resources/application-x-ms-dos-executable.png':'terminal',
 'configurator/resources/application-x-sharedlib.png':'puzzle', 'configurator/resources/configure-shortcuts.png':'keyboard',
 'configurator/resources/help-about.png':'info', 'configurator/resources/network-vpn.png':'network',
 'configurator/resources/vcs-conflicting.png':'warning', 'configurator/resources/vcs-normal.png':'check_circle',
 'configurator/resources/vcs-removed.png':'minus_circle',
 'plugins/builtindirectory/builtindirectory.png':'book', 'plugins/webapi/webapi.png':'code',
 'plugins/ldap/application-x-kexi-connectiondata.png':'table', 'plugins/ldap/attribute.png':'tag',
 'plugins/ldap/computer.png':'monitor', 'plugins/ldap/configure.png':'gear',
 'plugins/ldap/distribute-vertical-margin.png':'sliders', 'plugins/ldap/folder-stash.png':'folder',
 'plugins/ldap/user-group-properties.png':'users',
 'plugins/platform/linux/tux.png':'terminal', 'plugins/platform/windows/windows-logo.png':'grid',
 'plugins/filetransfer/file-finished.png':'file_ok', 'plugins/filetransfer/file-scheduled.png':'clock',
 'plugins/filetransfer/file-transferring.png':'file_arrows', 'plugins/filetransfer/file-waiting.png':'hourglass',
 'plugins/remoteaccess/application-exit.png':'logout', 'plugins/remoteaccess/preferences-desktop-keyboard.png':'keyboard',
 'plugins/remoteaccess/preferences-system-windows-effect-desktopgrid.png':'grid',
}
# the platform-specific special cases that should stay dark-friendly: all plain icons get a -dark twin if one exists
TILES = {
 'core/resources/presentation-none.png':('grid', SKY),
 'plugins/demo/demo.png':('monitor_play', ORANGE), 'plugins/demo/presentation-fullscreen.png':('present_full', ORANGE),
 'plugins/demo/presentation-window.png':('present_win', AMBER), 'plugins/demo/window-duplicate.png':('dup', PLUM),
 'plugins/screenlock/system-lock-screen.png':('lock', INDIGO), 'plugins/textmessage/dialog-information.png':('message', SKY),
 'plugins/desktopservices/desktop-services.png':('rocket', GREEN), 'plugins/desktopservices/internet-web-browser.png':('globe', SKY),
 'plugins/desktopservices/preferences-desktop-launch-feedback.png':('window', INDIGO),
 'plugins/filetransfer/collect-files.png':('download', TILE_NAVY), 'plugins/filetransfer/distribute-files.png':('upload', ORANGE),
 'plugins/filetransfer/distribute-files-dark.png':('upload', ORANGE),
 'plugins/powercontrol/system-reboot.png':('reboot', SKY), 'plugins/powercontrol/system-shutdown.png':('power', RED),
 'plugins/powercontrol/preferences-system-power-management.png':('bolt', GREEN),
 'plugins/remoteaccess/kmag.png':('monitor_eye', INDIGO), 'plugins/remoteaccess/krdc.png':('monitor_cursor', TILE_NAVY),
 'plugins/remoteaccess/camera-photo.png':('camera', ORANGE), 'plugins/screenshot/camera-photo.png':('camera', ORANGE),
 'plugins/usersessioncontrol/login-user.png':('login', GREEN), 'plugins/usersessioncontrol/logout-user.png':('logout', AMBER),
 'master/resources/computers.png':('monitor', TILE_NAVY),
}
BADGES = {'core/resources/toast-success.png':('check', GREEN), 'core/resources/toast-error.png':('x', RED),
          'core/resources/toast-information.png':('info', NAVY), 'core/resources/toast-warning.png':('warning', AMBER),
          'core/resources/toast-close.png':('x', MUTED)}
STATES = {'master/resources/host-online.png':(GREEN, 'check', False), 'master/resources/host-offline.png':(MUTED, 'power', True),
          'master/resources/host-dns-error.png':(AMBER, 'help', False), 'master/resources/host-access-denied.png':(RED, 'lock', False),
          'master/resources/host-service-error.png':(ORANGE, 'warning', False)}

def existing_size(path):
    return Image.open(os.path.join(ROOT, path)).size[0]

PAGE_ICONS = {'general': 'sliders', 'service': 'gear', 'master': 'grid', 'access': 'shield', 'keys': 'key',
              'locations': 'network', 'apps': 'window', 'files': 'upload', 'webapi': 'code', 'ldap': 'users',
              'platform': 'terminal', 'generic': 'puzzle'}


def pages():
    """sidebar icons of the Setup application (light + dark variants)"""
    for name, glyph in PAGE_ICONS.items():
        save(svg_plain(glyph, NAVY, 48, 1.7), f'configurator/resources/page-{name}.png')
        save(svg_plain(glyph, LIGHT, 48, 1.7), f'configurator/resources/page-{name}-dark.png')


def extras():
    """background of locked screens and example screenshot"""
    from PIL import ImageDraw, ImageFilter
    # locked screen background (shown on student computers): navy gradient + brand mark
    w, h = 250, 300
    bg = Image.new('RGB', (w, h))
    px = bg.load()
    for y in range(h):
        t = y / (h - 1)
        c = tuple(int(a + (b - a) * t) for a, b in zip((0x2E, 0x2A, 0x86), (0x14, 0x12, 0x38)))
        for x in range(w):
            px[x, y] = c
    glow = Image.new('RGBA', (w, h), (0, 0, 0, 0))
    ImageDraw.Draw(glow).ellipse((25, 55, 225, 255), fill=(255, 107, 74, 70))
    bg = Image.alpha_composite(bg.convert('RGBA'), glow.filter(ImageFilter.GaussianBlur(28)))
    mark = Image.open(os.path.join(ROOT, 'branding/vyrja-mark.png')).convert('RGBA')
    k = 120 / max(mark.size)
    mark = mark.resize((int(mark.width * k), int(mark.height * k)), Image.LANCZOS)
    card = Image.new('RGBA', (150, 150), (0, 0, 0, 0))
    ImageDraw.Draw(card).rounded_rectangle((0, 0, 149, 149), radius=36, fill=(250, 249, 253, 255))
    card.alpha_composite(mark, ((150 - mark.width) // 2, (150 - mark.height) // 2))
    bg.alpha_composite(card, ((w - 150) // 2, 75))
    bg.convert('RGB').save(os.path.join(ROOT, 'plugins/screenlock/locked-screen-background.png'))
    # example screenshot used on the "screenshots" panel placeholder
    svg = ('<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 320 200" width="320" height="200">'
           '<defs><linearGradient id="d" x1="0" y1="0" x2="1" y2="1"><stop offset="0" stop-color="#3A368F"/><stop offset="1" stop-color="#FF6B4A"/></linearGradient></defs>'
           '<rect width="320" height="200" fill="url(#d)"/>'
           '<rect x="34" y="26" width="170" height="110" rx="8" fill="#FAF9FD"/><rect x="34" y="26" width="170" height="20" rx="8" fill="#ECEBFA"/>'
           '<circle cx="48" cy="36" r="4" fill="#FF6B4A"/><circle cx="62" cy="36" r="4" fill="#F5A524"/><circle cx="76" cy="36" r="4" fill="#2E9E5B"/>'
           '<rect x="48" y="60" width="100" height="8" rx="4" fill="#26236B"/><rect x="48" y="78" width="140" height="6" rx="3" fill="#C9C6E6"/>'
           '<rect x="48" y="92" width="120" height="6" rx="3" fill="#C9C6E6"/><rect x="48" y="106" width="130" height="6" rx="3" fill="#C9C6E6"/>'
           '<rect x="120" y="70" width="170" height="100" rx="8" fill="#FFFFFF" fill-opacity=".92"/><rect x="134" y="86" width="60" height="60" rx="10" fill="#26236B"/>'
           '<path d="M146 98l16 28 16-28" fill="none" stroke="#FF6B4A" stroke-width="7" stroke-linecap="round" stroke-linejoin="round"/>'
           '<rect x="206" y="92" width="70" height="8" rx="4" fill="#26236B"/><rect x="206" y="110" width="56" height="6" rx="3" fill="#C9C6E6"/>'
           '<rect x="0" y="180" width="320" height="20" fill="#14123A"/><rect x="8" y="185" width="10" height="10" rx="3" fill="#FF6B4A"/></svg>')
    cairosvg.svg2png(bytestring=svg.encode(), write_to=os.path.join(ROOT, 'master/resources/example-screenshot.png'))


def main():
    n = 0
    for path, glyph in PLAIN.items():
        size = existing_size(path)
        dark = path[:-4] + '-dark.png'
        has_dark = os.path.exists(os.path.join(ROOT, dark))
        save(svg_plain(glyph, NAVY if has_dark else MID, size, 1.8), path); n += 1
        if has_dark:
            save(svg_plain(glyph, LIGHT, existing_size(dark), 1.8), dark); n += 1
    for path, (glyph, color) in TILES.items():
        save(svg_tile(glyph, color, existing_size(path)), path); n += 1
    for path, (glyph, color) in BADGES.items():
        save(svg_badge(glyph, color, existing_size(path)), path); n += 1
    for path, (color, glyph, dim) in STATES.items():
        save(svg_state(color, glyph, existing_size(path), dim), path); n += 1
    # remaining dark twins that have no plain source listed above
    for dark in ['core/resources/document-open-dark.png']:
        pass
    save(svg_plain('check', '#ffffff', 32, 3.2), 'core/resources/check-white.png')
    pages()
    extras()
    print('icons written:', n)

if __name__ == '__main__':
    main()
