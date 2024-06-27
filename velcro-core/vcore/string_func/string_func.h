/*
 * Copyright (c) Contributors to the VelcroFramework.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef V_FRAMEWORK_CORE_STRING_FUNC_STRING_FUNC_H
#define V_FRAMEWORK_CORE_STRING_FUNC_STRING_FUNC_H

#include <vcore/io/path/path_fwd.h>
#include <vcore/std/function/function_fwd.h>
#include <vcore/std/string/fixed_string.h>
#include <vcore/std/string/string.h>
#include <vcore/std/containers/set.h>
#include <vcore/std/containers/vector.h>
#include <vcore/std/optional.h>
#include <vcore/casting/numeric_cast.h>

//---------------------------------------------------------------------
// FILENAME CONSTRAINTS
//---------------------------------------------------------------------

#if V_TRAIT_OS_USE_WINDOWS_FILE_PATHS
#   define V_CORRECT_FILESYSTEM_SEPARATOR '\\'
#   define V_CORRECT_FILESYSTEM_SEPARATOR_STRING "\\"
#   define V_DOUBLE_CORRECT_FILESYSTEM_SEPARATOR "\\\\"
#   define V_WRONG_FILESYSTEM_SEPARATOR '/'
#   define V_WRONG_FILESYSTEM_SEPARATOR_STRING "/"
#   define V_NETWORK_PATH_START "\\\\"
#   define V_NETWORK_PATH_START_SIZE 2
#   define V_CORRECT_AND_WRONG_FILESYSTEM_SEPARATOR "/\\"
#else
#   define V_CORRECT_FILESYSTEM_SEPARATOR '/'
#   define V_CORRECT_FILESYSTEM_SEPARATOR_STRING "/"
#   define V_DOUBLE_CORRECT_FILESYSTEM_SEPARATOR "//"
#   define V_WRONG_FILESYSTEM_SEPARATOR '\\'
#   define V_WRONG_FILESYSTEM_SEPARATOR_STRING "\\"
#   define V_NETWORK_PATH_START "\\\\"
#   define V_NETWORK_PATH_START_SIZE 2
#   define V_CORRECT_AND_WRONG_FILESYSTEM_SEPARATOR "/\\"
#endif
#define V_FILESYSTEM_EXTENSION_SEPARATOR '.'
#define V_FILESYSTEM_WILDCARD '*'
#define V_FILESYSTEM_DRIVE_SEPARATOR ':'
#define V_FILESYSTEM_INVALID_CHARACTERS "><|\"?*\r\n"

#define V_FILENAME_ALLOW_SPACES
#define V_SPACE_CHARACTERS " \t"
#define V_MATCH_ALL_FILES_WILDCARD_PATTERN  "*"
#define V_FILESYSTEM_SEPARATOR_WILDCARD V_CORRECT_FILESYSTEM_SEPARATOR_STRING V_MATCH_ALL_FILES_WILDCARD_PATTERN

//These do not change per platform they relate to how we represent paths in our database
//which is not dependent on platform file system
#define V_CORRECT_DATABASE_SEPARATOR '/'
#define V_CORRECT_DATABASE_SEPARATOR_STRING "/"
#define V_DOUBLE_CORRECT_DATABASE_SEPARATOR "//"
#define V_WRONG_DATABASE_SEPARATOR '\\'
#define V_WRONG_DATABASE_SEPARATOR_STRING "\\"
#define V_CORRECT_AND_WRONG_DATABASE_SEPARATOR "/\\"
#define V_DATABASE_EXTENSION_SEPARATOR '.'
#define V_DATABASE_INVALID_CHARACTERS "><|\"?*\r\n"
//////////////////////////////////////////////////////////////////////////

//! Namespace for string functions.
/*!
The StringFunc namespace is where we put string functions that extend some std::string functionality
to regular c-strings, make easier implementations of common but complex VStd::strings routines,
implement features that are not in std (or not yet in std), and a host of functions to more easily
manipulate file paths. Some simply do additional error checking on top of the std, or make it more
readable.
*/
namespace V {
    // Forwards


    namespace StringFunc {
        /*! Checks if the string begins with the given prefix, if prefixValue is a prefix of searchValue, this function will return true
        * \param[in] searchValue The input string to examine for a given prefix
        * \param[in] prefixValue The prefix value to examine searchValue for
        * \param[in] bCaseSensitive If true, comparison will be case sensitive
        */
        bool StartsWith(VStd::string_view searchValue, VStd::string_view prefixValue, bool bCaseSensitive = false);

        /*! Checks if the string ends with the given suffix, if suffixValue is a suffix of searchValue, this function will return true
        * \param[in] searchValue The input string to examine for a given suffix
        * \param[in] suffixValue The suffix value to examine searchValue for
        * \param[in] bCaseSensitive If true, comparison will be case sensitive
        */
        bool EndsWith(VStd::string_view searchValue, VStd::string_view suffixValue, bool bCaseSensitive = false);

        //! Equal
        /*! Equality for non VStd::strings.
        Ease of use to compare c-strings with case sensitivity.
        Example: Case Insensitive compare of a c-string with "Hello"
        StringFunc::Equal("hello", "Hello") = true
        Example: Case Sensitive compare of a c-string with "Hello"
        StringFunc::Equal("hello", "Hello", true) = false
        Example: Case Sensitive compare of the first 3 characters of a c-string with "Hello"
        StringFunc::Equal("Hello World", "Hello", true, 3) = true
        */
        bool Equal(const char* inA, const char* inB, bool bCaseSensitive = false, size_t n = 0);
        bool Equal(VStd::string_view inA, VStd::string_view inB, bool bCaseSensitive = false);

        //! Contains
        /*! Checks if the supplied character or string is contained within the @in parameter
        *
        Example: Case Insensitive contains finds character
        StringFunc::Contains("Hello", 'L') == true
        Example: Case Sensitive contains finds character
        StringFunc::Contains("Hello", 'l', true) = true
        Example: Case Insensitive contains does not find string
        StringFunc::Contains("Well Hello", "Mello") == false
        Example: Case Sensitive contains does not find character
        StringFunc::Contains("HeLlo", 'h', true) == false
        */
        bool Contains(VStd::string_view in, char ch, bool bCaseSensitive = false);
        bool Contains(VStd::string_view in, VStd::string_view sv, bool bCaseSensitive = false);

        //! Find
        /*! Find for non VStd::strings. Ease of use to find the first or last occurrence of a character or substring in a c-string with case sensitivity.
        Example: Case Insensitive find first occurrence of a character a in a c-string
        StringFunc::Find("Hello", 'l') == 2
        Example: Case Insensitive find last occurrence of a character a in a c-string
        StringFunc::Find("Hello", 'l') == 3
        Example: Case Sensitive find last occurrence of a character a in a c-string
        StringFunc::Find("HeLlo", 'L', true, true) == 2
        Example: Case Sensitive find first occurrence of substring "Hello" a in a c-string
        StringFunc::Find("Well Hello", "Hello", false, true) == 5
        */
        size_t Find(VStd::string_view in, char c, size_t pos = 0, bool bReverse = false, bool bCaseSensitive = false);
        size_t Find(VStd::string_view in, VStd::string_view str, size_t pos = 0, bool bReverse = false, bool bCaseSensitive = false);

        //! First and Last Character
        /*! First and Last Characters in a c-string. Ease of use to get the first or last character
         *! of a c-string.
         Example: If the first character in a c-string is a 'd' do something
         StringFunc::FirstCharacter("Hello") == 'H'
         Example: If the last character in a c-string is a V_CORRECT_FILESYSTEM_SEPARATOR do something
         StringFunc::LastCharacter("Hello") == 'o'
         */
        char FirstCharacter(const char* in);
        char LastCharacter(const char* in);

