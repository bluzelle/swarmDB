//#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE "CMapClassModule"

#include "CMap.h"

#include <boost/test/unit_test.hpp>
#include <boost/random.hpp>

static const unsigned long MAX_WORDS = 2000;
static const char *WORDS_FILENAME = "../words.txt";

unsigned long hash(const char *str);
void cmapInsertThreadFunction(std::vector<std::string>& words, CMap<unsigned long, std::string>* sut);

std::vector<std::string> ReadWords(const std::string &fileName, const uint16_t n = 1000);

// --run_test=cmap_concurrent_insert
BOOST_AUTO_TEST_CASE(cmap_concurrent_insert)
{
    std::vector<std::string> words = ReadWords(WORDS_FILENAME, MAX_WORDS);
    BOOST_CHECK(words.size() == MAX_WORDS);
    CMap<unsigned long, std::string> sut;

    std::vector<boost::thread *> threads;
    const uint16_t numThreads = 10;
    for (int i = 0; i < numThreads; ++i)
        {
        threads.emplace_back(new boost::thread(cmapInsertThreadFunction, words, &sut));
        }

    for (auto t : threads)
        {
        t->join();
        delete t;
        }
    threads.clear();

    BOOST_CHECK(sut.size() == words.size());
    for (auto w : words)
        {
        unsigned long h = hash(w.c_str());
        BOOST_CHECK(w == sut.get(h));
        }
}

///////////////////////////////////////////////////////////////////////////////
void cmapInsertThreadFunction(std::vector<std::string>& words, CMap<unsigned long, std::string>* sut)
{
    std::time_t now = std::time(0);
    boost::random::mt19937 gen{static_cast<std::uint32_t>(now)};
    unsigned long max = words.size();
    boost::random::uniform_int_distribution<unsigned long> dist{0, max-1};
    while(sut->size() < words.size())
        {
        std::string str = words[dist(gen)];
        sut->cInsert(hash(str.c_str()),str);
        }
}