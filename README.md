# V1495 control programs

This repository contains a couple of programs communicating with CAEN V1495 VME
module. They are meant to be used along with the firmware being developed at
https://github.com/WCTE/V1495_firmware

`v1495-registers` reads a JSON file containing a single JSON object mapping
register addresses to 32-bit values, e.g.,

```json
{
    "0x300c": "0x01234567",
    "0x3010": "0x89abcdef"
}
```
It then connects to a V1495 and writes the values to the corresponding registers.

`v1495-counters` connects to a V1495, prints contents of its counters and
optionally resets the counters. Counters addresses are hardcoded into the
program with the help of `make-counters.pl` which generates a C++ array from
`config.json` and `VME1495_counters.txt`. `AR_[ABD]COUNTERS` names are taken
from `"input_signals"` entry in `config.json`, and `AR_LVL[12]_COUNTERS` ---
from `"level_[12]_logics"`; missing counters are numbered.

Run `v1495-registers --help` or `v1495-counters --help` for more detailed
information on how to use the programs.

# Installation

Following packages are required to compile the program:

* A C++ compiler
* Perl
* [caen++](https://github.com/jini-zh/caenpp/tree/wcte) (wcte branch)
* [jsoncpp](https://github.com/open-source-parsers/jsoncpp)
* [JSON::XS](https://metacpan.org/pod/JSON::XS)

Compile the programs by executing `make` in the current directory. No further
installation is currently provided.