        //! Append and Prepend
        /*! Append and Prepend to a VStd::string. Increase readability and error checking.
         *! While appending has an ease of use operator +=, prepending currently does not.
         Example: Append character V_CORRECT_FILESYSTEM_SEPARATOR to a VStd::string
         StringFunc::Append(s = "C:", V_CORRECT_FILESYSTEM_SEPARATOR); s == "C:\\"
         Example: Append string " World" to a VStd::string
         StringFunc::Append(s = "Hello", " World");  s == "Hello World"
         Example: Prepend string V_NETWORK_PATH_START to a VStd::string
         StringFunc::Prepend(s = "18machine", V_NETWORK_PATH_START); s = "\\\\18machine"
         */
        VStd::string& Append(VStd::string& inout, const char s);
        VStd::string& Append(VStd::string& inout, const char* str);
        VStd::string& Prepend(VStd::string& inout, const char s);
        VStd::string& Prepend(VStd::string& inout, const char* str);

        //! LChop and RChop a VStd::string
        /*! Increase readability and error checking. RChop removes n characters from the right
         *! side (the end) of a VStd::string. LChop removes n characters from the left side (the
         *! beginning) of a string.
         Example: Remove (Chop off) the last 3 characters from a VStd::string
         StringFunc::RChop(s = "Hello", 3); s == "He"
         Example: Remove (Chop off) the first 3 characters from a VStd::string
         StringFunc::LChop(s = "Hello", 3); s == "lo"
         */
        VStd::string& LChop(VStd::string& inout, size_t num = 1);
        VStd::string& RChop(VStd::string& inout, size_t num = 1);

        VStd::string_view LChop(VStd::string_view in, size_t num = 1);
        VStd::string_view RChop(VStd::string_view in, size_t num = 1);

        //! LKeep and RKeep a VStd::string
        /*! Increase readability and error checking. RKeep keeps the string to the right of a position
         *! in a VStd::string. LKeep keep the string to the left of a position in a VStd::string.
         *! Optionally keep the character at position in the string.
         Example: Keep the string to the right of the 3rd character
         StringFunc::RKeep(s = "Hello World", 2); s == "o World"
         Example: Keep the string to the left of the 3rd character
         StringFunc::LKeep(s = "Hello", 3); s == "Hel"
         */
        VStd::string& LKeep(VStd::string& inout, size_t pos, bool bKeepPosCharacter = false);
        VStd::string& RKeep(VStd::string& inout, size_t pos, bool bKeepPosCharacter = false);

        //! Replace
        /*! Replace the first, last or all character(s) or substring(s) in a VStd::string with
         *! case sensitivity.
         Example: Case Insensitive Replace all 'o' characters with 'a' character
         StringFunc::Replace(s = "Hello World", 'o', 'a'); s == "Hella Warld"
         Example: Case Sensitive Replace all 'o' characters with 'a' character
         StringFunc::Replace(s = "HellO World", 'O', 'o'); s == "Hello World"
         */
        bool Replace(VStd::string& inout, const char replaceA, const char withB, bool bCaseSensitive = false, bool bReplaceFirst = false, bool bReplaceLast = false);
        bool Replace(VStd::string& inout, const char* replaceA, const char* withB, bool bCaseSensitive = false, bool bReplaceFirst = false, bool bReplaceLast = false);

        /**
        * Simple string trimming (trims spaces, tabs, and line-feed/cr characters from both ends)
        *
        * \param[in, out] value     The value to trim
        * \param[in] leading        Flag to trim the leading whitespace characters from the string
        * \param[in] trailing       Flag to trim the trailing whitespace characters from the string
        * \returns The reference to the trimmed value
        */
        VStd::string& TrimWhiteSpace(VStd::string& value, bool leading, bool trailing);

        //! LStrip
        /*! Strips leading characters in the stripCharacters set
        * Example
        * Example: Case Insensitive Strip leading 'a' characters
        * StringFunc::LStrip(s = "Abracadabra", 'a'); s == "bracadabra"
        * Example: Case Sensitive Strip leading 'a' characters
        * StringFunc::LStrip(s = "Abracadabra", 'a'); s == "Abracadabra"
        */
        //! RStrip
        /*! Strips trailing characters in the stripCharacters set
        * Example
        * Example: Case Insensitive Strip trailing 'a' characters
        * StringFunc::RStrip(s = "AbracadabrA", 'a'); s == "Abracadabr"
        * Example: Case Sensitive Strip trailing 'a' characters
        * StringFunc::RStrip(s = "AbracadabrA", 'a'); s == "AbracadabrA"
        */
        //! StripEnds
        /*! Strips leading and trailing characters in the stripCharacters set
        Example: Case Insensitive Strip all 'a' characters
        StringFunc::StripEnds(s = "Abracadabra", 'a'); s == "bracadabr"
        Example: Case Sensitive Strip all 'a' characters
        StringFunc::StripEnds(s = "Abracadabra", 'a'); s == "Abracadabr"
        */
        VStd::string_view LStrip(VStd::string_view in, VStd::string_view stripCharacters = " ");
        VStd::string_view RStrip(VStd::string_view in, VStd::string_view stripCharacters = " ");
        VStd::string_view StripEnds(VStd::string_view in, VStd::string_view stripCharacters = " ");

        //! Strip
        /*! Strip away the leading, trailing or all character(s) or substring(s) in a VStd::string with
        *! case sensitivity.
        Example: Case Insensitive Strip all 'a' characters
        StringFunc::Strip(s = "Abracadabra", 'a'); s == "brcdbr"
        Example: Case Insensitive Strip first 'b' character (No Match)
        StringFunc::Strip(s = "Abracadabra", 'b', true, false); s == "Abracadabra"
        Example: Case Sensitive Strip first 'a' character (No Match)
        StringFunc::Strip(s = "Abracadabra", 'a', true, false); s == "Abracadabra"
        Example: Case Insensitive Strip last 'a' character
        StringFunc::Strip(s = "Abracadabra", 'a', false, false, true); s == "Abracadabr"
        Example: Case Insensitive Strip first and last 'a' character 
        StringFunc::Strip(s = "Abracadabra", 'a', false, true, true); s == "bracadabr"
        Example: Case Sensitive Strip first and last 'l' character (No Match)
        StringFunc::Strip(s = "HeLlo HeLlo HELlO", 'l', true, true, true); s == "HeLlo HeLlo HELlO"
        Example: Case Insensitive Strip first and last "hello" character
        StringFunc::Strip(s = "HeLlo HeLlo HELlO", "hello", false, true, true); s == " HeLlo "
        */
        bool Strip(VStd::string& inout, const char stripCharacter = ' ', bool bCaseSensitive = false, bool bStripBeginning = false, bool bStripEnding = false);
        bool Strip(VStd::string& inout, const char* stripCharacters = " ", bool bCaseSensitive = false, bool bStripBeginning = false, bool bStripEnding = false);

        //! Tokenize
        /*! Tokenize a c-string, into a vector of VStd::string(s) optionally keeping empty string
         *! and optionally keeping space only strings
         Example: Tokenize the words of a sentence.
         StringFunc::Tokenize("Hello World", d, ' '); s[0] == "Hello", s[1] == "World"
         Example: Tokenize a comma and end line delimited string
         StringFunc::Tokenize("Hello,World\nHello,World", d, ' '); s[0] == "Hello", s[1] == "World"
         s[2] == "Hello", s[3] == "World"
         */
        void Tokenize(VStd::string_view in, VStd::vector<VStd::string>& tokens, const char delimiter, bool keepEmptyStrings = false, bool keepSpaceStrings = false);
        void Tokenize(VStd::string_view in, VStd::vector<VStd::string>& tokens, VStd::string_view delimiters = "\\//, \t\n", bool keepEmptyStrings = false, bool keepSpaceStrings = false);
        void Tokenize(VStd::string_view in, VStd::vector<VStd::string>& tokens, const VStd::vector<VStd::string_view>& delimiters, bool keepEmptyStrings = false, bool keepSpaceStrings = false);

