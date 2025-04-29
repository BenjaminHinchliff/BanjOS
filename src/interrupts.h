#pragma once

#define CLI asm volatile("cli")
#define STI asm volatile("sti")

void IRQ_init();
