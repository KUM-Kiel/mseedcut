# mseedcut

## Building

```
$ make
```

This creates a mseedcut binary in `build/`.
Copy this to a directory in your `$PATH`.

## Usage

```text
$ mseedcut <MiniSEED File>
```

This program cuts a MiniSEED file into pieces of 64MB.
The files are named after the original with a three digit
number appended to their name.

Be careful to only use it on MiniSEED files, as normal
SEED files have a header, which would end up only in the
first file.

As a second limitation this program only works with files
with a blocksize of 4096 bytes, which is the most common
for MiniSEED.

## License

MIT. See LICENSE.

## Author

Lukas Joeressen \<info@kum-kiel.de\><br />
http://www.kum-kiel.de/
