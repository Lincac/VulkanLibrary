#pragma once

#include <cctype>
#include <cstring>

namespace mat::demo {

    inline int fuzzyMatchScore(const char* text, const char* query) {
        if (text == nullptr || query == nullptr) {
            return -1;
        }
        if (query[0] == '\0') {
            return -1;
        }

        int score = 0;
        int consecutive = 0;
        int queryIndex = 0;

        for (int i = 0; text[i] != '\0'; ++i) {
            const char textCh = static_cast<char>(std::tolower(static_cast<unsigned char>(text[i])));
            const char queryCh =
                static_cast<char>(std::tolower(static_cast<unsigned char>(query[queryIndex])));

            if (textCh != queryCh) {
                consecutive = 0;
                continue;
            }

            score += 10;
            if (i == 0 || queryIndex == 0) {
                score += 8;
            }
            if (consecutive > 0) {
                score += 6;
            }
            consecutive++;
            queryIndex++;

            if (query[queryIndex] == '\0') {
                score += 5;
                return score;
            }
        }

        return -1;
    }

}  // namespace mat::demo
