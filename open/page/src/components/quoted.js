// Workbench for mutating more.txt.
// Usage: node quoted.js
// Reads more.txt, parses each 4-line record, runs transform(), writes output.txt.
// When a transform is ready to ship, copy output.txt over more.txt by hand.

import fs from 'node:fs'
import path from 'node:path'
import { fileURLToPath } from 'node:url'

const __dirname = path.dirname(fileURLToPath(import.meta.url))
const INPUT = path.join(__dirname, 'more.txt')
const OUTPUT = path.join(__dirname, 'output.txt')

const raw = fs.readFileSync(INPUT, 'utf8')

const records = raw
	.trim()
	.split(/\n\s*\n/)
	.map(block => {
		const lines = block.split('\n')
		return {
			text: lines[0],
			author: lines[1],
			url: lines[2],
			context: lines[3],
		}
	})

const contexts = [...new Set(records.map(r => r.context))].sort()
const out = contexts.join('\n') + '\n'

fs.writeFileSync(OUTPUT, out)
console.log(`read ${records.length} records, wrote ${contexts.length} unique contexts to output.txt`)
