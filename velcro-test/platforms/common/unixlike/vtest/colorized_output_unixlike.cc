#include <vcore/base.h>
#include <gtest/gtest.h>

namespace UnitTest
{
    namespace Platform
    {
        void EnableVirtualConsoleProcessingForStdout()
        {
            // UnixLike tty doesn't need ansi escapes enabled for them
        }

        bool TerminalSupportsColor()
        {
            const char* const term = testing::internal::posix::GetEnv("TERM");
            const bool term_supports_color = term && (
                vstricmp(term, "xterm") == 0 ||
                vstricmp(term, "xterm-color") == 0 ||
                vstricmp(term, "xterm-256color") == 0 ||
                vstricmp(term, "screen") == 0 ||
                vstricmp(term, "screen-256color") == 0 ||
                vstricmp(term, "tmux") == 0 ||
                vstricmp(term, "tmux-256color") == 0 ||
                vstricmp(term, "rxvt-unicode") == 0 ||
                vstricmp(term, "rxvt-unicode-256color") == 0 ||
                vstricmp(term, "linux") == 0 ||
                vstricmp(term, "cygwin") == 0
                );

            return term_supports_color;
        }
    }
}
