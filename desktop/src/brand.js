import tauriConfiguration from '../src-tauri/tauri.conf.json' with {type: 'json'}//the standard way to import json as a module, which vite and node both read
import cargoManifest from '../src-tauri/Cargo.toml?raw'//the file as text, which vite hands over for any import marked raw
import {parse as parseToml} from 'smol-toml'

/*
The product's two names, read from the two files that already hold them when vite builds the page, so the places they're written stay the places every build of the app already reads. A fork renames the app there, and the page, the rust core, the engine, and the installers follow.

brandName is the name as people read it, productName in tauri.conf.json: ftorrent here, and Fuji in a sibling project. It goes wherever a person reads a name: the window's title, the tray, every sentence on the page and in the settings file's comments, the Start menu shortcut, the Windows uninstall entry, the ProgIDs that Windows' Settings shows beside a file type, and the install folder under local application data, which Windows names in display case by convention.

brandStem is the stem of the executable's name, ftorrent of ftorrent.exe, which is the crate's name in Cargo.toml, since Cargo names the executable from it: ftorrent here, fuji there. Stem is the word the languages use for a file name without its extension, Rust's file_stem and Python's stem. It goes wherever a file system or a program matches a name: the executable, the engine's folder, the settings file, the lock file, the pipe a second launch writes to, the log folder, the hidden session folder in each download folder, the custom extension and link scheme, and the published installer names. The two are the same string for ftorrent and differ for most forks, and nothing derives one from the other, since a product called Candy Crush may be candycrush or candy-crush inside; each is read from its own file.

The rust core reads the same two from its package info, name and crate_name, and the pipelines read the same two files. A few files have to write a name out rather than read it, because nothing can read it there, tauri.conf.json's resources and the engine's PyInstaller spec among them; the section The two names in the desktop README lists every one, with what goes wrong if a fork misses it.
*/

export const brandName        = tauriConfiguration.productName            //the name people read, like ftorrent or Fuji
export const brandStem        = parseToml(cargoManifest).package.name     //the name files carry, like ftorrent or fuji; read from Cargo.toml, derived from nothing
export const brandDescription = tauriConfiguration.bundle.shortDescription//the one line the installers and the settings app show beside the name
export const brandHomepage    = tauriConfiguration.bundle.homepage        //the project's web address, https://ftorrent.com/, which the about page links to
export const brandHost        = new URL(brandHomepage).host                //the address the way a person says it, ftorrent.com, for a link's words and the settings file's comments
export const brandDocs        = `https://docs.${brandHost}/`                //the project's writing, docs.ftorrent.com, which the Help menu opens; a fork whose documentation lives elsewhere writes its own address here
