import { mkdirSync, readFileSync, writeFileSync } from 'node:fs';
import { join } from 'node:path';

const [source, destination] = process.argv.slice(2);
mkdirSync(destination, { recursive: true });
const text = readFileSync(join(source, 'coredll.def'), 'utf8');
const exports = text.split(/\bEXPORTS\b/)[1];
if (!exports) throw new Error('Missing export table: coredll');
const names = exports.split(/\r?\n/).map(line => line.replace(/;.*/, '').trim().split(/\s+/)[0])
    .filter(name => /^[A-Za-z_][A-Za-z0-9_]*$/.test(name));
writeFileSync(join(destination, 'coredll.txt'), [...new Set(names)].join('\n') + '\n');
