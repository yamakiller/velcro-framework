
#include <core/std/containers/array.h>
#include <core/io/path/path_parser.inl>

namespace V::IO {
    struct PathView::PathIterable {
        inline static constexpr size_t MaxPathParts = 64;
        using PartKindPair = VStd::pair<VStd::string_view, V::IO::parser::PathPartKind>;
        using PartKindArray = VStd::array<PartKindPair, MaxPathParts>;
        constexpr PathIterable() = default;

        [[nodiscard]] constexpr bool empty() const noexcept;
        constexpr auto size() const noexcept-> size_t;
        constexpr auto begin() noexcept-> PartKindArray::iterator;
        constexpr auto begin() const noexcept -> PartKindArray::const_iterator;
        constexpr auto cbegin() const noexcept -> PartKindArray::const_iterator;
        constexpr auto end() noexcept ->  PartKindArray::iterator;
        constexpr auto end() const noexcept -> PartKindArray::const_iterator;
        constexpr auto cend() const noexcept -> PartKindArray::const_iterator;
        constexpr auto rbegin() noexcept -> PartKindArray::reverse_iterator;
        constexpr auto rbegin() const noexcept -> PartKindArray::const_reverse_iterator;
        constexpr auto crbegin() const noexcept -> PartKindArray::const_reverse_iterator;
        constexpr auto rend() noexcept -> PartKindArray::reverse_iterator;
        constexpr auto rend() const noexcept -> PartKindArray::const_reverse_iterator;
        constexpr auto crend() const noexcept -> PartKindArray::const_reverse_iterator;

        [[nodiscard]] constexpr bool IsAbsolute() const noexcept;

    private:
        template <typename... Args>
        constexpr PartKindPair& emplace_back(Args&&... args) noexcept;
        constexpr void pop_back() noexcept;
        constexpr const PartKindPair& back() const noexcept;
        constexpr PartKindPair& back() noexcept;

        constexpr const PartKindPair& front() const noexcept;
        constexpr PartKindPair& front() noexcept;

        constexpr void clear() noexcept;

        friend constexpr auto PathView::AppendNormalPathParts(PathIterable& pathIterable, const V::IO::PathView&) noexcept -> void;
        friend constexpr auto PathView::MakeRelativeTo(PathIterable& pathIterable, const V::IO::PathView&, const V::IO::PathView&) noexcept -> void;
        PartKindArray m_parts{};
        size_t m_size{};
    };

    // public
    [[nodiscard]] constexpr auto PathView::PathIterable::empty() const noexcept -> bool
    {
        return m_size == 0;
    }
    constexpr auto PathView::PathIterable::size() const noexcept -> size_t
    {
        return m_size;
    }

    constexpr auto PathView::PathIterable::begin() noexcept -> PartKindArray::iterator
    {
        return m_parts.begin();
    }
    constexpr auto PathView::PathIterable::begin() const noexcept -> PartKindArray::const_iterator
    {
        return m_parts.begin();
    }
    constexpr auto PathView::PathIterable::cbegin() const noexcept -> PartKindArray::const_iterator
    {
        return begin();
    }
    constexpr auto PathView::PathIterable::end() noexcept -> PartKindArray::iterator
    {
        return begin() + size();
    }
    constexpr auto PathView::PathIterable::end() const noexcept -> PartKindArray::const_iterator
    {
        return begin() + size();
    }
    constexpr auto PathView::PathIterable::cend() const noexcept -> PartKindArray::const_iterator
    {
        return end();
    }
    constexpr auto PathView::PathIterable::rbegin() noexcept -> PartKindArray::reverse_iterator
    {
        return PartKindArray::reverse_iterator(begin() + size());
    }
    constexpr auto PathView::PathIterable::rbegin() const noexcept -> PartKindArray::const_reverse_iterator
    {
        return PartKindArray::const_reverse_iterator(begin() + size());
    }
    constexpr auto PathView::PathIterable::crbegin() const noexcept -> PartKindArray::const_reverse_iterator
    {
        return rbegin();
    }
    constexpr auto PathView::PathIterable::rend() noexcept -> PartKindArray::reverse_iterator
    {
        return PartKindArray::reverse_iterator(begin());
    }
    constexpr auto PathView::PathIterable::rend() const noexcept -> PartKindArray::const_reverse_iterator
    {
        return PartKindArray::const_reverse_iterator(begin());
    }
    constexpr auto PathView::PathIterable::crend() const noexcept -> PartKindArray::const_reverse_iterator
    {
        return rend();
    }

    [[nodiscard]] constexpr auto PathView::PathIterable::IsAbsolute() const noexcept -> bool
    {
        return !empty() && (front().second == parser::PathPartKind::PK_RootSep
            || (size() > 1 && front().second == parser::PathPartKind::PK_RootName && m_parts[1].second == parser::PathPartKind::PK_RootSep));
    }

    // private
    template <typename... Args>
    constexpr auto PathView::PathIterable::emplace_back(Args&&... args) noexcept -> PartKindPair&
    {
        V_Assert(m_size < MaxPathParts, "PathIterable cannot be made out of a path with more than %zu parts", MaxPathParts);
        m_parts[m_size++] = PartKindPair{ VStd::forward<Args>(args)... };
        return back();
    }
    constexpr auto PathView::PathIterable::pop_back() noexcept -> void
    {
        V_Assert(m_size > 0, "Cannot pop_back() from a PathIterable with 0 parts");
        --m_size;
    }
    constexpr auto PathView::PathIterable::back() const noexcept -> const PartKindPair&
    {
        V_Assert(!empty(), "back() was invoked on PathIterable with 0 parts");
        return m_parts[m_size - 1];
    }
    constexpr auto PathView::PathIterable::back() noexcept -> PartKindPair&
    {
        V_Assert(!empty(), "back() was invoked on PathIterable with 0 parts");
        return m_parts[m_size - 1];
    }

    constexpr auto PathView::PathIterable::front() const noexcept -> const PartKindPair&
    {
        V_Assert(!empty(), "front() was invoked on PathIterable with 0 parts");
        return m_parts[0];
    }
    constexpr auto PathView::PathIterable::front() noexcept -> PartKindPair&
    {
        V_Assert(!empty(), "front() was invoked on PathIterable with 0 parts");
        return m_parts[0];
    }

    constexpr auto PathView::PathIterable::clear() noexcept -> void
    {
        m_size = 0;
    }
}