#ifndef V_FRAMEWORK_CORE_IO_STREAMER_REQUEST_PATH_H
#define V_FRAMEWORK_CORE_IO_STREAMER_REQUEST_PATH_H


#include <numeric>
#include <vcore/base.h>
#include <vcore/std/string/string.h>

namespace V
{
    namespace IO
    {
        //! The path to the file used by a request.
        //! RequestPath uses lazy evaluation so the relative path is
        //! not resolved to an absolute path until the path is requested
        //! or a check is done for validity.
        class RequestPath
        {
        public:
            RequestPath() = default;
            RequestPath(const RequestPath& rhs);
            RequestPath(RequestPath&& rhs);

            RequestPath& operator=(const RequestPath& rhs);
            RequestPath& operator=(RequestPath&& rhs);

            bool operator==(const RequestPath& rhs) const;
            bool operator!=(const RequestPath& rhs) const;

            void InitFromRelativePath(VStd::string path);
            void InitFromAbsolutePath(VStd::string path);

            bool IsValid() const;
            void Clear();

            const char* GetAbsolutePath() const;
            const char* GetRelativePath() const;
            size_t GetHash() const;

        private:
            constexpr static const size_t s_invalidPathHash = std::numeric_limits<size_t>::max();
            constexpr static const size_t s_emptyPathHash = std::numeric_limits<size_t>::min();

            void ResolvePath() const;
            size_t FindAliasOffset(const VStd::string& path) const;

            mutable VStd::string m_path;
            mutable size_t m_absolutePathHash = s_emptyPathHash;
            mutable size_t m_relativePathOffset = 0;
        };
    }
}

namespace VStd
{
    template<>
    struct hash<V::IO::RequestPath>
    {
        using argument_type = V::IO::RequestPath;
        using result_type = VStd::size_t;
        inline result_type operator()(const argument_type& value) const { return value.GetHash(); }
    };
} // namespace VStd

#endif // V_FRAMEWORK_CORE_IO_STREAMER_REQUEST_PATH_H