        //! TokenizeVisitor
        /*! Tokenize a string_view and invoke a handler for each token found.
         *! The keep empty string option will invoke the handler with empty strings
         *! The keep space string option will invoke the handler with space strings
         Example: Tokenize the words of a sentence.
         StringFunc::TokenizeVisitor("Hello World", visitor, ' '); Invokes visitor("Hello") and visitor("World")
         Example: Tokenize a comma and newline delimited string
         StringFunc::TokenizeVisitor("Hello,World\nHello,World", visitor, ',\n ');
         Invokes visitor("Hello"), visitor("World"), visitor("Hello"), visitor("World")
         Example: Tokenize a space delimited string while keeping empty strings and space strings
         StringFunc::TokenizeVisitor("Hello World  More   Tokens", visitor, ' ', true, true); Invokes visitor("Hello"), visitor("World"), visitor(""),
            visitor("More"), visitor(""), visitor(""), "visitor("Tokens")
         */
        //! TokenVisitor
        /*! Callback which is invoked each time a token is found within the string
        */
        using TokenVisitor = VStd::function<void(VStd::string_view)>;
        void TokenizeVisitor(VStd::string_view in, const TokenVisitor& tokenVisitor, const char delimiter, bool keepEmptyStrings = false, bool keepSpaceStrings = false);
        void TokenizeVisitor(VStd::string_view in, const TokenVisitor& tokenVisitor, VStd::string_view delimiters = V_CORRECT_AND_WRONG_DATABASE_SEPARATOR ",\t\n",
            bool keepEmptyStrings = false, bool keepSpaceStrings = false);

        //! TokenizeVisitorReverse
        /*! Same as TokenizeVisitor, except that parsing of tokens start from the end of the view
         *! The keep empty string option will invoke the handler with empty strings
         *! The keep space string option will invoke the handler with space strings
        Example: Tokenize the words of a sentence.
        StringFunc::TokenizeVisitorReverse("Hello World", visitor, ' '); Invokes visitor("World") and visitor("Hello")
        Example: Tokenize a comma and newline delimited string
        StringFunc::TokenizeVisitorReverse("Hello,World\nHello,World", visitor, ',\n ');
        Invokes visitor("World"), visitor("Hello"), visitor("World"), visitor("Hello")
        Example: Tokenize a space delimited string while keeping empty strings and space strings
        StringFunc::TokenizeVisitorReverse("Hello World  More   Tokens", visitor, ' ', true, true); Invokes visitor("Tokens"), visitor(""), visitor(""),
            visitor("More"), visitor(""), visitor("World"), "visitor("Hello")
        */
        void TokenizeVisitorReverse(VStd::string_view in, const TokenVisitor& tokenVisitor, const char delimiter, bool keepEmptyStrings = false, bool keepSpaceStrings = false);
        void TokenizeVisitorReverse(VStd::string_view in, const TokenVisitor& tokenVisitor, VStd::string_view delimiters = V_CORRECT_AND_WRONG_DATABASE_SEPARATOR ",\t\n",
            bool keepEmptyStrings = false, bool keepSpaceStrings = false);

        //! TokenizeFirst
        /*! Returns an optional to a string_view of the characters that is before the first delimiter found in the set of delimiters.
         *! The optional is valid unless the view is empty, A valid optional indicates that at least one character of the @inout parameter
         *! has been processed and removed from it. This allows TokenizeFirst to be invoked in a loop
         Example: Tokenize the words of a sentence.
         VStd::string_view input = "Hello World";
         VStd::optional<VStd::string_view> output;
         output = StringFunc::TokenizeNext(input, " "); input = "World", output(valid) = "Hello"
         output = StringFunc::TokenizeNext(input, " "); input = "", output(valid) = "World"
         output = StringFunc::TokenizeNext(input, " "); input = "", output(invalid)
         Example: Tokenize a  delimited string with multiple whitespace 
         VStd::string_view input = "Hello World  More   Tokens";
         VStd::optional<VStd::string_view> output;
         output = StringFunc::TokenizeNext(input, ' '); input = "World  More   Tokens", output(valid) = "Hello"
         output = StringFunc::TokenizeNext(input, ' '); input = " More   Tokens", output(valid) = "World"
         output = StringFunc::TokenizeNext(input, ' '); input = "More   Tokens", output(valid) = ""
         output = StringFunc::TokenizeNext(input, ' '); input = "  Tokens", output(valid) = "More"
         output = StringFunc::TokenizeNext(input, ' '); input = " Tokens", output(valid) = ""
         output = StringFunc::TokenizeNext(input, ' '); input = "Tokens", output(valid) = ""
         output = StringFunc::TokenizeNext(input, ' '); input = "", output(valid) = "Tokens"
         output = StringFunc::TokenizeNext(input, ' '); input = "", output(invalid)
         */
        VStd::optional<VStd::string_view> TokenizeNext(VStd::string_view& inout, const char delimiter);
        VStd::optional<VStd::string_view> TokenizeNext(VStd::string_view& inout, VStd::string_view delimiters);

        //! TokenizeLast
        /*! Returns an optional to a string_view of the characters that is after the last delimiter found in the set of delimiters.
         *! The optional is valid unless the view is empty, A valid optional indicates that at least one character of the @inout parameter
         *! has been processed and removed from it.

         Example: Tokenize the words of a sentence.
         VStd::string_view input = "Hello World";
         VStd::optional<VStd::string_view> output;
         output = StringFunc::TokenizeLast(input, " "); input = "Hello", output(valid) = "World"
         output = StringFunc::TokenizeLast(input, " "); input = "", output(valid) = "Hello"
         output = StringFunc::TokenizeLast(input, " "); input = "", output(invalid)
         Example: Tokenize a  delimited string with multiple whitespace
         VStd::string_view input = "Hello World  More   Tokens";
         VStd::optional<VStd::string_view> output;
         output = StringFunc::TokenizeLast(input, ' '); input = "Hello World  More  ", output(valid) = "Tokens"
         output = StringFunc::TokenizeLast(input, ' '); input = "Hello World  More ", output(valid) = ""
         output = StringFunc::TokenizeLast(input, ' '); input = "Hello World  More", output(valid) = ""
         output = StringFunc::TokenizeLast(input, ' '); input = "Hello World ", output(valid) = "More"
         output = StringFunc::TokenizeLast(input, ' '); input = "Hello World", output(valid) = ""
         output = StringFunc::TokenizeLast(input, ' '); input = "Hello", output(valid) = "World"
         output = StringFunc::TokenizeLast(input, ' '); input = "", output(valid) = "Hello"
         output = StringFunc::TokenizeLast(input, ' '); input = "", output(invalid)
         */
        VStd::optional<VStd::string_view> TokenizeLast(VStd::string_view& inout, const char delimiter);
        VStd::optional<VStd::string_view> TokenizeLast(VStd::string_view& inout, VStd::string_view delimiters);

        //recognition and conversion from text

