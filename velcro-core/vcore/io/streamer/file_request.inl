#include <limits>

namespace V::IO
{
    template<typename T> T* FileRequest::GetCommandFromChain()
    {
        FileRequest* current = this;
        while (current)
        {
            if (T* command = VStd::get_if<T>(&current->GetCommand()); command != nullptr)
            {
                return command;
            }
            current = current->m_parent;
        }
        return nullptr;
    }

    template<typename T> const T* FileRequest::GetCommandFromChain() const
    {
        const FileRequest* current = this;
        while (current)
        {
            if (const T* command = VStd::get_if<T>(&current->GetCommand()); command != nullptr)
            {
                return command;
            }
            current = current->m_parent;
        }
        return nullptr;
    }

    constexpr size_t FileRequest::GetMaxNumDependencies()
    {
        // Leave a bit of room for requests that don't check the available space for dependencies
        // because they only add a small (usually one or two) number of dependencies.
        return std::numeric_limits<decltype(m_dependencies)>::max() - 255;
    }
} // namespace V::IO