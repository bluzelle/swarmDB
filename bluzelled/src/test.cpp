#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE "BaseClassModule"

#include "CMap.h"
#include "CSet.h"
#include "Node.h"
#include "NodeUtilities.h"

#include <vector>
#include <boost/test/unit_test.hpp>
#include <boost/random.hpp>
#include <boost/timer/timer.hpp>
#include <boost/thread.hpp>
#include <boost/chrono.hpp>
#include <fstream>

unsigned long hash(const char *str);
std::vector<std::string> ReadWords(const std::string& fileName, const uint16_t n=1000);
void cmapInsertThreadFunction(std::vector<std::string>& words, CMap<unsigned long, std::string>* sut);
void csetInsertThreadFunction(std::vector<std::string>& words, CSet<std::string>* sut);
void csetRemoveThreadFunction(std::vector<std::string>& words, CSet<std::string>* sut);
boost::mutex mutex;

static const unsigned long  MAX_WORDS = 2000;
static const char*          WORDS_FILENAME = "../words.txt";

BOOST_AUTO_TEST_CASE( cmap_concurrent_insert )
{
    std::vector<std::string> words = ReadWords(WORDS_FILENAME, MAX_WORDS);
    BOOST_CHECK(words.size() == MAX_WORDS);
    CMap<unsigned long, std::string> sut;

    std::vector<boost::thread*> threads;
    const uint16_t numThreads = 10;
    for(int i=0;i<numThreads;++i)
        {
        threads.emplace_back(new boost::thread(cmapInsertThreadFunction,words,&sut));
        }

    for(auto t : threads)
        {
        t->join();
        delete t;
        }
    threads.clear();

    BOOST_CHECK(sut.size() == words.size());
    for( auto w : words )
        {
        unsigned long h = hash(w.c_str());
        BOOST_CHECK(w == sut.get(h));
        }
}

BOOST_AUTO_TEST_CASE( cset_concurrent_insert )
{
    std::vector<std::string> words = ReadWords(WORDS_FILENAME, MAX_WORDS);
    BOOST_CHECK(words.size() == MAX_WORDS);
    const uint16_t numThreads = 10;
    CSet<std::string> sut;
    std::vector<boost::thread*> threads;
    for(int i=0; i<numThreads; ++i)
        {
        threads.emplace_back(new boost::thread(csetInsertThreadFunction,words,  &sut));
        }

    for(auto t : threads)
        {
        t->join();
        delete t;
        }
    threads.clear();

    BOOST_CHECK(sut.size() == words.size());
}

