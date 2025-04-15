#pragma once

int printk(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));

#ifndef NDEBUG
#define dprintk(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define dprintk(fmt, ...)
#endif // !NDEBUG
