#include <algorithm>
#include <cmath>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <streambuf>
#include <vector>

template<typename Derived>
struct Comparable
{
    bool operator>=(Derived const& other) const { return !(static_cast<Derived const&>(*this) < other); }
    bool operator<=(Derived const& other) const { return !(other < static_cast<Derived const&>(*this)); }
    bool operator>(Derived const& other) const { return !(*this <= other); }
    bool operator==(Derived const& other) const { return !(static_cast<Derived const&>(*this) < other) && !(other < static_cast<Derived const&>(*this)); }
    bool operator!=(Derived const& other) const { return !(*this == other); }
};

class WordStats : public Comparable<WordStats>
{
public:
    WordStats();
    void setTotalNumberOfLines(size_t totalNumberOfLines);
    size_t nbOccurrences() const;
    void addOneOccurrence(size_t lineNumber);
    size_t span() const;
    double proportion() const;
private:
    size_t totalNumberOfLines_ = 0;
    size_t totalNumberOfLines() const;
    size_t nbOccurrences_ = 0;
    size_t lowestOccurringLine_ = 0;
    size_t highestOccurringLine_ = 0;
};

WordStats::WordStats() : nbOccurrences_(){}

size_t WordStats::nbOccurrences() const
{
    return nbOccurrences_;
}

void WordStats::addOneOccurrence(size_t lineNumber)
{
    ++nbOccurrences_;
    
    if (nbOccurrences_ == 1)
    {
        lowestOccurringLine_ = highestOccurringLine_ = lineNumber;
    }
    else
    {
        lowestOccurringLine_ = std::min(lowestOccurringLine_, lineNumber);
        highestOccurringLine_ = std::max(highestOccurringLine_, lineNumber);
    }
}

size_t WordStats::span() const
{
    if (nbOccurrences_ == 0)
    {
        return 0;
    }
    else
    {
        return highestOccurringLine_ - lowestOccurringLine_ + 1;
    }
}

double WordStats::proportion() const
{
    if (totalNumberOfLines() == 0) return 0;
    
    return span() / static_cast<double>(totalNumberOfLines());
}

size_t WordStats::totalNumberOfLines() const
{
    return totalNumberOfLines_;
}

void WordStats::setTotalNumberOfLines(size_t totalNumberOfLines)
{
    totalNumberOfLines_ = totalNumberOfLines;
}

bool operator<(WordStats const& wordStats1, WordStats const& wordStats2)
{
    return wordStats1.nbOccurrences() < wordStats2.nbOccurrences();
}

class WordData : public Comparable<WordData>
{
public:
    WordData(std::string word, size_t lineNumber);
    std::string const& word() const;
    size_t lineNumber() const { return lineNumber_; }
private:
    std::string word_;
    size_t lineNumber_;
};

WordData::WordData(std::string word, size_t lineNumber) : word_(std::move(word)), lineNumber_(lineNumber){}

bool operator<(WordData const& wordData1, WordData const& wordData2)
{
    if (wordData1.word() < wordData2.word()) return true;
    if (wordData2.word() < wordData1.word()) return false;
    return wordData1.lineNumber() < wordData2.lineNumber();
}

std::string const& WordData::word() const
{
    return word_;
}

bool isDelimiter(char c)
{
    auto const isAllowedInName = isalnum(c) || c == '_';
    return !isAllowedInName;
}

template<typename EndOfWordPredicate>
std::vector<WordData> getWordDataFromCode(std::string const& code, EndOfWordPredicate isEndOfWord)
{
    auto words = std::vector<WordData>{};
    auto endWord = begin(code);
    auto beginWord = std::find_if_not(begin(code), end(code), isDelimiter);
    size_t line = 0;
    
    while (beginWord != end(code))
    {
        auto const linesBetweenWords = std::count(endWord, beginWord, '\n');
        line += linesBetweenWords;
        endWord = std::find_if(std::next(beginWord), end(code), isEndOfWord);
        words.emplace_back(std::string(beginWord, endWord), line);
        beginWord = std::find_if_not(endWord, end(code), isDelimiter);
    }
    return words;
}

enum class HowToDelimitWords
{
    EntireWords,
    WordsInCamelCase
};

template<HowToDelimitWords>
std::vector<WordData> getWordDataFromCode(std::string const& code);

template<>
std::vector<WordData> getWordDataFromCode<HowToDelimitWords::EntireWords>(std::string const& code)
{
    return getWordDataFromCode(code, isDelimiter);
}

template<>
std::vector<WordData> getWordDataFromCode<HowToDelimitWords::WordsInCamelCase>(std::string const& code)
{
    return getWordDataFromCode(code, [](char c){ return isDelimiter(c) || isupper(c); });
}

std::map<std::string, WordStats> wordStats(std::vector<WordData> const& wordData, size_t numberOfLines)
{
    auto wordStats = std::map<std::string, WordStats>{};
    for (auto const& oneWordData : wordData)
    {
        auto& oneWordStats = wordStats[oneWordData.word()];
        oneWordStats.setTotalNumberOfLines(numberOfLines);
        oneWordStats.addOneOccurrence(oneWordData.lineNumber());
    }
    return wordStats;
}

size_t numberOfLines(std::string const& code)
{
    return std::count(begin(code), end(code), '\n') + 1;
}

using WordCount = std::vector<std::pair<std::string, WordStats>>;

WordCount getWordCount(std::string const& code)
{
    auto const words = getWordDataFromCode<HowToDelimitWords::WordsInCamelCase>(code);
    auto const wordCount = wordStats(words, numberOfLines(code));
    
    auto sortedWordCount = WordCount(begin(wordCount), end(wordCount));
    std::sort(begin(sortedWordCount), end(sortedWordCount), [](auto const& p1, auto const& p2){ return p1.second > p2.second; });
    
    return sortedWordCount;
}

void print(WordCount const& entries)
{
    if (entries.empty()) return;
    auto const longestWord = *std::max_element(begin(entries), end(entries), [](auto const& p1, auto const& p2){ return p1.first.size() < p2.first.size(); });
    auto const longestWordSize = (int)longestWord.first.size();
    
    std::cout
    << std::setw(longestWordSize + 1) << std::left << "Word"
    << '|'
    << std::setw(4) << std::right << "#"
    << '|'
    << std::setw(4) << std::right << "span"
    << '|'
    << std::setw(11) << std::right << "proportion"
    << '\n';
    std::cout << std::string(longestWordSize + 1 + 1 + 4 + 1 + 4, '-') << '\n';
    
    for (auto const& entry : entries)
    {
        std::cout
        << std::setw(longestWordSize + 1) << std::left << entry.first
        << '|'
        << std::setw(4) << std::right << entry.second.nbOccurrences()
        << '|'
        << std::setw(4) << std::right << entry.second.span()
        << '|'
        << std::setw(10) << std::right << std::round(entry.second.proportion() * 100 * 100) / 100. << '%'
        << '\n';
    }
}

int main()
{
    auto fileStream = std::ifstream{"yourCode.txt"};
    auto const code = std::string(std::istreambuf_iterator<char>(fileStream), std::istreambuf_iterator<char>());
    print(getWordCount(code));
}
