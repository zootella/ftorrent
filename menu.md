# Menus

An audit, not a plan: every menu ftorrent shows on each platform, read from the code on 2026-10-09, so it can be held against what a machine actually shows. The code is `menu_install` and `tray_install` in `desktop/src-tauri/src/lifecycle.rs`, one of each per platform, and the page's side of it in `App.vue`, which hears the `menu` event and opens the route it names. Items the operating system or Tauri's menu library adds on their own are marked **(system)**, with the label and shortcut the library gives them. Shortcuts are written the way each platform shows them. A line of dashes is a separator.

What the items do, in every menu that has them: **About** and **Settings** open that page in the one window, and on the Mac bring the window forward first, since the menu bar is there while the window sits minimized. **Help** opens `https://docs.ftorrent.com/help` in the system's browser through `process_open`, and leaves the window where it is. **Show** brings the window back, restored and focused. **Exit** or **Quit** ends the process, which stops the engine and writes the settings.

## Windows

Two places: the menu bar across the top of ftorrent's window, and the icon in the taskbar's notification area. The window's close button hides the window, so the tray icon and File, Exit are the ways back and out. A letter after `&` is an access key, underlined while Alt is held.

```
Menu bar, in the window
  File                              Alt+F
    Exit                            access key x; no shortcut, since Alt+F4 is the system's close, which hides
  Tools                             Alt+T
    Options...                      access key O; opens the Settings page, titled Options on Windows
  Help                              Alt+H
    View Help               F1      access key V; opens docs.ftorrent.com/help in the browser
    ---
    About ftorrent                  access key A; opens the About page

Notification area icon                tooltip "ftorrent"; the glyph in black on a light taskbar, white on a dark one, following the theme as it changes
  left click                        shows the window
  right click, the menu
    Show ftorrent                   access key S
    Exit                            access key x

Window's system menu (Alt+Space)      (system) Restore, Move, Size, Minimize, Maximize, Close Alt+F4; Close hides, like the close button
Taskbar button                        (system) the window's own; ftorrent adds no jump list
```

## macOS

Three places: the menu bar when ftorrent is the active app, the icon near the clock, and the Dock icon. The red button hides the window and takes the Dock icon with it, by switching the app to the Accessory policy, so a hidden ftorrent has no menu bar, no Dock icon, and no ⌘-Tab entry, and the menu bar icon is the only menu until Show brings it all back. Labels that carry the app's name take it from the product name, so a fork reads its own.

```
Menu bar, while ftorrent is active
  Apple menu                        (system)
  ftorrent                          the app menu
    About ftorrent                  opens the About page, window brought forward
    ---
    Settings…               ⌘,      opens the Settings page, window brought forward
    ---
    Services                ▸       (system) filled by macOS
    ---
    Hide ftorrent           ⌘H      (system)
    Hide Others             ⌥⌘H     (system)
    Show All                        (system)
    ---
    Quit ftorrent           ⌘Q      quits
  File
    Close Window            ⌘W      hides the window, the same as the red button; a fullscreen window leaves fullscreen first
  Edit                              (system) the six act in the web view's text fields; all start gray, and the page lights each while something can answer it, through menu_enable: a focused text field lights Undo, Redo, Paste, and Select All, selected text lights Copy, and both light Cut
    Undo                    ⌘Z
    Redo                    ⇧⌘Z
    ---
    Cut                     ⌘X
    Copy                    ⌘C
    Paste                   ⌘V
    Select All              ⌘A
    ---
    AutoFill                ▸       (system) macOS adds these three to any menu titled Edit, and they can't be declined by the menu
    Start Dictation…        fn D    (system)
    Emoji & Symbols         fn      (system)
  View
    Toggle Full Screen      ⌃⌘F     (system) the library's label on the system's own fullscreen action; macOS may show its own Enter Full Screen here as well, to confirm
  Window                            registered with macOS as the Window menu, so it keeps the list of windows below
    Minimize                ⌘M      (system)
    Zoom                            (system)
    ---
    Bring All to Front              (system)
    [window list]                   (system) ftorrent's window, and whatever arrangement items this macOS adds to a Window menu
  Help                              registered with macOS as the Help menu, which is what adds the search field
    [search field]                  (system) finds any menu item by name
    ftorrent Help           ⇧⌘/     opens docs.ftorrent.com/help in the browser; the window stays where it is; how macOS draws the shortcut, ⇧⌘/ or ⌘?, to confirm

Menu bar icon, near the clock          tooltip "ftorrent"; a template glyph macOS paints in the bar's own color; present for as long as ftorrent runs, and the way back once the window is hidden
  click, either button, the menu
    Show ftorrent                   brings the window, the Dock icon, and the menu bar back
    ---
    Quit ftorrent                   quits; no ⌘Q here, since a hidden ftorrent has no menu bar for it

Dock icon                             present only while the window is shown; leaves with it
  right click or press, the menu      (system) macOS's own menu for a running app: the window's name, Options with Keep in Dock, Open at Login, and Show in Finder, Hide, Quit; ftorrent adds nothing
  click                               brings the window forward if it is hidden by ⌘H or minimized
```

## Linux

No menus. Tauri adds no default menu bar off the Mac and ftorrent sets none, no tray icon is built, and the window's close button quits outright, which stops the engine and writes the settings. Everything a menu would reach is on the page: the Settings and About pages through the page's own navigation, and quitting through the close button.

```
(nothing)
```

## The web view's own menu, every platform

The engine inside the window, WebView2, WebKit, or WebKitGTK, has a right-click menu of its own, with items like Reload and Print. A release build turns it away everywhere except in a text field, where the engine's Cut, Copy, and Paste stay. A development build keeps it everywhere, for inspecting the page. This is `main.js`, not the Rust menus.
