// This file is part of Search++.
// Copyright 2026 by by Randy Fellmy <https://www.coises.com/>.

// The source code contained in this file is independent of Notepad++ code.
// It is released under the MIT (Expat) license:
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and 
// associated documentation files (the "Software"), to deal in the Software without restriction, 
// including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all copies or substantial 
// portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT 
// LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


// =================================================================================================
// THE CODE IN THIS FILE USES TEMPLATE SPECIALIZATION TO REPLACE SOME INTERNAL BOOST::REGEX ROUTINES
// THESE ARE NOT DOCUMENTED INTERFACES AND THIS CODE MAY FAIL IF BOOST::REGEX IS UPDATED
// =================================================================================================


// Make the \X escape work properly for Unicode grapheme segmentation
// reference: https://unicode.org/reports/tr29/#Grapheme_Cluster_Boundaries

enum class PairClusters {yes, no, maybe};

inline PairClusters pairCluster(const char32_t c1, const char32_t c2) {
    using enum PairClusters;
    if (c1 < 128 && c2 < 128) return c1 == '\r' && c2 == '\n' ? yes : no;
    const auto gb1 = u_getIntPropertyValue(c1, UCHAR_GRAPHEME_CLUSTER_BREAK);
    const auto gb2 = u_getIntPropertyValue(c2, UCHAR_GRAPHEME_CLUSTER_BREAK);
    if (gb1 == U_GCB_CR || gb1 == U_GCB_LF || gb1 == U_GCB_CONTROL) return no;
    if (gb2 == U_GCB_CR || gb2 == U_GCB_LF || gb2 == U_GCB_CONTROL) return no;
    if ((gb1 == U_GCB_L && (gb2 == U_GCB_L || gb2 == U_GCB_V || gb2 == U_GCB_LV || gb2 == U_GCB_LVT))
        || ((gb1 == U_GCB_LV || gb1 == U_GCB_V) && (gb2 == U_GCB_V || gb2 == U_GCB_T))
        || ((gb1 == U_GCB_LVT || gb1 == U_GCB_T) && gb2 == U_GCB_T))
        return yes;
    if (gb2 == U_GCB_EXTEND || gb2 == U_GCB_ZWJ) return yes;
    if (gb1 == U_GCB_PREPEND || gb2 == U_GCB_SPACING_MARK) return yes;
    if (gb1 == U_GCB_REGIONAL_INDICATOR && gb2 == U_GCB_REGIONAL_INDICATOR) return maybe;
    if (gb1 == U_GCB_ZWJ && u_hasBinaryProperty(c2, UCHAR_EXTENDED_PICTOGRAPHIC)) return maybe;
    const auto ib1 = u_getIntPropertyValue(c1, UCHAR_INDIC_CONJUNCT_BREAK);
    const auto ib2 = u_getIntPropertyValue(c2, UCHAR_INDIC_CONJUNCT_BREAK);
    if ((ib1 == U_INCB_EXTEND || ib1 == U_INCB_LINKER) && ib2 == U_INCB_CONSONANT) return maybe;
    return no;
}

template<typename DocumentIterator> bool multipleCluster(const DocumentIterator& position, const DocumentIterator& backstop) {

    const char32_t c = *position;

    if (u_getIntPropertyValue(c, UCHAR_GRAPHEME_CLUSTER_BREAK) == U_GCB_REGIONAL_INDICATOR) {
        auto p = position;
        size_t count = 0;
        while (p != backstop && u_getIntPropertyValue(*--p, UCHAR_GRAPHEME_CLUSTER_BREAK) == U_GCB_REGIONAL_INDICATOR) ++count;
        return count & 1;
    }

    if (u_hasBinaryProperty(c, UCHAR_EXTENDED_PICTOGRAPHIC)) {
        auto p = position;
        if (p == backstop || *--p != 0x200D) return false;
        while (p != backstop && u_getIntPropertyValue(*--p, UCHAR_GRAPHEME_CLUSTER_BREAK) == U_GCB_EXTEND);
        return u_hasBinaryProperty(*p, UCHAR_EXTENDED_PICTOGRAPHIC);
    }

    if (u_getIntPropertyValue(c, UCHAR_INDIC_CONJUNCT_BREAK) == U_INCB_CONSONANT) {
        bool foundLinker = false;
        auto p = position;
        while (p != backstop) {
            --p;
            switch (u_getIntPropertyValue(*p, UCHAR_INDIC_CONJUNCT_BREAK)) {
            case U_INCB_CONSONANT :
                return foundLinker;
            case U_INCB_EXTEND:
                break;
            case U_INCB_LINKER:
                foundLinker = true;
                break;
            case U_INCB_NONE:
                return false;
            }
        }
        return false;
    }

    return false;

}