        //! Recognition of basic types
        /*! Provides for recognition and conversion of basic types (int float and bool) from c-string
        Example: Recognize a integer from a c-string
        StringFunc::LooksLikeInt("123") == true
        StringFunc::ToInt("123") == 123
        Example: Recognize a bool from a c-string
        StringFunc::LooksLikeBool("true") == true
        StringFunc::ToBool("true") == true
        Example: Recognize a float from a c-string
        StringFunc::LooksLikeFloat("1.23") == true
        StringFunc::ToFloat("1.23") == 1.23f
        */
        int ToInt(const char* in);
        bool LooksLikeInt(const char* in, int* pInt = nullptr);
        double ToDouble(const char* in);
        bool LooksLikeDouble(const char* in, double* pDouble = nullptr);
        float ToFloat(const char* in);
        bool LooksLikeFloat(const char* in, float* pFloat = nullptr);
        bool ToBool(const char* in);
        bool LooksLikeBool(const char* in, bool* pBool = nullptr);

        //! ToHexDump and FromHexDump
        /*! Convert a c-string to and from hex
        Example:convert a c-string to hex
        StringFunc::ToHexDump("abcCcdeEfFF",a); a=="6162634363646545664646"
        StringFunc::FromHexDump("6162634363646545664646", a); a=="abcCcdeEfFF"
        */
        bool ToHexDump(const char* in, VStd::string& out);
        bool FromHexDump(const char* in, VStd::string& out);


        //! Join
        /*! Joins a collection of strings that are convertible to string view. Applies a separator string in between elements.
        Note:
            The joinTarget variable will be appended to, if you need it cleared first
            you must reset it yourself before calling join
        Example: Join a list of the strings "test", "string" and "joining"
            VStd::list<VStd::string> example; 
            // add three strings: "test", "string" and "joining"
            VStd::string output;
            Join(output, example.begin(), example.end(), " -- ");
            // output == "test -- string -- joining"
            VStd::fixed_string<128> fixedOutput;
            Join(fixedOutput, example.begin(), example.end(), ",");
            // fixedOutput == "test,string,joining"
        */
        template<typename TStringType, typename TConvertableToStringViewIterator, typename TSeparatorString>
        inline void Join(
            TStringType& joinTarget, 
            const TConvertableToStringViewIterator& iteratorBegin, 
            const TConvertableToStringViewIterator& iteratorEnd, 
            const TSeparatorString& separator)
        {
            if (iteratorBegin == iteratorEnd)
            {
                return;
            }

            using CharType = typename TStringType::value_type;
            using CharTraitsType = typename TStringType::traits_type;
            size_t size = joinTarget.size() + VStd::basic_string_view<CharType, CharTraitsType>(*iteratorBegin).size();
            for (auto currentIterator = VStd::next(iteratorBegin); currentIterator != iteratorEnd; ++currentIterator)
            {
                size += VStd::basic_string_view<CharType, CharTraitsType>(*currentIterator).size();

                // Special case for when the separator is just the character type
                if constexpr (VStd::is_same_v<VStd::remove_cvref_t<TSeparatorString>, CharType>)
                {
                    size += 1;
                }
                else
                {
                    size += VStd::basic_string_view<CharType, CharTraitsType>(separator).size();
                }
            }

            joinTarget.reserve(size);
            joinTarget += *iteratorBegin;
            for (auto currentIterator = VStd::next(iteratorBegin); currentIterator != iteratorEnd; ++currentIterator)
            {
                joinTarget += separator;
                joinTarget += *currentIterator;
            }
        }

        //////////////////////////////////////////////////////////////////////////
        //! StringFunc::NumberFormatting Namespace
        /*! For string functions supporting string representations of numbers
        */
        namespace NumberFormatting
        {
            //! GroupDigits
            /*! Modifies the string representation of a number to add group separators, typically commas in thousands, e.g. 123456789.00 becomes 123,456,789.00
            *
            * \param buffer - The buffer containing the number which is to be modified in place
            * \param bufferSize - The length of the buffer in bytes
            * \param decimalPosHint - Optional position where the decimal point (or end of the number if there is no decimal) is located, will improve performance if supplied
            * \param digitSeparator - Grouping separator to use (default is comma ',')
            * \param decimalSeparator - Decimal separator to use (default is period '.')
            * \param groupingSize - Number of digits to group together (default is 3, i.e. thousands)
            * \param firstGroupingSize - If > 0, an alternative grouping size to use for the first group (some languages use this, e.g. Hindi groups 12,34,56,789.00)
            * \returns The length of the string in the buffer (including terminating null byte) after modifications
            */
            int GroupDigits(char* buffer, size_t bufferSize, size_t decimalPosHint = 0, char digitSeparator = ',', char decimalSeparator = '.', int groupingSize = 3, int firstGroupingSize = 0);
        }

        //////////////////////////////////////////////////////////////////////////
        //! StringFunc::AssetPath Namespace
        /*! For string functions for support asset path calculations 
        */
        namespace AssetPath
        {
            //! CalculateBranchToken
            /*! Calculate the branch token that is used for asset processor connection negotiations
            *
            * \param appRootPath - The absolute path of the app root to base the token calculation on
            * \param token       - The result of the branch token calculation
            */
            void CalculateBranchToken(const VStd::string& appRootPath, VStd::string& token);
        }

        //////////////////////////////////////////////////////////////////////////
        //! StringFunc::AssetDatabasePath Namespace
        /*! For string functions for physical paths and how the database expects them to appear.
         *! The AssetDatabse expects physical path to be expressed in term of three things:
         *!     1. The AssetDatabaseRoot: which is a simple string name that maps to a folder under
         *!     the ProjectRoot() where the asset resides, currently the only root we have is "intermediateassets"
         *!     and it currently maps to a folder called "\\intermediateassets\\"
         *!     2. The AssetDatabasePath: which is a string of the relative path from the AssetDatabaseRoot. This
         *!     includes the filename without the extension if it has one. It has no starting separator
         */
        namespace AssetDatabasePath
        {
            //! Normalize
            /*! Normalizes a asset database path and returns returns StringFunc::AssetDatabasePath::IsValid()
            *! strips all V_DATABASE_INVALID_CHARACTERS
            *! all V_WRONG_DATABASE_SEPARATOR's are replaced with V_CORRECT_DATABASE_SEPARATOR's
            *! make sure it has an ending V_CORRECT_DATABASE_SEPARATOR
            *! if V_FILENAME_ALLOW_SPACES is not set then it strips V_SPACE_CHARACTERS
            Example: Normalize a VStd::string root
            StringFunc::Root::Normalize(a="C:/linux/game/") == true; a=="C:/linux/game/"
            StringFunc::Root::Normalize(a="/linux/game/") == false; Doesn't have a drive
            StringFunc::Root::Normalize(a="C:/linux/game") == true; a=="C:/linux/game/"
            */
            bool Normalize(VStd::string& inout);

            //! IsValid
            /*! Returns if this string has the requirements to be a root
            *! fail if no drive
            *! make sure it has an ending V_CORRECT_DATABASE_SEPARATOR
            *! makes sure all V_WRONG_DATABASE_SEPARATOR's are replaced with V_CORRECT_DATABASE_SEPARATOR's
            *! if V_FILENAME_ALLOW_SPACES is not set then it makes sure it doesn't have any V_SPACE_CHARACTERS
            Example: Is the VStd::string a valid root
            StringFunc::Root::IsValid(a="C:/linux/game/") == true
            StringFunc::Root::IsValid(a="C:/linux/game/") == false; Wrong separators
            StringFunc::Root::IsValid(a="/linux/game/") == false; Doesn't have a drive
            StringFunc::Root::IsValid(a="C:/linux/game") == false; Doesn't have an ending separator
            */
            bool IsValid(const char* in);

