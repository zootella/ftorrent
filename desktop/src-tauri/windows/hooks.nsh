; Hooks into the NSIS installer Tauri generates, named in bundle.windows.nsis.installerHooks in tauri.conf.json. Tauri includes this file at the top of its installer script and inserts each NSIS_HOOK_ macro defined here at a fixed point in its install and uninstall sections; the other macros are helpers those call.

; Uninstalling takes back what associate.js told Windows ftorrent can open. Those are writes the running program made, not the installer, so a bare NSIS uninstall knows nothing of them and would leave ftorrent listed in Open with and in Settings, pointing at a program that's gone. Everything is under HKEY_CURRENT_USER, like the install itself, and names come from the same product name associate.js reads, so a fork that renames the app cleans up after itself too.
; This runs after the uninstall rather than before it because the section's first real step is asking to close a running ftorrent, and a user who says no there stops the uninstall; the pre hook would already have taken the keys, leaving a working install that can't open anything. Here the files are gone and $INSTDIR still holds the folder they were in.
; Removing ftorrent in Settings runs this, and so does running a newer installer by hand, as of Tauri 2.11.4: its reinstall page selects uninstalling first and runs the old uninstall.exe without /UPDATE, so the registrations go here and come back on the new copy's first launch, which the finish page offers. It's the old copy's uninstaller that runs, so a change to this file reaches upgrades one version later. An update, which runs the new installer with /UPDATE, never runs the old uninstaller at all, so the guard below is for uninstall.exe /UPDATE run directly, and for a later template that calls it that way.
!macro NSIS_HOOK_POSTUNINSTALL
	${If} $UpdateMode <> 1 ; an update replaces the program and leaves its registrations in place, the same guard Tauri's template puts on the shortcuts and the Run value just before this hook
		DeleteRegValue HKCU "Software\RegisteredApplications" "${PRODUCTNAME}" ; first, the reverse of the order associate.js writes in, so Settings stops listing ftorrent before what that listing points at is removed
		DeleteRegKey HKCU "Software\${PRODUCTNAME}\Capabilities"
		DeleteRegKey /ifempty HKCU "Software\${PRODUCTNAME}" ; Tauri keeps the install folder in a key beside Capabilities, so the parent goes only once nothing is left in it

		!insertmacro FTORRENT_UNREGISTER_FILE ".torrent" "${PRODUCTNAME}.torrent"
		!insertmacro FTORRENT_UNREGISTER_FILE ".${PRODUCTNAME}" "${PRODUCTNAME}.${PRODUCTNAME}"
		!insertmacro FTORRENT_UNREGISTER_LINK "magnet" "${PRODUCTNAME}.url.magnet"
		!insertmacro FTORRENT_UNREGISTER_LINK "${PRODUCTNAME}" "${PRODUCTNAME}.url.${PRODUCTNAME}"
		DeleteRegKey HKCU "Software\Classes\Applications\${MAINBINARYNAME}.exe" ; the executable's own key, with the types it supports

		System::Call "shell32::SHChangeNotify(i 0x08000000, i 0, p 0, p 0)" ; SHCNE_ASSOCCHANGED, the same call registry.rs makes, so Explorer drops the icons and menus it cached
	${EndIf}
!macroend

!macro FTORRENT_UNREGISTER_FILE extension program ; a kind of file ftorrent offered to open
	DeleteRegKey HKCU "Software\Classes\${program}" ; ftorrent's own ProgID, which no other program writes, taken whole
	DeleteRegValue HKCU "Software\Classes\${extension}\OpenWithProgids" "${program}" ; the offer, one value in a list other programs share
	DeleteRegKey /ifempty HKCU "Software\Classes\${extension}\OpenWithProgids" ; and the list and the extension's key only if that leaves them holding nothing: /ifempty spares a key with any subkey or any value, its default included, so another program's registration keeps its place
	DeleteRegKey /ifempty HKCU "Software\Classes\${extension}"
!macroend

!macro FTORRENT_UNREGISTER_LINK scheme program ; a kind of link ftorrent offered to open
	DeleteRegKey HKCU "Software\Classes\${program}" ; ftorrent's own ProgID for the scheme, taken whole
	ReadRegStr $R0 HKCU "Software\Classes\${scheme}\shell\open\command" "" ; the shared class named for the scheme, which associate.js writes only while no other program holds it
	${If} $R0 == "$\"$INSTDIR\${MAINBINARYNAME}.exe$\" $\"%1$\"" ; so it goes only if it still runs ftorrent, the same test Tauri's template uses for its deep links; a class another client has taken over since stays theirs
		DeleteRegKey HKCU "Software\Classes\${scheme}"
	${EndIf}
!macroend
