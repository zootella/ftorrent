import tauriConfiguration from '../src-tauri/tauri.conf.json' with {type: 'json'}//the standard way to import json as a module, which vite and node both read

//the product's name and one-line description, read from tauri.conf.json when vite builds the page, so the one place they're written is the file every build of the app already reads. A fork renames the app there, and the page, the rust core, and the engine follow: rust reads the same file through its package info, and hands the engine the name in init

export const brandName        = tauriConfiguration.productName            //ftorrent, and the name in every file, folder, registry key, and line of text the page makes from it
export const brandDescription = tauriConfiguration.bundle.shortDescription//the one line the installers and the settings app show beside the name
