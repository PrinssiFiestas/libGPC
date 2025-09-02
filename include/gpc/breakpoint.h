/* Copyright (c) 2011-2021, Scott Tsai
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

// Original library:
// https://github.com/scottt/debugbreak/tree/master
//
// This library is modified to suit the needs of libGPC. Most notably, names
// have changed from `debug_break` to `GP_BREAKPOINT` with `GP_` namespace.
// Also, inline functions have been changed to macros, which allows checking
// their existance in the preprocessor and puts the breakpoint in source code,
// not this header. This also means that the #error directives can be removed.
//
// Some additional platforms are added and bugs fixed.

#ifndef GP_BREAKPOINT_INCLUDED
#define GP_BREAKPOINT_INCLUDED 1

#ifdef __cplusplus
extern "C" {
#endif


// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------


#ifdef GP_DOXYGEN

/** Set breakpoint.
 * #defined in some platforms. Usually crashes porgram if running process is not
 * attached to a debugger, otherwise stepping and continuing execution is
 * allowed.
 */
#define GP_BREAKPOINT /* set breakpoint */

/** Set breakpoint trap.
 * #defined in most platforms. Usually crashes program if running process is not
 * attached to a debugger, otherwise continuing execution may or may not be
 * allowed, but examining variables and backtrace will be in most platforms. May
 * cause compiler optimizations to remove following code.
 */
#define GP_BREAKPOINT_TRAP /* set breakpoint or trap */

/** Set breakpoint or do nothing.
 * Available in all platforms. Sets breakpoint if @ref GP_BREAKPOINT is
 * #defined and NDEBUG is not #defined, no-op otherwise.
 */
#define GP_DEBUG_BREAKPOINT /* set breakpoint or do nothing */

/** Set breakpoint or trap or do nothing.
 * Available in all platforms. Sets breakpoint/trap if @ref GP_BREAKPOINT_TRAP
 * is #defined and NDEBUG is not #defined, no-op otherwise.
 */
#define GP_DEBUG_BREAKPOINT_TRAP /* set breakpoint or trap or do nothing */

#endif // GP_DOXYGEN

/** Check if debugger is not present.
 * @return 1 if no debugger present, 0 if debugger present, -1 if undetermined.
 * `errno` MAY be set in the latter case.
 */
int gp_debugger_is_detached(void);

/** Cached check if debugger was never present.
 * Like @ref gp_debugger_is_detached(), but only does the check once and caches
 * the result to avoid internal IO and parsing overhead.
 * @return on first call: 1 if no debugger preset, 0 if debugger preset, -1 if
 * undetermined, subsequent calls return the previous return value.
 */
int gp_debugger_was_detached(void);

#ifdef NDEBUG
/** Cached check if debugger was never present.
 * Like @ref gp_debugger_is_detached(), but only does the check once and caches
 * the result to avoid internal IO and parsing overhead.
 * @return on first call: 1 if no debugger preset, 0 if debugger preset, -1 if
 * undetermined, subsequent calls return the previous return value.
 */
static inline int gp_debugger_detached(void) { return gp_debugger_was_detached(); }
#else
/** Check if debugger is not present.
 * @return 1 if no debugger present, 0 if debugger present, -1 if undetermined.
 * `errno` MAY be set in the latter case.
 */
static inline int gp_debugger_detached(void) { return gp_debugger_is_detached(); }
#endif


// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------


// GP_BREAKPOINT_METHOD values
#define GP_BREAKPOINT_NOT_AVAILABLE         0
#define GP_BREAKPOINT_USE_TRAP_INSTRUCTION  1 // inline assembly
#define GP_BREAKPOINT_USE_BUILTIN_TRAP      2 // __builtin_trap()
#define GP_BREAKPOINT_USE_SIGTRAP           3 // raise(SIGTRAP)
#define GP_BREAKPOINT_USE_BUILTIN_DEBUGTRAP 4 // __builtin_debugtrap()
#define GP_BREAKPOINT_USE_DEBUGBREAK        5 // __debugbreak()
#define GP_BREAKPOINT_INVALID_METHOD        6

#ifdef _MSC_VER
    #define GP_BREAKPOINT_METHOD GP_BREAKPOINT_USE_DEBUGBREAK
// ----------------------------------------------------------------------------
// Not present in original library
#elif defined(__FILC__)
    #define GP_BREAKPOINT_METHOD == GP_BREAKPOINT_USE_BUILTIN_TRAP
#elif defined(__DMC__) && defined (_M_IX86)
	#define GP_BREAKPOINT_METHOD GP_BREAKPOINT_USE_TRAP_INSTRUCTION
    #define GP_TRAP_INSTRUCTION __asm int 3h
