//  Copyright (c) 2020 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#if defined(DOXYGEN)
namespace hpx { namespace functional {
    inline namespace unspecified {
        /// The `hpx::functional::tag_dispatch` name defines a constexpr object
        /// that is invocable with one or more arguments. The first argument
        /// is a 'tag' (typically a DPO). It is only invocable if an overload
        /// of tag_dispatch() that accepts the same arguments could be found via
        /// ADL.
        ///
        /// The evaluation of the expression `hpx::tag_dispatch(tag, args...)` is
        /// equivalent to evaluating the unqualified call to
        /// `tag_dispatch(decay-copy(tag), std::forward<Args>(args)...)`.
        ///
        /// `hpx::functional::tag_dispatch` is implemented against P1895.
        ///
        /// Example:
        /// Defining a new customization point `foo`:
        /// ```
        /// namespace mylib {
        ///     inline constexpr
        ///         struct foo_fn final : hpx::functional::tag<foo_fn>
        ///         {
        ///         } foo{};
        /// }
        /// ```
        ///
        /// Defining an object `bar` which customizes `foo`:
        /// ```
        /// struct bar
        /// {
        ///     int x = 42;
        ///
        ///     friend constexpr int tag_dispatch(mylib::foo_fn, bar const& x)
        ///     {
        ///         return b.x;
        ///     }
        /// };
        /// ```
        ///
        /// Using the customization point:
        /// ```
        /// static_assert(42 == mylib::foo(bar{}), "The answer is 42");
        /// ```
        inline constexpr unspecified tag_dispatch = unspecified;
    }    // namespace unspecified

    /// `hpx::functional::is_tag_dispatchable<Tag, Args...>` is std::true_type if
    /// an overload of `tag_dispatch(tag, args...)` can be found via ADL.
    template <typename Tag, typename... Args>
    struct is_tag_dispatchable;

    /// `hpx::functional::is_tag_dispatchable_v<Tag, Args...>` evaluates to
    /// `hpx::functional::is_tag_dispatchable<Tag, Args...>::value`
    template <typename Tag, typename... Args>
    constexpr bool is_tag_dispatchable_v =
        is_tag_dispatchable<Tag, Args...>::value;

    /// `hpx::functional::is_nothrow_tag_dispatchable<Tag, Args...>` is
    /// std::true_type if an overload of `tag_dispatch(tag, args...)` can be
    /// found via ADL and is noexcept.
    template <typename Tag, typename... Args>
    struct is_nothrow_tag_dispatchable;

    /// `hpx::functional::is_tag_dispatchable_v<Tag, Args...>` evaluates to
    /// `hpx::functional::is_tag_dispatchable<Tag, Args...>::value`
    template <typename Tag, typename... Args>
    constexpr bool is_nothrow_tag_dispatchable_v =
        is_nothrow_tag_dispatchable<Tag, Args...>::value;

    /// `hpx::functional::tag_dispatch_result<Tag, Args...>` is the trait
    /// returning the result type of the call hpx::functional::tag_dispatch. This
    /// can be used in a SFINAE context.
    template <typename Tag, typename... Args>
    using tag_dispatch_result =
        invoke_result<decltype(tag_dispatch), Tag, Args...>;

    /// `hpx::functional::tag_dispatch_result_t<Tag, Args...>` evaluates to
    /// `hpx::functional::tag_dispatch_result_t<Tag, Args...>::type`
    template <typename Tag, typename... Args>
    using tag_dispatch_result_t =
        typename tag_dispatch_result<Tag, Args...>::type;

    /// `hpx::functional::tag<Tag>` defines a base class that implements
    /// the necessary tag dispatching functionality for a given type `Tag`
    template <typename Tag>
    struct tag;

    /// `hpx::functional::tag_noexcept<Tag>` defines a base class that implements
    /// the necessary tag dispatching functionality for a given type `Tag`
    /// The implementation has to be noexcept
    template <typename Tag>
    struct tag_noexcept;
}}    // namespace hpx::functional
#else

#include <hpx/config.hpp>
#include <hpx/functional/invoke_result.hpp>
#include <hpx/functional/traits/is_invocable.hpp>

#include <type_traits>
#include <utility>

namespace hpx { namespace functional {

    template <auto& Tag>
    using tag_t = typename std::decay<decltype(Tag)>::type;

    namespace tag_dispatch_t_ns {

        // poison pill
        void tag_dispatch();

        struct tag_dispatch_t
        {
            template <typename Tag, typename... Ts>
            HPX_HOST_DEVICE HPX_FORCEINLINE constexpr auto operator()(
                Tag tag, Ts&&... ts) const
                noexcept(noexcept(
                    tag_dispatch(std::declval<Tag>(), std::forward<Ts>(ts)...)))
                    -> decltype(tag_dispatch(
                        std::declval<Tag>(), std::forward<Ts>(ts)...))
            {
                return tag_dispatch(tag, std::forward<Ts>(ts)...);
            }

