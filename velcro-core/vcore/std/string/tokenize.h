/*
 * Copyright (c) Contributors to the VelcroFramework.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * 
 */
#ifndef V_FRAMEWORK_CORE_STD_STRING_TOKENIZE_H
#define V_FRAMEWORK_CORE_STD_STRING_TOKENIZE_H

#include <vcore/std/string/string.h>
#include <vcore/std/containers/vector.h>

namespace VStd
{
    template<class S>
    void tokenize(const S& str, const S& delimiters, vector<S>& tokens) {
        tokens.clear();
        if (str.empty()) {
            return;
        }

        // skip delimiters at beginning.
        string::size_type lastPos = str.find_first_not_of(delimiters, 0);

        // find first "non-delimiter".
        string::size_type pos = str.find_first_of(delimiters, lastPos);

        while (string::npos != pos || string::npos != lastPos) {
            // found a token, add it to the vector.
            tokens.push_back(str.substr(lastPos, pos - lastPos));

            // skip delimiters.  Note the "not_of"
            lastPos = str.find_first_not_of(delimiters, pos);

            // find next "non-delimiter"
            pos = str.find_first_of(delimiters, lastPos);
        }
    }

    template<class S>
    void tokenize_keep_empty(const S& str, const S& delimiters, vector<S>& tokens) {
        tokens.clear();
        if (str.empty()) {
            return;
        }

        string::size_type delimPos = 0, tokenPos = 0, pos = 0;

        while (true) {
            delimPos = str.find_first_of(delimiters, pos);
            tokenPos = str.find_first_not_of(delimiters, pos);

            if (string::npos != delimPos) {
                if (string::npos != tokenPos) {
                    if (tokenPos < delimPos) {
                        tokens.push_back(str.substr(pos, delimPos - pos));
                    } else {
                        tokens.push_back("");
                    }
                } else {
                    tokens.push_back("");
                }
                pos = delimPos + 1;
            } else {
                if (string::npos != tokenPos) {
                    tokens.push_back(str.substr(pos));
                } else {
                    tokens.push_back("");
                }
                break;
            }
        }
    }
}


#endif // V_FRAMEWORK_CORE_STD_STRING_TOKENIZE_H