#ifndef V_FRAMEWORK_CORE_PLATFORM_RESTRICTED_FILE_DEFAULT_H
#define V_FRAMEWORK_CORE_PLATFORM_RESTRICTED_FILE_DEFAULT_H

// V_RESTRICTED_FILE 采用不带引号的字符串并将配置的平台目录和文件名前缀添加到前面.
// 例如, V_RESTRICTED_FILE(platform_default_h, V_RESTRICTED_PLATFORM) 将会被组装为 "ringo/platform_default_h_ringo.inl"
// 平台为 'ringo'. 这个文件结构是固定的,并要求一组特定于平台的文件夹位于调用受限文件机制的文件的当前目录中.
//
// 这个宏的结构非常微妙 - 它仅取决于预处理器的求值状态，并且不按照 C++ 字符串连接规则运行，因此在何处/何时/是否进行字符串化非常重要.
//

#define V_RESTRICTED_FILE_STR(f) #f
#define V_RESTRICTED_FILE_EVAL(f) V_RESTRICTED_FILE_STR(f)
#define V_RESTRICTED_FILE_PASTE3(a,b,c) a##b##c
#define V_RESTRICTED_FILE_EXPLICIT(f,p) V_RESTRICTED_FILE_EVAL(V_RESTRICTED_FILE_PASTE3(f,_,p).inl)
#define V_RESTRICTED_FILE(f) V_RESTRICTED_FILE_EXPLICIT(f,V_RESTRICTED_PLATFORM)


#endif // V_FRAMEWORK_CORE_PLATFORM_RESTRICTED_FILE_DEFAULT_H