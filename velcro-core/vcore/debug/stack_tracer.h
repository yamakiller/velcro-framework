/*
 * Copyright (c) Contributors to the VelcroFramework.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * 
 */
#ifndef V_FRAMEWORK_CORE_DEBUG_STACK_TRACER_H
#define V_FRAMEWORK_CORE_DEBUG_STACK_TRACER_H

#include <vcore/base.h>
#include <vcore/vcore_traits_platform.h>

namespace V {
    namespace Debug {
        struct StackFrame {
            StackFrame(): ProgramCounter(0)  {}
            V_FORCE_INLINE bool IsValid() const { return ProgramCounter != 0; }

            uintptr_t  ProgramCounter;       ///< Currently we store and use only the program counter.
        };

        class StackRecorder {
        public:
            /**
            * 记录当前的调用栈帧 (当前进程、当前线程).
            * \param frames 用于存储堆栈的帧数组.
            * \param suppressCount 这指定要隐藏堆栈的层数. 
            *        默认为 0,这只会隐藏该函数本身.
            * \param nativeThread 指向线程本机类型的指针，用于捕获当前运行堆栈以外的堆栈
            */
            static unsigned int Record(StackFrame* frames, unsigned int maxNumOfFrames, unsigned int suppressCount = 0, void* nativeThread = 0);
        };

        class SymbolStorage
        {
        public:
            // 模组数据头信息
            struct ModuleDataInfoHeader {
                // We use chars so we don't need to care about big-little endian.
                char    PlatformId;
                char    NumModules;
            };
            /// 模组信息
            struct ModuleInfo {
                char    ModName[256];
                char    FileName[1024];
                u64     FileVersion;
                u64     BaseAddress;
                u32     Size;
            };

            typedef char StackLine[256];

            /**
             * 用于加载在不同系统捕获的模块信息数据.
             */
            /// Load module data symbols (deprecated platform export)
            static void         LoadModuleData(const void* moduleInfoData, unsigned int moduleInfoDataSize);

            /// Return pointer to the data with module information. Data is platform dependent
            static void         StoreModuleInfoData(void* data, unsigned int dataSize);

            /// Return number of loaded modules.
            static unsigned int GetNumLoadedModules();
            /// Return information for a loaded module.
            static const ModuleInfo*    GetModuleInfo(unsigned int moduleIndex);

            /// Set/Get map filename or symbol path, used to decode frames or consoles or when no other symbol information is available.
            static void         SetMapFilename(const char* fileName);
            static const char*  GetMapFilename();

            /// Registers listeners for dynamically loaded modules used for loading correct symbols for SymbolStorage
            static void         RegisterModuleListeners();
            static void         UnregisterModuleListeners();

            /**
             * 将帧解码为可读的文本行.
             * IMPORTANT: textLines 应指向至少 numFrames 长的 StackLine 数组.
             */
            static void     DecodeFrames(const StackFrame* frames, unsigned int numFrames, StackLine* textLines);

            static void     FindFunctionFromIP(void* address, StackLine* func, StackLine* file, StackLine* module, int& line, void*& baseAddr);
        };
    }
}

#endif // V_FRAMEWORK_CORE_DEBUG_STACK_TRACER_H