            friend constexpr bool operator==(tag_dispatch_t, tag_dispatch_t)
            {
                return true;
            }

            friend constexpr bool operator!=(tag_dispatch_t, tag_dispatch_t)
            {
                return false;
            }
        };
    }    // namespace tag_dispatch_t_ns

    namespace tag_dispatch_ns {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        HPX_INLINE_CONSTEXPR_VARIABLE
        tag_dispatch_t_ns::tag_dispatch_t tag_dispatch = {};
#else
        HPX_DEVICE static tag_dispatch_t_ns::tag_dispatch_t const tag_dispatch =
            {};
#endif
    }    // namespace tag_dispatch_ns

    ///////////////////////////////////////////////////////////////////////////
    template <typename Tag, typename... Args>
    using is_tag_dispatchable =
        hpx::is_invocable<decltype(tag_dispatch_ns::tag_dispatch), Tag,
            Args...>;

    template <typename Tag, typename... Args>
    HPX_INLINE_CONSTEXPR_VARIABLE bool is_tag_dispatchable_v =
        is_tag_dispatchable<Tag, Args...>::value;

    namespace detail {
        template <typename Sig, bool Dispatchable>
        struct is_nothrow_tag_dispatchable_impl;

        template <typename Sig>
        struct is_nothrow_tag_dispatchable_impl<Sig, false> : std::false_type
        {
        };

        template <typename Tag, typename... Args>
        struct is_nothrow_tag_dispatchable_impl<
            decltype(tag_dispatch_ns::tag_dispatch)(Tag, Args...), true>
          : std::integral_constant<bool,
                noexcept(tag_dispatch_ns::tag_dispatch(
                    std::declval<Tag>(), std::declval<Args>()...))>
        {
        };
    }    // namespace detail

    // CUDA versions less than 11.2 have a template instantiation bug which
    // leaves out certain template arguments and leads to us not being able to
    // correctly check this condition. We default to the more relaxed
    // noexcept(true) to not falsely exclude correct overloads. However, this
    // may lead to noexcept(false) overloads falsely being candidates.
#if !defined(HPX_CUDA_VERSION) || (HPX_CUDA_VERSION >= 1102)
    template <typename Tag, typename... Args>
    struct is_nothrow_tag_dispatchable
      : detail::is_nothrow_tag_dispatchable_impl<
            decltype(tag_dispatch_ns::tag_dispatch)(Tag, Args...),
            is_tag_dispatchable_v<Tag, Args...>>
    {
    };
#else
    template <typename Tag, typename... Args>
    struct is_nothrow_tag_dispatchable : std::true_type
    {
    };
#endif

    template <typename Tag, typename... Args>
    HPX_INLINE_CONSTEXPR_VARIABLE bool is_nothrow_tag_dispatchable_v =
        is_nothrow_tag_dispatchable<Tag, Args...>::value;

    template <typename Tag, typename... Args>
    using tag_dispatch_result =
        hpx::util::invoke_result<decltype(tag_dispatch_ns::tag_dispatch), Tag,
            Args...>;

    template <typename Tag, typename... Args>
    using tag_dispatch_result_t =
        typename tag_dispatch_result<Tag, Args...>::type;

    ///////////////////////////////////////////////////////////////////////////////
    namespace tag_base_ns {

        // poison pill
        void tag_dispatch();

        ///////////////////////////////////////////////////////////////////////////
        // helper base class implementing the tag_dispatch logic for DPOs
        template <typename Tag>
        struct tag
        {
            template <typename... Args>
            HPX_HOST_DEVICE HPX_FORCEINLINE constexpr auto operator()(
                Args&&... args) const
                noexcept(is_nothrow_tag_dispatchable_v<Tag, Args...>)
                    -> tag_dispatch_result_t<Tag, Args...>
            {
                return tag_dispatch(static_cast<Tag const&>(*this),
                    std::forward<Args>(args)...);
            }
        };

        template <typename Tag>
        struct tag_noexcept
        {
            template <typename... Args,
                typename Enable = std::enable_if_t<
                    is_nothrow_tag_dispatchable_v<Tag, Args...>>>
            HPX_HOST_DEVICE HPX_FORCEINLINE constexpr auto operator()(
                Args&&... args) const noexcept
                -> tag_dispatch_result_t<Tag, decltype(args)...>
            {
                return tag_dispatch(static_cast<Tag const&>(*this),
                    std::forward<Args>(args)...);
            }
        };
    }    // namespace tag_base_ns

    inline namespace tag_dispatch_base_ns {

        template <typename Tag>
        using tag = tag_base_ns::tag<Tag>;

        template <typename Tag>
        using tag_noexcept = tag_base_ns::tag_noexcept<Tag>;
    }    // namespace tag_dispatch_base_ns

    inline namespace tag_dispatch_f_ns {

        using tag_dispatch_ns::tag_dispatch;
    }    // namespace tag_dispatch_f_ns
}}       // namespace hpx::functional

#endif
