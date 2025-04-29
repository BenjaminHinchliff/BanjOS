#!/usr/bin/env python3

import os

HAS_ERROR_CODE = [
    8,  # double fault
    10, # invalid tss
    11, # segment not present
    12, # stack segment fault
    13, # general protection fault
    14, # page fault
    17, # alignment check
]

STUB_TEMPLATE = "isr_no_err_stub {}"
ERR_STUB_TEMPLATE = "isr_err_stub {}"

REPLACE_TOKEN = "{{HANDLER_STUBS}}"

def generate_handler_stubs(inpath: os.PathLike | str, opath: os.PathLike | str) -> None:
    with open(inpath, "r") as ifh, open(opath, "w") as ofh:
        template = ifh.read()
        stubs = []
        for i in range(256):
            if i in HAS_ERROR_CODE:
                stubs.append(ERR_STUB_TEMPLATE.format(i))
            else:
                stubs.append(STUB_TEMPLATE.format(i))
        replaced = template.replace(REPLACE_TOKEN, "\n".join(stubs))
        ofh.write(replaced)

if __name__ == "__main__":
    generate_handler_stubs("data/isr_handler_template.asm", "src/isr_handler.asm")
