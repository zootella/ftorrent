import {spawnSync} from 'node:child_process'
import {homedir} from 'node:os'
import {fileURLToPath} from 'node:url'

/*
The release build, tauri build with one thing added: the Rust compiler writes the home folder of the machine building as ~ in every source path it puts into the program. pnpm compile and pnpm installer run tauri through here, and pnpm local doesn't, since a development build never leaves the machine.

Rust keeps the file and line of every place the code can panic, an unwrap, an expect, an index past the end, so a crash can say where it happened, and a release build keeps them, though it drops the debugging information. A file of this crate is written relative to the crate, like src/net.rs. A dependency is written by its full path in Cargo's download cache, and the parts of the standard library compiled into the program by their path in rustup's copy of its source, and both of those sit in the home folder of whoever built it, which on most machines is named for them. rustc's --remap-path-prefix rewrites the start of each path as it's written, so a dependency's path reads ~/.cargo/registry/src/ and on, still naming the crate, its version, the file, and the line.

The flag goes to cargo in CARGO_ENCODED_RUSTFLAGS, cargo's variable for compiler flags from a script, which separates flags with the ASCII unit separator, so a home folder with a space in it stays one flag. The home folder is read here, as the build runs, so nothing committed names anyone. Flags already in the environment come first and are kept, from either of cargo's two variables, since the encoded one, once set, is the only one cargo reads. A cargo config file's rustflags are skipped for the same reason, and the crate has none; one added later belongs here instead.

Cargo counts a change of flags as a different build, so the first release build after this setting changes compiles every crate again, once. The Linux packages build in containers, which run tauri build themselves, with Cargo's cache in /usr/local/cargo, nobody's home folder; a Linux machine running pnpm installer for itself comes through here like any other.
*/

const separator = '\x1f'//the unit separator, between flags in CARGO_ENCODED_RUSTFLAGS
let given = process.env.CARGO_ENCODED_RUSTFLAGS ? process.env.CARGO_ENCODED_RUSTFLAGS.split(separator) : (process.env.RUSTFLAGS || '').split(' ').filter(Boolean)//RUSTFLAGS is split on spaces, as cargo splits it
let flags = [...given, `--remap-path-prefix=${homedir()}=~`]//rustc matches the prefix by whole folders, so a home of /Users/ann never touches /Users/anne

let cli = fileURLToPath(import.meta.resolve('@tauri-apps/cli/tauri.js'))//the tauri command's own script, which the shim in node_modules/.bin runs with node too; running it directly needs no shell on windows, where the shim is a .cmd file only a shell starts
let built = spawnSync(process.execPath, [cli, 'build', ...process.argv.slice(2)], {//this same node, and the arguments after build.js, like --no-bundle, passed through
	stdio: 'inherit',
	env: {...process.env, CARGO_ENCODED_RUSTFLAGS: flags.join(separator)},
})
if (built.error) throw built.error//tauri couldn't be started at all
process.exit(built.status ?? 1)//tauri's own exit code, so the && after this in package.json stops where tauri failed; a build ended by a signal has no code, and counts as failed