#elif defined(__alpha__) && !defined(__osf__) && defined(__GNUC__) && __GNUC__ >= 2
	#define GP_BREAKPOINT_METHOD GP_BREAKPOINT_USE_TRAP_INSTRUCTION
    #define GP_TRAP_INSTRUCTION __asm__ __volatile__ ("bpt")
// ----------------------------------------------------------------------------
#elif __GNUC__

    #if defined(__i386__) || defined(__x86_64__)
    	#define GP_BREAKPOINT_METHOD GP_BREAKPOINT_USE_TRAP_INSTRUCTION
        #define GP_TRAP_INSTRUCTION __asm__ volatile("int $0x03")
    #elif defined(__thumb__)
    	#define GP_BREAKPOINT_METHOD GP_BREAKPOINT_USE_TRAP_INSTRUCTION
    /* FIXME: handle __THUMB_INTERWORK__ */

    	/* Known problem:
    	 * After a breakpoint hit, can't 'stepi', 'step', or 'continue' in GDB.
    	 * 'step' would keep getting stuck on the same instruction.
    	 *
    	 * Workaround: use the new GDB commands 'debugbreak-step' and
    	 * 'debugbreak-continue' that become available
    	 * after you source the script from GDB:
    	 *
    	 * $ gdb -x debugbreak-gdb.py <... USUAL ARGUMENTS ...>
    	 *
    	 * 'debugbreak-step' would jump over the breakpoint instruction with
    	 * roughly equivalent of:
    	 * (gdb) set $instruction_len = 2
    	 * (gdb) tbreak *($pc + $instruction_len)
    	 * (gdb) jump   *($pc + $instruction_len)
    	 */

    	/* See 'arm-linux-tdep.c' in GDB source.
    	 * Both instruction sequences below work. */
        #if 1
    	    /* 'eabi_linux_thumb_le_breakpoint' */
    	    #define GP_TRAP_INSTRUCTION __asm__ volatile(".inst 0xde01")
        #else
    	    /* 'eabi_linux_thumb2_le_breakpoint' */
    	    #define GP_TRAP_INSTRUCTION __asm__ volatile(".inst.w 0xf7f0a000")
        #endif

    #elif defined(__arm__) && !defined(__thumb__)
    	#define GP_BREAKPOINT_METHOD GP_BREAKPOINT_USE_TRAP_INSTRUCTION
    	/* See 'arm-linux-tdep.c' in GDB source,
    	 * 'eabi_linux_arm_le_breakpoint' */
    	/* Known problem:
    	 * Same problem and workaround as Thumb mode */
        #define GP_TRAP_INSTRUCTION  volatile(".inst 0xe7f001f0")
    #elif (defined(__aarch64__) && defined(__APPLE__)
    	#define GP_BREAKPOINT_METHOD GP_BREAKPOINT_USE_BUILTIN_DEBUGTRAP
    #elif defined(__aarch64__)
    	#define GP_BREAKPOINT_METHOD GP_BREAKPOINT_USE_TRAP_INSTRUCTION
    	/* See 'aarch64-tdep.c' in GDB source,
    	 * 'aarch64_default_breakpoint' */
    	#define GP_TRAP_INSTRUCTION __asm__ volatile(".inst 0xd4200000")
    #elif defined(__powerpc__)
    	/* PPC 32 or 64-bit, big or little endian */
    	#define GP_BREAKPOINT_METHOD GP_BREAKPOINT_USE_TRAP_INSTRUCTION
    	/* See 'rs6000-tdep.c' in GDB source,
    	 * 'rs6000_breakpoint' */
    	/* Known problem:
    	 * After a breakpoint hit, can't 'stepi', 'step', or 'continue' in GDB.
    	 * 'step' stuck on the same instruction ("twge r2,r2").
    	 *
    	 * The workaround is the same as ARM Thumb mode: use debugbreak-gdb.py
    	 * or manually jump over the instruction. */
    	#define GP_TRAP_INSTRUCTION __asm__ volatile(".4byte 0x7d821008")
    #elif defined(__riscv)
    	/* RISC-V 32 or 64-bit, whether the "C" extension
    	 * for compressed, 16-bit instructions are supported or not */
    	#define GP_BREAKPOINT_METHOD GP_BREAKPOINT_USE_TRAP_INSTRUCTION
    	/* See 'riscv-tdep.c' in GDB source,
    	 * 'riscv_sw_breakpoint_from_kind' */
    	#define GP_TRAP_INSTRUCTION __asm__ volatile(".4byte 0x00100073")
    #else
        #define GP_BREAKPOINT_METHOD GP_BREAKPOINT_USE_BUILTIN_TRAP
    #endif

