#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE "BaseClassModule"
#define BOOST_USE_VALGRIND

#include "CMap.h"
#include "CSet.h"
#include "Node.h"
#include "NodeUtilities.h"
#include "NodeManager.h"

#include <vector>
#include <boost/test/unit_test.hpp>
#include <boost/random.hpp>
#include <boost/timer/timer.hpp>
#include <boost/thread.hpp>
#include <boost/chrono.hpp>
#include <fstream>

unsigned long hash(const char *str);
std::vector<std::string> ReadWords(const std::string &fileName, const uint16_t n = 1000);
void cmapInsertThreadFunction(std::vector<std::string> &words, CMap<unsigned long, std::string> *sut);
void csetInsertThreadFunction(std::vector<std::string> &words, CSet<std::string> *sut);
void csetRemoveThreadFunction(std::vector<std::string> &words, CSet<std::string> *sut);
Nodes* create_test_nodes(int n);

boost::mutex mutex;

static const unsigned long MAX_WORDS = 2000;
static const char *WORDS_FILENAME = "../words.txt";


//  --run_test=test_number_of_nodes_to_create
BOOST_AUTO_TEST_CASE(test_number_of_nodes_to_create)
{
    const unsigned long k_min_nodes = 5;
    const unsigned long k_max_nodes = 25;
    unsigned long total_nodes = 0;
    unsigned long actual = number_of_nodes_to_create(k_min_nodes, k_max_nodes, total_nodes);
    total_nodes += actual;
    BOOST_CHECK_EQUAL(actual, 5);
    while(total_nodes < k_max_nodes)
        {
        actual = number_of_nodes_to_create(k_min_nodes, k_max_nodes, total_nodes);
        BOOST_CHECK_EQUAL(actual, 1);
        total_nodes += actual;
        }
    actual = number_of_nodes_to_create(k_min_nodes, k_max_nodes, total_nodes);
    BOOST_CHECK_EQUAL(actual, 0);

    total_nodes = k_max_nodes * 2;
    actual = number_of_nodes_to_create(k_min_nodes, k_max_nodes, total_nodes);
    BOOST_CHECK_EQUAL(actual, 0);
}


BOOST_AUTO_TEST_CASE(cset_concurrent_insert)
{
    std::vector<std::string> words = ReadWords(WORDS_FILENAME, MAX_WORDS);
    BOOST_CHECK(words.size() == MAX_WORDS);
    const uint16_t numThreads = 10;
    CSet <std::string> sut;
    std::vector<boost::thread *> threads;
    for (int i = 0; i < numThreads; ++i)
        {
        threads.emplace_back(new boost::thread(csetInsertThreadFunction, words, &sut));
        }

    for (auto t : threads)
        {
        t->join();
        delete t;
        }
    threads.clear();

    BOOST_CHECK(sut.size() == words.size());
}

BOOST_AUTO_TEST_CASE(cset_find)
{
    std::vector<std::string> words = ReadWords(WORDS_FILENAME, MAX_WORDS);

    BOOST_CHECK(words.size() == MAX_WORDS);

    CSet<std::string> sut;

    for (auto w : words)
        {
        BOOST_CHECK(sut.insert(w));
        }

    for (auto w : words)
        {
        auto it = sut.find(w);
        std::string found = *it;
        BOOST_CHECK(0 == found.compare(w));
        }
}


//  --run_test=cset_remove
BOOST_AUTO_TEST_CASE(cset_remove)
{
    // simple remove tests
    const long max_words = 100;
    std::vector<std::string> words = ReadWords(WORDS_FILENAME, max_words);
    CSet<std::string> sut;
    for (auto w : words)
        {
        BOOST_CHECK(sut.cInsert(w));
        }
    BOOST_CHECK(words.size() == sut.size());
    boost::thread* thread = new boost::thread(csetRemoveThreadFunction,words,&sut);
    thread->join();
    delete thread;
    BOOST_CHECK(0 == sut.size());
}

//BOOST_AUTO_TEST_CASE(test_all_nodes_alive)
//{
//    Nodes nodes;
//    Node node0;
//    Node node1;
//    Node node2;
//
//    BOOST_CHECK(!all_nodes_alive(nodes));
//
//    nodes.emplace_back(&node0);
//    nodes.emplace_back(&node1);
//    nodes.emplace_back(&node2);
//
//    // NOTE: the assumption here is that 100ms is long enough for 3 nodes to
//    // get initialized and running in the alive state.
//    boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
//
//    BOOST_CHECK(all_nodes_alive(nodes));
//    node0.kill();
//    BOOST_CHECK(!all_nodes_alive(nodes));
//    node0.join();
//    BOOST_CHECK(!all_nodes_alive(nodes));
//
//    node1.kill();
//    node1.join();
//
//    node2.kill();
//    node2.join();
//}

//BOOST_AUTO_TEST_CASE(test_wait_for_all_nodes_to_start) {
//    const int kTEST_NODE_COUNT = 3;
//    Nodes *nodes;
//    nodes = create_test_nodes(kTEST_NODE_COUNT);
//
//
//
//    wait_for_all_nodes_to_start(*nodes);
//
//    // NOTE: remember that wait_for_all_nodes_to_start calls all_nodes_alive.
//    for (auto node : *nodes)
//        {
//        BOOST_CHECK(node->state() == Task::alive);
//        }
//
//    for(auto node : *nodes)
//        {
//        node->kill();
//        }
//
//    for(auto node : *nodes)
//        {
//        node->join();
//        }
//
//    for(auto node : *nodes)
//        {
//        delete node;
//        }
//
//    delete nodes;
//}


///////////////////////////////////////////////////////////////////////////////
// Test Helpers
unsigned long hash(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    return hash;
}

void csetInsertThreadFunction(std::vector<std::string>& words, CSet<std::string>* sut)
{
    std::time_t now = std::time(0);
    boost::random::mt19937 gen{static_cast<std::uint32_t>(now)};
    unsigned long max = words.size();
    boost::random::uniform_int_distribution<unsigned long> dist{0, max - 1};
    while (sut->size() < words.size())
        {
        std::string str = words[dist(gen)];
        sut->cInsert(str);
        boost::this_thread::yield();
        }
}

void csetRemoveThreadFunction(std::vector<std::string> &words, CSet<std::string> *sut)
{
    std::time_t now = std::time(0);
    boost::random::mt19937 gen{static_cast<std::uint32_t>(now)};
    unsigned long max = words.size();
    boost::random::uniform_int_distribution<unsigned long> dist{0, max - 1};

    while (sut->size() > 0)
        {
        std::string word = words[dist(gen)];
        auto found = sut->find(word);
        if(found != sut->end())
            {
            sut->remove(found);
            }
        boost::this_thread::yield();
        }
}

std::vector<std::string> ReadWords(const std::string &fileName, const uint16_t n)
{
    std::vector<std::string> words;
    std::ifstream wordFile;
    std::string line;
    wordFile.open(fileName);
    if (wordFile.is_open())
        {
        int16_t wordCount = n;
        while (getline(wordFile, line) && wordCount > 0)
            {
            words.emplace_back(line);
            wordCount--;
            }
        }
    return words;
}


