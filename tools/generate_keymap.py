#!/usr/bin/env python3

import csv
from numbers import Number
import re
from typing import cast
import inflect
import argparse

SPECIAL_REPLACEMENTS = {
    "*": "STAR",
    "+": "PLUS",
    "-": "MINUS",
    "/": "SLASH",
    "\\": "BACKSLASH",
    "=": "EQUALS",
    "(": "LEFT PARENTHESIS",
    ")": "RIGHT PARENTHESIS",
    "{": "LEFT BRACE",
    "}": "RIGHT BRACE",
    "[": "LEFT BRACKET",
    "]": "RIGHT BRACKET",
    "<": "LESS THAN",
    ">": "GREATER THAN",
    "!": "EXCLAMATION MARK",
    "@": "AT SYMBOL",
    "#": "POUND SIGN",
    "$": "DOLLAR SIGN",
    "^": "CARET",
    "&": "AMPERSAND",
    "_": "UNDERSCORE",
    "`": "GRAVE ACCENT",
    "~": "TILDE",
    "|": "PIPE",
    ".": "PERIOD",
    ",": "COMMA",
    ";": "SEMICOLON",
    ":": "COLON",
    '"': "DOUBLE QUOTE",
    "'": "SINGLE QUOTE",
    "?": "QUESTION MARK",
}

ENUM_HEADER = """#pragma once

enum KeyCode {
"""

ENUM_FOOTER = """};
"""

SWITCH_DECL = """char scan_code_to_char(enum KeyCode scan_code);
"""

SWITCH_HEADER = """#include "keycodes.h"

char scan_code_to_char(enum KeyCode scan_code) {
  switch (scan_code) {
"""

SWITCH_CASE_TEMPLATE = """  case {}:
    return '{}';
"""

SWITCH_FOOTER = """
  default:
    return '\\0';
  }
}
"""


def remove_parens(s: str) -> str:
    return re.sub(r"\(([^)]*)\)", r"\1", s)


def replace_special(s: str) -> str:
    return "".join(
        SPECIAL_REPLACEMENTS[c] if c in SPECIAL_REPLACEMENTS else c for c in s
    )


p = inflect.engine()


def expand_numbers(s: str) -> str:
    return re.sub(
        r"(\w?)(\d+)",
        lambda m: (f"{m.group(1)} " if m.group(1) else "")
        + cast(str, p.number_to_words(cast(Number, int(m.group(2))))),
        s,
    )


def generate_keymap(keycodes_path: str, header_path: str, source_path: str):
    with open(keycodes_path, "r") as f, open(header_path, "w") as h, open(source_path, "w") as s:
        tsv_file = csv.reader(f)

        keymap = {}
        charmap = {}

        next(tsv_file)
        for code, name, char in tsv_file:
            codes = code.split()
            if len(codes) == 1:
                keymap[codes[0]] = "KEYCODE_" + expand_numbers(
                    replace_special(remove_parens(name))
                ).upper().strip().replace("-", "_").replace(" ", "_")
                if char:
                    charmap[keymap[codes[0]]] = char

        keymap = dict(sorted(keymap.items(), key=lambda e: e[0]))
        charmap = dict(sorted(charmap.items(), key=lambda e: e[0]))

        h.write(ENUM_HEADER)
        for code, name in keymap.items():
            h.write(f"    {name} = {code},\n")
        h.write(ENUM_FOOTER)

        h.write(SWITCH_DECL)

        s.write(SWITCH_HEADER)
        for code, char in charmap.items():
            s.write(
                SWITCH_CASE_TEMPLATE.format(
                    code, re.sub(r"['\\](?![a-z])", lambda m: rf"\{m.group(0)}", char)
                )
            )
        s.write(SWITCH_FOOTER)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        prog="generate_keymap",
        description="Generate a keymap file from a list of scancodes",
    )
    parser.add_argument("infile", help="input file")
    parser.add_argument("-d", "--header", help="output header file")
    parser.add_argument("-o", "--output", help="output file")

    args = parser.parse_args()
    generate_keymap(args.infile, args.header, args.output)