template<typename DocumentIterator>
bool implement_match_combining(DocumentIterator& position, const DocumentIterator& last, const DocumentIterator& backstop) {

    if (position == last) return false;

    char32_t c1 = *position;

    if (position != backstop) /* we could be within a grapheme; if so, we must return false */ {
        auto prior = position;
        --prior;
        char32_t c0 = *prior;
        PairClusters pc01 = pairCluster(c0, c1);
        if (pc01 == PairClusters::yes) return false;
        if (pc01 == PairClusters::maybe && multipleCluster(position, backstop)) return false;
    }

    for (char32_t c2; ++position != last; c1 = c2) {
        c2 = *position;
        PairClusters pc12 = pairCluster(c1, c2);
        if (pc12 == PairClusters::no) break;
        if (pc12 == PairClusters::yes) continue;
        if (!multipleCluster(position, backstop)) break;
    }

    return true;

}

template <>
bool boost::BOOST_REGEX_DETAIL_NS::perl_matcher<RegularExpressionU::DocumentIterator,
    std::allocator<boost::sub_match<RegularExpressionU::DocumentIterator>>, utf32_regex_traits>::match_combining() {
    if (!implement_match_combining(position, last, backstop)) return false;
    pstate = pstate->next.p;
    return true;
}

template <>
bool boost::BOOST_REGEX_DETAIL_NS::perl_matcher<RegularExpressionDBCS::DocumentIterator,
    std::allocator<boost::sub_match<RegularExpressionDBCS::DocumentIterator>>, utf32_regex_traits>::match_combining() {
    if (!implement_match_combining(position, last, backstop)) return false;
    pstate = pstate->next.p;
    return true;
}

// Single-byte character sets do not contain combining characters

template <>
bool boost::BOOST_REGEX_DETAIL_NS::perl_matcher<RegularExpressionSBCS::DocumentIterator,
    std::allocator<boost::sub_match<RegularExpressionSBCS::DocumentIterator>>, utf32_regex_traits>::match_combining() {
    if (position == last) return false;
    ++position;
    pstate = pstate->next.p;
    return true;
}


// This is the logic used to test for matches to [...] and \p{...} expressions
// Most of it is copied verbatim from \boost\regex\v5\perl_matcher.hpp lines 140 to 250;
// but the named class matching is altered to ignore case insensitivity.

namespace boost { namespace BOOST_REGEX_DETAIL_NS {

template <typename iterator>
iterator implement_re_is_set_member(iterator next, iterator last,
    const re_set_long<utf32_regex_traits::char_class_type>* set_,
    const regex_data<char32_t, utf32_regex_traits>& e, bool icase) {

    typedef char32_t                             charT;        // ADDED: typedef template parameters
    typedef utf32_regex_traits                   traits_type;  // ADDED: typedef template parameters
    typedef utf32_regex_traits::char_class_type  char_classT;  // ADDED: typedef template parameters

    const charT* p = reinterpret_cast<const charT*>(set_ + 1);
    iterator ptr;
    unsigned int i;
    //bool icase = e.m_flags & regex_constants::icase;

    if (next == last) return next;

    typedef typename traits_type::string_type traits_string_type;
    const ::boost::regex_traits_wrapper<traits_type>& traits_inst = *(e.m_ptraits);

    // dwa 9/13/00 suppress incorrect MSVC warning - it claims this is never
    // referenced
    (void)traits_inst;

    // try and match a single character, could be a multi-character
    // collating element...
    for (i = 0; i < set_->csingles; ++i)
    {
        ptr = next;
        if (*p == static_cast<charT>(0))
        {
            // treat null string as special case:
            if (traits_inst.translate(*ptr, icase))
            {
                ++p;
                continue;
            }
            return set_->isnot ? next : (ptr == next) ? ++next : ptr;
        }
        else
        {
            while (*p && (ptr != last))
            {
                if (traits_inst.translate(*ptr, icase) != *p)
                    break;
                ++p;
                ++ptr;
            }

            if (*p == static_cast<charT>(0)) // if null we've matched
                return set_->isnot ? next : (ptr == next) ? ++next : ptr;

            p = re_skip_past_null(p);     // skip null
        }
    }

    charT col = traits_inst.translate(*next, icase);

    if (set_->cranges || set_->cequivalents)
    {
        traits_string_type s1;
        //
        // try and match a range, NB only a single character can match
        if (set_->cranges)
        {
            if ((e.m_flags & regex_constants::collate) == 0)
                s1.assign(1, col);
            else
            {
                charT a[2] = { col, charT(0), };
                s1 = traits_inst.transform(a, a + 1);
            }
            for (i = 0; i < set_->cranges; ++i)
            {
                if (STR_COMP(s1, p) >= 0)
                {
                    do { ++p; } while (*p);
                    ++p;
                    if (STR_COMP(s1, p) <= 0)
                        return set_->isnot ? next : ++next;
                }
                else
                {
                    // skip first string
                    do { ++p; } while (*p);
                    ++p;
                }
                // skip second string
                do { ++p; } while (*p);
                ++p;
            }
        }
        //
        // try and match an equivalence class, NB only a single character can match
        if (set_->cequivalents)
        {
            charT a[2] = { col, charT(0), };
            s1 = traits_inst.transform_primary(a, a + 1);
            for (i = 0; i < set_->cequivalents; ++i)
            {
                if (STR_COMP(s1, p) == 0)
                    return set_->isnot ? next : ++next;
                // skip string
                do { ++p; } while (*p);
                ++p;
            }
        }
    }
    if (traits_inst.isctype(*next, set_->cclasses) == true)                                // CHANGE: col => *next
        return set_->isnot ? next : ++next;
    if ((set_->cnclasses != 0) && (traits_inst.isctype(*next, set_->cnclasses) == false))  // CHANGE: col => *next
        return set_->isnot ? next : ++next;
    return set_->isnot ? ++next : next;
}

template <>
RegularExpressionU::DocumentIterator re_is_set_member(
    RegularExpressionU::DocumentIterator next, RegularExpressionU::DocumentIterator last,
    const re_set_long<utf32_regex_traits::char_class_type>* set_, const regex_data<char32_t, utf32_regex_traits>& e, bool icase) {
    return implement_re_is_set_member(next, last, set_, e, icase);
}

template <>
RegularExpressionSBCS::DocumentIterator re_is_set_member(
    RegularExpressionSBCS::DocumentIterator next, RegularExpressionSBCS::DocumentIterator last,
    const re_set_long<utf32_regex_traits::char_class_type>* set_, const regex_data<char32_t, utf32_regex_traits>& e, bool icase) {
    return implement_re_is_set_member(next, last, set_, e, icase);
}

template <>
RegularExpressionDBCS::DocumentIterator re_is_set_member(
    RegularExpressionDBCS::DocumentIterator next, RegularExpressionDBCS::DocumentIterator last,
    const re_set_long<utf32_regex_traits::char_class_type>* set_, const regex_data<char32_t, utf32_regex_traits>& e, bool icase) {
    return implement_re_is_set_member(next, last, set_, e, icase);
}

template <>
char32_t* re_is_set_member(char32_t* next, char32_t* last,
    const re_set_long<utf32_regex_traits::char_class_type>* set_, const regex_data<char32_t, utf32_regex_traits>& e, bool icase) {
    return implement_re_is_set_member(next, last, set_, e, icase);
}

}}