            //! Split
            /*! Splits a full path into pieces, returns if it was successful
            *! EX: StringFunc::AssetDatabasePath::Split("C:/project/intermediateassets/somesubdir/anothersubdir/someFile.ext", &root, &path, &file, &extension) = true; root=="intermediateassets", path=="somesubdir/anothersubdir/", file=="someFile", extension==".ext"
            */
            bool Split(const char* in, VStd::string* pDstProjectRootOut = nullptr, VStd::string* pDstDatabaseRootOut = nullptr, VStd::string* pDstDatabasePathOut = nullptr, VStd::string* pDstDatabaseFileOut = nullptr, VStd::string* pDstFileExtensionOut = nullptr);

            //! Join
            /*! Joins two pieces of a asset database path returns if it was successful
            *! This has similar behavior to Python pathlib '/' operator and os.path.join
            *! Specifically, that it uses the last absolute path as the anchor for the resulting path
            *! https://docs.python.org/3/library/pathlib.html#pathlib.PurePath
            *! This means that joining StringFunc::AssetDatabasePath::Join("/etc/", "/usr/bin") results in "/usr/bin"
            *! not "/etc/usr/bin" 
            *! EX: StringFunc::AssetDatabasePath::Join("linux/game","info/some.file", a) == true; a== "linux/game/info/some.file"
            *! EX: StringFunc::AssetDatabasePath::Join("linux/game/info", "game/info/some.file", a) == true; a== "linux/game/info/game/info/some.file"
            *! EX: StringFunc::AssetDataPathPath::Join("linux/game/info", "/game/info/some.file", a) == true; a== "/game/info/some.file"
            *! (Replaces root directory part)
            */
            bool Join(const char* pFirstPart, const char* pSecondPart, VStd::string& out, bool bCaseInsensitive = true, bool bNormalize = true);
        };

        //////////////////////////////////////////////////////////////////////////
        //! StringFunc::Root Namespace
        /*! For string functions that deal with path roots
        */
        namespace Root
        {
            //! Normalize
            /*! Normalizes a root and returns returns StringFunc::Root::IsValid()
             *! strips all V_FILESYSTEM_INVALID_CHARACTERS
             *! all V_WRONG_FILESYSTEM_SEPARATOR's are replaced with V_CORRECT_FILESYSTEM_SEPARATOR's
             *! make sure it has an ending V_CORRECT_FILESYSTEM_SEPARATOR
             *! if V_FILENAME_ALLOW_SPACES is not set then it strips V_SPACE_CHARACTERS
             Example: Normalize a VStd::string root
             StringFunc::Root::Normalize(a="C:\\linux/game/") == true; a=="C:\\linux\\game\\"
             StringFunc::Root::Normalize(a="\\linux/game/") == false; Doesn't have a drive
             StringFunc::Root::Normalize(a="C:\\linux/game") == true; a=="C:\\linux\\game\\"
             */
            bool Normalize(VStd::string& inout);

            //! IsValid
            /*! Returns if this string has the requirements to be a root
            *! fail if no drive
            *! make sure it has an ending V_CORRECT_FILESYSTEM_SEPARATOR
            *! makes sure all V_WRONG_FILESYSTEM_SEPARATOR's are replaced with V_CORRECT_FILESYSTEM_SEPARATOR's
            *! if V_FILENAME_ALLOW_SPACES is not set then it makes sure it doesn't have any V_SPACE_CHARACTERS
            Example: Is the VStd::string a valid root
            StringFunc::Root::IsValid(a="C:\\linux\\game\\") == true
            StringFunc::Root::IsValid(a="C:/linux/game/") == false; Wrong separators
            StringFunc::Root::IsValid(a="\\linux\\game\\") == false; Doesn't have a drive
            StringFunc::Root::IsValid(a="C:\\linux\\game") == false; Doesn't have an ending separator
            */
            bool IsValid(const char* in);
        }

        //////////////////////////////////////////////////////////////////////////
        //! StringFunc::RelativePath Namespace
        /*! For string functions that deal with relative paths
        */
        namespace RelativePath
        {
            //! Normalize
            /*! Normalizes a relative path and returns returns StringFunc::RelativePath::IsValid()
             *! strips all V_FILESYSTEM_INVALID_CHARACTERS
             *! makes sure it does not have a drive
             *! makes sure it does not start with a V_CORRECT_FILESYSTEM_SEPARATOR
             *! make sure it ends with a V_CORRECT_FILESYSTEM_SEPARATOR
             *! makes sure all V_WRONG_FILESYSTEM_SEPARATOR's are replaced with V_CORRECT_FILESYSTEM_SEPARATOR's
             Example: Normalize a VStd::string root
             StringFunc::RelativePath::Normalize(a="\\linux/game/") == true; a=="linux\\game\\"
             StringFunc::RelativePath::Normalize(a="linux/game/") == true; a=="linux\\game\\"
             StringFunc::RelativePath::Normalize(a="C:\\linux/game") == false; Has a drive
             */
            bool Normalize(VStd::string& inout);

            //! IsValid
            /*! Returns if this string has the requirements to be a relative path
             *! makes sure it has no V_FILESYSTEM_INVALID_CHARACTERS
             *! makes sure all V_WRONG_FILESYSTEM_SEPARATOR's are replaced with V_CORRECT_FILESYSTEM_SEPARATOR's
             *! fails if it HasDrive()
             *! make sure it has an ending V_CORRECT_FILESYSTEM_SEPARATOR
             *! if V_FILENAME_ALLOW_SPACES is not set then it makes sure it doesn't have any spaces
             Example: Is the VStd::string a valid root
             StringFunc::Root::IsValid(a="linux\\game\\") == true
             StringFunc::Root::IsValid(a="linux/game/") == false; Wrong separators
             StringFunc::Root::IsValid(a="\\linux\\game\\") == false; Starts with a separator
             StringFunc::Root::IsValid(a="C:\\linux\\game") == false; Has a drive
             */
            bool IsValid(const char* in);
        }