BOOST_AUTO_TEST_CASE( cset_find )
{
    std::vector<std::string> words = ReadWords(WORDS_FILENAME, MAX_WORDS);

    BOOST_CHECK(words.size() == MAX_WORDS);

    CSet<std::string> sut;

    for(auto w : words)
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

BOOST_AUTO_TEST_CASE( cset_remove )
{
    std::vector<std::string> words = ReadWords(WORDS_FILENAME, MAX_WORDS);
    BOOST_CHECK(words.size() == MAX_WORDS);


    CSet<std::string> sut;
    for(auto w : words)
        {
        BOOST_CHECK(sut.insert(w));
        }

    BOOST_CHECK(words.size() == sut.size());

    const uint16_t numThreads = 10;
    std::vector<boost::thread*> threads;
    for(int i=0; i<numThreads; ++i)
        {
        threads.emplace_back
                (
                        new boost::thread
                                (
                                0==i
                                ? csetInsertThreadFunction
                                :csetRemoveThreadFunction
                                        , words,  &sut)
                );
        }

    for(auto t : threads)
        {
        BOOST_CHECK(t->joinable());
        t->join();
        BOOST_CHECK(nullptr != t);
        delete t;
        }

    BOOST_CHECK(0 == sut.size());
}

BOOST_AUTO_TEST_CASE( test_all_nodes_alive)
{
    Nodes nodes;
    Node node0;
    Node node1;
    Node node2;

    BOOST_CHECK(!all_nodes_alive(nodes));

    nodes.emplace_back(&node0);
    nodes.emplace_back(&node1);
    nodes.emplace_back(&node2);

    // NOTE: the assumption here is that 100ms is long enough for 3 nodes to
    // get initialized and running in the alive state.
    boost::this_thread::sleep_for(boost::chrono::milliseconds(100));

    BOOST_CHECK(all_nodes_alive(nodes));
    node0.kill();
    BOOST_CHECK(!all_nodes_alive(nodes));
    node0.join();
    BOOST_CHECK(!all_nodes_alive(nodes));

    node1.kill();
    node1.join();

    node2.kill();
    node2.join();
}

BOOST_AUTO_TEST_CASE( test_wait_for_all_nodes_to_start)
{
    Nodes nodes;
    Node node0;
    Node node1;
    Node node2;

    // NOTE: remember that wait_for_all_nodes_to_start calls all_nodes_alive,
    // but the next block *assumes* the tasks haven't started yet...
    for(auto node : nodes)
        {
        BOOST_CHECK(node->state() != Task::alive);
        }

    nodes.emplace_back(&node0);
    nodes.emplace_back(&node1);
    nodes.emplace_back(&node2);

    wait_for_all_nodes_to_start(nodes);

    // NOTE: remember that wait_for_all_nodes_to_start calls all_nodes_alive.
    for(auto node : nodes)
        {
        BOOST_CHECK(node->state() == Task::alive);
        }
}

BOOST_AUTO_TEST_CASE( test_reaper )
{
//    const int TEST_NODE_COUNT = 3;
//    Nodes nodes;
//
//    for(int i = 0; i < TEST_NODE_COUNT ; ++i)
//        {
//        nodes.emplace_back(new Node());
//        }
//
//    BOOST_CHECK( 3 == nodes.size() );
//    wait_for_all_nodes_to_start(nodes);
//    reaper(&nodes);
//    boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
//    BOOST_CHECK( 3 == nodes.size() );
//
//    nodes[1]->kill();
//    boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
//
//
//    const int alive_count = std::accumulate(
//            nodes.begin(),
//            nodes.end(),
//            0,
//            [](Node* node){return (Task::alive == node->state()) ? 1 : 0;}
//    );
//
//    BOOST_CHECK(2==alive_count);
//
//
//
//
//
//
//    reaper(&nodes);
//    boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
//    BOOST_CHECK( 2 == nodes.size() );
//
//    for(auto node : nodes)
//        {
//
//        }
//
//
//
//    boost::this_thread::sleep_for(boost::chrono::milliseconds(100));




}



///////////////////////////////////////////////////////////////////////////////
// Test Helpers
unsigned long hash(const char *str)
{
    unsigned long hash = 5381;
    int c;
    while ( (c = *str++) )
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    return hash;
}

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

void csetInsertThreadFunction(std::vector<std::string>& words, CSet<std::string>* sut)
{
    std::time_t now = std::time(0);
    boost::random::mt19937 gen{static_cast<std::uint32_t>(now)};
    unsigned long max = words.size();
    boost::random::uniform_int_distribution<unsigned long> dist{0, max-1};
    while(sut->size() < words.size())
        {
        std::string str = words[dist(gen)];
        sut->cInsert(str);
        boost::this_thread::yield();
        }
}

void csetRemoveThreadFunction(std::vector<std::string>& words, CSet<std::string>* sut)
{
    std::time_t now = std::time(0);
    boost::random::mt19937 gen{static_cast<std::uint32_t>(now)};
    unsigned long max = words.size();
    boost::random::uniform_int_distribution<unsigned long> dist{0, max-1};

    while(sut->size()>0)
        {
        std::string word = words[dist(gen)];
        auto found = sut->find(word);
        sut->remove(found);
        boost::this_thread::yield();
        }
}

std::vector<std::string> ReadWords(const std::string& fileName, const uint16_t n)
{
    std::vector<std::string> words;
    std::ifstream wordFile;
    std::string line;
    wordFile.open(fileName);
    if(wordFile.is_open())
        {
        int16_t wordCount = n;
        while(getline(wordFile,line) && wordCount > 0)
            {
            words.emplace_back(line);
            wordCount--;
            }
        }
    return words;
}


