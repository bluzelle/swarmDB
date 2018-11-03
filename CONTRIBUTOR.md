# Contributing to Bluzelle

Welcome to wanting to contribute to Bluzelle! Thank you for taking your time to contribute. 

The following is a set of guidelines for contributing to the Bluzelle ecosystem (this includes swarmDB, the drivers, and more). These are of course mostly just guidelines, not rules. Feel free to use your best judgement, and you can always contribute proposals to change this guide via a pull request!

#### Table Of Contents

[Code of Conduct](#code-of-conduct)
[The Quick Start Guide](#the-quick-start-guide)
[Getting Started](#getting-started)
[How Can I Contribute?](#how-can-icontribute)
[Coding Style Guide](#coding-style-guide)

## Code of Conduct

This project and its participants are governed by the [Bluzelle Code of Conduct](CODE_OF_CONDUCT.md). By participating in this project, you are expected to uphold the code. Report unacceptable behaviour to [opensource@bluzelle.com](mailto:opensource@bluzelle.com).

The general idea here is that you should be **respectful**, **positive** and **considerate**. When asking questions, please be **patient** for a response, **specific** in what you are asking, and always think **collaboratively**. You are reminded that you should not **spam**, post links to sites that violate our Terms of Use, or post third party advertisements. 

## The Quick Start Guide

Join our community resources:
* [mailing list](https://groups.google.com/forum/#!forum/bluzelle) - this is the Google group where Bluzelle developers discuss development plans. Users are also encourage to ask questions here. You can either subscribe to the list, or read it as a forum.
* [Gitter](https://gitter.im/bluzelle) - if chat is something you prefer, and you're looking for answers quicker, get to the Bluzelle Gitter community. We have several rooms and you could post to the Lobby or anywhere you feel is appropriate. Remember though that you *may* not get answers immediately, as the Bluzelle project is distributed worldwide, and it might take some time for you to get an answer. Please be patient.

## Getting Started

Bluzelle is growing to be a large open source project. There are many repositories on [GitHub](http://github.com/bluzelle) and it is a good idea to familiarise yourself with them.

* [bluzelle/swarmDB](https://github.com/bluzelle/swarmDB) - this is the core Bluzelle decentralised database.

Do pay attention to the branches within swarmDB as there are plenty!

### Client Libraries
* [pyBluzelle](https://github.com/bluzelle/pyBluzelle) - this is the Python client library for Bluzelle swarmDB.
* [swarmclient-js](https://github.com/bluzelle/swarmclient-js) - this is the JavaScript client library for Bluzelle swarmDB.
* [bluzelle-php](https://github.com/bluzelle/bluzelle-php) - this is the PHP client library for Bluzelle swarmDB.
* [swarmclient-rb](https://github.com/bluzelle/swarmclient-rb) - this is the Ruby client library for Bluzelle swarmDB.

## How Can I Contribute?

### How to report bugs

The Swarmdb project is tracking issues within github issues. If you find a bug within the software or documentation please open a new [issue](https://github.com/bluzelle/swarmDB/issues). Your bug will be placed on our backlog and addressed based on priotiry.

### How do I suggest enhancements?

Enhancements can be requested in two ways. 

1. Like bugs you can open a new [issue](https://github.com/bluzelle/swarmDB/issues)
2. More proactively you can make the code or documentation change and submit it as a pull request

### Making your first code contribution

### Branching Model
Basic standards will be in place across all Bluzelle repositories that are based off the [Git Flow model](https://datasift.github.io/gitflow/IntroducingGitFlow.html). Below you will find a description of each branch and it’s intended purpose.

#### Devel

**Branch**: Devel

**Purpose**: The Devel branch the official integration branch. 

**Perquisite for commits**: Merges to this branch are by pull request only and must pass the Travis Build and have at least one code review. Any committer can merge a pull request once the above requisites are satisfied. Only squash merges are allowed in this branch.

**Tags**: None

**Build artifacts**: Build automation may push to an “unstable” repository for testing

#### Master

**Branch**: Master

**Purpose**: Master will ***ONLY*** have commits related to official releases (major, minor and patch). 

**Prerequisite for commit**: Merges to this branch are by pull request only and must pass the Travis Build and have at least one code review. ONLY official branch owners can merge a pull request. Only merges are allowed on this branch.

**Tags**: `${major}-${minor}-${TRAVIS-BUILD}`

**Build artifacts**: Build automation must push to an official repo either public or the Bluzelle artifactory repositories


#### Feature Branch

**Branch**: `/task/${username}/${taskname}`

**Purpose**: This is a feature branch and is meant to last for the duration of feature development. There is no implication of stability in a feature branch and developers have absolute freedom on what happens within these branches. After merging to the devel branch the feature branch is deleted. 

**Prerequisite for commit**: None

**Tags**: None

**Build artifacts**: None

#### Commit Messages
Commit messages must follow the basic structure in the example provided below. An “issuekey” may be either a GitHub Issue or Jira Issue. Commit messages will contain a single line description of the change, followed by a more in depth description of the commit. 

Example:

~~~~
${issuekey} This is a one line commit message

This is a longer commit paragraph for more detail. It may also contain bullet points:

feature change 1
feature change 2
side effect 1
~~~~


### Pull requests and signing the Bluzelle Contributor License Agreement (CLA)
Once you have submitted a pull request, you will also have to sign the Bluzelle Contributor License Agreement (CLA). This is done automatically, as long as you have a GitHub account, and submit the pull request. Without signing the CLA, your contribution cannot be accepted and there will be no review for the pull request to get it merged. 

We use an automated platform to track digital signatures tied to your GitHub username. 

## Coding Style Guide

Use this document as a guideline, if you don't find what you need here please contact a Bluzelle employee with your comments or suggestions. 
Another great resource is the Google C++ Style guide (https://google.github.io/styleguide/cppguide.html)

### Project Layout

    * CMake is used to generate a build environment. (ie. makefiles, xcode project etc.)
    * Source and test file naming is based on the class being defined/tested:

        ex. node module
        node
        ├── CMakeLists.txt
        ├── node_base.hpp
        ├── node.cpp
        ├── node.hpp
        ├── session_base.hpp
        ├── session.cpp
        ├── session.hpp
        └── test
            ├── CMakeLists.txt
            ├── node_test.cpp
            └── session_test.cpp


### Code style

    * Write your code as though you are writing it for publication. 
    * Your code should compile with no warnings using the C++ 17 flag.
    * Allman coding style using 4 space characters for indentation. (https://en.wikipedia.org/wiki/Indentation_style#Allman_style)
    * Try to limit line length to 120 characters. Continuation of code should be the next line with one indentation level.
    * Header files should use newer "#pragma once" instead of traditional #ifdef guard macros.
    * Source and test file naming is based on the class being defined/tested: 

        class this_is_an_example: 
        {
            ...
            this_is_an_example.hpp
            this_is_an_example.cpp
            this_is_an_example_test.cpp

    * When modifying existing code continue the style that it was created to maintain consistency.
    * Function/method definitions should have return type on a separate line:

        bool
        class::method()
        {
           ...
           return true;
        }

    * Use #include<cstdint> for POD types such as uint32_t, uint64_t etc.
    * Do not use "auto" for POD types
    * Use auto for types that can not be defined or are rather verbose such as a lambda or an iterator.
    * Use snake case for class & variable naming:

        class this_is_snake_case
        { 
            ...

    variables:

         uint32_t widget_count;

    * Class layout should be public, protected & private:

        class foo
        {
        public:
        
            // initialization list layout example...
            foo(uint32_t foo_counter, uint32_t max_widgets)
                : foo_counter(foo_counter)
                , max_widgets(max_widgets)
            {
            }

        private:
        
            uint32_t foo_counter = 0; // prefer class initialization of variable if not passed through constructor
            const uin32_t max_widgets;
        };

    * Constructor layout should align with the first parameter:
        
        foo::foo(unsigned int age,
                 float weight_in_tonnes,
                 string name,
                 string city,
                 string country)
               : this->age(age)
               , this->weight_in_tonnes(weight_in_tonnes)
               , this->name(name)
               , this->city(city)
               , this->country(country)
        {
            ...
        }
        
    * Do not use Hungarian notation.
    * Class and variable names should be descriptive; avoid abbreviation unless using a standard nomenclature such as TCP, UDP, HTTP & CURL.
    * Code within namespace should be indented
    * Even single lines of code for conditionals and loops should be enclosed by braces. 
    
        switch(my_value)
        {
            case MY_CONST_GLOBAL:
            {
                ...
            }
            break;
            
            default:
            {
                ...
            }
            break;
        }
                   
    * When using multiple levels of namespaces use c++17's syntax:

        namespace bzn::utils
        {
            class ....

    * Use "using namespace" sparingly and only where you should follow the DRY principle.
    * Use Test Driven Development
    * Use CI's code coverage reporting to help identify untested code and exception cases
    * Create a mockable interface class to help test code that would require complex setup or dependencies such as a server

        class foo_base
        {
        public:
            virtual ~foo_base() = default;
            void function_one() const = 0;
            ...
            
    * Do not use dynamic cast unless there is a very compelling reason
    * Const casts are not to be used under any circumstances
    * Reinterpret casts are only to be used in situations where “void” pointers are being upcast
    * Const should be used in every place it can be used. For example, imagine the following function get_window in some class prototype: 
        
        const Window* get_window(const Foo* const some_variable_name) const
    
    * Derived classes from an interface should be marked as final unless the intent is to use it as a base class.
    * Use references everywhere applicable for function parameters.
    * Globals will be declared const within an unnamed namespace
    
        namespace
        {
            const uint32_t MY_CONST_GLOBAL = 1337;
        }
        
    * As a fundamental principle, there should be no entropy whatsoever in the code.Some common examples of entropy include the use of static initialization where the order of instantiation is often non-deterministic.
    * Variable names must CLEARLY describe the type and usage
    * Use std::shared_ptr & std::unique_ptr instead of raw pointers.
    * Use const where possible (functions & variables)
    * Do not use typedef, but use "using xyz_t" language feature where new types have a suffix of _t:

        using foo_count_t = uint32_t;

    * Use new enum classes and specify size type:

        enum class state : uint8_t
        {
             ...

    * Access member variables and functions always using the "this->" pointer. Do not mark member variables with a prefix/postfix pattern.

        this->function_one();
        this->a_member_variable = 0;

    * Use Doxygen style comments for all public methods. Base class should have them and derived classes do not require it. 
    * Comment any code that is not obvious, in terms of what it does.
    * Reference symbol should be added to the type and not the variable.

        void function(const some_type& my_type);

### Best practices

    * Follow SOLID & DRY principles

        Keep functions small and focused. (https://en.wikipedia.org/wiki/SOLID)

    * Before writing new code to support a feature, follow these steps:

        1. Check the C++ std library algorithms etc.
        2. If not in std library then look at boost.
        3. If boost can't help then consult with the Bluzelle team, third party libraries 
        4. If you can't find a third party library then consider implementing your own.

    * #includes should use <path/header_file.hpp> and not "header_file.hpp"

        #include <include/boost_asio_beast.hpp>
        #include <node/node_base.hpp>
        #include <node/session_base.hpp>
        #include <options/options_base.hpp>

    * Preferred include ordering:

        1. global project headers
        2. current lib/code headers
        3. other project lib headers
        4. third_party lib headers
        5. standard headers

    * Use functional style as apposed to raw loops such has std::find_if, std::remove_if etc.
    * Use exceptions only when the code can not function with out "something". Typically this would be during construction of a class.
    * Long functions are difficult to change and difficult to understand. Each function should attempt to do exactly one thing. You should be able to sum it up in one sentence (and the method name should be a summary of that sentence). If you find a function is getting too long, split it into sensible parts.
    * Functions taking large numbers of parameters (ie: more than six) are a generally a bad idea and are indicative of either a function trying to do too much or poor class organization. Split the function up into sensible parts.
    * Conditionals and loops should not be nested more than three deep. Any such code needs to be refactored. Either pull the inner parts of the code into separate methods or pull the complex conditions into functions. A line of code should never exceed approximately seventy characters.
