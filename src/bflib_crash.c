/******************************************************************************/
// Bullfrog Engine Emulation Library - for use to remake classic games like
// Syndicate Wars, Magic Carpet or Dungeon Keeper.
/******************************************************************************/
/** @file bflib_crash.c
 *     Program failure handling system.
 * @par Purpose:
 *     Installs handlers to capture crashes; makes backtrace and clean exit.
 * @par Comment:
 *     None.
 * @author   Tomasz Lis
 * @date     09 Nov 2010 - 11 Nov 2010
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#include "pre_inc.h"
#include "bflib_crash.h"
#include <signal.h>
#include <stdarg.h>
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <excpt.h>
#include <imagehlp.h>
#include <dbghelp.h>
#include <psapi.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>

#include "bflib_basics.h"
#include "bflib_memory.h"
#include "bflib_video.h"
#include "post_inc.h"

/******************************************************************************/
static const char* sigstr(int s)
{
  switch(s)
    {
    case SIGINT : return "Interrupt (ANSI)";
    case SIGILL : return "Illegal instruction (ANSI)";
    case SIGABRT : return "Abort (ANSI)";
    case SIGFPE : return "Floating-point exception (ANSI)";
    case SIGSEGV : return "Segmentation violation (ANSI)";
    case SIGTERM : return "Termination (ANSI)";
#if defined(__linux__)
    case SIGHUP : return "Hangup (POSIX)";
    case SIGQUIT : return "Quit (POSIX)";
    case SIGTRAP : return "Trace trap (POSIX)";
    case SIGBUS : return "BUS error (4.2 BSD)";
    case SIGKILL : return "Kill, unblockable (POSIX)";
    case SIGUSR1 : return "User-defined signal 1 (POSIX)";
    case SIGUSR2 : return "User-defined signal 2 (POSIX)";
    case SIGPIPE : return "Broken pipe (POSIX)";
    case SIGALRM : return "Alarm clock (POSIX)";
    case SIGCHLD : return "Child status has changed (POSIX)";
    case SIGCONT : return "Continue (POSIX)";
    case SIGSTOP : return "Stop, unblockable (POSIX)";
    case SIGTSTP : return "Keyboard stop (POSIX)";
    case SIGTTIN : return "Background read from tty (POSIX)";
    case SIGTTOU : return "Background write to tty (POSIX)";
    case SIGURG : return "Urgent condition on socket (4.2 BSD)";
    case SIGXCPU : return "CPU limit exceeded (4.2 BSD)";
    case SIGXFSZ : return "File size limit exceeded (4.2 BSD)";
    case SIGVTALRM : return "Virtual alarm clock (4.2 BSD)";
    case SIGPROF : return "Profiling alarm clock (4.2 BSD)";
    case SIGWINCH : return "Window size change (4.3 BSD, Sun)";
    case SIGIO : return "I/O now possible (4.2 BSD)";
    case SIGSYS : return "Bad system call";
    case SIGSTKFLT : return "Stack fault";
    case SIGPWR : return "Power failure restart (System V)";
#else
    case SIGBREAK : return "Ctrl-Break (Win32)";
#endif
    }
  return "unknown signal";
}

void exit_handler(void)
{
    LbErrorLog("Application exit called.\n");
}

void ctrl_handler(int sig_id)
{
    signal(sig_id, SIG_DFL);
    LbErrorLog("Failure signal: %s.\n",sigstr(sig_id));
    LbScreenReset();
    LbErrorLogClose();
    raise(sig_id);
}