        //////////////////////////////////////////////////////////////////////////
        //! StringFunc::Path Namespace
        /*! For string functions that deal with general pathing.
         *! A path is made up of one or more "component" parts:
         *! a root (which can be made of one or more components)
         *! the relative (which can be made of one or more components)
         *! the full name (which can be only 1 component, and is the only required component)
         *! The general form of a path is:
         *! |                               full path                                                   |
         *! |                       path                                    |       fullname            |
         *! |           root                |       relativepath            | filename  |   extension   |
         *! |   drive       |               folder path                     |
         *! |   drive       |
         *! |   component   |   component   |   component   |   component   |
         *! On PC and similar platforms,
         *! [drive [A-Z] V_FILESYSTEM_DRIVE_SEPARATOR or]V_CORRECT_FILESYSTEM_SEPARATOR rootPath V_CORRECT_FILESYSTEM_SEPARATOR relativePath V_CORRECT_FILESYSTEM_SEPARATOR filename<V_FILESYSTEM_EXTENSION_SEPARATOR ext>
         *!         NETWORK_START[machine] V_CORRECT_FILESYSTEM_SEPARATOR network share
         *! Example: "C:\\linux\\some.file"
         *! Example: "\\\\18byrne\\share\\linux\\some.file"
         *!
         *! For ease of understanding all examples assume Windows PC:
         *! V_CORRECT_FILESYSTEM_SEPARATOR is '\\' character
         *! V_WRONG_FILESYSTEM_SEPARATOR is '/' character
         *! V_FILESYSTEM_EXTENSION_SEPARATOR is '.' character
         *! V_FILESYSTEM_DRIVE_SEPARATOR is ':' character
         *! V_NETWORK_START "\\\\" string
         *!
         *! "root" is defined as the beginning of a valid path (it is a point from which a relative path/resource can be appended)
         *! a root must have a "drive"
         *! a root may or may not have a V_FILESYSTEM_DRIVE_SEPARATOR
         *! a root may or may not have a V_NETWORK_START
         *! a root always ends with a V_CORRECT_FILESYSTEM_SEPARATOR
         *! => C:\\linux\\Main\\Source\\GameAssets\\
         *! => D:\\
         *! => \\\\18username\\linux\\Main\\Source\\GameAssets\\
         *!
         *! "filename" is defined as all the characters after the last V_CORRECT_FILESYSTEM_SEPARATOR up to
         *! but not including V_FILESYSTEM_EXTENSION_SEPARATOR if any.
         *! EX: D:\\linux\\Main\\Source\\GameAssets\\gameinfo\\Characters\\some.xml
         *! => some
         *!
         *! "extension" is defined as the last V_FILESYSTEM_EXTENSION_SEPARATOR + all characters afterward
         *! Note extension is considered part of the fullname component and its size takes away
         *! from the MAX_FILE_NAME_LEN, so filename + V_FILESYSTEM_EXTENSION_SEPARATOR + extension (if any), has to be <= MAX_FILE_NAME_LEN
         *! Note all functions that deal with extensions should be tolerant of missing V_FILESYSTEM_EXTENSION_SEPARATOR
         *! EX. ReplaceExtension("xml") and (".xml") both should arrive at the same result
         *! Note also when stripping name the extension is also stripped
         *! EX: D:\\linux\\Main\\Source\\GameAssets\\gameinfo\\Characters\\some.xml
         *! => .xml
         *!
         *! "fullname" is defined as name + ext (if any)
         *! EX: D:\\linux\\Main\\Source\\GameAssets\\gameinfo\\Characters\\some.xml
         *! => some.xml
         *!
         *! "relativePath" is defined as anything in between a root and the filename
         *! Note: relative paths DO NOT start with a V_CORRECT_FILESYSTEM_SEPARATOR but ALWAYS end with one
         *! EX: D:\\linux\\Main\\Source\\GameAssets\\gameinfo\\Characters\\some.xml
         *! => gameinfo\\Characters\\
         *!
         *! "path" is defined as all characters up to and including the last V_CORRECT_FILESYSTEM_SEPARATOR
         *! EX: D:\\linux\\Main\\Source\\GameAssets\\gameinfo\\Characters\\some.xml
         *! => D:\\linux\\Main\\Source\\GameAssets\\gameinfo\\Characters\\
         *!
         *! "folder path" is defined as all characters in between and including the first and last V_CORRECT_FILESYSTEM_SEPARATOR
         *! EX: D:\\linux\\Main\\Source\\GameAssets\\gameinfo\\Characters\\some.xml
         *! => \\linux\\Main\\Source\\GameAssets\\gameinfo\\Characters\\
         *! EX: /app_home/gameinfo/Characters/some.xml
         *! => /gameinfo/Characters/
         *!
         *! "drive" on PC and similar platforms
         *! is defined as all characters up to and including the first V_FILESYSTEM_DRIVE_SEPARATOR or
         *! all characters up to but not including the first V_CORRECT_FILESYSTEM_SEPARATOR after a NETWORK_START
         *! D:\\linux\\Main\\Source\\GameAssets\\gameinfo\\Characters\\some.xml
         *! => D:
         *! EX: \\\\18username\\linux\\Main\\Source\\GameAssets\\gameinfo\\Characters\\some.xml
         *! => \\\\18username
         *!
         *! "fullpath" is defined as having a root, relative path and filename
         *! EX: D:\\linux\\Main\\Source\\GameAssets\\gameinfo\\Characters\\some.xml
         *!
         *! "component" refers to any piece of a path. All components have a V_CORRECT_FILESYSTEM_SEPARATOR except file name.
         *! EX: D:\\linux\\Main\\Source\\GameAssets\\gameinfo\\Characters\\some.xml
         *! => "D:\\", "linux\\", "Main\\", "Source\\", "GameAssets\\", "gameinfo\\", "Characters\\", "some.xml"
         *! EX: /app_home/gameinfo/Characters/some.xml
         *! => "/app_home/", "gameinfo/", "Characters/", "some.xml"
         */
        namespace Path
        {
            using FixedString = V::IO::FixedMaxPathString;
            //! Normalize
            /*! Normalizes a path and returns returns StringFunc::Path::IsValid()
             *! strips all V_FILESYSTEM_INVALID_CHARACTERS
             *! all V_WRONG_FILESYSTEM_SEPARATOR's are replaced with V_CORRECT_FILESYSTEM_SEPARATOR's
             *! all V_DOUBLE_CORRECT_SEPARATOR's (not in the "drive") are replaced with V_CORRECT_FILESYSTEM_SEPARATOR's
             *! makes sure it does not have a drive
             *! makes sure it does not start with a V_CORRECT_FILESYSTEM_SEPARATOR
             Example: Normalize a VStd::string path
             StringFunc::Path::Normalize(a="\\linux/game/") == true; a=="linux\\game\\"
             StringFunc::Path::Normalize(a="linux/game/") == true; a=="linux\\game\\"
             StringFunc::Path::Normalize(a="C:\\linux/game") == true; a=="C:\\linux\\game\\"
             */
            bool Normalize(VStd::string& inout);
            bool Normalize(FixedString& inout);

            //! IsValid
            /*! Returns if this string has the requirements to be a path
             *! make sure its not empty
             *! make sure it has no V_FILESYSTEM_INVALID_CHARACTERS
             *! make sure it has no WRONG_SEPARATORS
             *! make sure it HasDrive() if bHasDrive is set
             *! if V_FILENAME_ALLOW_SPACES is not set then it makes sure it doesn't have any spaces
             *! make sure total length <= V_MAX_PATH_LEN
             Example: Is the VStd::string a valid root
             StringFunc::Path::IsValid(a="linux\\game\\") == true
             StringFunc::Path::IsValid(a="linux/game/") == false; Wrong separators
             StringFunc::Path::IsValid(a="C:\\linux\\game\\") == true
             StringFunc::Path::IsValid(a="C:\\linux\\game") == true
             StringFunc::Path::IsValid(a="C:\\linux\\game\\some.file") == true
             */
            bool IsValid(const char* in, bool bHasDrive = false, bool bHasExtension = false, VStd::string* errors = NULL);

            //! ConstructFull
            /*! Constructs a full path from pieces and does some minimal smart normalization to make it easier, returns if it was successful
             *! EX: StringFunc::Path::ContructFull("C:\\linux", "some.file", a) == true; a == "C:\\linux\\some.file"
             *! EX: StringFunc::Path::ContructFull("C:\\linux", "game/info\\some.file", a, true) == true; a == "C:\\linux\\game\\info\\some.file"
             *! EX: StringFunc::Path::ContructFull("C:\\linux", "/some", "file", a, true) == true; a== "C:\\linux\\some.file"
             *! EX: StringFunc::Path::ContructFull("C:\\linux", "some", ".file", a) == true; a== "C:\\linux\\some.file"
             *! EX: StringFunc::Path::ContructFull("C:\\linux", "info", "some", "file", a) == true; a=="C:\\linux\\info\\some.file"
             */
            bool ConstructFull(const char* pRootPath, const char* pFileName, VStd::string& out, bool bNormalize = false);
            bool ConstructFull(const char* pRootPath, const char* pFileName, const char* pFileExtension, VStd::string& out, bool bNormalize = false);
            bool ConstructFull(const char* pRoot, const char* pRelativePath, const char* pFileName, const char* pFileExtension, VStd::string& out, bool bNormalize = false);