#else
#include <signal.h>
    #ifdef SIGTRAP
        #define GP_BREAKPOINT_METHOD GP_BREAKPOINT_USE_SIGTRAP
    #else
        #define GP_BREAKPOINT_METHOD GP_BREAKPOINT_NOT_AVAILABLE
    #endif
#endif // _MSC_VER

#if GP_BREAKPOINT_METHOD == GP_BREAKPOINT_USE_TRAP_INSTRUCTION
    #define GP_BREAKPOINT_IMPLEMENTATION      GP_TRAP_INSTRUCTION
    #define GP_BREAKPOINT_TRAP_IMPLEMENTATION GP_TRAP_INSTRUCTION
#elif GP_BREAKPOINT_METHOD == GP_BREAKPOINT_USE_BUILTIN_DEBUGTRAP
    #define GP_BREAKPOINT_IMPLEMENTATION      __builtin_debugtrap()
    #define GP_BREAKPOINT_TRAP_IMPLEMENTATION __builtin_debugtrap()
#elif GP_BREAKPOINT_METHOD == GP_BREAKPOINT_USE_BUILTIN_TRAP
    #define GP_BREAKPOINT_TRAP_IMPLEMENTATION __builtin_trap()
#elif GP_BREAKPOINT_METHOD == GP_BREAKPOINT_USE_SIGTRAP
#include <signal.h>
    #define GP_BREAKPOINT_TRAP_IMPLEMENTATION raise(SIGTRAP)
#elif GP_BREAKPOINT_METHOD == GP_BREAKPOINT_USE_DEBUGBREAK
    #define GP_BREAKPOINT_IMPLEMENTATION      __debugbreak()
    #define GP_BREAKPOINT_TRAP_IMPLEMENTATION __debugbreak()
#elif GP_BREAKPOINT_METHOD >= GP_BREAKPOINT_INVALID_METHOD
    #error Invalid GP_BREAKPOINT_METHOD definition
#endif

#if defined(GP_BREAKPOINT_IMPLEMENTATION) && !defined(NDEBUG)
    // expressionify, so can be used with comma operator, and preferably put the
    // breakpoint in user source code instead of this file.
    #if !__cplusplus && __GNUC__ && !defined(GP_PEDANTIC) && GP_BREAKPOINT_METHOD == GP_BREAKPOINT_USE_TRAP_INSTRUCTION
    #define GP_BREAKPOINT       ({GP_BREAKPOINT_IMPLEMENTATION;})
    #define GP_DEBUG_BREAKPOINT ({GP_BREAKPOINT_IMPLEMENTATION;})
    #elif GP_BREAKPOINT_METHOD == GP_BREAKPOINT_USE_TRAP_INSTRUCTION
    static inline void gp_breakpoint(void) { GP_BREAKPOINT_IMPLEMENTATION; }
    #define GP_BREAKPOINT       gp_breakpoint()
    #define GP_DEBUG_BREAKPOINT gp_breakpoint()
    #else
    #define GP_BREAKPOINT       GP_BREAKPOINT_IMPLEMENTATION
    #define GP_DEBUG_BREAKPOINT GP_BREAKPOINT_IMPLEMENTATION
    #endif // expressionify
#else
    #define GP_DEBUG_BREAKPOINT ((void)0)
#endif

#if defined(GP_BREAKPOINT_TRAP_IMPLEMENTATION) && !defined(NDEBUG)
    // expressionify, so can be used with comma operator, and preferably put the
    // breakpoint in user source code instead of this file.
    #if !__cplusplus && __GNUC__ && !defined(GP_PEDANTIC) && GP_BREAKPOINT_METHOD == GP_BREAKPOINT_USE_TRAP_INSTRUCTION
    #define GP_BREAKPOINT_TRAP       ({GP_BREAKPOINT_TRAP_IMPLEMENTATION;})
    #define GP_DEBUG_BREAKPOINT_TRAP ({GP_BREAKPOINT_TRAP_IMPLEMENTATION;})
    #elif GP_BREAKPOINT_METHOD == GP_BREAKPOINT_USE_TRAP_INSTRUCTION
    static inline void gp_breakpoint_trap(void) { GP_BREAKPOINT_TRAP_IMPLEMENTATION; }
    #define GP_BREAKPOINT_TRAP       gp_breakpoint_trap()
    #define GP_DEBUG_BREAKPOINT_TRAP gp_breakpoint_trap()
    #else
    #define GP_BREAKPOINT_TRAP       GP_BREAKPOINT_TRAP_IMPLEMENTATION
    #define GP_DEBUG_BREAKPOINT_TRAP GP_BREAKPOINT_TRAP_IMPLEMENTATION
    #endif // expressionify
#else
    #define GP_DEBUG_BREAKPOINT_TRAP ((void)0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* ifndef GP_BREAKPOINT_INCLUDED */
