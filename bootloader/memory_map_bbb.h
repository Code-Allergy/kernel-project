#ifndef MEMORY_MAP_BBB_H
#define MEMORY_MAP_BBB_H

//----------------------------------------//
//            BBB memory map             //
//----------------------------------------//

// for handler mode can use:
//	0b11111  system mode (same stack as thread mode)
//	0b10011  supervisor mode (separate stack, not supported right now)
//
#define MODE_USR 0b10000	// thread-mode, unprivileged
#define MODE_THR 0b11111	// thread-mode, privileged
#define MODE_HND 0b11111	// handler-mode (always privileged)


#endif