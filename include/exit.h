#pragma once

#define HLT asm volatile("hlt");
#define EXIT asm volatile("cli; hlt");