            //! Split
            /*! Splits a full path into pieces, returns if it was successful
            *! EX: StringFunc::Path::Split("C:\\linux\\game\\info\\some.file", &drive, &folderPath, &fileName, &extension) = true; drive==C: , folderPath=="\\linux\\game\\info\\", filename=="some", extension==".file"
            */
            bool Split(const char* in, VStd::string* pDstDriveOut = nullptr, VStd::string* pDstFolderPathOut = nullptr, VStd::string* pDstNameOut = nullptr, VStd::string* pDstExtensionOut = nullptr);

            //! Join
            /*! Joins two pieces of a path returns if it was successful
            *! This has similar behavior to Python pathlib '/' operator and os.path.join
            *! Specifically, that it uses the last absolute path as the anchor for the resulting path
            *! https://docs.python.org/3/library/pathlib.html#pathlib.PurePath
            *! This means that joining StringFunc::Path::Join("C:\\VELCRO" "F:\\VELCRO") results in "F:\\VELCRO"
            *! not "C:\\VELCRO\\F:\\VELCRO"
            *! EX: StringFunc::Path::Join("C:\\linux\\game","info\\some.file", a) == true; a== "C:\\linux\\game\\info\\some.file"
            *! EX: StringFunc::Path::Join("C:\\linux\\game\\info", "game\\info\\some.file", a) == true; a== "C:\\linux\\game\\info\\game\\info\\some.file"
            *! EX: StringFunc::Path::Join("C:\\linux\\game\\info", "\\game\\info\\some.file", a) == true; a== "C:\\game\\info\\some.file"
            *! (Replaces root directory part)
            */
            bool Join(const char* pFirstPart, const char* pSecondPart, VStd::string& out, bool bCaseInsensitive = true, bool bNormalize = true);
            bool Join(const char* pFirstPart, const char* pSecondPart, FixedString& out, bool bCaseInsensitive = true, bool bNormalize = true);

            //! HasDrive
            /*! returns if the c-string has a "drive"
            *! EX: StringFunc::Path::HasDrive("C:\\linux\\game\\info\\some.file") == true
            *! EX: StringFunc::Path::HasDrive("\\linux\\game\\info\\some.file") == false
            *! EX: StringFunc::Path::HasDrive("\\\\18usernam\\linux\\game\\info\\some.file") == true
            */
            bool HasDrive(const char* in, bool bCheckAllFileSystemFormats = false);

            //! HasExtension
            /*! returns if the c-string has an "extension"
            *! make sure its not empty
            *! make sure V_FILESYSTEM_EXTENSION_SEPARATOR is not the first or last character
            *! make sure V_FILESYSTEM_EXTENSION_SEPARATOR occurs after the last V_CORRECT_FILESYSTEM_SEPARATOR (if it has one)
            *! make sure its at most MAX_EXTENSION_LEN chars including the V_FILESYSTEM_EXTENSION_SEPARATOR character
            *! EX: StringFunc::Path::HasExtension("C:\\linux\\game\\info\\some.file") == true
            *! EX: StringFunc::Path::HasExtension("\\linux\\game\\info\\some") == false
            */
            bool HasExtension(const char* in);

            //! IsExtension
            /*! returns if the c-string has an "extension", optionally case case sensitive
            *! make sure its HasExtension()
            *! tolerate not having an V_FILESYSTEM_EXTENSION_SEPARATOR on the comparison
            *! EX: StringFunc::Path::IsExtension("C:\\linux\\game\\info\\some.file", "file") == true
            *! EX: StringFunc::Path::IsExtension("C:\\linux\\game\\info\\some.file", ".file") == true
            *! EX: StringFunc::Path::IsExtension("\\linux\\game\\info\\some") == false
            *! EX: StringFunc::Path::IsExtension("\\linux\\game\\info\\some.file", ".FILE", true) == false
            */
            bool IsExtension(const char* in, const char* pExtension, bool bCaseInsenitive = true);

            //! IsRelative
            /*! returns if the c-string fulfills the requirements to be "relative"
            *! make sure its not empty
            *! make sure its does not have a "drive"
            *! make sure its doesn't start with V_CORRECT_FILESYSTEM_SEPARATOR
            *! EX: StringFunc::Path::IsRelative("linux\\game\\info\\some.file") == true
            *! EX: StringFunc::Path::IsRelative("\\linux\\game\\info\\some.file") == false
            *! EX: StringFunc::Path::IsRelative("C:\\linux\\game\\info\\some.file") == false
            */
            bool IsRelative(const char* in);

            //! StripDrive
            /*! gets rid of the drive component if it has one, returns if it removed one or not
            *! EX: StringFunc::Path::StripDrive(a="C:\\linux\\game\\info\\some.file") == true; a=="\\linux\\game\\info\\some.file"
            */
            bool StripDrive(VStd::string& inout);

            //! StripPath
            /*! gets rid of the path if it has one, returns if it removed one or not
            *! EX: StringFunc::Path::StripPath(a="C:\\linux\\game\\info\\some.file") == true; a=="some.file"
            */
            void StripPath(VStd::string& out);

            //! StripFullName
            /*! gets rid of the full file name if it has one, returns if it removed one or not
            *! EX: StringFunc::Path::StripFullName(a="C:\\linux\\game\\info\\some.file") == true; a=="C:\\linux\\game\\info"
            */
            void StripFullName(VStd::string& out);

            //! StripExtension
            /*! gets rid of the extension if it has one, returns if it removed one or not
            *! EX: StringFunc::Path::StripExtension(a="C:\\linux\\game\\info\\some.file") == true; a=="C:\\linux\\game\\info\\some"
            */
            void StripExtension(VStd::string& inout);

            //! StripComponent
            /*! gets rid of the first or last if it has one, returns if it removed one or not
            *! EX: StringFunc::Path::StripComponent(a="D:\\linux\\Main\\Source\\GameAssets\\gameinfo\\Characters\\some.xml") == true; a=="linux\\Main\\Source\\GameAssets\\gameinfo\\Characters\\some.xml"
            *!  EX: StringFunc::Path::StripComponent(a="linux\\Main\\Source\\GameAssets\\gameinfo\\Characters\\some.xml") == true; a=="Main\\Source\\GameAssets\\gameinfo\\Characters\\some.xml"
            *!  EX: StringFunc::Path::StripComponent(a="Main\\Source\\GameAssets\\gameinfo\\Characters\\some.xml") == true; a=="Source\\GameAssets\\gameinfo\\Characters\\some.xml"
            *!  EX: StringFunc::Path::StripComponent(a="Main\\Source\\GameAssets\\gameinfo\\Characters\\some.xml", true) == true; a=="Main\\Source\\GameAssets\\gameinfo\\Characters"
            *!  EX: StringFunc::Path::StripComponent(a="Main\\Source\\GameAssets\\gameinfo\\Characters", true) == true; a=="Main\\Source\\GameAssets\\gameinfo"
            */
            bool StripComponent(VStd::string& inout, bool bLastComponent = false);

            //! GetDrive
            /*! if a c-string has a drive put it in VStd::string, returns if it was sucessful
            *! EX: StringFunc::Path::GetDrive("C:\\linux\\game\\info\\some.file",a) == true; a=="C:"
            *! EX: StringFunc::Path::GetDrive("\\\\18username\\linux\\game\\info\\some.file",a) == true; a=="\\\18username"
            */
            bool GetDrive(const char* in, VStd::string& out);