static void
_backtrace(int depth , LPCONTEXT context)
{
    DWORD64 keeperFxBaseAddr = 0x00000000;

    char mapFileLine[512];
    FILE *mapFile = fopen("keeperfx.map", "r");
    if (mapFile)
    {
        // Get base address from map file
        while (fgets(mapFileLine, sizeof(mapFileLine), mapFile) != NULL)
        {
            if (sscanf(mapFileLine, " %*x __image_base__ = %llx", &keeperFxBaseAddr) == 1)
            {
                SYNCDBG(0, "KeeperFX base address in map file: %llx\n", keeperFxBaseAddr);
                break;
            }
        }

        memset(mapFileLine, 0, sizeof(mapFileLine));

        if(keeperFxBaseAddr != 0x00000000)
        {
            fseek(mapFile, 0, SEEK_SET);
        }
        else
        {
            fclose(mapFile);
        }
    }
    else
    {
        LbWarnLog("No keeperfx.map file found for stacktrace map lookups\n");
    }

    STACKFRAME frame;
    LbMemorySet(&frame,0,sizeof(frame));

    frame.AddrPC.Offset = context->Eip;
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrStack.Offset = context->Esp;
    frame.AddrStack.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = context->Ebp;
    frame.AddrFrame.Mode = AddrModeFlat;

    HANDLE process = GetCurrentProcess();
    HANDLE thread = GetCurrentThread();

    // Loop through all traces in the stack
    while (StackWalk(IMAGE_FILE_MACHINE_I386, process, thread, &frame, context, 0, SymFunctionTableAccess, SymGetModuleBase, 0))
    {
        --depth;
        if (depth < 0)
        {
            break;
        }

        // Get the base address in the module
        // This is where the address space of the functions start
        DWORD module_base = SymGetModuleBase(process, frame.AddrPC.Offset);

        // Get the name of the module
        // The module will be the keeperfx bin or a library
        const char * module_name = "[unknown module]";
        char module_name_raw[MAX_PATH];
        if (module_base && GetModuleFileNameA((HINSTANCE)module_base, module_name_raw, MAX_PATH))
        {
            module_name = strrchr(module_name_raw,'\\');
            if (module_name != NULL)
            {
                module_name++;
            }
            else
            {
                module_name = module_name_raw;
            }
        }

        // Symbol information for looking up symbols
        char symbol_info[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
        PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)symbol_info;
        pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        pSymbol->MaxNameLen = MAX_SYM_NAME;

        // The distance between the original function and the call in the trace
        DWORD64 displacement;

        // First check if we can find the symbol by its address
        // This works if there are any debug symbols available and also most OS libraries
        if (SymFromAddr(process, frame.AddrPC.Offset, &displacement, pSymbol))
        {
            LbJustLog("[#%-2d]  in %14-s : %-40s [%04x:%08x+0x%llx, base %08x] (symbol lookup)\n",
                      depth, module_name, pSymbol->Name, context->SegCs, frame.AddrPC.Offset, displacement, module_base);
            
            continue;
        }
        else
        {
            // Look up using the keeperfx.map file
            if(mapFile){

                DWORD64 checkAddr = frame.AddrPC.Offset - (module_base - keeperFxBaseAddr);

                bool addrFound = false;
                DWORD64 prevAddr = 0x00000000;
                char prevName[512];

                // Loop through all lines in the mapFile
                // This should be pretty fast on modern systems
                while (fgets(mapFileLine, sizeof(mapFileLine), mapFile) != NULL)
                {

                    DWORD64 addr;
                    char name[512];
                    if (
                        sscanf(mapFileLine, "%llx %[^\t\n]", &addr, name) == 2 ||
                        sscanf(mapFileLine, " .text %llx %[^\t\n]", &addr, name) == 2
                    ) {
                        // The offsets in our trace do not point to the start of the function.
                        // However, only the address of the start of our functions is stored in the map file.
                        // So we'll trace back to the last address.
                        if (checkAddr > prevAddr && checkAddr < addr)
                        {
                            displacement = checkAddr - prevAddr;

                            LbJustLog(
                                "[#%-2d]  in %14-s : %-40s [0x%llx+0x%llx] \t(map lookup for %04x:%08x, base: %08x)\n",
                                depth, module_name, prevName, prevAddr, displacement, context->SegCs, frame.AddrPC.Offset, module_base);

                            addrFound = true;

                            break;
                        }
                    }

                    prevAddr = addr;
                    strcpy(prevName, name);
                }

                // Reset buffers
                fseek(mapFile, 0, SEEK_SET);
                memset(mapFileLine, 0, sizeof(mapFileLine));

                if(addrFound)
                {
                    continue;
                }
            }
        }

        // Fallback
        LbJustLog("[#%-2d]  in %13-s at %04x:%08x, base %08x\n", depth, module_name, context->SegCs, frame.AddrPC.Offset, module_base);
    }

    if(mapFile){
        fclose(mapFile);
    }
}

static LONG CALLBACK ctrl_handler_w32(LPEXCEPTION_POINTERS info)
{
    switch (info->ExceptionRecord->ExceptionCode) {
    case EXCEPTION_ACCESS_VIOLATION:
        switch (info->ExceptionRecord->ExceptionInformation[0])
        {
        case 0:
            LbErrorLog("Attempt to read from inaccessible memory address.\n");
            break;
        case 1:
            LbErrorLog("Attempt to write to inaccessible memory address.\n");
            break;
        case 8:
            LbErrorLog("User-mode data execution prevention (DEP) violation.\n");
            break;
        default:
            LbErrorLog("Memory access violation, code %d.\n",(int)info->ExceptionRecord->ExceptionInformation[0]);
            break;
        }
        break;
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
        LbErrorLog("Attempt of integer division by zero.\n");
        break;
    default:
        LbErrorLog("Failure code %x received.\n",info->ExceptionRecord->ExceptionCode);
        break;
    }
    if (!SymInitialize(GetCurrentProcess(), 0, TRUE)) {
        LbErrorLog("Failed to init symbol context\n");
    }
    else {
            _backtrace(16 , info->ContextRecord);
            SymCleanup(GetCurrentProcess());
    }
    LbScreenReset();
    LbErrorLogClose();
    return EXCEPTION_EXECUTE_HANDLER;
}

void LbErrorParachuteInstall(void)
{
    signal(SIGINT,ctrl_handler);
    signal(SIGILL,ctrl_handler);
    signal(SIGABRT,ctrl_handler);
    signal(SIGFPE,ctrl_handler);
    signal(SIGSEGV,ctrl_handler);
    signal(SIGTERM,ctrl_handler);
#if defined(__linux__)
    signal(SIGHUP,ctrl_handler);
    signal(SIGQUIT,ctrl_handler);
    signal(SIGSYS,ctrl_handler);
#else
    signal(SIGBREAK,ctrl_handler);
#endif
    atexit(exit_handler);
    SetUnhandledExceptionFilter(ctrl_handler_w32);
}

void LbErrorParachuteUpdate(void)
{
    SetUnhandledExceptionFilter(ctrl_handler_w32);
}
/******************************************************************************/