// This is the constructor for the regular expression creator.
// Most of it is copied verbatim from boost\regex\v5\basic_regex_creator.hpp lines 267 to 290;
// but the lower, upper and alpha masks, which are used only to modify the lower and upper masks
// when case insensitive matching is in effect, are left as zeros.
// (This is hacky, but simpler than overriding the places where these values are used.)

template <>
boost::BOOST_REGEX_DETAIL_NS::basic_regex_creator<char32_t, utf32_regex_traits>::basic_regex_creator
    (regex_data<char32_t, utf32_regex_traits>* data)
    : m_pdata(data), m_traits(*(data->m_ptraits)), m_last_state(0), m_icase(false), m_repeater_id(0),
    m_has_backrefs(false), m_bad_repeats(0), m_has_recursions(false), m_word_mask(0),
    m_mask_space(0), m_lower_mask(0), m_upper_mask(0), m_alpha_mask(0) {
    typedef char32_t           charT;                             // ADDED: typedef template parameters
    typedef utf32_regex_traits traits;                            // ADDED: typedef template parameters
    m_pdata->m_data.clear();
    m_pdata->m_status = ::boost::regex_constants::error_ok;
    static const charT w = 'w';
    static const charT s = 's';
    // static const charT l[5] = { 'l', 'o', 'w', 'e', 'r', };    // CHANGE: removed
    // static const charT u[5] = { 'u', 'p', 'p', 'e', 'r', };    // CHANGE: removed
    // static const charT a[5] = { 'a', 'l', 'p', 'h', 'a', };    // CHANGE: removed
    m_word_mask = m_traits.lookup_classname(&w, &w +1);
    m_mask_space = m_traits.lookup_classname(&s, &s +1);
    // m_lower_mask = m_traits.lookup_classname(l, l + 5);        // CHANGE: removed
    // m_upper_mask = m_traits.lookup_classname(u, u + 5);        // CHANGE: removed
    // m_alpha_mask = m_traits.lookup_classname(a, a + 5);        // CHANGE: removed
    m_pdata->m_word_mask = m_word_mask;
    BOOST_REGEX_ASSERT(m_word_mask != 0); 
    BOOST_REGEX_ASSERT(m_mask_space != 0); 
    // BOOST_REGEX_ASSERT(m_lower_mask != 0);                     // CHANGE: removed
    // BOOST_REGEX_ASSERT(m_upper_mask != 0);                     // CHANGE: removed
    // BOOST_REGEX_ASSERT(m_alpha_mask != 0);                     // CHANGE: removed
}