            //! GetParentDir
            /*! Retrieves the parent directory using the supplied @in string
            *!  If @in ends with a trailing slash, it is treated the same as a directory without the trailing slash
            *!  EX: StringFunc::Path::GetParentDir("D:\\linux\\Main\\Source\\GameAssets\\gameinfo\\Characters\\some.xml",a) == true; a=="D:\\linux\\Main\\Source\\GameAssets\\gameinfo\\Characters"
            *!  EX: StringFunc::Path::GetParentDir("D:\\linux\\Main\\Source\\GameAssets\\gameinfo\\Characters\\",a) == true; a=="D:\\linux\\Main\\Source\\GameAssets\\gameinfo"
            *!  EX: StringFunc::Path::GetParentDir("D:\\linux\\Main\\Source\\GameAssets\\gameinfo\\Characters",a) == true; a=="D:\\linux\\Main\\Source\\GameAssets\\gameinfo"
            *!  EX: StringFunc::Path::GetParentDir("D:\\",a) == true; a=="D:\\"
            */
            VStd::optional<VStd::string_view> GetParentDir(VStd::string_view path);

            //! GetFullPath
            /*! if a c-string has a fullpath put it in VStd::string, returns if it was sucessful
            *! EX: StringFunc::Path::GetFullPath("D:\\linux\\Main\\Source\\GameAssets\\gameinfo\\Characters\\some.xml",a) == true; a=="D:\\linux\\Main\\Source\\GameAssets\\gameinfo\\Characters"
            */
            bool GetFullPath(const char* in, VStd::string& out);

            //! GetFolderPath
            /*! if a c-string has a folderpath put it in VStd::string, returns if it was sucessful
            *! EX: StringFunc::Path::GetFolderPath("D:\\linux\\Main\\Source\\GameAssets\\gameinfo\\Characters\\some.xml",a) == true; a=="\\linux\\Main\\Source\\GameAssets\\gameinfo\\Characters"
            */
            bool GetFolderPath(const char* in, VStd::string& out);

            //! GetFolder
            /*! if a c-string has a beginning or ending folder put it in VStd::string, returns if it was sucessful
            *! EX: StringFunc::Path::GetFolder("D:\\linux\\Main\\Source\\GameAssets\\gameinfo\\Characters\\some.xml", a) == true; a=="linux"
            *! EX: StringFunc::Path::GetFolder("D:\\linux\\Main\\Source\\GameAssets\\gameinfo\\Characters\\some.xml", a, true) == true; a=="Characters"
            */
            bool GetFolder(const char* in, VStd::string& out, bool bFirst = false);

            //! GetFullFileName
            /*! if a c-string has afull file name put it in VStd::string, returns if it was sucessful
            *! EX: StringFunc::Path::GetFullFileName("D:\\linux\\Main\\Source\\GameAssets\\gameinfo\\Characters\\some.xml", a) == true; a=="some.xml"
            *! EX: StringFunc::Path::GetFullFileName("D:\\linux\\Main\\Source\\GameAssets\\gameinfo\\Characters\\", a) == false;
            */
            bool GetFullFileName(const char* in, VStd::string& out);

            //! GetFileName
            /*! if a c-string has a file name put it in VStd::string, returns if it was sucessful
            *! EX: StringFunc::Path::GetFileName("D:\\linux\\Main\\Source\\GameAssets\\gameinfo\\Characters\\some.xml", a) == true; a=="some"
            *! EX: StringFunc::Path::GetFileName("D:\\linux\\Main\\Source\\GameAssets\\gameinfo\\Characters\\", a) == false;
            */
            bool GetFileName(const char* in, VStd::string& out);

            //! GetExtension
            /*! if a c-string has an extension put it in VStd::string, returns if it was sucessful
            *! EX: StringFunc::Path::GetFileName("D:\\linux\\Main\\Source\\GameAssets\\gameinfo\\Characters\\some.xml", a) == true; a==".xml"
            *! EX: StringFunc::Path::GetFileName("D:\\linux\\Main\\Source\\GameAssets\\gameinfo\\Characters\\", a) == false;
            *! EX: StringFunc::Path::GetExtension("D:\\linux\\Main\\Source\\GameAssets\\gameinfo\\Characters\\some.xml", a, false) == true; a=="xml"
            */
            bool GetExtension(const char* in, VStd::string& out, bool includeDot = true);

            //! ReplaceFullName
            /*! if a VStd::string has a full name then replace it with pFileName and optionally with pExtension
            *! EX: StringFunc::Path::ReplaceFullName(a="D:\\linux\\some.file", "other", ".xml"); a=="D:\\linux\\other.xml"
            *! EX: StringFunc::Path::ReplaceFullName(a="D:\\linux\\some.file", "other.xml"); a=="D:\\linux\\other.xml"
            *! EX: StringFunc::Path::ReplaceFullName(a="D:\\linux\\some.file", "other.file", "xml"); a=="D:\\linux\\other.xml"
            *! EX: StringFunc::Path::ReplaceFullName(a="D:\\linux\\some.file", "other.file", ""); a=="D:\\linux\\other"
            */
            void ReplaceFullName(VStd::string& inout, const char* pFileName, const char* pFileExtension = nullptr);

            //! ReplaceExtension
            /*! if a VStd::string HasExtension() then replace it with pDrive
            *! EX: StringFunc::Path::ReplaceExtension(a="D:\\linux\\some.file", "xml"); a=="D:\\linux\\some.xml"
            *! EX: StringFunc::Path::ReplaceExtension(a="D:\\linux\\some.file", ".xml"); a=="D:\\linux\\some.xml"
            *! EX: StringFunc::Path::ReplaceExtension(a="D:\\linux\\some.file", ""); a=="D:\\linux\\some"
            */
            void ReplaceExtension(VStd::string& inout, const char* pFileExtension);

            //! AppendSeparator 
            /*! Appends the correct separator to the path
            *! EX: StringFunc::Path::AppendSeparator("C:\\project\\intermediateassets", &path);
            *! path=="C:\\project\\intermediateassets\\"
            */
            VStd::string& AppendSeparator(VStd::string& inout);
        } // namespace Path

        namespace Json 
        {
            //! ToEscapedString
            /* Escape a string to make it compatible for saving to the json format
            *! EX: StringFunc::Json::ToEscapeString(""'"")
            *! output=="\"'\""
            */
            VStd::string& ToEscapedString(VStd::string& inout);
        } // namespace Json

        namespace Base64
        {
            //! Base64Encode
            /* Encodes Binary data to base-64 format which allows it to be stored safely in json or xml data
            *! EX: StringFunc::Base64::Base64Encode(reinterpret_cast<V::u8*>("NUL\0InString"), V_ARRAY_SIZE("NUL\0InString") - 1);
            *! output = "TlVMAEluU3RyaW5n"
            */
            VStd::string Encode(const V::u8* in, const size_t size);
            //! Base64Decode
            /* Decodes Base64 Text data to binary data and appends result int out array
            *! If the the supplied text is not valid Base64 then false will be result @out array will be unmodified
            *! EX: StringFunc::Base64::Base64Decode("TlVMAEluU3RyaW5n", strlen("TlVMAEluU3RyaW5n"));
            *! output = {'N','U', 'L', '\0', 'I', 'n', 'S', 't', 'r', 'i', 'n', 'g'}
            */
            bool Decode(VStd::vector<V::u8>& out, const char* in, const size_t size);

        };

        namespace Utf8
        {
            /**
             * Check to see if a string contains any byte that cannot be encoded in 7-bit ASCII.
             * @param in A string with UTF-8 encoding.
             * @return true if the string passed in contains any byte that cannot be encoded in 7-bit ASCII, otherwise false.
             */
            bool CheckNonAsciiChar(const VStd::string& in);
        }
    } // namespace StringFunc
} // namespace VelcroFramework


#endif // V_FRAMEWORK_CORE_STRING_FUNC_STRING_FUNC_H